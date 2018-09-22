import sys
import unittest

try:
    from unittest.mock import patch
except ImportError:
    from mock import patch

try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

sys.path.insert(0, 'tests')

import pictools
import serial


def ping_read():
    return [b'\x00\x64\x00\x00', b'\xc3\x6b']


def ping_write():
    return ((b'\x00\x64\x00\x00\xc3\x6b', ), )


def disconnect_read():
    return [b'\x00\x66\x00\x00', b'\xad\x0b']


def disconnect_write():
    return ((b'\x00\x66\x00\x00\xad\x0b', ), )


def reset_read():
    return [b'\x00\x67\x00\x00', b'\x9a\x3b']


def reset_write():
    return ((b'\x00\x67\x00\x00\x9a\x3b', ), )


class PicToolsTest(unittest.TestCase):

    def test_reset(self):
        argv = ['pictools', 'reset']

        stdout = StringIO()
        serial.Serial.read.side_effect = [
            *ping_read(),
            *disconnect_read(),
            *reset_read()
        ]

        with patch('sys.stdout', stdout):
            with patch('sys.argv', argv):
                pictools.main()

        expected_output = """\
Programmer is alive.
Disconnected from PIC.
PIC reset.
"""
        actual_output = stdout.getvalue()
        self.assertEqual(actual_output, expected_output)

        expected_writes = [
            ping_write(),
            disconnect_write(),
            reset_write()
        ]
        actual_writes = serial.Serial.write.call_args_list
        self.assertEqual(actual_writes, expected_writes)


if __name__ == '__main__':
    unittest.main()
