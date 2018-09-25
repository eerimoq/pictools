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
#include "../programmer.h"
#include "kernel/time_mock.h"
#include "sync/chan_mock.h"
#include "drivers/basic/pin_mock.h"
#include "drivers/network/icsp_soft_mock.h"

static const uint32_t ramapp_upload_instructions[] = {
#include "../ramapp_upload_instructions.i"
};

static int write_send_command(uint8_t command)
{
    mock_write_icsp_soft_instruction_write(&command, 5, 0);

    return (0);
}

static int write_enter_serial_execution_mode(void)
{
    uint8_t command;
    uint8_t status;

    write_send_command(0x20);
    write_send_command(0xe0);

    command = 0;
    status = 0xff;
    mock_write_icsp_soft_data_transfer(&status, &command, 8, 0);

    command = 0x8b;
    mock_write_icsp_soft_data_write(&command, 8, 0);

    write_send_command(0xa0);
    write_send_command(0x30);
    write_send_command(0x20);
    write_send_command(0xe0);

    command = 0x0b;
    mock_write_icsp_soft_data_write(&command, 8, 0);

    write_send_command(0xa0);

    return (0);
}

static int write_xfer_data_32(uint32_t request,
                              uint32_t response)
{
    request = htonl(request);

    mock_write_icsp_soft_data_transfer((uint8_t *)&response,
                                       (uint8_t *)&request,
                                       32,
                                       0);

    return (0);
}

static int write_xfer_instruction(uint32_t instruction)
{
    struct time_t time;

    write_send_command(0x50);

    time.seconds = 0;
    time.nanoseconds = 500000000;
    mock_write_time_get(&time, 0);
    write_xfer_data_32(0x0032000, 0xfffffff);
    mock_write_time_get(&time, 0);

    write_send_command(0x90);
    write_xfer_data_32(bits_reverse_32(instruction), 0);
    write_send_command(0x50);
    write_xfer_data_32(0x0030000, 0);

    return (0);
}

static int write_upload_ramapp(void)
{
    size_t i;

    for (i = 0; i < membersof(ramapp_upload_instructions); i++) {
        write_xfer_instruction(ramapp_upload_instructions[i]);
    }

    write_xfer_instruction(0);

    return (0);
}

static int write_read_command_request(uint8_t *header_p,
                                      size_t header_size,
                                      uint8_t *payload_crc_p,
                                      size_t payload_crc_size)
{
    struct time_t time;

    time.seconds = 0;
    time.nanoseconds = 500000000;
    mock_write_chan_read_with_timeout(header_p,
                                      header_size,
                                      &time,
                                      header_size);
    mock_write_chan_read_with_timeout(payload_crc_p,
                                      payload_crc_size,
                                      &time,
                                      payload_crc_size);

    return (0);
}

static int write_handle_connect(void)
{
    mock_write_icsp_soft_init(&pin_d2_dev,
                              &pin_d3_dev,
                              &pin_d4_dev,
                              0);
    mock_write_icsp_soft_start(0);
    write_enter_serial_execution_mode();
    write_upload_ramapp();
    write_send_command(0x70);

    return (0);
}

static int write_programmer_process_packet(uint8_t *header_p,
                                           size_t header_size,
                                           uint8_t *payload_crc_p,
                                           size_t payload_crc_size,
                                           uint8_t *response_p,
                                           size_t response_size)
{
    write_read_command_request(header_p,
                               header_size,
                               payload_crc_p,
                               payload_crc_size);
    mock_write_chan_write(response_p,
                          response_size,
                          response_size);

    return (0);
}

static int connect(struct programmer_t *programmer_p)
{
    uint8_t request_header[] = { 0x00, 0x65, 0x00, 0x00 };
    uint8_t request_crc[] = { 0xf4, 0x5b };
    uint8_t response[] = { 0x00, 0x65, 0x00, 0x00, 0xf4, 0x5b };

    BTASSERT(programmer_init(programmer_p) == 0);

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_crc[0],
                                    sizeof(request_crc),
                                    &response[0],
                                    sizeof(response));
    write_handle_connect();

    BTASSERTI(programmer_process_packet(programmer_p), ==, 0);

    return (0);
}

