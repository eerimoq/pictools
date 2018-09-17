/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018, Erik Moqvist
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * This file is part of the PIC tools project.
 */

#include "simba.h"

#define TYPE_SIZE                                           2
#define SIZE_SIZE                                           2
#define MAXIMUM_PAYLOAD_SIZE                             1024
#define CRC_SIZE                                            2

#define PAYLOAD_OFFSET                (TYPE_SIZE + SIZE_SIZE)

/* Command types. */
#define COMMAND_TYPE_FAILED                                -1
#define COMMAND_TYPE_PING                                   1
#define COMMAND_TYPE_ERASE                                  2
#define COMMAND_TYPE_READ                                   3
#define COMMAND_TYPE_WRITE                                  4
#define COMMAND_TYPE_FAST_WRITE                           106

#define PIC32_ETAP_FASTDATA ((volatile uint32_t *) 0xff200000)

static struct flash_driver_t flash;
static uint8_t buf[PAYLOAD_OFFSET + MAXIMUM_PAYLOAD_SIZE + CRC_SIZE + 2];

static ssize_t fast_data_read(uint8_t *buf_p, size_t size)
{
    uint32_t data;
    size_t number_of_words;
    size_t i;

    number_of_words = DIV_CEIL(size, 4);

    for (i = 0; i < number_of_words; i++) {
        data = *PIC32_ETAP_FASTDATA;
        buf_p[4 * i + 0] = (data >> 24);
        buf_p[4 * i + 1] = (data >> 16);
        buf_p[4 * i + 2] = (data >> 8);
        buf_p[4 * i + 3] = (data >> 0);
    }

    return (size);
}

static ssize_t fast_data_write(uint8_t *buf_p, size_t size)
{
    uint32_t data;
    size_t number_of_words;
    size_t i;

    number_of_words = DIV_CEIL(size, 4);

    for (i = 0; i < number_of_words; i++) {
        data = ((buf_p[4 * i + 0] << 24)
                | (buf_p[4 * i + 1] << 16)
                | (buf_p[4 * i + 2] << 8)
                | (buf_p[4 * i + 3] << 0));
        *PIC32_ETAP_FASTDATA = data;
    }

    return (size);
}

static ssize_t handle_erase(uint8_t *buf_p, size_t size)
{
    uint32_t address;

    address = ((buf_p[0] << 24) | (buf_p[1] << 16) | (buf_p[2] << 8) | buf_p[3]);
    size = ((buf_p[4] << 24) | (buf_p[5] << 16) | (buf_p[6] << 8) | buf_p[7]);

    return (flash_erase(&flash, address, size));
}

static ssize_t handle_read(uint8_t *buf_p, size_t size)
{
    uint32_t address;
    size_t i;
    uint8_t *dst_p;
    uint8_t *src_p;

    address = ((buf_p[0] << 24) | (buf_p[1] << 16) | (buf_p[2] << 8) | buf_p[3]);
    size = ((buf_p[4] << 24) | (buf_p[5] << 16) | (buf_p[6] << 8) | buf_p[7]);

    dst_p = buf_p;
    src_p = (uint8_t *)address;

    for (i = 0; i < size; i++) {
        dst_p[i] = src_p[i];
    }

    return (size);
}

static ssize_t handle_write(uint8_t *buf_p, size_t size)
{
    uint32_t address;
    ssize_t res;

    address = ((buf_p[0] << 24) | (buf_p[1] << 16) | (buf_p[2] << 8) | buf_p[3]);
    size = ((buf_p[4] << 24) | (buf_p[5] << 16) | (buf_p[6] << 8) | buf_p[7]);

    res = flash_write(&flash, address, &buf_p[8], size);

    if (res == size) {
        if (memcmp(&buf_p[8], (void *)address, size) == 0) {
            res = 0;
        } else {
            res = -EFLASHWRITE;
        }
    }

    return (res);
}

