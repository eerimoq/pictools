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
#include "programmer.h"

/* ICSP pin devices. */
#define pin_pgec_dev                               pin_d2_dev
#define pin_pged_dev                               pin_d3_dev
#define pin_mclrn_dev                              pin_d4_dev

/* Device status. */
#define STATUS_CPS                     bits_reverse_8(BIT(7))
#define STATUS_NVMERR                  bits_reverse_8(BIT(5))
#define STATUS_CFGRDY                  bits_reverse_8(BIT(3))
#define STATUS_FCBUSY                  bits_reverse_8(BIT(2))
#define STATUS_DEVRST                  bits_reverse_8(BIT(0))

#define CONTROL_PRACC                bits_reverse_32(BIT(18))
#define CONTROL_PROBEN               bits_reverse_32(BIT(15))
#define CONTROL_PROBTRAP             bits_reverse_32(BIT(14))
#define CONTROL_EJTAGBRK             bits_reverse_32(BIT(12))

/* MCHP TAP instructions. */
#define MTAP_COMMAND                     bits_reverse_8(0x07)
#define MTAP_SW_MTAP                     bits_reverse_8(0x04)
#define MTAP_SW_ETAP                     bits_reverse_8(0x05)
#define MTAP_IDCODE                      bits_reverse_8(0x01)

/* EJTAG TAP instructions. */
#define ETAP_ADDRESS                     bits_reverse_8(0x08)
#define ETAP_DATA                        bits_reverse_8(0x09)
#define ETAP_CONTROL                     bits_reverse_8(0x0a)
#define ETAP_EJTAGBOOT                   bits_reverse_8(0x0c)
#define ETAP_FASTDATA                    bits_reverse_8(0x0e)

/* MTAP commands. */
#define MCHP_STATUS                      bits_reverse_8(0x00)
#define MCHP_ASSERT_RST                  bits_reverse_8(0xd1)
#define MCHP_DE_ASSERT_RST               bits_reverse_8(0xd0)
#define MCHP_ERASE                       bits_reverse_8(0xfc)

/* Protocol. */
#define TYPE_SIZE                                           2
#define SIZE_SIZE                                           2
#define MAXIMUM_PAYLOAD_SIZE                             1024
#define CRC_SIZE                                            2

#define PAYLOAD_OFFSET                (TYPE_SIZE + SIZE_SIZE)

/* Command types. */
#define COMMAND_TYPE_FAILED                                -1
#define COMMAND_TYPE_PING                                 100
#define COMMAND_TYPE_CONNECT                              101
#define COMMAND_TYPE_DISCONNECT                           102
#define COMMAND_TYPE_RESET                                103
#define COMMAND_TYPE_DEVICE_STATUS                        104
#define COMMAND_TYPE_CHIP_ERASE                           105
#define COMMAND_TYPE_FAST_WRITE                           106
#define COMMAND_TYPE_VERSION                              107

/* Packet sizes. */
#define PACKET_FAST_WRITE_REQUEST_SIZE                     16
#define PACKET_FAST_WRITE_DATA_SIZE                       256

#define CTRL_TIMEOUT_NS                             500000000
#define ERASE_TIMEOUT_S                                     3

/* Error codes. */
#define EENTERSERIALEXECUTIONMODE                       10000
#define ERAMAPPUPLOAD                                   10001

static const uint32_t ramapp_upload_instructions[] = {
#include "ramapp_upload_instructions.i"
};

static int send_command(struct icsp_soft_driver_t *icsp_p,
                        uint8_t command)
{
    return (icsp_soft_instruction_write(icsp_p, &command, 5));
}

static int xfer_data_32(struct icsp_soft_driver_t *icsp_p,
                        uint32_t request,
                        uint32_t *response_p)
{
    int res;

    request = htonl(request);

    res = icsp_soft_data_transfer(icsp_p,
                                  (uint8_t *)response_p,
                                  (uint8_t *)&request,
                                  32);

    if (res != 0) {
        return (res);
    }

    *response_p = ntohl(*response_p);

    return (0);
}

