import sys
import unittest
import bincopy
import struct
import binascii

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


def programmer_ping_read():
    return [b'\x00\x64\x00\x00', b'\xc3\x6b']


def programmer_ping_write():
    return ((b'\x00\x64\x00\x00\xc3\x6b', ), )


def connect_read():
    return [b'\x00\x65\x00\x00', b'\xf4\x5b']


def connect_write():
    return ((b'\x00\x65\x00\x00\xf4\x5b', ), )


def disconnect_read():
    return [b'\x00\x66\x00\x00', b'\xad\x0b']


def disconnect_write():
    return ((b'\x00\x66\x00\x00\xad\x0b', ), )


def reset_read():
    return [b'\x00\x67\x00\x00', b'\x9a\x3b']


def reset_write():
    return ((b'\x00\x67\x00\x00\x9a\x3b', ), )


def ping_read():
    return [b'\x00\x01\x00\x00', b'\xb3\xf0']


def ping_write():
    return ((b'\x00\x01\x00\x00\xb3\xf0', ), )


def flash_erase_chip_read():
    return [b'\x00\x69\x00\x00', b'\x81\x3a']


def flash_erase_chip_write():
    return ((b'\x00\x69\x00\x00\x81\x3a', ), )


def flash_write_read():
    return [b'\x00\x04\x00\x00', b'\x58\x00']


def flash_write_write(address, size, data, crc):
    payload = struct.pack('>II', address, size) + data
    header = b'\x00\x04' + struct.pack('>H', len(payload))
    footer = struct.pack('>H', crc)

    return ((header + payload + footer, ), )


def flash_read_read(data, crc):
    return [
        b'\x00\x03' + struct.pack('>H', len(data)),
        data,
        struct.pack('>H', crc)
    ]


def flash_read_write(address, size, crc):
    payload = struct.pack('>II', address, size)
    header = b'\x00\x03' + struct.pack('>H', len(payload))
    footer = struct.pack('>H', crc)

    return ((header + payload + footer, ), )


def flash_write_fast_read():
    return [b'\x00\x6a\x00\x00', b'\xd8\x6a']


def flash_write_fast_write(address, size, data_crc, crc):
    payload = struct.pack('>III', address, size, data_crc)
    header = b'\x00\x6a' + struct.pack('>H', len(payload))
    footer = struct.pack('>H', crc)

    return ((header + payload + footer, ), )


class PicToolsTest(unittest.TestCase):

    def setUp(self):
        serial.Serial.reset_mock()

    def assert_calls(self, actual_args, expected_args):
        def cformat(value):
            return binascii.hexlify(value).decode('ascii')

        self.assertEqual(len(actual_args), len(expected_args))

        for actual, expected in zip(actual_args, expected_args):
            if actual != expected:
                print('Expected: {}'.format(cformat(expected[0][0])))
                print('Actual:   {}'.format(cformat(actual[0][0])))

            self.assertEqual(actual, expected)

    def test_reset(self):
        argv = ['pictools', 'reset']

        serial.Serial.read.side_effect = [
            *programmer_ping_read(),
            *disconnect_read(),
            *reset_read()
        ]

        with patch('sys.argv', argv):
            pictools.main()

        expected_writes = [
            programmer_ping_write(),
            disconnect_write(),
            reset_write()
        ]
        self.assertEqual(serial.Serial.write.call_args_list,
                         expected_writes)

    def test_ping(self):
        argv = ['pictools', 'ping']

        serial.Serial.read.side_effect = [
            *programmer_ping_read(),
            *connect_read(),
            *ping_read()
        ]

        with patch('sys.argv', argv):
            pictools.main()

        expected_writes = [
            programmer_ping_write(),
            connect_write(),
            ping_write()
        ]
        self.assertEqual(serial.Serial.write.call_args_list,
                         expected_writes)

    def test_flash_write(self):
        argv = ['pictools', 'flash_write', 'test_flash_write.s19']

        serial.Serial.read.side_effect = [
            *programmer_ping_read(),
            *connect_read(),
            *ping_read(),
            *flash_write_read()
        ]

        with open('test_flash_write.s19', 'w') as fout:
            binfile = bincopy.BinFile()
            binfile.add_binary(b'\x00', 0x1d000000)
            fout.write(binfile.as_srec())

        with patch('sys.argv', argv):
            pictools.main()

        expected_writes = [
            programmer_ping_write(),
            connect_write(),
            ping_write(),
            flash_write_write(0x1d000000, 1, b'\x00', 0x3e2a)
        ]

        self.assert_calls(serial.Serial.write.call_args_list,
                          expected_writes)

    def test_flash_write_chip_erase(self):
        argv = ['pictools',
                'flash_write',
                '--chip-erase',
                'test_flash_write.s19']

        serial.Serial.read.side_effect = [
            *programmer_ping_read(),
            *disconnect_read(),
            *reset_read(),
            *flash_erase_chip_read(),
            *connect_read(),
            *flash_write_read()
        ]

        with open('test_flash_write.s19', 'w') as fout:
            binfile = bincopy.BinFile()
            binfile.add_binary(b'\x12', 0x1d000004)
            fout.write(binfile.as_srec())

        with patch('sys.argv', argv):
            pictools.main()

        expected_writes = [
            programmer_ping_write(),
            disconnect_write(),
            reset_write(),
            flash_erase_chip_write(),
            connect_write(),
            flash_write_write(0x1d000004, 1, b'\x12', 0x0af8)
        ]

        self.assert_calls(serial.Serial.write.call_args_list,
                          expected_writes)

    def test_flash_write_fast(self):
        argv = ['pictools', 'flash_write', 'test_flash_write.s19']

        chunk = bytearray(range(256))
        serial.Serial.read.side_effect = [
            *programmer_ping_read(),
            *connect_read(),
            *ping_read(),
            b'\x00\x00',
            *flash_write_fast_read()
        ]

        with open('test_flash_write.s19', 'w') as fout:
            binfile = bincopy.BinFile()
            binfile.add_binary(chunk, 0x1d000000)
            fout.write(binfile.as_srec())

        with patch('sys.argv', argv):
            pictools.main()

        expected_writes = [
            programmer_ping_write(),
            connect_write(),
            ping_write(),
            flash_write_fast_write(0x1d000000, 256, 0x3fbd, 0xbd58),
            ((chunk, ), )
        ]

        self.assert_calls(serial.Serial.write.call_args_list,
                          expected_writes)

    def test_flash_write_verify(self):
        argv = ['pictools',
                'flash_write',
                '--verify',
                'test_flash_write.s19']

        serial.Serial.read.side_effect = [
            *programmer_ping_read(),
            *connect_read(),
            *ping_read(),
            *flash_write_read(),
            *flash_read_read(b'\x00', 0xb9e1)
        ]

        with open('test_flash_write.s19', 'w') as fout:
            binfile = bincopy.BinFile()
            binfile.add_binary(b'\x00', 0x1d000000)
            fout.write(binfile.as_srec())

        with patch('sys.argv', argv):
            pictools.main()

        expected_writes = [
            programmer_ping_write(),
            connect_write(),
            ping_write(),
            flash_write_write(0x1d000000, 1, b'\x00', 0x3e2a),
            flash_read_write(0x1d000000, 1, 0xae0d)
        ]

        self.assert_calls(serial.Serial.write.call_args_list,
                          expected_writes)


if __name__ == '__main__':
    unittest.main()
