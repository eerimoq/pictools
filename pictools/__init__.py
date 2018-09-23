"""Erase, read from and write to PIC flash memory. Uploads the PE to
SRAM using ICSP, which in turn accesses the flash memory.

                  +---------------+
 +------+         |               |          +---------+
 |      |   USB   |  Programmer   |   ICSP   |         |
 |  PC  o---------o               o----------o   PIC   |
 |      |         | (Arduino Due) |          |         |
 +------+         |               |          +---------+
                  +---------------+

"""

import time
import os
import sys
import re
import argparse
import struct
import serial
import binascii
import bincopy
import subprocess
from distutils.spawn import find_executable
from tqdm import tqdm
import bitstruct


__version__ = '0.15.0'


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

# Error codes.
EINVAL                    = 22
ERANGE                    = 34
EPROTO                    = 71
EMSGSIZE                  = 90
EISCONN                   = 106
ENOTCONN                  = 107
ETIMEDOUT                 = 110
EBADCRC                   = 1007
EFLASHWRITE               = 1008
EFLASHERASE               = 1009
EENTERSERIALEXECUTIONMODE = 10000
ERAMAPPUPLOAD             = 10001

ERROR_CODE_MESSAGE = {
    -EINVAL: "invalid argument",
    -ERANGE: "bad value, likely a memory address out of range",
    -EPROTO: "communication between programmer and PIC failed",
    -EMSGSIZE: "bad message size",
    -EISCONN: "PIC already connected",
    -ENOTCONN: "PIC is not connected",
    -ETIMEDOUT: "PIC command timeout",
    -EBADCRC: "invalid packet checksum",
    -EFLASHWRITE: "flash write failed",
    -EFLASHERASE: "flash erase failed",
    -EENTERSERIALEXECUTIONMODE: "enter serial execution mode failed",
    -ERAMAPPUPLOAD: "ramapp (PE) upload failed"
}

# Command types. Anything less than zero is error codes.
COMMAND_TYPE_FAILED = -1
COMMAND_TYPE_PING   =  1
COMMAND_TYPE_ERASE  =  2
COMMAND_TYPE_READ   =  3
COMMAND_TYPE_WRITE  =  4

PROGRAMMER_COMMAND_TYPE_FAST_WRITE_ACK = 0
PROGRAMMER_COMMAND_TYPE_PING           =  100
PROGRAMMER_COMMAND_TYPE_CONNECT        =  101
PROGRAMMER_COMMAND_TYPE_DISCONNECT     =  102
PROGRAMMER_COMMAND_TYPE_RESET          =  103
PROGRAMMER_COMMAND_TYPE_DEVICE_STATUS  =  104
PROGRAMMER_COMMAND_TYPE_CHIP_ERASE     =  105
PROGRAMMER_COMMAND_TYPE_FAST_WRITE     =  106

ERASE_TIMEOUT = 5
SERIAL_TIMEOUT = 1

READ_CHUNK_SIZE = 504
FAST_WRITE_SIZE = 256

PROGRAM_FLASH_ADDRESS       = 0x1d000000
PROGRAM_FLASH_SIZE          = 0x00040000
PROGRAM_FLASH_END           = 0x1d040000
SFRS_ADDRESS                = 0x1f800000
SFRS_SIZE                   = 0x00010000
SFRS_END                    = 0x1f810000
BOOT_FLASH_ADDRESS          = 0x1fc00000
BOOT_FLASH_SIZE             = 0x00001700
BOOT_FLASH_END              = 0x1fc01700
CONFIGURATION_BITS_ADDRESS  = 0x1fc01700
CONFIGURATION_BITS_SIZE     = 0x00000100
CONFIGURATION_BITS_END      = 0x1fc01800
DEVICE_ID_ADDRESS           = 0x1f803660
UDID_ADDRESS                = 0x1fc41840

COMMAND_TYPE_TO_STRING = {
    -1: 'FAILED',
    0: 'PROGRAMMER_FAST_WRITE_ACK',
    1: 'PING',
    2: 'ERASE',
    3: 'READ',
    4: 'WRITE',
    100: 'PROGRAMMER_PING',
    101: 'PROGRAMMER_CONNECT',
    102: 'PROGRAMMER_DISCONNECT',
    103: 'PROGRAMMER_RESET',
    104: 'PROGRAMMER_DEVICE_STATUS',
    105: 'PROGRAMMER_CHIP_ERASE',
    106: 'PROGRAMMER_FAST_WRITE'
}