static int xfer_instruction(struct icsp_soft_driver_t *icsp_p,
                            uint32_t instruction)
{
    int res;
    uint32_t response;
    struct time_t time;
    struct time_t end_time;

    res = send_command(icsp_p, ETAP_CONTROL);

    if (res != 0) {
        return (res);
    }

    time.seconds = 0;
    time.nanoseconds = 500000000;

    time_get(&end_time);
    time_add(&end_time, &end_time, &time);

    do {
        res = xfer_data_32(icsp_p,
                           bits_reverse_32(0x0004c000),
                           &response);

        time_get(&time);

        if (time_compare_greater_than_t == time_compare(&time, &end_time)) {
            res = -ETIMEDOUT;
        }
    } while (((response & CONTROL_PRACC) == 0) && (res == 0));

    if (res != 0) {
        return (res);
    }

    res = send_command(icsp_p, ETAP_DATA);

    if (res != 0) {
        return (res);
    }

    res = xfer_data_32(icsp_p, bits_reverse_32(instruction), &response);

    if (res != 0) {
        return (res);
    }

    res = send_command(icsp_p, ETAP_CONTROL);

    if (res != 0) {
        return (res);
    }

    res = xfer_data_32(icsp_p, bits_reverse_32(0x0000c000), &response);

    return (res);
}

static int enter_serial_execution_mode(struct icsp_soft_driver_t *icsp_p)
{
    int res;
    uint8_t command;
    uint8_t status;

    res = send_command(icsp_p, MTAP_SW_MTAP);

    if (res != 0) {
        return (res);
    }

    res = send_command(icsp_p, MTAP_COMMAND);

    if (res != 0) {
        return (res);
    }

    command = MCHP_STATUS;
    res = icsp_soft_data_transfer(icsp_p, &status, &command, 8);

    if (res != 0) {
        return (res);
    }

    if ((status & STATUS_CPS) == 0) {
        return (-EPROTO);
    }

    command = MCHP_ASSERT_RST;
    res = icsp_soft_data_write(icsp_p, &command, 8);

    if (res != 0) {
        return (res);
    }

    res = send_command(icsp_p, MTAP_SW_ETAP);

    if (res != 0) {
        return (res);
    }

    res = send_command(icsp_p, ETAP_EJTAGBOOT);

    if (res != 0) {
        return (res);
    }

    res = send_command(icsp_p, MTAP_SW_MTAP);

    if (res != 0) {
        return (res);
    }

    res = send_command(icsp_p, MTAP_COMMAND);

    if (res != 0) {
        return (res);
    }

    command = MCHP_DE_ASSERT_RST;
    res = icsp_soft_data_write(icsp_p, &command, 8);

    if (res != 0) {
        return (res);
    }

    res = send_command(icsp_p, MTAP_SW_ETAP);

    return (res);
}

static int upload_ramapp(struct icsp_soft_driver_t *icsp_p)
{
    int res;
    size_t i;

    res = 0;

    for (i = 0; i < membersof(ramapp_upload_instructions); i++) {
        res = xfer_instruction(icsp_p, ramapp_upload_instructions[i]);

        if (res != 0) {
            break;
        }
    }

    /* Start the uploaded application. */
    if (res == 0) {
        res = xfer_instruction(icsp_p, 0x00000000);
    }

    return (res);
}

static int read_device_status(struct icsp_soft_driver_t *icsp_p)
{
    int res;
    uint8_t command;
    uint8_t status;

    res = send_command(icsp_p, MTAP_SW_MTAP);

    if (res != 0) {
        return (res);
    }

    res = send_command(icsp_p, MTAP_COMMAND);

    if (res != 0) {
        return (res);
    }

    command = MCHP_STATUS;
    res = icsp_soft_data_transfer(icsp_p, &status, &command, 8);

    if (res != 0) {
        return (res);
    }

    return (status);
}

