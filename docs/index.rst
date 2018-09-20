.. pictools documentation master file, created by
   sphinx-quickstart on Sat Apr 25 11:54:09 2015.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

.. toctree::
   :maxdepth: 1

Welcome to the PIC tools documentation!
=======================================

`PIC tools` is a collection of tools to ease PIC development. PIC is
family of microcontrollers made by `Microchip`_.

Features:

- A PIC programmer based on an `Arduino Due`_. Today only the
  `PIC32MM0256GPM064`_ family is supported.

Project homepage: https://github.com/eerimoq/pictools

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

#. Install pictools.

   .. code-block:: python

      pip install pictools

#. Upload the programmer application to the Arduino Due. This requires
   that ``bossac`` is already installed on the host machine.

   .. code-block:: text

      > pictools --port /dev/arduino programmer_upload
      Uploading programmer application version 0.6.0.
      Erase flash
      Write 23248 bytes to flash
      [==============================] 100% (91/91 pages)
      Set boot flash true
      CPU reset.
      Upload complete.

   If necessary, give ``-u`` to the upload command above to unlock any
   locked flash regions.

Command line tool
=================

Descriptions and example usages of the most commonly used subcommands
in the command line tool ``pictools``.

Add the ``--mcu`` option before the subcommand to select your MCU.

Write to flash
--------------

Write given file ``hello_world.s19`` to flash. Optionally performs
erase and read back verify operations.

.. code-block:: text

   > pictools --port /dev/arduino flash_write --chip-erase hello_world.s19
   Programmer is alive.
   Resetted PIC.
   Erasing the chip.
   Chip erase complete.
   Connected to PIC.
   Writing /home/erik/workspace/pictools/hello_world.s19 to flash.
   100%|████████████████████████████| 12052/12052 [00:00<00:00, 65081.89 bytes/s]
   Write complete.

Read from flash
---------------

Read from the flash memory.

.. code-block:: text

   > pictools --port /dev/arduino flash_read 0x1d000000 0x1000 memory.s19
   Programmer is alive.
   PIC is alive.
   Reading 0x1d000000-0x1d001000.
   100%|██████████████████████████████| 4096/4096 [00:00<00:00, 60882.44 bytes/s]
   Read complete.

Read the whole flash
--------------------

Read program flash, boot flash and configuration memory.

.. code-block:: text

   > pictools --port /dev/arduino flash_read_all memory.s19
   Programmer is alive.
   PIC is alive.
   Reading 0x1d000000-0x1d040000.
   100%|██████████████████████████| 262144/262144 [00:04<00:00, 61062.96 bytes/s]
   Read complete.
   Reading 0x1fc00000-0x1fc01700.
   100%|██████████████████████████████| 5888/5888 [00:00<00:00, 59621.36 bytes/s]
   Read complete.
   Reading 0x1fc01700-0x1fc01800.
   100%|████████████████████████████████| 256/256 [00:00<00:00, 54662.82 bytes/s]
   Read complete.

Erase a flash range
-------------------

Erase given flash range.

.. code-block:: text

   > pictools --port /dev/arduino flash_erase 0x1d000000 0x1000
   Programmer is alive.
   PIC is alive.
   Erasing 0x1d000000-0x1d001000.
   Erase complete.

Chip erase
----------

Erases program flash, boot flash and configuration memory.

.. code-block:: text

   > pictools --port /dev/arduino flash_erase_chip
   Erasing the chip.
   Programmer is alive.
   Chip erase complete.

Reset
-----

Reset the PIC.

.. code-block:: text

   > pictools --port /dev/arduino reset
   Programmer is alive.
   Resetted PIC.

Print the configuration memory
------------------------------

Print the configuration memory.

.. code-block:: text

   > pictools --port /dev/arduino configuration_print
   Programmer is alive.
   PIC is alive.
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

   > pictools --port /dev/arduino device_id_print
   Programmer is alive.
   PIC is alive.
   DEVID
     VER: 2
     DEVID: 0x0773c053

Print the UDID
--------------

Print the unique chip id.

.. code-block:: text

   > pictools --port /dev/arduino udid_print
   Programmer is alive.
   PIC is alive.
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

   > pictools --port /dev/arduino programmer_ping
   Programmer is alive.

Ping the PIC
--------------

Test if the PIC is alive and executing the RAM application.

.. code-block:: text

   > pictools --port /dev/arduino ping
   Programmer is alive.
   PIC is alive.

Test matrix
===========

A list of all supported MCUs and their test results.

`Write time` is the time it takes to connect to the PIC, perform a
chip erase and write zeros to the whole memory. An example measurement
for PIC32MM0256GPM064 can be seen below.

.. code-block:: text

   > time pictools -p /dev/arduino -m pic32mm0256gpm064 flash_write -c zeros.s19
   Programmer is alive.
   Resetted PIC.
   Erasing the chip.
   Chip erase complete.
   Connected to PIC.
   Writing /home/erik/workspace/pictools/zeros.s19 to flash.
   100%|██████████████████████████| 268288/268288 [00:04<00:00, 66317.50 bytes/s]
   Write complete.

   real    0m4.635s
   user    0m0.486s
   sys     0m0.073s

+-------------------+-------------+------------+----------------------------+
| MCU               | Memory size | Write time | Comment                    |
+===================+=============+============+============================+
| PIC32MM0064GPM028 |         70k | Not tested |                            |
+-------------------+-------------+------------+----------------------------+
| PIC32MM0128GPM028 |        134k | Not tested |                            |
+-------------------+-------------+------------+----------------------------+
| PIC32MM0256GPM028 |        262k | Not tested |                            |
+-------------------+-------------+------------+----------------------------+
| PIC32MM0064GPM036 |         70k | Not tested |                            |
+-------------------+-------------+------------+----------------------------+
| PIC32MM0128GPM036 |        134k | Not tested |                            |
+-------------------+-------------+------------+----------------------------+
| PIC32MM0256GPM036 |        262k | Not tested |                            |
+-------------------+-------------+------------+----------------------------+
| PIC32MM0064GPM048 |         70k | Not tested |                            |
+-------------------+-------------+------------+----------------------------+
| PIC32MM0128GPM048 |        134k | Not tested |                            |
+-------------------+-------------+------------+----------------------------+
| PIC32MM0256GPM048 |        262k | Not tested |                            |
+-------------------+-------------+------------+----------------------------+
| PIC32MM0064GPM064 |         70k | Not tested |                            |
+-------------------+-------------+------------+----------------------------+
| PIC32MM0128GPM064 |        134k | Not tested |                            |
+-------------------+-------------+------------+----------------------------+
| PIC32MM0256GPM064 |        262k |      4.6 s |                            |
+-------------------+-------------+------------+----------------------------+

Similar projects
================

There are a bunch of projects similar to `PIC tools`. Here are a few
of them:

- https://github.com/WallaceIT/picberry

- https://github.com/sergev/pic32prog

- http://usbpicprog.org/

- https://wiki.kewl.org/dokuwiki/projects:pickle

- http://picpgm.picprojects.net/

- https://github.com/jaromir-sukuba/a-p-prog

- https://github.com/G4me4u/pic-programmer-arduino

- https://github.com/rweather/ardpicprog

.. _programmer/dist: https://github.com/eerimoq/pictools/tree/master/programmer/dist

.. _Arduino Due: https://store.arduino.cc/arduino-due

.. _Microchip: https://www.microchip.com/

.. _PIC32MM0256GPM064: https://www.microchip.com/wwwproducts/en/PIC32MM0256GPM064