static int write_ramapp_write(uint8_t *buf_p, size_t size)
{
    size_t offset;

    for (offset = 0; offset < size; offset += 4) {
        if ((size - offset) >= 4) {
            mock_write_icsp_soft_fast_data_write(&buf_p[offset],
                                                 4,
                                                 0);
        } else {
            mock_write_icsp_soft_fast_data_write(&buf_p[offset],
                                                 size - offset,
                                                 0);
        }
    }

    return (0);
}

static int write_ramapp_read(uint8_t *buf_p, size_t size)
{
    size_t i;
    uint32_t data;

    for (i = 0; i < size / 4; i++) {
        data = ((buf_p[4 * i + 0] << 24)
                | (buf_p[4 * i + 1] << 16)
                | (buf_p[4 * i + 2] << 8)
                | (buf_p[4 * i + 3] << 0));
        mock_write_icsp_soft_fast_data_read(&data, 0);
    }

    switch (size % 4) {

    case 3:
        data = ((buf_p[4 * i] << 24)
                | (buf_p[4 * i + 1] << 16)
                | (buf_p[4 * i + 2] << 8));
        mock_write_icsp_soft_fast_data_read(&data, 0);
        break;

    case 2:
        data = ((buf_p[4 * i] << 24)
                | (buf_p[4 * i + 1] << 16));
        mock_write_icsp_soft_fast_data_read(&data, 0);
        break;

    case 1:
        data = (buf_p[4 * i] << 24);
        mock_write_icsp_soft_fast_data_read(&data, 0);
        break;

    default:
        break;
    }

    return (0);
}

static int write_chip_erase(void)
{
    struct time_t time;
    uint8_t command;
    uint8_t status;

    write_send_command(0x20);
    write_send_command(0xe0);

    command = 0x3f;
    status = 0xff;
    mock_write_icsp_soft_data_transfer(&status, &command, 8, 0);

    command = 0x0b;
    status = 0x10;
    mock_write_icsp_soft_data_transfer(&status, &command, 8, 0);

    time.seconds = 1;
    time.nanoseconds = 0;

    mock_write_time_get(&time, 0);

    command = 0;
    status = 0x10;
    mock_write_icsp_soft_data_transfer(&status, &command, 8, 0);

    mock_write_time_get(&time, 0);

    return (0);
}

static int write_read_device_status(uint8_t status)
{
    uint8_t command;

    write_send_command(0x20);
    write_send_command(0xe0);

    command = 0;
    mock_write_icsp_soft_data_transfer(&status, &command, 8, 0);

    return (0);
}

static int test_ping(void)
{
    uint8_t request_header[] = { 0x00, 0x64, 0x00, 0x00 };
    uint8_t request_crc[] = { 0xc3, 0x6b };
    uint8_t response[] = { 0x00, 0x64, 0x00, 0x00, 0xc3, 0x6b };
    struct programmer_t programmer;

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_crc[0],
                                    sizeof(request_crc),
                                    &response[0],
                                    sizeof(response));

    BTASSERT(programmer_init(&programmer) == 0);
    BTASSERTI(programmer_process_packet(&programmer), ==, 0);

    return (0);
}

static int test_connect(void)
{
    struct programmer_t programmer;

    BTASSERT(connect(&programmer) == 0);

    return (0);
}

static int test_connect_connected(void)
{
    struct programmer_t programmer;
    uint8_t request_header[] = { 0x00, 0x65, 0x00, 0x00 };
    uint8_t request_crc[] = { 0xf4, 0x5b };
    uint8_t response[] = {
        0xff, 0xff, 0x00, 0x04, 0xff, 0xff, 0xff, 0x96, 0xed, 0x46
    };

    BTASSERT(connect(&programmer) == 0);

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_crc[0],
                                    sizeof(request_crc),
                                    &response[0],
                                    sizeof(response));

    BTASSERTI(programmer_process_packet(&programmer), ==, 0);

    return (0);
}

