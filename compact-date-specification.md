Compact Date Format
===================

Compact date format is an encoding scheme to store a complete date in as few bytes as possible for data transmission. Any Gregorian or propleptic Gregorian date can be recorded to the nanosecond using this encoding.

Dates are stored relative to the epoch date of January 1st, 2000, with a UTC timezone unless accompanying timezone information is provided (accompanying time zone information is outside of the scope of this document).



Features
--------

 * Encodes a complete date & time into as few as 5 bytes.
 * Maintenance-free (no leap second tables to update).
 * Efficient conversion to/from human readable fields (no multiplication or division).
 * Supports unlimited positive and negative years.
 * Supports time units down to the nanosecond.
 * Supports leap years and leap seconds.
 * Dates are relative to UTC by default.



Encoded Structure
-----------------

The date structure is conceptually an unsigned integer with the following bit encoded fields:

| Field                | Bits | Min | Max       | Notes                      |
| -------------------- | ---- | --- | --------- | -------------------------- |
| Sub-second Magnitude |    2 |   0 |         3 |                            |
| Sub-seconds          | 0-30 |   0 | 999999999 | Variable bit-width         |
| Second               |    6 |   0 |        60 | 60 to support leap seconds |
| Minute               |    6 |   0 |        59 |                            |
| Hour                 |    5 |   0 |        23 |                            |
| Day                  |    5 |   1 |        31 |                            |
| Month                |    4 |   1 |        12 |                            |
| Year                 |    * |   * |         * | RVLQ, relative to 2000     |

