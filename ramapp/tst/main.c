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
#include "../ramapp.h"
#include "drivers/storage/flash_mock.h"

uint32_t etap_fast_data_read(void)
{
    uint32_t value;

    harness_mock_read("etap_fast_data_read(): return (res)",
                      &value,
                      sizeof(value));

    return (value);
}

static void write_etap_fast_data_read(uint32_t value)
{
    harness_mock_write("etap_fast_data_read(): return (res)",
                       &value,
                       sizeof(value));
}

void etap_fast_data_write(uint32_t value)
{
    size_t shift;

    harness_mock_read("etap_fast_data_write(shift)",
                      &shift,
                      sizeof(shift));

    value >>= shift;

    harness_mock_assert("etap_fast_data_write(value)",
                        &value,
                        sizeof(value));
}

static void write_etap_fast_data_write(uint32_t value, size_t shift)
{
    harness_mock_write("etap_fast_data_write(shift)",
                       &shift,
                       sizeof(shift));

    harness_mock_write("etap_fast_data_write(value)",
                       &value,
                       sizeof(value));
}

uint8_t load_flash_8(uint32_t address, size_t index)
{
    uint8_t res;

    harness_mock_assert("load_flash_8(address)",
                        &address,
                        sizeof(address));

    harness_mock_assert("load_flash_8(index)",
                        &index,
                        sizeof(index));

    harness_mock_read("load_flash_8(): return (res)",
                      &res,
                      sizeof(res));

    return (res);
}

static void write_load_flash_8(uint32_t address, size_t index, uint8_t res)
{
    harness_mock_write("load_flash_8(address)",
                       &address,
                       sizeof(address));

    harness_mock_write("load_flash_8(index)",
                       &index,
                       sizeof(index));

    harness_mock_write("load_flash_8(): return (res)",
                       &res,
                       sizeof(res));
}

uint32_t load_flash_32(uint32_t address, size_t index)
{
    uint32_t res;

    harness_mock_assert("load_flash_32(address)",
                        &address,
                        sizeof(address));

    harness_mock_assert("load_flash_32(index)",
                        &index,
                        sizeof(index));

    harness_mock_read("load_flash_32(): return (res)",
                      &res,
                      sizeof(res));

    return (res);
}

static void write_load_flash_32(uint32_t address, size_t index, uint32_t res)
{
    harness_mock_write("load_flash_32(address)",
                       &address,
                       sizeof(address));

    harness_mock_write("load_flash_32(index)",
                       &index,
                       sizeof(index));

    harness_mock_write("load_flash_32(): return (res)",
                       &res,
                       sizeof(res));
}

static void write_cmp32(uint8_t *buf_p, uint32_t address, size_t size)
{
    size_t i;
    uint32_t data;

    for (i = 0; i < size / 4; i++) {
        data = *(uint32_t *)(&buf_p[4 * i]);
        write_load_flash_32(address, i, data);
    }
}

int cmp8(void *buf_p, uint32_t address, size_t size)
{
    int res;

    harness_mock_assert("cmp8(size)", &size, sizeof(size));
    harness_mock_assert("cmp8(address)", &address, sizeof(address));
    harness_mock_assert("cmp8(buf_p)", buf_p, size);
    harness_mock_read("cmp8(): return (res)",
                      &res,
                      sizeof(res));

    return (res);
}

static void write_cmp8(void *buf_p, uint32_t address, size_t size, int res)
{
    harness_mock_write("cmp8(buf_p)", buf_p, size);
    harness_mock_write("cmp8(address)", &address, sizeof(address));
    harness_mock_write("cmp8(size)", &size, sizeof(size));
    harness_mock_write("cmp8(): return (res)", &res, sizeof(res));
}

static void write_fast_data_read(uint8_t *buf_p, size_t size)
{
    uint32_t data;
    size_t number_of_words;
    size_t i;

    number_of_words = DIV_CEIL(size, 4);

    for (i = 0; i < number_of_words; i++) {
        data = ((buf_p[4 * i + 0] << 24)
                | (buf_p[4 * i + 1] << 16)
                | (buf_p[4 * i + 2] << 8)
                | (buf_p[4 * i + 3]));
        write_etap_fast_data_read(data);
    }
}

