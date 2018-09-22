PIC programmer
==============

Upload this application to the Arduino Due.

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
      0         -         -  Fast write packet acknowledge. A truncated
                             packet without size, payload and crc.
    100         0         0  Ping the programmer.
    101         0         0  Connect to the PIC. Uploads the ramapp (PE) to
                             the PIC.
    102         0         0  Disconnect from the PIC by setting MCLRN, PGED
                             and PGEC to inputs.
    103         0         0  Reset the PIC. Requires that the PIC is
                             disconnected.
    104         0         1  Read the PIC status.
    105         0         0  Perform a chip erase.
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

Start fast write packet. The final response to this packet is sent
after all data packets have been exchanged.

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

Data acknowledge packet. A truncated packet with type 0.

.. code-block:: text

   +---+
   | 0 |
   +---+

Example fast write sequence with a request, multiple data packets and
a response:

.. code-block:: text

       +-----+                            +-------------+
       | PC  |                            | programmer  |
       +-----+                            +-------------+
          |                                      |
          |   Fast write request of 18176 bytes  |
          |------------------------------------->|
          |                                      |
          |              Data 0-255              |
          |------------------------------------->|
          |                                      |
          |              Data ack                |
          |<-------------------------------------|
          |                                      |
          |             Data 256-511             |
          |------------------------------------->|
          |                                      |
          |              Data ack                |
          |<-------------------------------------|
          |                                      |
          |             Data 512-767             |
          |------------------------------------->|
          |                                      |
          |              Data ack                |
          |<-------------------------------------|
          |                                      |
          .                                      .
          .                                      .
          .                                      .
          |           Data 17920-18175           |
          |------------------------------------->|
          |                                      |
          |              Data ack                |
          |<-------------------------------------|
          |                                      |
          |         Fast write response          |
          |<-------------------------------------|
          |                                      |
          |                                      |