static int test_disconnect(void)
{
    struct programmer_t programmer;
    uint8_t request_header[] = { 0x00, 0x66, 0x00, 0x00 };
    uint8_t request_crc[] = { 0xad, 0x0b };
    uint8_t response[] = { 0x00, 0x66, 0x00, 0x00, 0xad, 0x0b };

    BTASSERT(connect(&programmer) == 0);

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_crc[0],
                                    sizeof(request_crc),
                                    &response[0],
                                    sizeof(response));
    mock_write_icsp_soft_stop(0);

    BTASSERTI(programmer_process_packet(&programmer), ==, 0);

    return (0);
}

static int test_disconnect_not_connected(void)
{
    struct programmer_t programmer;
    uint8_t request_header[] = { 0x00, 0x66, 0x00, 0x00 };
    uint8_t request_crc[] = { 0xad, 0x0b };
    uint8_t response[] = {
        0xff, 0xff, 0x00, 0x04,
        0xff, 0xff, 0xff, 0x95, /* ENOTCONN. */
        0xdd, 0x25
    };

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_crc[0],
                                    sizeof(request_crc),
                                    &response[0],
                                    sizeof(response));

    BTASSERT(programmer_init(&programmer) == 0);
    BTASSERTI(programmer_process_packet(&programmer), ==, 0);

    return (0);
}

static int test_reset(void)
{
    struct programmer_t programmer;
    uint8_t request_header[] = { 0x00, 0x67, 0x00, 0x00 };
    uint8_t request_crc[] = { 0x9a, 0x3b };
    uint8_t response[] = { 0x00, 0x67, 0x00, 0x00, 0x9a, 0x3b };

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_crc[0],
                                    sizeof(request_crc),
                                    &response[0],
                                    sizeof(response));

    mock_write_pin_init(&pin_d4_dev, PIN_OUTPUT, 0);
    mock_write_pin_write(0, 0);
    mock_write_pin_set_mode(PIN_INPUT, 0);

    BTASSERT(programmer_init(&programmer) == 0);
    BTASSERTI(programmer_process_packet(&programmer), ==, 0);

    return (0);
}

static int test_bad_command_type(void)
{
    struct programmer_t programmer;
    uint8_t request_header[] = { 0x99, 0x99, 0x00, 0x00 };
    uint8_t request_crc[] = { 0x57, 0x80 };
    uint8_t response[] = {
        0xff, 0xff, 0x00, 0x04, 0xff, 0xff, 0xff, 0xff, 0x10, 0xc9
    };

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_crc[0],
                                    sizeof(request_crc),
                                    &response[0],
                                    sizeof(response));

    BTASSERT(programmer_init(&programmer) == 0);
    BTASSERTI(programmer_process_packet(&programmer), ==, 0);

    return (0);
}

static int test_bad_command_crc(void)
{
    struct programmer_t programmer;
    uint8_t request_header[] = { 0x99, 0x99, 0x00, 0x00 };
    uint8_t request_crc[] = { 0x57, 0x81 };
    uint8_t response[] = {
        0xff, 0xff, 0x00, 0x04, 0xff, 0xff, 0xfc, 0x11, 0x59, 0x7a
    };

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_crc[0],
                                    sizeof(request_crc),
                                    &response[0],
                                    sizeof(response));

    BTASSERT(programmer_init(&programmer) == 0);
    BTASSERTI(programmer_process_packet(&programmer), ==, 0);

    return (0);
}

static int test_command_read_header_timeout(void)
{
    struct programmer_t programmer;
    uint8_t request_header[] = { 0x99, 0x99, 0x00, 0x00 };
    struct time_t time;

    time.seconds = 0;
    time.nanoseconds = 500000000;
    mock_write_chan_read_with_timeout(&request_header[0],
                                      sizeof(request_header),
                                      &time,
                                      -ETIMEDOUT);
    BTASSERT(programmer_init(&programmer) == 0);
    BTASSERTI(programmer_process_packet(&programmer), ==, -ETIMEDOUT);

    return (0);
}

