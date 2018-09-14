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
    run('programmer_ping')
    run('ping')
    run('flash_erase_chip')
    run('flash_read_all memory.s19')
    run('flash_write --chip-erase --verify memory.s19')
    run('flash_read 0x1d000000 0x1000 memory.s19')
    run('flash_erase 0x1d000000 0x1000')
    run('configuration_print')
    run('device_id_print')
    run('udid_print')
    run('reset')

    binfile = bincopy.BinFile()
    binfile.add_binary(0x40000 * b'\x00', 0x1d000000)
    binfile.add_binary(0x1800 * b'\x00', 0x1fc00000)

    with open('zeros.s19', 'w') as fout:
        fout.write(binfile.as_srec())

    run('flash_write --chip-erase zeros.s19', timed=True)
    run('flash_erase_chip')


if __name__ == '__main__':
    main()
