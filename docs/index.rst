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
   :target: _static/images/hardware-setup.jpg

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
   > bossac --port arduino -e -w -b -R programmer/dist/0.1.0/programmer.hex
   Erase flash
   Write 65126 bytes to flash
   [==============================] 100% (255/255 pages)
   Set boot flash true
   CPU reset.

Inhibit Arduino Due reset when opening the serial port to the
programmer on Linux:

.. code-block:: text

   stty -F /dev/arduino -hup

Example usage
=============

An example programming a binary to a PIC32.

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

.. _programmer/dist: https://github.com/eerimoq/pic32tools/tree/master/programmer/dist

.. _Arduino Due: https://store.arduino.cc/arduino-due

.. _Microchip: https://www.microchip.com/

.. _PIC32MM0256GPM064: https://www.microchip.com/wwwproducts/en/PIC32MM0256GPM064