static int chip_erase(struct icsp_soft_driver_t *icsp_p)
{
    int res;
    uint8_t command;
    uint8_t status;
    struct time_t time;
    struct time_t end_time;

    res = send_command(icsp_p, MTAP_SW_MTAP);

    if (res != 0) {
        return (res);
    }

    res = send_command(icsp_p, MTAP_COMMAND);

    if (res != 0) {
        return (res);
    }

    command = MCHP_ERASE;
    res = icsp_soft_data_transfer(icsp_p, &status, &command, 8);

    if (res != 0) {
        return (res);
    }

    command = MCHP_DE_ASSERT_RST;
    res = icsp_soft_data_transfer(icsp_p, &status, &command, 8);

    if (res != 0) {
        return (res);
    }

    thrd_sleep_ms(10);

    time.seconds = ERASE_TIMEOUT_S;
    time.nanoseconds = 0;

    time_get(&end_time);
    time_add(&end_time, &end_time, &time);

    command = MCHP_STATUS;

    do {
        res = icsp_soft_data_transfer(icsp_p, &status, &command, 8);
        status &= (STATUS_FCBUSY | STATUS_CFGRDY);

        time_get(&time);

        if (time_compare_greater_than_t == time_compare(&time, &end_time)) {
            res = -ETIMEDOUT;
        }
    } while ((status != STATUS_CFGRDY) && (res == 0));

    return (res);
}

/**
 * Read a packet from the ramapp in the PIC.
 *
 * @return Number of read bytes or negative error code.
 */
static ssize_t ramapp_read(struct programmer_t *self_p, uint8_t *buf_p)
{
    int res;
    uint32_t data;
    size_t size;
    size_t number_of_words;
    size_t i;

    /* Read type and size. */
    res = icsp_soft_fast_data_read(&self_p->icsp, &data);

    if (res != 0) {
        return (res);
    }

    buf_p[0] = (data >> 24);
    buf_p[1] = (data >> 16);
    buf_p[2] = (data >> 8);
    buf_p[3] = (data >> 0);

    size = ((buf_p[2] << 8) | buf_p[3]);

    if (size > MAXIMUM_PAYLOAD_SIZE) {
        return (-EPROTO);
    }

    /* Read payload and crc. */
    number_of_words = DIV_CEIL(size + CRC_SIZE, 4);

    for (i = 0; i < number_of_words; i++) {
        res = icsp_soft_fast_data_read(&self_p->icsp, &data);

        if (res != 0) {
            return (res);
        }

        buf_p[4 * i + 4] = (data >> 24);
        buf_p[4 * i + 5] = (data >> 16);
        buf_p[4 * i + 6] = (data >> 8);
        buf_p[4 * i + 7] = (data >> 0);
    }

    return (PAYLOAD_OFFSET + size + CRC_SIZE);
}

/**
 * Write a packet to the ramapp in the PIC.
 *
 * @return zero(0) or negative error code.
 */
static ssize_t ramapp_write(struct programmer_t *self_p,
                            uint8_t *buf_p,
                            size_t size)
{
    uint32_t data;
    size_t number_of_words;
    size_t i;
    int res;

    number_of_words = DIV_CEIL(size, 4);

    for (i = 0; i < number_of_words; i++) {
        data = ((buf_p[4 * i + 0] << 24)
                | (buf_p[4 * i + 1] << 16)
                | (buf_p[4 * i + 2] << 8)
                | (buf_p[4 * i + 3] << 0));

        res = icsp_soft_fast_data_write(&self_p->icsp, data);

        if (res != 0) {
            return (res);
        }
    }

    return (size);
}

static ssize_t handle_ramapp_command(struct programmer_t *self_p,
                                     uint8_t *buf_p,
                                     size_t size)
{
    ssize_t res;

    res = -ENOTCONN;

    if (self_p->is_connected) {
        res = ramapp_write(self_p, buf_p, size);

        if (res != size) {
            return (res);
        }

        res = ramapp_read(self_p, buf_p);
    }

    return (res);
}

static ssize_t handle_connect(struct programmer_t *self_p,
                              uint8_t *buf_p,
                              size_t size)
{
    int res;

    res = 0;

    if (self_p->is_connected) {
        return (-EISCONN);
    }

    icsp_soft_init(&self_p->icsp,
                   &pin_pgec_dev,
                   &pin_pged_dev,
                   &pin_mclrn_dev);
    icsp_soft_start(&self_p->icsp);

    res = enter_serial_execution_mode(&self_p->icsp);

    if (res != 0) {
        return (-EENTERSERIALEXECUTIONMODE);
    }

    res = upload_ramapp(&self_p->icsp);

    if (res != 0) {
        return (-ERAMAPPUPLOAD);
    }

    res = send_command(&self_p->icsp, ETAP_FASTDATA);

    if (res != 0) {
        return (res);
    }

    self_p->is_connected = 1;

    return (res);
}

