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

#ifndef __COMPAT_H__
#define __COMPAT_H__

#include "simba.h"

#if !defined(UNIT_TEST)

#define PIC32_ETAP_FASTDATA ((volatile uint32_t *) 0xff200000)

static inline uint32_t etap_fast_data_read()
{
    return (*PIC32_ETAP_FASTDATA);
}

static inline void etap_fast_data_write(uint32_t value)
{
    *PIC32_ETAP_FASTDATA = value;
}

static inline uint8_t load_flash_8(uint32_t address, size_t index)
{
    uint8_t *buf_p;

    buf_p = (uint8_t *)address;

    return (buf_p[index]);
}

static inline uint32_t load_flash_32(uint32_t address, size_t index)
{
    uint32_t *buf_p;

    buf_p = (uint32_t *)address;

    return (buf_p[index]);
}

static inline int memcmp8(void *buf_p, uint32_t address, size_t size)
{
    return (memcmp(buf_p, (void *)(uintptr_t)address, size));
}

#else

extern uint32_t etap_fast_data_read(void);
extern void etap_fast_data_write(uint32_t value);
extern uint8_t load_flash_8(uint32_t address, size_t index);
extern uint32_t load_flash_32(uint32_t address, size_t index);
extern int memcmp8(void *buf_p, uint32_t address, size_t size);

#endif

#endif
