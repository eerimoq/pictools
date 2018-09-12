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

.PHONY: generate_ramapp_upload_instructions_i dist

NAME = programmer
BOARD ?= arduino_due
VERSION ?= 0.5.0

CDEFS += \
	CONFIG_MINIMAL_SYSTEM=1 \
	CONFIG_ICSP_SOFT=1 \
	CONFIG_PIN=1

RAMAPP_UPLOAD_INSTRUCTIONS_I = ramapp_upload_instructions.i
RAMAPP_OUT = ../ramapp/build/defcon26_badge/ramapp.out

DIST_PATH = dist/${VERSION}
BUILD_PATH = build/arduino_due
PYTHON_PACKAGE_PATH = ../pictools

include $(SIMBA_ROOT)/make/app.mk

generate_ramapp_upload_instructions_i:
	python3 ../pictools/__init__.py generate_ramapp_upload_instructions \
	    $(RAMAPP_OUT) \
	    $(RAMAPP_UPLOAD_INSTRUCTIONS_I)

dist:
	mkdir -p ${DIST_PATH}
	cp ${BUILD_PATH}/programmer.{bin,out,hex,map} ${DIST_PATH}
	cp ${BUILD_PATH}/programmer.bin ${PYTHON_PACKAGE_PATH}
	echo $(VERSION) > ${PYTHON_PACKAGE_PATH}/programmer-version.txt