static ssize_t handle_disconnect(struct programmer_t *self_p,
                                 uint8_t *buf_p,
                                 size_t size)
{
    if (!self_p->is_connected) {
        return (-ENOTCONN);
    }

    icsp_soft_stop(&self_p->icsp);

    self_p->is_connected = 0;

    return (0);
}

static ssize_t handle_reset(struct programmer_t *self_p,
                            uint8_t *buf_p,
                            size_t size)
{
    struct pin_driver_t mclrn;

    if (self_p->is_connected) {
        return (-EISCONN);
    }

    pin_init(&mclrn, &pin_mclrn_dev, PIN_OUTPUT);
    pin_write(&mclrn, 0);
    time_busy_wait_us(2);
    pin_set_mode(&mclrn, PIN_INPUT);
    thrd_sleep_ms(20);

    return (0);
}

static ssize_t handle_device_status(struct programmer_t *self_p,
                                    uint8_t *buf_p,
                                    size_t size)
{
    int status;

    if (self_p->is_connected) {
        return (-EISCONN);
    }

    icsp_soft_init(&self_p->icsp,
                   &pin_pgec_dev,
                   &pin_pged_dev,
                   &pin_mclrn_dev);
    icsp_soft_start(&self_p->icsp);

    status = read_device_status(&self_p->icsp);

    icsp_soft_stop(&self_p->icsp);

    buf_p[4] = bits_reverse_8(status);
    status = 1;

    return (status);
}

static ssize_t handle_chip_erase(struct programmer_t *self_p,
                                 uint8_t *buf_p,
                                 size_t size)
{
    int res;

    if (self_p->is_connected) {
        return (-EISCONN);
    }

    icsp_soft_init(&self_p->icsp,
                   &pin_pgec_dev,
                   &pin_pged_dev,
                   &pin_mclrn_dev);
    icsp_soft_start(&self_p->icsp);

    res = chip_erase(&self_p->icsp);

    icsp_soft_stop(&self_p->icsp);

    return (res);
}

static ssize_t handle_version(uint8_t *buf_p, size_t size)
{
    strcpy((char *)&buf_p[4], VERSION_STR);

    return (strlen((char *)&buf_p[4]));
}

static ssize_t handle_fast_write(struct programmer_t *self_p,
                                 uint8_t *buf_p,
                                 size_t size)
{
    uint16_t response;
    int res;
    struct time_t timeout;

    if (!self_p->is_connected) {
        return (-ENOTCONN);
    }

    if (size != PACKET_FAST_WRITE_REQUEST_SIZE) {
        return (-EMSGSIZE);
    }

    size = ((buf_p[8] << 24)
            | (buf_p[9] << 16)
            | (buf_p[10] << 8)
            | (buf_p[11] << 0));

    if ((size % PACKET_FAST_WRITE_DATA_SIZE) != 0) {
        return (-EINVAL);
    }

    if (size == 0) {
        return (-EINVAL);
    }

    /* Forward the request to the PIC. */
    res = ramapp_write(self_p, buf_p, PACKET_FAST_WRITE_REQUEST_SIZE);

    if (res != PACKET_FAST_WRITE_REQUEST_SIZE) {
        return (res);
    }

    /* Perform data transfer. */
    response = 0;
    timeout.seconds = 0;
    timeout.nanoseconds = CTRL_TIMEOUT_NS;

    while (size > 0) {
        res = chan_read_with_timeout(sys_get_stdin(),
                                     &buf_p[4],
                                     PACKET_FAST_WRITE_DATA_SIZE,
                                     &timeout);

        if (res != PACKET_FAST_WRITE_DATA_SIZE) {
            return (-ETIMEDOUT);
        }

        res = ramapp_write(self_p, &buf_p[4], PACKET_FAST_WRITE_DATA_SIZE);

        if (res != PACKET_FAST_WRITE_DATA_SIZE) {
            return (res);
        }

        chan_write(sys_get_stdout(), &response, sizeof(response));
        size -= PACKET_FAST_WRITE_DATA_SIZE;
    }

    return (ramapp_read(self_p, buf_p));
}

