.. pic32tools documentation master file, created by
   sphinx-quickstart on Sat Apr 25 11:54:09 2015.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. toctree::
   :maxdepth: 1

Welcome to the PIC32 tools documentation!
=========================================

PIC32 tools is a collection of tools to ease PIC32 development. PIC32
is a collection of MCU families created by `Microchip`_.

Features:

- A PIC32 programmer based on an `Arduino Due`_. Today only
  `PIC32MM0256GPM064`_ is supported.

Project homepage: https://github.com/eerimoq/pic32tools

Hardware setup
==============

.. image:: images/hardware-setup.jpg
   :width: 50%
   :target: _images/hardware-setup.jpg

The programmer (to the right) connected to a PC with serial over
USB. The DEF CON 26 Badge (to the left) with PIC32MM MCU to be
programmed.

+-----------+--------+---------------+
| Signal    | Color  | Arduino pin   |
+===========+========+===============+
| MCLRN     | white  | D4            |
+-----------+--------+---------------+
| VDD (3V3) | grey   | 3V3           |
+-----------+--------+---------------+
| VSS (GND) | purple | GND           |
+-----------+--------+---------------+
| PGED      | blue   | D3            |
+-----------+--------+---------------+
| PGEC      | green  | D2            |
+-----------+--------+---------------+

Installation
============

Python script
-------------

.. code-block:: python

    pip install pic32tools

Programmer
----------

Upload the latest pre-built release to the programmer (an Arduino
Due). All releases are found in `programmer/dist`_.

.. code-block:: text

   # Enter software upload mode.
   > python3 -c "import serial ; serial.Serial('/dev/arduino', 1200)"

   # Upload the software.
   > bossac --port arduino -e -w -b -R programmer/dist/0.1.0/programmer.bin
   Erase flash
   Write 23320 bytes to flash
   [==============================] 100% (92/92 pages)
   Set boot flash true
   CPU reset.

Inhibit Arduino Due reset when opening the serial port to the
programmer on Linux:

.. code-block:: text

   stty -F /dev/arduino -hup

Command line tool
=================

Descriptions and example usages of the most commonly used subcommands
in the command line tool ``pic32tools``.

Write to flash
--------------

Write given file ``hello_world.s19`` to flash. Optionally performs
erase and verify operations.

.. code-block:: text

   > pic32tools --port /dev/arduino flash_write --erase --verify hello_world.s19
   Programmer is alive.
   PIC32 is alive.
   Erasing 0x1fc017c0-0x1fc017dc.
   Erase complete.
   Erasing 0x1d000000-0x1d002e38.
   Erase complete.
   Erasing 0x1d00ae38-0x1d00aee8.
   Erase complete.
   Erasing 0x1fc00000-0x1fc00010.
   Erase complete.
   Writing /home/erik/hello_world.s19 to flash.
   Writing 0x1fc017c0-0x1fc017dc.
   100%|███████████████████████████████████| 28/28 [00:00<00:00, 7692.44 bytes/s]
   Write complete.
   Writing 0x1d000000-0x1d002e38.
   100%|████████████████████████████| 11832/11832 [00:00<00:00, 16485.67 bytes/s]
   Write complete.
   Writing 0x1d00ae38-0x1d00aee8.
   100%|████████████████████████████████| 176/176 [00:00<00:00, 14892.62 bytes/s]
   Write complete.
   Writing 0x1fc00000-0x1fc00010.
   100%|███████████████████████████████████| 16/16 [00:00<00:00, 4293.32 bytes/s]
   Write complete.
   Verifying written data.
   Verifying 0x1fc017c0-0x1fc017dc.
   100%|███████████████████████████████████| 28/28 [00:00<00:00, 7056.03 bytes/s]
   Verify complete.
   Verifying 0x1d000000-0x1d002e38.
   100%|████████████████████████████| 11832/11832 [00:00<00:00, 18983.52 bytes/s]
   Verify complete.
   Verifying 0x1d00ae38-0x1d00aee8.
   100%|████████████████████████████████| 176/176 [00:00<00:00, 15659.35 bytes/s]
   Verify complete.
   Verifying 0x1fc00000-0x1fc00010.
   100%|███████████████████████████████████| 16/16 [00:00<00:00, 5022.74 bytes/s]
   Verify complete.

Read from flash
---------------

Read from the flash memory.