RAMAPP_UPLOAD_INSTRUCTIONS_I_FMT = '''\
/**
 * This file was generated by pictools.py version {}.
 */

/* Upload the application. */
{}

/* Start the uploaded application. */
0xa00041b9, /* lui t9, 0xa000 */
0x00015339, /* ori t9, t9, 0x1 */
0x0f3c0019  /* jr t9 */
'''

CONFIGURATION_FMT = '''\
FDEVOPT
  USERID: {}
  FVBUSIO: {}
  FUSBIDIO: {}
  ALTI2C: {}
  SOSCHP: {}
FICD
  ICS: {}
  JTAGEN: {}
FPOR
  LPBOREN: {}
  RETVR: {}
  BOREN: {}
FWDT
  FWDTEN: {}
  RCLKSEL: {}
  RWDTPS: {}
  WINDIS: {}
  FWDTWINSZ: {}
  SWDTPS: {}
FOSCSEL
  FCKSM: {}
  SOSCSEL: {}
  OSCIOFNC: {}
  POSCMOD: {}
  IESO: {}
  SOSCEN: {}
  PLLSRC: {}
  FNOSC: {}
FSEC
  CP: {}\
'''

DEVICE_ID_FMT = '''\
DEVID
  VER: {}
  DEVID: 0x{:08x}\
'''

UDID_FMT = '''\
UDID
  UDID1: 0x{:08x}
  UDID2: 0x{:08x}
  UDID3: 0x{:08x}
  UDID4: 0x{:08x}
  UDID5: 0x{:08x}\
'''

DEVICE_STATUS_FMT = '''\
STATUS: 0x{:02x}
  CPS:    {}
  NVMERR: {}
  CFGRDY: {}
  FCBUSY: {}
  DEVRST: {}\
'''

PROGRAM_FLASH_SIZE_KB = {
    'pic32mm0064gpm028': 64,
    'pic32mm0128gpm028': 128,
    'pic32mm0256gpm028': 256,
    'pic32mm0064gpm036': 64,
    'pic32mm0128gpm036': 128,
    'pic32mm0256gpm036': 256,
    'pic32mm0064gpm048': 64,
    'pic32mm0128gpm048': 128,
    'pic32mm0256gpm048': 256,
    'pic32mm0064gpm064': 64,
    'pic32mm0128gpm064': 128,
    'pic32mm0256gpm064': 256
}

SUPPORTED_MCUS = [
    'pic32mm0064gpm028',
    'pic32mm0128gpm028',
    'pic32mm0256gpm028',
    'pic32mm0064gpm036',
    'pic32mm0128gpm036',
    'pic32mm0256gpm036',
    'pic32mm0064gpm048',
    'pic32mm0128gpm048',
    'pic32mm0256gpm048',
    'pic32mm0064gpm064',
    'pic32mm0128gpm064',
    'pic32mm0256gpm064'
]

GPR_STRINGS = [
    # 0.
    'zero',
    # 1.
    'at',
    # 2-3.
    'v0',
    'v1',
    # 4-7.
    'a0',
    'a1',
    'a2',
    'a3',
    # 8-15.
    't0',
    't1',
    't2',
    't3',
    't4',
    't5',
    't6',
    't7',
    # 16-23
    's0',
    's1',
    's2',
    's3',
    's4',
    's5',
    's6',
    's7',
    # 24-25.
    't8',
    't9',
    # 26-27.
    'k0',
    'k1',
    # 28.
    'gp',
    # 29.
    'sp',
    # 30.
    'fp',
    # 31.
    'ra'
]