static ssize_t handle_fast_write(uint8_t *buf_p, size_t size)
{
    uint32_t address;
    ssize_t res;
    uint32_t actual_crc;
    uint32_t expected_crc;
    uint8_t buf[2][256];
    int index;
    int i;

    address = ((buf_p[0] << 24) | (buf_p[1] << 16) | (buf_p[2] << 8) | buf_p[3]);
    size = ((buf_p[4] << 24) | (buf_p[5] << 16) | (buf_p[6] << 8) | buf_p[7]);
    expected_crc = ((buf_p[8] << 24)
                    | (buf_p[9] << 16)
                    | (buf_p[10] << 8)
                    | (buf_p[11] << 0));

    /* Start the first row. */
    fast_data_read(&buf[0][0], 256);

    res = flash_async_write_row(&flash, address, &buf[0][0]);

    if (res != 0) {
        return (res);
    }

    actual_crc = crc_ccitt(0xffff, &buf[0][0], 256);

    /* Middle rows. */
    index = 1;

    for (i = 256; i < size; i += 256) {
        fast_data_read(&buf[index][0], 256);

        res = flash_async_wait(&flash);

        if (res != 0) {
            return (res);
        }

        res = flash_async_write_row(&flash, address + i, &buf[index][0]);

        if (res != 0) {
            return (res);
        }

        actual_crc = crc_ccitt(actual_crc, &buf[index][0], 256);

        index ^= 1;

        if (memcmp(&buf[index][0], (void *)(address + i - 256), 256) != 0) {
            return (-EFLASHWRITE);
        }
    }

    /* Wait for the last row. */
    res = flash_async_wait(&flash);

    if (res != 0) {
        return (res);
    }

    index ^= 1;

    if (memcmp(&buf[index][0], (void *)(address + i - 256), 256) != 0) {
        return (-EFLASHWRITE);
    }

    if (actual_crc != expected_crc) {
        return (-EBADCRC);
    }

    return (0);
}

static ssize_t handle_command(uint8_t *buf_p, size_t size)
{
    ssize_t res;
    int type;

    type = ((buf_p[0] << 8) | buf_p[1]);

    switch (type) {

    case COMMAND_TYPE_PING:
        res = 0;
        break;

    case COMMAND_TYPE_ERASE:
        res = handle_erase(&buf[PAYLOAD_OFFSET], size);
        break;

    case COMMAND_TYPE_READ:
        res = handle_read(&buf[PAYLOAD_OFFSET], size);
        break;

    case COMMAND_TYPE_WRITE:
        res = handle_write(&buf[PAYLOAD_OFFSET], size);
        break;

    case COMMAND_TYPE_FAST_WRITE:
        res = handle_fast_write(&buf[PAYLOAD_OFFSET], size);
        break;

    default:
        res = -ENOCOMMAND;
        break;
    }

    return (res);
}

static ssize_t read_command_request(uint8_t *buf_p)
{
    ssize_t size;
    uint16_t actual_crc;
    uint16_t expected_crc;

    /* Read type and size. */
    fast_data_read(&buf_p[0], PAYLOAD_OFFSET);

    size = ((buf_p[2] << 8) | buf_p[3]);

    if (size > MAXIMUM_PAYLOAD_SIZE) {
        return (-EINVAL);
    }

    /* Read payload and crc. */
    fast_data_read(&buf_p[PAYLOAD_OFFSET], size + CRC_SIZE);

    expected_crc = ((buf_p[PAYLOAD_OFFSET + size] << 8)
                    | buf_p[PAYLOAD_OFFSET + size + 1]);
    actual_crc = crc_ccitt(0xffff, &buf_p[0], PAYLOAD_OFFSET + size);

    if (actual_crc != expected_crc) {
        return (-EBADCRC);
    }

    return (size);
}

static void write_command_response(uint8_t *buf_p, ssize_t size)
{
    uint16_t crc;

    /* Failure. */
    if (size < 0) {
        buf_p[0] = (COMMAND_TYPE_FAILED >> 8);
        buf_p[1] = COMMAND_TYPE_FAILED;
        buf_p[4] = (size >> 24);
        buf_p[5] = (size >> 16);
        buf_p[6] = (size >> 8);
        buf_p[7] = (size >> 0);
        size = 4;
    }

    buf_p[2] = (size >> 8);
    buf_p[3] = size;

    size += PAYLOAD_OFFSET;

    crc = crc_ccitt(0xffff, buf_p, size);

    buf_p[size] = (crc >> 8);
    buf_p[size + 1] = crc;
    size += CRC_SIZE;

    fast_data_write(buf_p, size);
}

int main()
{
    ssize_t size;

    flash_module_init();
    flash_init(&flash, &flash_device[0]);

    while (1) {
        size = read_command_request(&buf[0]);

        if (size >= 0) {
            size = handle_command(&buf[0], size);
        }

        write_command_response(&buf[0], size);
    }

    return (0);
}
