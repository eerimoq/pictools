#!/usr/bin/env python3

import subprocess
import bincopy


def run(command, timed=False):
    command = command.split(' ')
    command = ['python3', '-m', 'pictools', '-p', '/dev/arduino'] + command

    if timed:
        command = ['/usr/bin/env', 'bash', '-c',
                   'time ' + ' '.join(command)]

    print("> {}".format(' '.join(command)))

    subprocess.check_call(command)

    print()


def main():
    # Create data.s19.
    binfile = bincopy.BinFile()
    full, partial = divmod(0x40000, 5)
    binfile.add_binary(full * b'\x12\x34\x56\x78\x9a', 0x1d000000)
    binfile.add_binary(b'\x12\x34\x56\x78\x9a'[:partial], 0x1d000000 + 5 * full)
    binfile.add_binary(0x1800 * b'\x00', 0x1fc00000)

    with open('data.s19', 'w') as fout:
        fout.write(binfile.as_srec())

    # Create zeros.s19.
    binfile = bincopy.BinFile()
    binfile.add_binary(0x40000 * b'\x00', 0x1d000000)
    binfile.add_binary(0x1800 * b'\x00', 0x1fc00000)

    with open('zeros.s19', 'w') as fout:
        fout.write(binfile.as_srec())

    # Run pictools commands.
    run('programmer_ping')
    run('flash_erase_chip')
    run('ping')
    run('flash_write --chip-erase --verify data.s19')
    run('flash_read 0x1d000000 0x1000 memory.s19')
    run('flash_erase 0x1d000000 0x1000')
    run('flash_read_all memory.s19')
    run('configuration_print')
    run('device_id_print')
    run('udid_print')
    run('reset')
    run('flash_write --chip-erase zeros.s19', timed=True)
    run('flash_erase_chip')
    run('programmer_version')


if __name__ == '__main__':
    main()