REGLIST_GPRS = {
    1: [16],
    2: [16, 17],
    3: [16, 17, 18],
    4: [16, 17, 18, 19],
    5: [16, 17, 18, 19, 20],
    6: [16, 17, 18, 19, 20, 21],
    7: [16, 17, 18, 19, 20, 21, 22],
    8: [16, 17, 18, 19, 20, 21, 22, 23],
    9: [16, 17, 18, 19, 20, 21, 22, 23, 30],
    16: [31],
    17: [16, 31],
    18: [16, 17, 31],
    19: [16, 17, 18, 31],
    20: [16, 17, 18, 19, 31],
    21: [16, 17, 18, 19, 20, 31],
    22: [16, 17, 18, 19, 20, 21, 31],
    23: [16, 17, 18, 19, 20, 21, 22, 31],
    24: [16, 17, 18, 19, 20, 21, 22, 23, 31],
    25: [16, 17, 18, 19, 20, 21, 22, 23, 30, 31],
}


class CommandFailedError(Exception):

    def __init__(self, error):
        super().__init__()
        self.error = error

    def __str__(self):
        return format_error(self.error)


class Serial(serial.Serial):
    """Serial with peek method.

    """

    def __init__(self, port, baudrate, timeout):
        super().__init__(port, baudrate=baudrate, timeout=timeout)
        self._input_buffer = b''

    def read(self, size=1):
        data = self._input_buffer[:size]
        self._input_buffer = self._input_buffer[size:]
        left = (size - len(data))

        if left > 0:
            data += super().read(left)

        return data

    def peek(self, size):
        data = self.read(size)
        self._input_buffer = (data + self._input_buffer)

        return data


def crc_ccitt(data):
    """Calculate a CRC of given data.

    """

    msb = 0xff
    lsb = 0xff

    for c in bytearray(data):
        x = c ^ msb
        x ^= (x >> 4)
        msb = (lsb ^ (x >> 3) ^ (x << 4)) & 255
        lsb = (x ^ (x << 5)) & 255

    return (msb << 8) + lsb


def format_error(error):
    try:
        return 'error: {}: '.format(-error) + ERROR_CODE_MESSAGE[error]
    except KeyError:
        return 'error: {}'.format(-error)


def format_command_type(command_type):
    try:
        return COMMAND_TYPE_TO_STRING[command_type]
    except KeyError:
        return str(command_type)


def flash_ranges(mcu):
    return [
        (PROGRAM_FLASH_ADDRESS, PROGRAM_FLASH_SIZE_KB[mcu] * 1024),
        (BOOT_FLASH_ADDRESS, BOOT_FLASH_SIZE + CONFIGURATION_BITS_SIZE)
    ]


def is_program_flash_range(address, size):
    return ((address >= PROGRAM_FLASH_ADDRESS)
            and (((address + size) <= PROGRAM_FLASH_END)))


def is_sfrs_range(address, size):
    return ((address >= SFRS_ADDRESS)
            and (((address + size) <= SFRS_END)))


def is_boot_flash_configuration_bits_range(address, size):
    return ((address >= BOOT_FLASH_ADDRESS)
            and (((address + size) <= CONFIGURATION_BITS_END)))


def physical_flash_address(address):
    return address & 0x1fffffff


def serial_open(port):
    return Serial(port, baudrate=460800, timeout=SERIAL_TIMEOUT)


def serial_open_ensure_connected_to_programmer(port):
    serial_connection = serial_open(port)

    programmer_ping(serial_connection)

    return serial_connection


def serial_open_ensure_connected(port):
    serial_connection = serial_open_ensure_connected_to_programmer(port)

    try:
        connect(serial_connection)
    except CommandFailedError as e:
        if e.error == -EISCONN:
            pass
        elif e.error in [-EPROTO, -EENTERSERIALEXECUTIONMODE, -ERAMAPPUPLOAD]:
            reset(serial_connection)
            connect(serial_connection)
        else:
            raise

    ping(serial_connection)

    return serial_connection


def serial_open_ensure_disconnected(port):
    serial_connection = serial_open_ensure_connected_to_programmer(port)

    try:
        disconnect(serial_connection)
    except CommandFailedError as e:
        if e.error != -ENOTCONN:
            raise

    reset(serial_connection)

    return serial_connection


def packet_write(serial_connection, command_type, payload):
    """Write given packet to given serial connection.

    """

    header = struct.pack('>HH', command_type, len(payload))
    crc = struct.pack('>H', crc_ccitt(header + payload))

    serial_connection.write(header + payload + crc)