static void write_fast_data_write(uint8_t *buf_p, size_t size)
{
    size_t i;
    uint32_t data;

    for (i = 0; i < size / 4; i++) {
        data = ((buf_p[4 * i + 0] << 24)
                | (buf_p[4 * i + 1] << 16)
                | (buf_p[4 * i + 2] << 8)
                | (buf_p[4 * i + 3] << 0));
        write_etap_fast_data_write(data, 0);
    }

    switch (size % 4) {

    case 3:
        data = ((buf_p[4 * i] << 16)
                | (buf_p[4 * i + 1] << 8)
                | (buf_p[4 * i + 2] << 0));
        write_etap_fast_data_write(data, 8);
        break;

    case 2:
        data = ((buf_p[4 * i] << 8)
                | (buf_p[4 * i + 1] << 0));
        write_etap_fast_data_write(data, 16);
        break;

    case 1:
        data = (buf_p[4 * i] << 0);
        write_etap_fast_data_write(data, 24);
        break;

    default:
        break;
    }
}

static void write_read_command_request(uint8_t *header_p,
                                       uint8_t *payload_crc_p,
                                       size_t payload_crc_size)
{
    write_fast_data_read(header_p, 4);
    write_fast_data_read(payload_crc_p, payload_crc_size);
}

static void write_write_command_response(uint8_t *buf_p, ssize_t size)
{
    write_fast_data_write(buf_p, size);
}

static int test_ping(void)
{
    struct ramapp_t ramapp;
    struct flash_driver_t flash;
    uint8_t request_header[] = { 0x00, 0x01, 0x00, 0x00 };
    uint8_t request_crc[] = { 0xb3, 0xf0 };
    uint8_t response[] = { 0x00, 0x01, 0x00, 0x00, 0xb3, 0xf0 };

    write_read_command_request(&request_header[0],
                               &request_crc[0],
                               sizeof(request_crc));
    write_write_command_response(&response[0],
                                 sizeof(response));

    BTASSERT(ramapp_init(&ramapp, &flash) == 0);
    BTASSERT(ramapp_process_packet(&ramapp) == 0);

    return (0);
}

static int test_erase(void)
{
    struct ramapp_t ramapp;
    struct flash_driver_t flash;
    uint8_t request_header[] = { 0x00, 0x02, 0x00, 0x08 };
    uint8_t request_payload_crc[] = {
        0x04, 0x03, 0x02, 0x01, /* Address. */
        0x01, 0x02, 0x03, 0x04, /* Size. */
        0xf6, 0x48
    };
    uint8_t response[] = { 0x00, 0x02, 0x00, 0x00, 0xea, 0xa0 };

    write_read_command_request(&request_header[0],
                               &request_payload_crc[0],
                               sizeof(request_payload_crc));
    mock_write_flash_erase(0x04030201, 0x01020304, 0);
    write_write_command_response(&response[0],
                                 sizeof(response));

    BTASSERT(ramapp_init(&ramapp, &flash) == 0);
    BTASSERT(ramapp_process_packet(&ramapp) == 0);

    return (0);
}

static int test_read(void)
{
    struct ramapp_t ramapp;
    struct flash_driver_t flash;
    uint8_t request_header[] = { 0x00, 0x03, 0x00, 0x08 };
    uint8_t request_payload_crc[] = {
        0x04, 0x03, 0x02, 0x01, /* Address. */
        0x00, 0x00, 0x00, 0x01, /* Size. */
        0x33, 0x23
    };
    uint8_t response[] = {
        0x00, 0x03, 0x00, 0x01,
        0xef, /* Data. */
        0xb5, 0x20
    };

    write_read_command_request(&request_header[0],
                               &request_payload_crc[0],
                               sizeof(request_payload_crc));
    write_load_flash_8(0x04030201, 0, 0xef);
    write_write_command_response(&response[0],
                                 sizeof(response));

    BTASSERT(ramapp_init(&ramapp, &flash) == 0);
    BTASSERT(ramapp_process_packet(&ramapp) == 0);

    return (0);
}

static int test_write(void)
{
    struct ramapp_t ramapp;
    struct flash_driver_t flash;
    uint8_t request_header[] = { 0x00, 0x04, 0x00, 0x09 };
    uint8_t request_payload_crc[] = {
        0x04, 0x03, 0x02, 0x01, /* Address. */
        0x00, 0x00, 0x00, 0x01, /* Size. */
        0xfe, /* Data. */
        0x4c, 0xef
    };
    uint8_t response[] = { 0x00, 0x04, 0x00, 0x00, 0x58, 0x00 };
    uint8_t data;

    write_read_command_request(&request_header[0],
                               &request_payload_crc[0],
                               sizeof(request_payload_crc));
    data = 0xfe;
    mock_write_flash_write(0x04030201,
                           &data,
                           sizeof(data),
                           sizeof(data));
    write_cmp8(&data, 0x04030201, sizeof(data), 0);
    write_write_command_response(&response[0],
                                 sizeof(response));

    BTASSERT(ramapp_init(&ramapp, &flash) == 0);
    BTASSERT(ramapp_process_packet(&ramapp) == 0);

    return (0);
}

