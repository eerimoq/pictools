#!/usr/bin/env python3

import subprocess


def run(command):
    command = 'python3 -u pictools.py --port /dev/arduino ' + command

    print()
    print("# Running '{}'.".format(command))
    print()

    command = command.split(' ')

    subprocess.check_call(command)


def main():
    run('programmer_ping')
    run('ping')
    run('flash_erase_chip')
    run('flash_read_all memory.s19')
    run('flash_write --chip-erase --verify memory.s19')
    run('flash_read 0x1d000000 0x1000 memory.s19')
    run('flash_erase 0x1d000000 0x1000')
    run('reset')
    run('configuration_print')
    run('device_id_print')
    run('udid_print')


if __name__ == '__main__':
    main()