def packet_read(serial_connection):
    """Read a packet from given serial connection.

    """

    header = serial_connection.read(4)

    if len(header) != 4:
        sys.exit('error: timeout reading the response packet header from the '
                 'programmer')

    command_type, payload_size = struct.unpack('>hH', header)

    if payload_size > 0:
        payload = serial_connection.read(payload_size)

        if len(payload) != payload_size:
            sys.exit('error: timeout reading the response packet payload from '
                     'the programmer')
    else:
        payload = b''

    crc = serial_connection.read(2)

    if len(crc) != 2:
        sys.exit('error: timeout reading the response packet crc from the '
                 'programmer')

    actual_crc = struct.unpack('>H', crc)[0]
    expected_crc = crc_ccitt(header + payload)

    if actual_crc != expected_crc:
        sys.exit('error: expected response packet crc 0x{:04x}, but '
                 'got 0x{:04x}'.format(expected_crc,
                                       actual_crc))

    return command_type, payload


def send_command(serial_connection, command_type, payload):
    """Send given command.

    """

    if payload is None:
        payload = b''

    packet_write(serial_connection, command_type, payload)


def receive_command(serial_connection, command_type):
    """Receive given command response and return the response payload.

    """

    response_command_type, response_payload = packet_read(serial_connection)

    if response_command_type == command_type:
        return response_payload
    elif response_command_type == COMMAND_TYPE_FAILED:
        error = struct.unpack('>i', response_payload)[0]

        raise CommandFailedError(error)
    else:
        sys.exit(
            'error: expected programmer command response type {}, but '
            'got {}'.format(format_command_type(command_type),
                            format_command_type(response_command_type)))


def execute_command(serial_connection, command_type, payload=None):
    """Execute given command and return the response payload.

    """

    send_command(serial_connection, command_type, payload)

    return receive_command(serial_connection, command_type)


def assert_receive_failure(serial_connection):
    """Receive a failure.

    """

    response_command_type, response_payload = packet_read(serial_connection)

    if response_command_type == COMMAND_TYPE_FAILED:
        error = struct.unpack('>i', response_payload)[0]

        raise CommandFailedError(error)

    sys.exit('error: expected command to fail, but got successful response '
             'for command {}'.format(response_command_type))


def read_to_file(serial_connection, ranges, outfile):
    binfile = bincopy.BinFile()

    for address, size in ranges:
        left = size

        print('Reading 0x{:08x}-0x{:08x}.'.format(address, address + size))

        with tqdm(total=left, unit=' bytes') as progress:
            while left > 0:
                if left > READ_CHUNK_SIZE:
                    size = READ_CHUNK_SIZE
                else:
                    size = left

                payload = struct.pack('>II', address, size)
                binfile.add_binary(execute_command(serial_connection,
                                                   COMMAND_TYPE_READ,
                                                   payload),
                                   address)
                address += size
                left -= size
                progress.update(size)

        print('Read complete.')

    with open(outfile, 'w') as fout:
        fout.write(binfile.as_srec())


def erase(serial_connection, address, size):
    """Erase flash memory.

    """

    payload = struct.pack('>II', address, size)

    print('Erasing 0x{:08x}-0x{:08x}.'.format(address, address + size))

    serial_connection.timeout = ERASE_TIMEOUT
    execute_command(serial_connection, COMMAND_TYPE_ERASE, payload)
    serial_connection.timeout = SERIAL_TIMEOUT

    print('Erase complete.')


def reset(serial_connection):
    execute_command(serial_connection, PROGRAMMER_COMMAND_TYPE_RESET)

    print('PIC reset.')


def chip_erase(serial_connection):
    print('Erasing the chip.')

    execute_command(serial_connection, PROGRAMMER_COMMAND_TYPE_CHIP_ERASE)

    print('Chip erase complete.')


def connect(serial_connection):
    execute_command(serial_connection, PROGRAMMER_COMMAND_TYPE_CONNECT)

    print('Connected to PIC.')