static int test_fast_write_one_row(void)
{
    struct ramapp_t ramapp;
    struct flash_driver_t flash;
    uint8_t request_header[] = { 0x00, 0x6a, 0x00, 0x0c };
    uint8_t request_payload_crc[] = {
        0x04, 0x03, 0x02, 0x01, /* Address. */
        0x00, 0x00, 0x01, 0x00, /* Size. */
        0x00, 0x00, 0x8f, 0xd6, /* Crc. */
        0x1c, 0x46
    };
    uint8_t response[] = { 0x00, 0x6a, 0x00, 0x00, 0xd8, 0x6a };
    uint8_t buf[256];

    write_read_command_request(&request_header[0],
                               &request_payload_crc[0],
                               sizeof(request_payload_crc));
    memset(&buf[0], 0x12, sizeof(buf));
    buf[3] = 0x21;
    write_fast_data_read(&buf[0], sizeof(buf));
    mock_write_flash_async_write(0x04030201,
                                 &buf[0],
                                 sizeof(buf),
                                 0);
    mock_write_flash_async_wait(0);
    write_cmp32(&buf[0], 0x04030201, sizeof(buf));
    write_write_command_response(&response[0],
                                 sizeof(response));

    BTASSERT(ramapp_init(&ramapp, &flash) == 0);
    BTASSERT(ramapp_process_packet(&ramapp) == 0);

    return (0);
}

static int test_fast_write_one_row_bad_compare(void)
{
    struct ramapp_t ramapp;
    struct flash_driver_t flash;
    uint8_t request_header[] = { 0x00, 0x6a, 0x00, 0x0c };
    uint8_t request_payload_crc[] = {
        0x04, 0x03, 0x02, 0x01, /* Address. */
        0x00, 0x00, 0x01, 0x00, /* Size. */
        0x00, 0x00, 0x8f, 0xd6, /* Crc. */
        0x1c, 0x46
    };
    uint8_t response[] = {
        0xff, 0xff, 0x00, 0x04,
        0xff, 0xff, 0xfc, 0x10, /* Error code. */
        0x49, 0x5b
    };
    uint8_t buf[256];

    write_read_command_request(&request_header[0],
                               &request_payload_crc[0],
                               sizeof(request_payload_crc));
    memset(&buf[0], 0x12, sizeof(buf));
    buf[3] = 0x22;
    write_fast_data_read(&buf[0], sizeof(buf));
    mock_write_flash_async_write(0x04030201,
                                 &buf[0],
                                 sizeof(buf),
                                 0);
    mock_write_flash_async_wait(0);
    buf[3] = 0x23;
    write_cmp32(&buf[0], 0x04030201, 4);
    write_write_command_response(&response[0],
                                 sizeof(response));

    BTASSERT(ramapp_init(&ramapp, &flash) == 0);
    BTASSERT(ramapp_process_packet(&ramapp) == 0);

    return (0);
}

static int test_fast_write_two_rows(void)
{
    struct ramapp_t ramapp;
    struct flash_driver_t flash;
    uint8_t request_header[] = { 0x00, 0x6a, 0x00, 0x0c };
    uint8_t request_payload_crc[] = {
        0x04, 0x03, 0x02, 0x01, /* Address. */
        0x00, 0x00, 0x02, 0x00, /* Size. */
        0x00, 0x00, 0x18, 0x87, /* Crc. */
        0x19, 0x0e
    };
    uint8_t response[] = { 0x00, 0x6a, 0x00, 0x00, 0xd8, 0x6a };
    uint8_t buf[256];
    int i;

    write_read_command_request(&request_header[0],
                               &request_payload_crc[0],
                               sizeof(request_payload_crc));
    memset(&buf[0], 0x12, sizeof(buf));

    for (i = 0; i < 2; i++) {
        buf[3] = 0x21 + i;
        write_fast_data_read(&buf[0], sizeof(buf));
        mock_write_flash_async_write(0x04030201 + sizeof(buf) * i,
                                     &buf[0],
                                     sizeof(buf),
                                     0);
        mock_write_flash_async_wait(0);
        write_cmp32(&buf[0], 0x04030201 + sizeof(buf) * i, sizeof(buf));
    }

    write_write_command_response(&response[0],
                                 sizeof(response));

    BTASSERT(ramapp_init(&ramapp, &flash) == 0);
    BTASSERT(ramapp_process_packet(&ramapp) == 0);

    return (0);
}