.. code-block:: text

   > pic32tools --port /dev/arduino flash_read 0x1d000000 0x1000 memory.s19
   Programmer is alive.
   PIC32 is alive.
   Reading 0x1d000000-0x1d001000.
   100%|██████████████████████████████| 4096/4096 [00:00<00:00, 18530.60 bytes/s]
   Read complete.

Read the whole flash
--------------------

Read program flash, boot flash and configuration memory.

.. code-block:: text

   > pic32tools --port /dev/arduino flash_read_all memory.s19
   Programmer is alive.
   PIC32 is alive.
   Reading 0x1d000000-0x1d040000.
   100%|██████████████████████████| 262144/262144 [00:13<00:00, 19075.20 bytes/s]
   Read complete.
   Reading 0x1fc00000-0x1fc01700.
   100%|██████████████████████████████| 5888/5888 [00:00<00:00, 18900.71 bytes/s]
   Read complete.
   Reading 0x1fc01700-0x1fc01800.
   100%|████████████████████████████████| 256/256 [00:00<00:00, 16448.50 bytes/s]
   Read complete.

Erase a flash range
-------------------

Erase given flash range.

.. code-block:: text

   > pic32tools --port /dev/arduino flash_erase 0x1d000000 0x1000
   Programmer is alive.
   PIC32 is alive.
   Erasing 0x1d000000-0x1d001000.
   Erase complete.

Chip erase
----------

Erases program flash, boot flash and configuration memory.

.. code-block:: text

   > pic32tools --port /dev/arduino flash_erase_chip
   Erasing the chip.
   Programmer is alive.
   Chip erase complete.

Reset
-----

Reset the PIC32.

.. code-block:: text

   > pic32tools --port /dev/arduino reset
   Programmer is alive.
   Resetted PIC32.

Print the configuration memory
------------------------------

Print the configuration memory.

.. code-block:: text

   > pic32tools --port /dev/arduino configuration_print
   Programmer is alive.
   PIC32 is alive.
   FDEVOPT
     USERID: 65535
     FVBUSIO: 0
     FUSBIDIO: 1
     ALTI2C: 1
     SOSCHP: 1
   FICD
     ICS: 2
     JTAGEN: 0
   FPOR
     LPBOREN: 1
     RETVR: 1
     BOREN: 3
   FWDT
     FWDTEN: 0
     RCLKSEL: 3
     RWDTPS: 20
     WINDIS: 1
     FWDTWINSZ: 3
     SWDTPS: 20
   FOSCSEL
     FCKSM: 1
     SOSCSEL: 0
     OSCIOFNC: 1
     POSCMOD: 3
     IESO: 1
     SOSCEN: 0
     PLLSRC: 1
     FNOSC: 0
   FSEC
     CP: 1

Print the device id
-------------------

Print the device id.

.. code-block:: text

   > pic32tools --port /dev/arduino device_id_print
   Programmer is alive.
   PIC32 is alive.
   DEVID
     VER: 2
     DEVID: 0x0773c053

Print the UDID
--------------

Print the unique chip id.

.. code-block:: text

   > pic32tools --port /dev/arduino udid_print
   Programmer is alive.
   PIC32 is alive.
   UDID
     UDID1: 0xff918406
     UDID2: 0xff524000
     UDID3: 0xffffff25
     UDID4: 0xffff0219
     UDID5: 0xffff0280

Ping the programmer
-------------------

Test if the programmer is alive.

.. code-block:: text

   > pic32tools --port /dev/arduino programmer_ping
   Programmer is alive.

Ping the PIC32
--------------

Test if the PIC32 is alive and executing the RAM application.

.. code-block:: text

   > pic32tools --port /dev/arduino ping
   Programmer is alive.
   PIC32 is alive.

Similar projects
================

There are a bunch of projects similar to PIC32 tools. Here are a few
of them:

- https://github.com/WallaceIT/picberry

- https://github.com/sergev/pic32prog

- http://usbpicprog.org/

- https://wiki.kewl.org/dokuwiki/projects:pickle

- http://picpgm.picprojects.net/

.. _programmer/dist: https://github.com/eerimoq/pic32tools/tree/master/programmer/dist

.. _Arduino Due: https://store.arduino.cc/arduino-due

.. _Microchip: https://www.microchip.com/

.. _PIC32MM0256GPM064: https://www.microchip.com/wwwproducts/en/PIC32MM0256GPM064