def disconnect(serial_connection):
    execute_command(serial_connection, PROGRAMMER_COMMAND_TYPE_DISCONNECT)

    print('Disconnected from PIC.')


def read_words(args, address, length):
    serial_connection = serial_open_ensure_connected(args.port)
    payload = struct.pack('>II', address, 4 * length)
    words = execute_command(serial_connection,
                            COMMAND_TYPE_READ,
                            payload)

    return bitstruct.byteswap(length * '4', words)


def ping(serial_connection):
    execute_command(serial_connection, COMMAND_TYPE_PING)

    print('PIC is alive.')


def programmer_ping(serial_connection):
    execute_command(serial_connection, PROGRAMMER_COMMAND_TYPE_PING)

    print('Programmer is alive.')


def do_reset(args):
    # This function performs a reset.
    serial_open_ensure_disconnected(args.port)


def do_device_status_print(args):
    status = execute_command(serial_open_ensure_connected_to_programmer(args.port),
                             PROGRAMMER_COMMAND_TYPE_DEVICE_STATUS)
    unpacked = bitstruct.unpack('u1p1u1p1u1u1p1u1', status)
    status = struct.unpack('B', status)[0]

    print(DEVICE_STATUS_FMT.format(status, *unpacked))


def do_flash_erase_chip(args):
    chip_erase(serial_open_ensure_disconnected(args.port))


def do_ping(args):
    # The open function pings the PIC.
    serial_open_ensure_connected(args.port)


def do_flash_erase(args):
    address = int(args.address, 0)
    size = int(args.size, 0)

    if not (is_program_flash_range(address, size)
            or is_boot_flash_configuration_bits_range(address, size)):
        sys.exit(
            'error: address 0x{:08x} and size {} is out of range'.format(
                address,
                size))

    erase(serial_open_ensure_connected(args.port), address, size)


def do_flash_read(args):
    address = int(args.address, 0)
    size = int(args.size, 0)

    if not (is_program_flash_range(address, size)
            or is_boot_flash_configuration_bits_range(address, size)
            or is_sfrs_range(address, size)):
        sys.exit(
            'error: address 0x{:08x} and size {} is out of range'.format(
                address,
                size))

    serial_connection = serial_open_ensure_connected(args.port)
    read_to_file(serial_connection, [(address, size)], args.outfile)


def do_flash_read_all(args):
    serial_connection = serial_open_ensure_connected(args.port)
    read_to_file(serial_connection,
                 flash_ranges(args.mcu),
                 args.outfile)