static int test_fast_write_two_rows_bad_crc(void)
{
    struct ramapp_t ramapp;
    struct flash_driver_t flash;
    uint8_t request_header[] = { 0x00, 0x6a, 0x00, 0x0c };
    uint8_t request_payload_crc[] = {
        0x04, 0x03, 0x02, 0x01, /* Address. */
        0x00, 0x00, 0x02, 0x00, /* Size. */
        0x00, 0x00, 0x18, 0x87, /* Crc. */
        0x19, 0x0e
    };
    uint8_t response[] = {
        0xff, 0xff, 0x00, 0x04,
        0xff, 0xff, 0xfc, 0x11, /* Error code. */
        0x59, 0x7a
    };
    uint8_t buf[256];
    int i;

    write_read_command_request(&request_header[0],
                               &request_payload_crc[0],
                               sizeof(request_payload_crc));
    memset(&buf[0], 0x12, sizeof(buf));
    buf[3] = 0x21;

    for (i = 0; i < 2; i++) {
        write_fast_data_read(&buf[0], sizeof(buf));
        mock_write_flash_async_write(0x04030201 + sizeof(buf) * i,
                                     &buf[0],
                                     sizeof(buf),
                                     0);
        mock_write_flash_async_wait(0);
        write_cmp32(&buf[0], 0x04030201 + sizeof(buf) * i, sizeof(buf));
    }

    write_write_command_response(&response[0],
                                 sizeof(response));

    BTASSERT(ramapp_init(&ramapp, &flash) == 0);
    BTASSERT(ramapp_process_packet(&ramapp) == 0);

    return (0);
}

static int test_fast_write_two_rows_bad_compare(void)
{
    struct ramapp_t ramapp;
    struct flash_driver_t flash;
    uint8_t request_header[] = { 0x00, 0x6a, 0x00, 0x0c };
    uint8_t request_payload_crc[] = {
        0x04, 0x03, 0x02, 0x01, /* Address. */
        0x00, 0x00, 0x02, 0x00, /* Size. */
        0x00, 0x00, 0x18, 0x87, /* Crc. */
        0x19, 0x0e
    };
    uint8_t response[] = {
        0xff, 0xff, 0x00, 0x04,
        0xff, 0xff, 0xfc, 0x10, /* Error code. */
        0x49, 0x5b
    };
    uint8_t buf[256];

    write_read_command_request(&request_header[0],
                               &request_payload_crc[0],
                               sizeof(request_payload_crc));
    memset(&buf[0], 0x12, sizeof(buf));
    buf[3] = 0x22;
    write_fast_data_read(&buf[0], sizeof(buf));
    mock_write_flash_async_write(0x04030201,
                                 &buf[0],
                                 sizeof(buf),
                                 0);
    write_fast_data_read(&buf[0], sizeof(buf));
    mock_write_flash_async_wait(0);
    buf[3] = 0x23;
    write_cmp32(&buf[0], 0x04030201, 4);
    write_write_command_response(&response[0],
                                 sizeof(response));

    BTASSERT(ramapp_init(&ramapp, &flash) == 0);
    BTASSERT(ramapp_process_packet(&ramapp) == 0);

    return (0);
}

int main()
{
    struct harness_testcase_t testcases[] = {
        { test_ping, "test_ping" },
        { test_erase, "test_erase" },
        { test_read, "test_read" },
        { test_write, "test_write" },
        { test_fast_write_one_row, "test_fast_write_one_row" },
        {
            test_fast_write_one_row_bad_compare,
            "test_fast_write_one_row_bad_compare"
        },
        { test_fast_write_two_rows, "test_fast_write_two_rows" },
        {
            test_fast_write_two_rows_bad_crc,
            "test_fast_write_two_rows_bad_crc"
        },
        {
            test_fast_write_two_rows_bad_compare,
            "test_fast_write_two_rows_bad_compare"
        },
        { NULL, NULL }
    };

    sys_start();

    harness_run(testcases);

    return (0);
}
