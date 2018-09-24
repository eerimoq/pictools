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
                                      uint8_t *crc_p)
{
    struct time_t time;

    time.seconds = 0;
    time.nanoseconds = 500000000;
    mock_write_chan_read_with_timeout(header_p,
                                      header_size,
                                      &time,
                                      header_size);
    mock_write_chan_read_with_timeout(crc_p, 2, &time, 2);

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
                                           uint8_t *crc_p,
                                           uint8_t *response_p,
                                           size_t response_size)
{
    write_read_command_request(header_p,
                               header_size,
                               crc_p);
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

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_crc[0],
                                    &response[0],
                                    sizeof(response));
    write_handle_connect();

    BTASSERTI(programmer_process_packet(programmer_p), ==, 0);

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
                                    &response[0],
                                    sizeof(response));

    BTASSERT(programmer_init(&programmer) == 0);
    BTASSERTI(programmer_process_packet(&programmer), ==, 0);

    return (0);
}

static int test_connect(void)
{
    struct programmer_t programmer;

    BTASSERT(programmer_init(&programmer) == 0);
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

    BTASSERT(programmer_init(&programmer) == 0);
    BTASSERT(connect(&programmer) == 0);

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_crc[0],
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

    BTASSERT(programmer_init(&programmer) == 0);
    BTASSERT(connect(&programmer) == 0);

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_crc[0],
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
        0xff, 0xff, 0x00, 0x04, 0xff, 0xff, 0xff, 0x95, 0xdd, 0x25
    };

    write_programmer_process_packet(&request_header[0],
                                    sizeof(request_header),
                                    &request_crc[0],
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
        { NULL, NULL }
    };

    sys_start();

    harness_run(testcases);

    return (0);
}
