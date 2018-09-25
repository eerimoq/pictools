#
# @section License
#
# The MIT License (MIT)
#
# Copyright (c) 2018, Erik Moqvist
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# This file is part of the PIC tools project.
#

NAME = programmer_suite
TYPE = suite
BOARD ?= linux

VERSION = 0.1.2-test

CDEFS += UNIT_TEST
CDEFS += CONFIG_HARNESS_MOCK_ENTRIES_MAX=50000

SRC += $(PROGRAMMER_ROOT)/programmer.c
SRC += $(SIMBA_ROOT)/src/hash/crc.c
SRC += $(SIMBA_ROOT)/src/collections/bits.c
SRC += $(SIMBA_ROOT)/tst/stubs/kernel/time_mock.c
SRC += $(SIMBA_ROOT)/tst/stubs/sync/chan_mock.c
SRC += $(SIMBA_ROOT)/tst/stubs/drivers/basic/pin_mock.c
SRC += $(SIMBA_ROOT)/tst/stubs/drivers/network/icsp_soft_mock.c

STUB += $(PROGRAMMER_ROOT)/programmer.c:icsp_soft_*
STUB += $(PROGRAMMER_ROOT)/programmer.c:pin_init,pin_write,pin_set_mode
STUB += $(PROGRAMMER_ROOT)/programmer.c:chan_read_with_timeout,chan_write
STUB += $(PROGRAMMER_ROOT)/programmer.c:time_get

include $(SIMBA_ROOT)/make/app.mk