All fields up to and including the upper bits of the year field are encoded as a single, big endian encoded value with a base size from 4 to 8 bytes (depending on the presence and type of sub-second data). The remaining lower year bits are encoded into a [RVLQ](https://github.com/kstenerud/vlq/blob/master/vlq-specification.md).

The `sub-second magnitude` field determines how many bits of sub-second data are present, which affects the minimum size of the entire structure, the minimum number of year bits available (including those in the initial RVLQ), and the year range possible at the minimal byte size:

| Value | Sub-second Data             | Min Bytes | Min Year Bits | Min Year Range |
| ----- | --------------------------- | --------- | ------------- | -------------- |
|   00  | No sub-second data          |         5 |            11 |     976 - 3023 |
|   01  | 10 bits of millisecond data |         6 |             9 |    1744 - 2255 |
|   10  | 20 bits of microsecond data |         7 |             7 |    1872 - 2127 |
|   11  | 30 bits of nanosecond data  |         9 |            13 |   -2096 - 6095 |

The lower bits of the base structure contain the upper bits of the year field, which is then continued in the RVLQ that follows (minimum 1 byte). More year bits can be added by extending the RVLQ.

| Field                | Bits | Notes                   |
| -------------------- | ---- | ----------------------- |
| Sub-second Magnitude |    2 | High bit                |
| Sub-seconds          | 0-30 |                         |
| Second               |    6 |                         |
| Minute               |    6 |                         |
| Hour                 |    5 |                         |
| Day                  |    5 |                         |
| Month                |    4 |                         |
| Year (high bits)     |  0-6 | Low bit                 |
| Continuation         |    1 | RVLQ start              |
| Year (next 7 bits)   |    7 |                         |
| ...                  |  ... | RVLQ continues (or not) |

The year field is encoded as a [zigzag signed integer](#zigzag-integer), with a value relative to the epoch date 00:00:00 on January 1st, 2000. Dates closer to this epoch can be stored in fewer bits.



Zigzag Integer
--------------

A zigzag signed integer is an encoding scheme which encodes positive and negative values that are closer to 0 in fewer bits. Its most popular use is in [Google's protocol buffers](https://developers.google.com/protocol-buffers/docs/encoding#signed-integers).

In zigzag encoding, the sign is encoded in the low bit rather than the high bit of the value, such that incrementing the encoded binary representation alternates between negative and positive values.

| Original | Encoded |
| -------- | ------- |
|        0 |       0 |
|       -1 |       1 |
|        1 |       2 |
|       -2 |       3 |
|        2 |       4 |

Assuming a signed integer size of 32, values would be encoded like so:

    (signed_value << 1) ^ (signed_value >> 31)

Where `<<` and `>>` are arithmetic bit shifts, and `^` is the logical XOR operator.



Proleptic Gregorian Calendar
-----------------------------

Dates prior to the introduction of the Gregorian Calendar in 1582 must be stored according to the proleptic Gregorian calender.



Time Zone
---------

Values must be relative to UTC, unless other outside accompanying data or agreements specify otherwise.



Invalid Encoding
----------------

If any field contains values outside of its allowed range, the time value is invalid.



Examples
--------

### Example 1:

    June 24, 2019, 17:53:04.180

Year is calculated as `2000` + `19 (0x13)`, encoded as zigzag: `0x26`.

The sub-second portion of this date goes to the millisecond, which requires a magnitude field of 1. Magnitude 1 implies a base structure of 40 bits, containing 2 high bits of year data.

Initial 40 bit structure:

| Field                | Width | Value | Encoded    |
| -------------------- | ----- | ----- | ---------- |
| Sub-second Magnitude |     2 |     1 |         01 |
| Sub-seconds          |    10 |   180 | 0010110100 |
| Second               |     6 |     4 |     000100 |
| Minute               |     6 |    53 |     110101 |
| Hour                 |     5 |    17 |      10001 |
| Day                  |     5 |    24 |      11000 |
| Month                |     4 |     6 |       0110 |
| Year (high bits)     |     2 |     0 |         00 |

RVLQ:

| Field                | Width | Value | Encoded    |
| -------------------- | ----- | ----- | ---------- |
| Continuation         |     1 |     0 |          0 |
| Year (next 7 bits)   |     7 |  0x26 |    0100110 |

Encoded value:

    Base:    01 0010110100 000100 110101 10001 11000 0110 00
             01001011 01000001 00110101 10001110 00011000
             0x4b     0x41     0x35     0x8e     0x18

    RVLQ:    0 0100110
             00100110
             0x26

    Encoded: [4b 41 35 8e 18 26]


### Example 2:

    January 7, 1998, 08:19:20

There is no sub-second portion to this date, and so we use a magnitude of 0. Magnitude 0 implies a base structure of 32 bits, containing 4 bits of year data.

Year is calculated as `2000` - `2`, encoded as zigzag: `0x03`.

Initial 32 bit structure:

| Field                | Width | Value | Encoded    |
| -------------------- | ----- | ----- | ---------- |
| Sub-second Magnitude |     2 |     0 |         00 |
| Sub-seconds          |     0 |     - |          - |
| Second               |     6 |    20 |     010100 |
| Minute               |     6 |    19 |     010011 |
| Hour                 |     5 |     8 |      01000 |
| Day                  |     5 |     7 |      00111 |
| Month                |     4 |     1 |       0001 |
| Year (high bits)     |     4 |     0 |       0000 |

RVLQ:

| Field                | Width | Value | Encoded    |
| -------------------- | ----- | ----- | ---------- |
| Continuation         |     1 |     0 |          0 |
| Year (next 7 bits)   |     7 |  0x03 |    0000011 |

Encoded value:

    Base:    00 010100 010011 01000 00111 0001 0000
             00010100 01001101 00000111 00010000
             0x14     0x4d     0x07     0x10

    RVLQ:    0 0000011
             00000011
             0x03

    Encoded: [14 4d 07 10 03]


### Example 3:

    August 31, 3190, 00:54:47.394129

Magnitude is 3 to support nanoseconds. Our base structure is 64 bits, containing 6 high year bits.

Year is calculated as `2000` + `1190 (0x4a6)`, encoded as zigzag: `0x94c`.
 1001 0100 1100
Initial 48 bit structure:

| Field                | Width | Value  | Encoded              |
| -------------------- | ----- | ------ | -------------------- |
| Sub-second Magnitude |     2 |      2 |                   10 |
| Sub-seconds          |    30 | 394129 | 01100000001110010001 |
| Second               |     6 |     47 |               101111 |
| Minute               |     6 |     54 |               110110 |
| Hour                 |     5 |      0 |                00000 |
| Day                  |     5 |     31 |                11111 |
| Month                |     4 |      8 |                 1000 |
| Year (high bits)     |     0 |      - |                    - |

The year value 0x94c (`100101001100`) is encoded into 14 bits of RVLQ data:

| Field                | Width | Value | Encoded    |
| -------------------- | ----- | ----- | ---------- |
| Continuation         |     1 |     1 |          1 |
| Year (next 7 bits)   |     7 |  0x03 |    0010010 |
| Continuation         |     1 |     0 |          0 |
| Year (next 7 bits)   |     7 |  0x03 |    1001100 |

Encoded value:

    Base:    10 01100000001110010001 101111 110110 00000 11111 1000
             10011000 00001110 01000110 11111101 10000001 11111000
             0x98     0x0e     0x46     0xfd     0x81     0xf8

    RVLQ:    1 0010010 0 1001100
             10010010 01001100
             0x92     0x4c

    Encoded: [98 0e 46 fd 81 f8 92 4c]



License
-------

Copyright 2019 Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/
