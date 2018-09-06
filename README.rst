About
=====

PIC32 tools is a collection of tools to ease PIC32 development. PIC32
is a collection of MCU families created by `Microchip`_.

Features:

- A PIC32 programmer based on an `Arduino Due`_. Today only
  `PIC32MM0256GPM064`_ is supported.

Project homepage: https://github.com/eerimoq/pic32tools

Documentation: http://pic32tools.readthedocs.org/en/latest

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

Contributing
============

#. Fork the repository.

#. Install prerequisites.

   .. code-block:: text

      pip install -r requirements.txt

#. Implement the new feature or bug fix.

#. Create a pull request.

.. _Arduino Due: https://store.arduino.cc/arduino-due

.. _Microchip: https://www.microchip.com/

.. _PIC32MM0256GPM064: https://www.microchip.com/wwwproducts/en/PIC32MM0256GPM064
