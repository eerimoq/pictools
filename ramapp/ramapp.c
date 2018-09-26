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
#include "ramapp.h"
#include "compat.h"

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

#define FLASH_ROW_SIZE                                     256

/**
 * Faster than memcmp.
 */
static int cmp32(uint32_t *b1_p, uint32_t address, size_t size)
{
    size_t i;

    for (i = 0; i < size / 4; i++) {
        if (b1_p[i] != load_flash_32(address, i)) {
            return (1);
        }
    }

    return (0);
}

static ssize_t fast_data_read(uint8_t *buf_p, size_t size)
{
    uint32_t data;
    size_t number_of_words;
    size_t i;

    number_of_words = DIV_CEIL(size, 4);

    for (i = 0; i < number_of_words; i++) {
        data = etap_fast_data_read();

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

        etap_fast_data_write(data);
    }

    return (size);
}

static ssize_t handle_erase(struct ramapp_t *self_p,
                            uint8_t *buf_p,
                            size_t size)
{
    uint32_t address;

    address = ((buf_p[0] << 24) | (buf_p[1] << 16) | (buf_p[2] << 8) | buf_p[3]);
    size = ((buf_p[4] << 24) | (buf_p[5] << 16) | (buf_p[6] << 8) | buf_p[7]);

    return (flash_erase(self_p->flash_p, address, size));
}

static ssize_t handle_read(struct ramapp_t *self_p,
                           uint8_t *buf_p,
                           size_t size)
{
    uint32_t address;
    size_t i;
    uint8_t *dst_p;

    address = ((buf_p[0] << 24) | (buf_p[1] << 16) | (buf_p[2] << 8) | buf_p[3]);
    size = ((buf_p[4] << 24) | (buf_p[5] << 16) | (buf_p[6] << 8) | buf_p[7]);

    dst_p = buf_p;

    for (i = 0; i < size; i++) {
        dst_p[i] = load_flash_8(address, i);
    }

    return (size);
}

static ssize_t handle_write(struct ramapp_t *self_p,
                            uint8_t *buf_p,
                            size_t size)
{
    uint32_t address;
    ssize_t res;

    address = ((buf_p[0] << 24) | (buf_p[1] << 16) | (buf_p[2] << 8) | buf_p[3]);
    size = ((buf_p[4] << 24) | (buf_p[5] << 16) | (buf_p[6] << 8) | buf_p[7]);

    res = flash_write(self_p->flash_p, address, &buf_p[8], size);

    if (res == size) {
        if (cmp8(&buf_p[8], address, size) == 0) {
            res = 0;
        } else {
            res = -EFLASHWRITE;
        }
    }

    return (res);
}

static ssize_t handle_fast_write(struct ramapp_t *self_p,
                                 uint8_t *buf_p,
                                 size_t size)
{
    uint32_t address;
    ssize_t res;
    uint32_t actual_crc;
    uint32_t expected_crc;
    uint8_t buf[2][FLASH_ROW_SIZE];
    int index;
    int i;

    address = ((buf_p[0] << 24) | (buf_p[1] << 16) | (buf_p[2] << 8) | buf_p[3]);
    size = ((buf_p[4] << 24) | (buf_p[5] << 16) | (buf_p[6] << 8) | buf_p[7]);
    expected_crc = ((buf_p[8] << 8) | (buf_p[9] << 0));

    /* Start the first row. */
    fast_data_read(&buf[0][0], FLASH_ROW_SIZE);

    res = flash_async_write(self_p->flash_p,
                            address,
                            &buf[0][0],
                            FLASH_ROW_SIZE);

    if (res != 0) {
        return (res);
    }

    actual_crc = crc_ccitt(0xffff, &buf[0][0], FLASH_ROW_SIZE);

    /* Middle rows. */
    index = 0;

    for (i = FLASH_ROW_SIZE; i < size; i += FLASH_ROW_SIZE) {
        index ^= 1;
        fast_data_read(&buf[index][0], FLASH_ROW_SIZE);

        res = flash_async_wait(self_p->flash_p);

        if (res != 0) {
            return (res);
        }

        /* Reading from flash at the same time as writing stalls the
           CPU until the write is complete. */
        res = cmp32((uint32_t *)&buf[index ^ 1][0],
                    address + i - FLASH_ROW_SIZE,
                    FLASH_ROW_SIZE);

        if (res != 0) {
            return (-EFLASHWRITE);
        }

        res = flash_async_write(self_p->flash_p,
                                address + i,
                                &buf[index][0],
                                FLASH_ROW_SIZE);

        if (res != 0) {
            return (res);
        }

        actual_crc = crc_ccitt(actual_crc,
                               &buf[index][0],
                               FLASH_ROW_SIZE);
    }

    /* Wait for the last row. */
    res = flash_async_wait(self_p->flash_p);

    if (res != 0) {
        return (res);
    }

    res = cmp32((uint32_t *)&buf[index][0],
                address + i - FLASH_ROW_SIZE,
                FLASH_ROW_SIZE);

    if (res != 0) {
        return (-EFLASHWRITE);
    }

    if (actual_crc != expected_crc) {
#if defined(UNIT_TEST)
        std_printf(OSTR("fast_write: actual_crc: 0x%04x, expected_crc: 0x%04x\r\n"),
                   actual_crc,
                   expected_crc);
#endif
        return (-EBADCRC);
    }

    return (0);
}

static ssize_t handle_command(struct ramapp_t *self_p,
                              uint8_t *buf_p,
                              size_t size)
{
    ssize_t res;
    int type;

    type = ((buf_p[0] << 8) | buf_p[1]);

    switch (type) {

    case COMMAND_TYPE_PING:
        res = 0;
        break;

    case COMMAND_TYPE_ERASE:
        res = handle_erase(self_p, &buf_p[PAYLOAD_OFFSET], size);
        break;

    case COMMAND_TYPE_READ:
        res = handle_read(self_p, &buf_p[PAYLOAD_OFFSET], size);
        break;

    case COMMAND_TYPE_WRITE:
        res = handle_write(self_p, &buf_p[PAYLOAD_OFFSET], size);
        break;

    case COMMAND_TYPE_FAST_WRITE:
        res = handle_fast_write(self_p, &buf_p[PAYLOAD_OFFSET], size);
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
#if defined(UNIT_TEST)
        std_printf(OSTR("actual_crc: 0x%04x, expected_crc: 0x%04x\r\n"),
                   actual_crc,
                   expected_crc);
#endif
        return (-EBADCRC);
    }

    return (size);
}

static int write_command_response(uint8_t *buf_p, ssize_t size)
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

    return (0);
}

int ramapp_init(struct ramapp_t *self_p,
                struct flash_driver_t *flash_p)
{
    self_p->flash_p = flash_p;

    return (0);
}

int ramapp_process_packet(struct ramapp_t *self_p)
{
    ssize_t size;
    uint8_t buf[PAYLOAD_OFFSET + MAXIMUM_PAYLOAD_SIZE + CRC_SIZE + 2];

    size = read_command_request(&buf[0]);

    if (size >= 0) {
        size = handle_command(self_p, &buf[0], size);
    }

    return (write_command_response(&buf[0], size));
}
