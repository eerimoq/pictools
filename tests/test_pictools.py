import sys
import unittest
from unittest.mock import Mock
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


def flash_erase_read():
    return [b'\x00\x02\x00\x00', b'\xea\xa0']


def flash_erase_write(address, size, crc):
    payload = struct.pack('>II', address, size)
    header = b'\x00\x02' + struct.pack('>H', len(payload))
    crc = struct.pack('>H', crc)

    return ((header + payload + crc, ), )


def flash_write_read():
    return [b'\x00\x04\x00\x00', b'\x58\x00']


def flash_write_write(address, size, data, crc):
    payload = struct.pack('>II', address, size) + data
    header = b'\x00\x04' + struct.pack('>H', len(payload))
    footer = struct.pack('>H', crc)

    return ((header + payload + footer, ), )


def flash_read_read(data, crc=None):
    header = b'\x00\x03' + struct.pack('>H', len(data))

    if crc is None:
        crc = pictools.crc_ccitt(header + data)

    return [
        header,
        data,
        struct.pack('>H', crc)
    ]


def flash_read_write(address, size, crc=None):
    payload = struct.pack('>II', address, size)
    header = b'\x00\x03' + struct.pack('>H', len(payload))

    if crc is None:
        crc = pictools.crc_ccitt(header + payload)

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
        argv = [
            'pictools',
            'flash_write',
            '--chip-erase',
            'test_flash_write.s19'
        ]

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

        chunks = [
            bytes(range(256)),
            b'\x45' + bytes(range(256))[1:]
        ]
        serial.Serial.read.side_effect = [
            *programmer_ping_read(),
            *connect_read(),
            *ping_read(),
            b'\x00\x00',
            b'\x00\x00',
            *flash_write_fast_read()
        ]

        with open('test_flash_write.s19', 'w') as fout:
            binfile = bincopy.BinFile()
            binfile.add_binary(chunks[0], 0x1d000000)
            binfile.add_binary(chunks[1], 0x1d000100)
            fout.write(binfile.as_srec())

        with patch('sys.argv', argv):
            pictools.main()

        expected_writes = [
            programmer_ping_write(),
            connect_write(),
            ping_write(),
            flash_write_fast_write(0x1d000000, 512, 0x9d6f, 0xe39b),
            ((chunks[0], ), ),
            ((chunks[1], ), )
        ]

        self.assert_calls(serial.Serial.write.call_args_list,
                          expected_writes)

    def test_flash_write_verify(self):
        argv = [
            'pictools',
            'flash_write',
            '--verify',
            'test_flash_write.s19'
        ]

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

    def test_flash_write_fast_data_packet_failure(self):
        argv = ['pictools', 'flash_write', 'test_flash_write.s19']

        chunk = bytes(range(256))
        serial.Serial.read.side_effect = [
            *programmer_ping_read(),
            *connect_read(),
            *ping_read(),
            b'\xff\xff',
            b'\x00\x04',
            b'\xff\xff\xfc\x10',
            b'\x49\x5b'
        ]

        with open('test_flash_write.s19', 'w') as fout:
            binfile = bincopy.BinFile()
            binfile.add_binary(chunk, 0x1d000000)
            fout.write(binfile.as_srec())

        with patch('sys.argv', argv):
            with self.assertRaises(SystemExit) as cm:
                pictools.main()

            self.assertEqual(str(cm.exception), 'error: 1008: flash write failed')

        expected_writes = [
            programmer_ping_write(),
            connect_write(),
            ping_write(),
            flash_write_fast_write(0x1d000000, 256, 0x3fbd, 0xbd58),
            ((chunk, ), )
        ]

        self.assert_calls(serial.Serial.write.call_args_list,
                          expected_writes)

    def test_flash_read_all(self):
        argv = ['pictools', 'flash_read_all', 'test_flash_read_all.s19']

        binfile = bincopy.BinFile('tests/files/test_flash_read_all.s19')
        flash_read_reads = []

        for _, data in binfile.segments.chunks(504):
            flash_read_reads += flash_read_read(data)

        serial.Serial.read.side_effect = [
            *programmer_ping_read(),
            *connect_read(),
            *ping_read(),
            *flash_read_reads
        ]

        with patch('sys.argv', argv):
            pictools.main()

        flash_read_writes = []

        for address, data in binfile.segments.chunks(504):
            flash_read_writes.append(flash_read_write(address, len(data)))

        expected_writes = [
            programmer_ping_write(),
            connect_write(),
            ping_write(),
            *flash_read_writes
        ]

        self.assert_calls(serial.Serial.write.call_args_list,
                          expected_writes)

        with open('test_flash_read_all.s19', 'r') as fin:
            actual = fin.read()

        with open('tests/files/test_flash_read_all.s19', 'r') as fin:
            expected = fin.read()

        self.assertEqual(actual, expected)

    def test_flash_erase(self):
        argv = [
            'pictools',
            'flash_erase',
            '0x1d001000',
            '0x3000'
        ]

        serial.Serial.read.side_effect = [
            *programmer_ping_read(),
            *connect_read(),
            *ping_read(),
            *flash_erase_read()
        ]

        with patch('sys.argv', argv):
            pictools.main()

        expected_writes = [
            programmer_ping_write(),
            connect_write(),
            ping_write(),
            flash_erase_write(0x1d001000, 0x3000, 0x7974)
        ]

        self.assert_calls(serial.Serial.write.call_args_list,
                          expected_writes)

    def read_words(self, argv, address, data, output_lines):
        stdout = StringIO()
        serial.Serial.read.side_effect = [
            *programmer_ping_read(),
            *connect_read(),
            *ping_read(),
            *flash_read_read(data)
        ]

        with patch('sys.argv', argv):
            with patch('sys.stdout', stdout):
                pictools.main()

        actual_output = stdout.getvalue()
        expected_output = '\n'.join(output_lines)
        self.assertEqual(actual_output, expected_output)

        expected_writes = [
            programmer_ping_write(),
            connect_write(),
            ping_write(),
            flash_read_write(address, len(data))
        ]

        self.assert_calls(serial.Serial.write.call_args_list,
                          expected_writes)

    def test_configuration_print(self):
        self.read_words(['pictools', 'configuration_print'],
                        0x1fc017c0,
                        bytes(range(40)),
                        [
                            'Programmer is alive.',
                            'Connected to PIC.',
                            'PIC is alive.',
                            'FDEVOPT',
                            '  USERID: 1798',
                            '  FVBUSIO: 0',
                            '  FUSBIDIO: 0',
                            '  ALTI2C: 0',
                            '  SOSCHP: 0',
                            'FICD',
                            '  ICS: 1',
                            '  JTAGEN: 0',
                            'FPOR',
                            '  LPBOREN: 1',
                            '  RETVR: 1',
                            '  BOREN: 0',
                            'FWDT',
                            '  FWDTEN: 0',
                            '  RCLKSEL: 0',
                            '  RWDTPS: 17',
                            '  WINDIS: 0',
                            '  FWDTWINSZ: 0',
                            '  SWDTPS: 16',
                            'FOSCSEL',
                            '  FCKSM: 0',
                            '  SOSCSEL: 1',
                            '  OSCIOFNC: 1',
                            '  POSCMOD: 1',
                            '  IESO: 0',
                            '  SOSCEN: 0',
                            '  PLLSRC: 1',
                            '  FNOSC: 4',
                            'FSEC',
                            '  CP: 0',
                            ''
                        ])

    def test_device_id_print(self):
        self.read_words(['pictools', 'device_id_print'],
                        0x1f803660,
                        b'\x12\x34\x56\x78',
                        [
                            'Programmer is alive.',
                            'Connected to PIC.',
                            'PIC is alive.',
                            'DEVID',
                            '  VER: 7',
                            '  DEVID: 0x08563412',
                            ''
                        ])

    def test_udid_print(self):
        self.read_words(['pictools', 'udid_print'],
                        0x1fc41840,
                        bytes(range(20)),
                        [
                            'Programmer is alive.',
                            'Connected to PIC.',
                            'PIC is alive.',
                            'UDID',
                            '  UDID1: 0x03020100',
                            '  UDID2: 0x07060504',
                            '  UDID3: 0x0b0a0908',
                            '  UDID4: 0x0f0e0d0c',
                            '  UDID5: 0x13121110',
                            ''
                        ])

    def test_programmer_upload(self):
        argv = ['pictools', 'programmer_upload']

        check_call = Mock()
        find_executable = Mock(return_value='found')
        sleep = Mock()

        with patch('subprocess.check_call', check_call):
            with patch('pictools.find_executable', find_executable):
                with patch('time.sleep', sleep):
                    with patch('sys.argv', argv):
                        pictools.main()

        serial.Serial.__init__.assert_called_once_with('/dev/ttyUSB1', 1200)
        sleep.assert_called_once_with(2)
        find_executable.assert_called_once_with('bossac')
        self.assertEqual(len(check_call.call_args_list), 1)
        self.assertEqual(check_call.call_args_list[0][0][0][:-1],
                         ['bossac', '--port', 'ttyUSB1', '-e', '-w', '-b', '-R'])

    def test_generate_ramapp_upload_instructions(self):
        argv = [
            'pictools',
            'generate_ramapp_upload_instructions',
            'tests/files/ramapp.out',
            'test_generate_ramapp_upload_instructions.i'
        ]

        with open('tests/files/ramapp.dis', 'rb') as fin:
            check_output = Mock(return_value=fin.read())

        with patch('subprocess.check_output', check_output):
            with patch('sys.argv', argv):
                pictools.main()

        with open('tests/files/ramapp.i', 'r') as fin:
            expected = fin.read()
            expected = expected.format(pictools.__version__)

        with open('test_generate_ramapp_upload_instructions.i', 'r') as fin:
            actual = fin.read()

        self.assertEqual(actual, expected)


if __name__ == '__main__':
    unittest.main()
