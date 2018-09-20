PIC32 RAM application (PE)
==========================

Erase, read and write PIC32 flash memories, and more.

Upload this application to PIC32 MCU using the ICSP protocol.

Protocol
--------

On command success the type field is copied from the request to the
response packet. On failure the response type is set to -1.

This is the packet format:

.. code-block:: text

   +---------+---------+-----------------+--------+
   | 2b type | 2b size | <size>b payload | 2b crc |
   +---------+---------+-----------------+--------+

   TYPE  REQ-SIZE  RSP-SIZE  DESCRIPTION
   ------------------------------------------------
     -1         -         4  Command failure.
      1         0         0  Ping.
      2         8         0  Erase flash.
      3         8         n  Read from flash.
      4       8+n         0  Write to flash.
    106        12         0  Fast write to flash.

Command failure
^^^^^^^^^^^^^^^

.. code-block:: text

   +----+---+------------+-----+
   | -1 | 4 | error code | crc |
   +----+---+------------+-----+

Ping
^^^^

.. code-block:: text

   +---+---+-----+
   | 1 | 0 | crc |
   +---+---+-----+

Erase flash
^^^^^^^^^^^

.. code-block:: text

   +---+---+------------+---------+-----+
   | 2 | 8 | 4b address | 4b size | crc |
   +---+---+------------+---------+-----+

Read flash
^^^^^^^^^^

.. code-block:: text

   +---+---+------------+---------+-----+
   | 3 | 8 | 4b address | 4b size | crc |
   +---+---+------------+---------+-----+

Write flash
^^^^^^^^^^^

.. code-block:: text

   +---+----------+------------+---------+--------------+-----+
   | 4 | 8 + size | 4b address | 4b size | <size>b data | crc |
   +---+----------+------------+---------+--------------+-----+

Fast write flash
^^^^^^^^^^^^^^^^

Start fast write packet. The response to this packet is sent after all
data packets have been received.

Address must be aligned on a 256 bytes boundary, a row, and size must
be a multiple of 256 bytes. Crc is a 32 bits CRC of all data packets
combined.

.. code-block:: text

   +-----+----+------------+---------+--------+-----+
   | 106 | 12 | 4b address | 4b size | 4b crc | crc |
   +-----+----+------------+---------+--------+-----+

Data packet. Contains data for one flash row.

.. code-block:: text

   +-----------+
   | 256b data |
   +-----------+

Example fast write sequence with a request, multiple data packets and
a response:

.. code-block:: text

   +-------------+                          +---------+
   | programmer  |                          | ramapp  |
   +-------------+                          +---------+
          |                                      |
          |   Fast write request of 18176 bytes  |
          |------------------------------------->|
          |                                      |
          |              Data 0-255              |
          |------------------------------------->|
          |                                      |
          |             Data 256-511             |
          |------------------------------------->|
          |                                      |
          |             Data 512-767             |
          |------------------------------------->|
          |                                      |
          .                                      .
          .                                      .
          .                                      .
          |           Data 17920-18175           |
          |------------------------------------->|
          |                                      |
          |         Fast write response          |
          |<-------------------------------------|
          |                                      |
          |                                      |