static ssize_t prepare_command_response(uint8_t *buf_p,
                                        ssize_t size)
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

    return (size);
}

static ssize_t handle_programmer_command(struct programmer_t *self_p,
                                         int type,
                                         uint8_t *buf_p,
                                         size_t size)
{
    ssize_t res;
    uint16_t actual_crc;
    uint16_t expected_crc;

    res = -EBADCRC;
    actual_crc = ((buf_p[size - CRC_SIZE] << 8)
                  | buf_p[size - CRC_SIZE + 1]);
    expected_crc = crc_ccitt(0xffff, &buf_p[0], size - CRC_SIZE);

    if (actual_crc == expected_crc) {
        switch (type) {

        case COMMAND_TYPE_PING:
            res = 0;
            break;

        case COMMAND_TYPE_CONNECT:
            res = handle_connect(self_p, buf_p, size);
            break;

        case COMMAND_TYPE_DISCONNECT:
            res = handle_disconnect(self_p, buf_p, size);
            break;

        case COMMAND_TYPE_RESET:
            res = handle_reset(self_p, buf_p, size);
            break;

        case COMMAND_TYPE_DEVICE_STATUS:
            res = handle_device_status(self_p, buf_p, size);
            break;

        case COMMAND_TYPE_CHIP_ERASE:
            res = handle_chip_erase(self_p, buf_p, size);
            break;

        case COMMAND_TYPE_FAST_WRITE:
            res = handle_fast_write(self_p, buf_p, size);

            if (res >= 0) {
                return (res);
            }

            break;

        case COMMAND_TYPE_VERSION:
            res = handle_version(buf_p, size);
            break;

        default:
            res = -1;
            break;
        }
    }

#if defined(UNIT_TEST)
    else {
        std_printf(OSTR("actual_crc: 0x%04x, expected_crc: 0x%04x\r\n"),
                   actual_crc,
                   expected_crc);
    }
#endif

    return (prepare_command_response(buf_p, res));
}

static ssize_t handle_command(struct programmer_t *self_p,
                              uint8_t *buf_p,
                              size_t size)
{
    ssize_t res;
    int type;

    type = ((buf_p[0] << 8) | buf_p[1]);

    if (type < 100) {
        res = handle_ramapp_command(self_p, buf_p, size);

        if (res < 0) {
            res = prepare_command_response(buf_p, res);
        }
    } else {
        res = handle_programmer_command(self_p, type, buf_p, size);
    }

    return (res);
}

static ssize_t read_command_request(uint8_t *buf_p)
{
    ssize_t size;
    ssize_t res;
    struct time_t timeout;

    timeout.seconds = 0;
    timeout.nanoseconds = CTRL_TIMEOUT_NS;

    /* Read type and size. */
    res = chan_read_with_timeout(sys_get_stdin(),
                                 &buf_p[0],
                                 PAYLOAD_OFFSET,
                                 &timeout);

    if (res != PAYLOAD_OFFSET) {
        return (-ETIMEDOUT);
    }

    size = ((buf_p[2] << 8) | buf_p[3]);

    if (size > MAXIMUM_PAYLOAD_SIZE) {
        return (-EMSGSIZE);
    }

    /* Read payload and crc. */
    res = chan_read_with_timeout(sys_get_stdin(),
                                 &buf_p[4],
                                 size + CRC_SIZE,
                                 &timeout);

    if (res != (size + CRC_SIZE)) {
        return (-ETIMEDOUT);
    }

    return (PAYLOAD_OFFSET + size + CRC_SIZE);
}

static int write_command_response(uint8_t *buf_p, ssize_t size)
{
    chan_write(sys_get_stdout(), &buf_p[0], size);

    return (0);
}

int programmer_init(struct programmer_t *self_p)
{
    self_p->is_connected = 0;

    return (0);
}

int programmer_process_packet(struct programmer_t *self_p)
{
    ssize_t size;
    uint8_t buf[PAYLOAD_OFFSET + MAXIMUM_PAYLOAD_SIZE + CRC_SIZE + 2];

    size = read_command_request(&buf[0]);

    if (size >= 0) {
        size = handle_command(self_p, &buf[0], size);

        if (size > 0) {
            size = write_command_response(&buf[0], size);
        }
    }

    return (size);
}