static int test_command_read_crc_timeout(void)
{
    struct programmer_t programmer;
    uint8_t request_header[] = { 0x00, 0x99, 0x00, 0x00 };
    uint8_t request_crc[] = { 0x00, 0x00 };
    struct time_t time;

    time.seconds = 0;
    time.nanoseconds = 500000000;
    mock_write_chan_read_with_timeout(&request_header[0],
                                      sizeof(request_header),
                                      &time,
                                      sizeof(request_header));
    mock_write_chan_read_with_timeout(&request_crc[0],
                                      sizeof(request_crc),
                                      &time,
                                      -ETIMEDOUT);

    BTASSERT(programmer_init(&programmer) == 0);
    BTASSERTI(programmer_process_packet(&programmer), ==, -ETIMEDOUT);

    return (0);
}

static int test_ramapp_command(void)
{
    struct programmer_t programmer;
    uint8_t request[] = { 0x00, 0x01, 0x00, 0x00, 0x57, 0x80 };
    uint8_t response[] = {
        0x00, 0x01, 0x00, 0x00, 0x59, 0x7a
    };

    BTASSERT(connect(&programmer) == 0);

    write_read_command_request(&request[0],
                               4,
                               &request[4],
                               2);
    write_ramapp_write(&request[0], sizeof(request));
    write_ramapp_read(&response[0], sizeof(response));
    mock_write_chan_write(&response[0],
                          sizeof(response),
                          sizeof(response));

    BTASSERTI(programmer_process_packet(&programmer), ==, 0);

    return (0);
}

static int test_ramapp_command_fast_data_write_fail(void)
{
    struct programmer_t programmer;
    uint8_t request[] = { 0x00, 0x01, 0x00, 0x00, 0x57, 0x80 };
    uint8_t response[] = {
        0xff, 0xff, 0x00, 0x04, 0xff, 0xff, 0xff, 0xfe, 0x00, 0xe8
    };

    BTASSERT(connect(&programmer) == 0);

    write_read_command_request(&request[0],
                               4,
                               &request[4],
                               2);
    mock_write_icsp_soft_fast_data_write(&request[0],
                                         4,
                                         -2);
    mock_write_chan_write(&response[0],
                          sizeof(response),
                          sizeof(response));

    BTASSERTI(programmer_process_packet(&programmer), ==, 0);

    return (0);
}

static int test_chip_erase(void)
{
    struct programmer_t programmer;
    uint8_t request_header[] = { 0x00, 0x69, 0x00, 0x00 };
    uint8_t request_crc[] = { 0x81, 0x3a };
    uint8_t response[] = {
        0x00, 0x69, 0x00, 0x00, 0x81, 0x3a
    };

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_crc[0],
                                    sizeof(request_crc),
                                    &response[0],
                                    sizeof(response));

    mock_write_icsp_soft_init(&pin_d2_dev,
                              &pin_d3_dev,
                              &pin_d4_dev,
                              0);
    mock_write_icsp_soft_start(0);

    write_chip_erase();

    mock_write_icsp_soft_stop(0);

    BTASSERT(programmer_init(&programmer) == 0);
    BTASSERTI(programmer_process_packet(&programmer), ==, 0);

    return (0);
}

static int test_version(void)
{
    struct programmer_t programmer;
    uint8_t request_header[] = { 0x00, 0x6b, 0x00, 0x00 };
    uint8_t request_crc[] = { 0xef, 0x5a };
    uint8_t response[] = {
        0x00, 0x6b, 0x00, 0x0a,
        '0', '.', '1', '.', '2', '-', 't', 'e', 's', 't',
        0x2a, 0x75
    };

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_crc[0],
                                    sizeof(request_crc),
                                    &response[0],
                                    sizeof(response));

    BTASSERT(programmer_init(&programmer) == 0);
    BTASSERTI(programmer_process_packet(&programmer), ==, 0);

    return (0);
}

