import unittest

try:
    from unittest.mock import patch
except ImportError:
    from mock import patch

try:
    from StringIO import StringIO
except ImportError:
    from io import StringIO

import pictools
import serial


class PicToolsTest(unittest.TestCase):

    def test_reset(self):
        argv = ['pictools', 'reset']
        expected_output = """\
Programmer is alive.
Disconnected from PIC.
PIC reset.
"""

        stdout = StringIO()
        serial.Serial.read.side_effect = [
            # Ping.
            b'\x00\x64\x00\x00', b'\xc3\x6b',
            # Disconnect.
            b'\x00\x66\x00\x00', b'\xad\x0b',
            # Reset.
            b'\x00\x67\x00\x00', b'\x9a\x3b'
        ]

        with patch('sys.stdout', stdout):
            with patch('sys.argv', argv):
                pictools.main()
                actual_output = stdout.getvalue()
                self.assertEqual(actual_output, expected_output)

        print(serial.Serial.write.call_args_list)


if __name__ == '__main__':
    unittest.main()