def create_chunks(binfile):
    chunks = []
    fast_chunks = []
    total = 0

    for address, data in binfile.segments:
        address = physical_flash_address(address)
        size = len(data)

        if not (is_program_flash_range(address, size)
                or is_boot_flash_configuration_bits_range(address, size)):
            sys.exit(
                'error: address 0x{:08x} and size {} is out of range'.format(
                    address,
                    size))

        if (address % FAST_WRITE_SIZE) != 0:
            offset = (FAST_WRITE_SIZE - (address % FAST_WRITE_SIZE))

            if offset > size:
                offset = size

            chunk = (address, data[:offset])
            chunks.append(chunk)
        else:
            offset = 0

        number_of_fast_chunks = ((size - offset) // FAST_WRITE_SIZE)
        fast_chunk_size = (FAST_WRITE_SIZE * number_of_fast_chunks)

        if fast_chunk_size > 0:
            fast_chunk = (address + offset, data[offset:offset + fast_chunk_size])
            fast_chunks.append(fast_chunk)

        offset += fast_chunk_size
        last_chunk_size = (size - offset)

        if last_chunk_size > 0:
            chunk = (address + offset, data[offset:])
            chunks.append(chunk)

        total += size

    return chunks, fast_chunks, total


def receive_fast_write_ack(serial_connection):
    response = serial_connection.peek(2)

    if len(response) != 2:
        sys.exit('error: timeout waiting for fast write response from '
                 'the programmer')

    command_type = struct.unpack('>h', response)[0]

    if command_type == PROGRAMMER_COMMAND_TYPE_FAST_WRITE_ACK:
        serial_connection.read(2)
    else:
        assert_receive_failure(serial_connection)


def do_flash_write(args):
    binfile = bincopy.BinFile(args.binfile)

    if args.chip_erase:
        serial_connection = serial_open_ensure_disconnected(args.port)
        chip_erase(serial_connection)
        connect(serial_connection)
    elif args.erase:
        serial_connection = serial_open_ensure_connected(args.port)

        erase_segments = []

        for address, data in binfile.segments:
            address = physical_flash_address(address)
            erase_segments.append((address, len(data)))

        for address, size in erase_segments:
            address = physical_flash_address(address)
            erase(serial_connection, address, size)
    else:
        serial_connection = serial_open_ensure_connected(args.port)

    chunks, fast_chunks, total = create_chunks(binfile)

    print('Writing {} to flash.'.format(os.path.abspath(args.binfile)))

    with tqdm(total=total, unit=' bytes') as progress:
        # Chunks.
        for address, data in chunks:
            header = struct.pack('>II', address, len(data))
            execute_command(serial_connection, COMMAND_TYPE_WRITE, header + data)
            progress.update(len(data))

        # Fast chunks.
        for address, data in fast_chunks:
            header = struct.pack('>III', address, len(data), crc_ccitt(data))
            send_command(serial_connection,
                         PROGRAMMER_COMMAND_TYPE_FAST_WRITE,
                         header)
            serial_connection.write(data[:FAST_WRITE_SIZE])

            for offset in range(FAST_WRITE_SIZE, len(data), FAST_WRITE_SIZE):
                serial_connection.write(data[offset:offset + FAST_WRITE_SIZE])
                receive_fast_write_ack(serial_connection)
                progress.update(FAST_WRITE_SIZE)

            receive_fast_write_ack(serial_connection)
            progress.update(FAST_WRITE_SIZE)
            receive_command(serial_connection, PROGRAMMER_COMMAND_TYPE_FAST_WRITE)

    print('Write complete.')

    if args.verify:
        print('Verifying written data.')

        with tqdm(total=total, unit=' bytes') as progress:
            for address, data in binfile.segments.chunks(READ_CHUNK_SIZE):
                address = physical_flash_address(address)
                payload = struct.pack('>II', address, len(data))
                read_data = execute_command(serial_connection,
                                            COMMAND_TYPE_READ,
                                            payload)

                if bytearray(read_data) != data:
                    sys.exit(
                        'error: verify failed at address 0x{:x}'.format(address))

                progress.update(len(data))

        print('Verify complete.')


def do_configuration_print(args):
    config = read_words(args, CONFIGURATION_BITS_ADDRESS + 0xc0, 10)
    unpacked = bitstruct.unpack('p32'                          # RESERVED
                                'u16u1u1p9u1u1p3'              # FDEVOPT
                                'p27u2u1p2'                    # FICD
                                'p28u1u1u2'                    # FPOR
                                'p16u1u2u5u1u2u5'              # FWDT
                                'p16u2p1u1p1u1u2u1u1p1u1p1u3'  # FOSCSEL
                                'u1p31'                        # FSEC
                                'p32'                          # RESERVED
                                'p32'                          # RESERVED
                                'p32',                         # RESERVED
                                config)

    print(CONFIGURATION_FMT.format(*unpacked))


def do_device_id_print(args):
    device_id = read_words(args, DEVICE_ID_ADDRESS, 1)
    unpacked = bitstruct.unpack('u4u28', device_id)

    print(DEVICE_ID_FMT.format(*unpacked))


def do_udid_print(args):
    udid = read_words(args, UDID_ADDRESS, 5)
    unpacked = bitstruct.unpack(5 * 'u32', udid)

    print(UDID_FMT.format(*unpacked))


def do_programmer_ping(args):
    serial_connection = serial_open(args.port)

    programmer_ping(serial_connection)


def do_programmer_upload(args):
    filename = os.path.join(SCRIPT_DIR, 'programmer-version.txt')

    with open(filename, 'r') as fin:
        version = fin.read().strip()

    print('Uploading programmer application version {}.'.format(version))

    # Enter software upload mode.
    serial_connection = serial.Serial(args.port, 1200)
    serial_connection.close()
    time.sleep(2)

    # Upload the software.
    port = args.port.replace('/dev/', '')
    binfile = os.path.join(SCRIPT_DIR, 'programmer.bin')
    command = ['bossac', '--port', port, '-e', '-w', '-b', '-R', binfile]

    if args.unlock:
        command.append('-u')

    if args.bossac_path is not None:
        os.environ['PATH'] = args.bossac_path + os.pathsep + os.environ['PATH']

    # Make sure bossac is found.
    if find_executable('bossac') is None:
        raise Exception(
            "error: 'programmer_upload' requires that 'bossac' is installed on "
            "the host machine. Please install it in PATH, or give '--bossac-path' "
            "with its location in the file system.")

    subprocess.check_call(command)

    print('Upload complete.')


def do_generate_ramapp_upload_instructions(args):
    instructions = []

    disassembly = subprocess.check_output([
        'mips-unknown-elf-objdump', '-d', args.elffile
    ]).decode('utf-8')

    instructions = []

    for line in disassembly.splitlines():
        mo = re.match(r'([a-f0-9]+):\t([^\t]+)', line)

        if mo:
            address = int(mo.group(1), 16)
            data = mo.group(2).replace(' ', '')
            size = len(data) // 2

            if len(data) == 8:
                data = data[4:] + data[:4]

            if len(instructions) > 0:
                prev = instructions[-1]
                prev_end = prev[0] + prev[1]
                padding_size = address - prev_end

                for i in range(padding_size // 2):
                    instructions.append((prev_end + 2 * i, 2, '0000'))

            instructions.append((address, size, data))

    pairs = []
    leftover = None

    for address, size, data in instructions:
        if size == 4:
            if leftover:
                pairs.append((data[4:], leftover))
                leftover = data[:4]
            else:
                pairs.append((data[:4], data[4:]))
        else:
            if leftover:
                pairs.append((data, leftover))
                leftover = None
            else:
                leftover = data

    def fhex(packed):
        return '0x' + binascii.hexlify(packed).decode('ascii')

    def lui(rs, immediate):
        instruction = fhex(bitstruct.pack('u16u6u5u5',
                                          immediate,
                                          0b010000,
                                          0b01101,
                                          rs))
        comment = 'lui {}, 0x{:x}'.format(GPR_STRINGS[rs], immediate)

        return (instruction, comment)

    def ori(rt, rs, immediate):
        instruction = fhex(bitstruct.pack('u16u6u5u5',
                                          immediate,
                                          0b010100,
                                          rt,
                                          rs))
        comment = 'ori {}, {}, 0x{:x}'.format(GPR_STRINGS[rt],
                                              GPR_STRINGS[rs],
                                              immediate)

        return (instruction, comment)

    def swm32(reglist, base, offset):
        instruction = fhex(bitstruct.pack('u4u12u6u5u5',
                                          0b1101,
                                          offset,
                                          0b001000,
                                          reglist,
                                          base))
        reglist = ', '.join([GPR_STRINGS[i] for i in REGLIST_GPRS[reglist]])
        comment = 'swm32 {}, 0x{:x}({})'.format(reglist,
                                                offset,
                                                GPR_STRINGS[base])

        return (instruction, comment)

    instructions = []

    # Store up to 10 instructions at a time to RAM using with the
    # SWM32 instruction. Maximum positive immediate offset in SWM32 is
    # 2047 (12 bits signed).
    for i in range(0, len(pairs), 512):
        address = (0xa0000000 + 4 * i)
        instructions.append(lui(4, address >> 16))
        instructions.append(ori(4, 4, address & 0xffff))
        chunkpairs = pairs[i:i + 512]

        for j in range(0, len(chunkpairs), 10):
            subpairs = chunkpairs[j:j + 10]
            reglist = (15 + len(subpairs))
            gprs = REGLIST_GPRS[reglist]

            for gpr, (high, low) in zip(gprs, subpairs):
                instructions.append(lui(6, int(high, 16)))
                instructions.append(ori(gpr, 6, int(low, 16)))

            instructions.append(swm32(reglist, 4, 4 * j))

    with open(args.outfile, "w") as fout:
        fout.write(RAMAPP_UPLOAD_INSTRUCTIONS_I_FMT.format(
            __version__,
            '\n'.join(['{}, /* {} */'.format(*i) for i in instructions])))


def main():
    description = (
        "Erase, read from and write to PIC flash memory, and more. Uploads "
        "the RAM application to the PIC RAM over ICSP, which in turn accesses "
        "the flash memory.")
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('-p', '--port',
                        default='/dev/ttyUSB1',
                        help='Programmer serial port (default: /dev/ttyUSB1).')
    parser.add_argument('-d', '--debug', action='store_true')
    parser.add_argument('-m', '--mcu',
                        choices=SUPPORTED_MCUS,
                        default='pic32mm0256gpm064')
    parser.add_argument('--version',
                        action='version',
                        version=__version__,
                        help='Print version information and exit.')

    # Python 3 workaround for help output if subcommand is missing.
    subparsers = parser.add_subparsers(dest='one of the above')
    subparsers.required = True

    subparser = subparsers.add_parser('reset',
                                      help='Reset the PIC.')
    subparser.set_defaults(func=do_reset)

    subparser = subparsers.add_parser(
        'ping',
        help='Test if the PIC is alive and executing the RAM application.')
    subparser.set_defaults(func=do_ping)

    subparser = subparsers.add_parser('flash_erase',
                                      help='Erase given flash range.')
    subparser.add_argument('address')
    subparser.add_argument('size')
    subparser.set_defaults(func=do_flash_erase)

    subparser = subparsers.add_parser('flash_read',
                                      help='Read from the flash memory.')
    subparser.add_argument('address')
    subparser.add_argument('size')
    subparser.add_argument('outfile')
    subparser.set_defaults(func=do_flash_read)

    subparser = subparsers.add_parser(
        'flash_read_all',
        help='Read program flash, boot flash and configuration memory.')
    subparser.add_argument('outfile')
    subparser.set_defaults(func=do_flash_read_all)

    subparser = subparsers.add_parser(
        'flash_write',
        help=('Write given file to flash and verify that it has been written. '
              'Optionally performs erase and read back verify operations.'))
    subparser.add_argument('-e', '--erase', action='store_true')
    subparser.add_argument('-c', '--chip-erase', action='store_true')
    subparser.add_argument('-v', '--verify',
                           action='store_true',
                           help='Read back verification.')
    subparser.add_argument('binfile')
    subparser.set_defaults(func=do_flash_write)

    subparser = subparsers.add_parser(
        'flash_erase_chip',
        help='Erases program flash, boot flash and configuration memory.')
    subparser.set_defaults(func=do_flash_erase_chip)

    subparser = subparsers.add_parser('configuration_print',
                                      help='Print the configuration memory.')
    subparser.set_defaults(func=do_configuration_print)

    subparser = subparsers.add_parser('device_id_print',
                                      help='Print the device id.')
    subparser.set_defaults(func=do_device_id_print)

    subparser = subparsers.add_parser('udid_print',
                                      help='Print the unique chip id.')
    subparser.set_defaults(func=do_udid_print)

    subparser = subparsers.add_parser('device_status_print',
                                      help='Print the device status.')
    subparser.set_defaults(func=do_device_status_print)

    subparser = subparsers.add_parser(
        'programmer_ping',
        help='Test if the programmer is alive.')
    subparser.set_defaults(func=do_programmer_ping)

    subparser = subparsers.add_parser(
        'programmer_upload',
        help='Upload the programmer application to the programmer.')
    subparser.add_argument('-u', '--unlock',
                           action='store_true',
                           help='Unlock memory protection bits (give -u to bossac).')
    subparser.add_argument('-b', '--bossac-path',
                           help='Path to bossac if not installed in PATH.')
    subparser.set_defaults(func=do_programmer_upload)

    subparser = subparsers.add_parser(
        'generate_ramapp_upload_instructions',
        help='Generate the RAM application C source file.')
    subparser.add_argument('elffile')
    subparser.add_argument('outfile')
    subparser.set_defaults(func=do_generate_ramapp_upload_instructions)

    args = parser.parse_args()

    if args.debug:
        args.func(args)
    else:
        try:
            args.func(args)
        except BaseException as e:
            sys.exit(str(e))
