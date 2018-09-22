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

.PHONY: programmer

test:
	python3 setup.py test
	codespell -d $$(git ls-files | grep -v 3pp | grep -v \.bin | grep -v \.out | grep -v images)

test-target:
	python3 -u tests/target.py

release-to-pypi:
	python setup.py sdist
	python setup.py bdist_wheel --universal
	twine upload dist/*

programmer:
	cd ramapp && $(MAKE)
	cd programmer && $(MAKE) generate_ramapp_upload_instructions_i
	cd programmer && $(MAKE) all

programmer-clean:
	cd ramapp && $(MAKE) clean
	cd programmer && $(MAKE) clean

programmer-dist:
	cd programmer && $(MAKE) dist