static int test_fast_write(void)
{
    struct programmer_t programmer;
    uint8_t request[] = {
        0x00, 0x6a, 0x00, 0x0c,
        0x1d, 0x00, 0x00, 0x00, /* Address. */
        0x00, 0x00, 0x01, 0x00, /* Size. */
        0x12, 0x34, 0x56, 0x78, /* Crc. */
        0x6a, 0xa9
    };
    uint8_t response[] = {
        0x00, 0x6a, 0x00, 0x00, 0x00, 0x00
    };
    uint8_t buf[256];
    struct time_t time;
    uint16_t ack;

    BTASSERT(connect(&programmer) == 0);

    /* Request to ramapp. */
    write_ramapp_write(&request[0], sizeof(request));
    write_read_command_request(&request[0],
                               4,
                               &request[4],
                               14);
    time.seconds = 0;
    time.nanoseconds = 500000000;

    /* Data packet. */
    memset(&buf[0], 1, sizeof(buf));
    mock_write_chan_read_with_timeout(&buf[0],
                                      sizeof(buf),
                                      &time,
                                      sizeof(buf));
    write_ramapp_write(&buf[0], sizeof(buf));
    ack = 0;
    mock_write_chan_write(&ack, sizeof(ack), sizeof(ack));

    /* Response from ramapp. */
    write_ramapp_read(&response[0], sizeof(response));

    mock_write_chan_write(&response[0],
                          sizeof(response),
                          sizeof(response));

    BTASSERTI(programmer_process_packet(&programmer), ==, 0);

    return (0);
}

static int test_fast_write_not_connected(void)
{
    struct programmer_t programmer;
    uint8_t request_header[] = { 0x00, 0x6a, 0x00, 0x0c };
    uint8_t request_payload_crc[] = {
        0x1d, 0x00, 0x00, 0x00, /* Address. */
        0x00, 0x00, 0x01, 0x00, /* Size. */
        0x12, 0x34, 0x56, 0x78, /* Crc. */
        0x6a, 0xa9
    };
    uint8_t response[] = {
        0xff, 0xff, 0x00, 0x04,
        0xff, 0xff, 0xff, 0x95, /* ENOTCONN. */
        0xdd, 0x25
    };

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_payload_crc[0],
                                    sizeof(request_payload_crc),
                                    &response[0],
                                    sizeof(response));

    BTASSERT(programmer_init(&programmer) == 0);
    BTASSERTI(programmer_process_packet(&programmer), ==, 0);

    return (0);
}

static int test_device_status(void)
{
    struct programmer_t programmer;
    uint8_t request_header[] = { 0x00, 0x68, 0x00, 0x00 };
    uint8_t request_crc[] = { 0xb6, 0x0a };
    uint8_t response[] = {
        0x00, 0x68, 0x00, 0x01,
        0x48, /* Status. */
        0x37, 0xe0
    };

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_crc[0],
                                    sizeof(request_crc),
                                    &response[0],
                                    sizeof(response));

    mock_write_icsp_soft_init(&pin_d2_dev,
                              &pin_d3_dev,
                              &pin_d4_dev,
                              0);
    mock_write_icsp_soft_start(0);

    write_read_device_status(0x12);

    mock_write_icsp_soft_stop(0);

    BTASSERT(programmer_init(&programmer) == 0);
    BTASSERTI(programmer_process_packet(&programmer), ==, 0);

    return (0);
}

int main()
{
    struct harness_testcase_t testcases[] = {
        { test_ping, "test_ping" },
        { test_connect, "test_connect" },
        { test_connect_connected, "test_connect_connected" },
        { test_disconnect, "test_disconnect" },
        { test_disconnect_not_connected, "test_disconnect_not_connected" },
        { test_reset, "test_reset" },
        { test_bad_command_type, "test_bad_command_type" },
        { test_bad_command_crc, "test_bad_command_crc" },
        { test_command_read_header_timeout, "test_command_read_header_timeout" },
        { test_command_read_crc_timeout, "test_command_read_crc_timeout" },
        { test_ramapp_command, "test_ramapp_command" },
        {
            test_ramapp_command_fast_data_write_fail,
            "test_ramapp_command_fast_data_write_fail"
        },
        { test_chip_erase, "test_chip_erase" },
        { test_version, "test_version" },
        { test_fast_write, "test_fast_write" },
        { test_fast_write_not_connected, "test_fast_write_not_connected" },
        { test_device_status, "test_device_status" },
        { NULL, NULL }
    };

    sys_start();

    harness_run(testcases);

    return (0);
}
