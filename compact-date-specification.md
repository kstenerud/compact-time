Compact Date Format
===================

Compact date format is an encoding scheme to store a complete date, down to the nanosecond, in as few bytes as possible.

Dates are stored relative to the epoch date of January 1st, 2000, with a UTC timezone unless accompanying timezone information is provided (accompanying time zone information is outside of the scope of this document).



Features
--------

 * Encodes a complete date & time into as few as 32 bits.
 * Maintenance-free (no leap second tables to update).
 * Easily converts to human readable fields.
 * Supports all years in both directions, in perpetuity.
 * Supports time units to the nanosecond.
 * Supports leap years and leap seconds.
 * Dates are in UTC by default.



Encoded Structure
-----------------

The date structure is conceptually an unsigned integer with the following bit encoded fields:

| Field                | Bits | Min | Max       | Notes                      |
| -------------------- | ---- | --- | --------- | -------------------------- |
| Year                 |    * |   * |         * | Variable bit-width         |
| Sub-seconds          | 0-30 |   0 | 999999999 | Variable bit-width         |
| Month                |    4 |   1 |        12 |                            |
| Day                  |    5 |   1 |        31 |                            |
| Hour                 |    5 |   0 |        23 |                            |
| Minute               |    6 |   0 |        59 |                            |
| Second               |    6 |   0 |        60 | 60 to support leap seconds |
| Sub-second Magnitude |    2 |   0 |         3 | Low bit                    |

All fields up to and including the lower bits of the year field are encoded as a single, little endian encoded value with a base size from 4 to 8 bytes (depending on the presence and type of sub-second data).

The `sub-second magnitude` field determines how many bits of sub-second data are present, and also by extension how many initial (lower) year bits are present and the structure's base size:

| Value | Sub-second Data                            | Base Size (bytes) | Year (low bits) |
| ----- | ------------------------------------------ | ----------------- | --------------- |
|   00  | No sub-second data                         |                 4 |               3 |
|   01  | 10 bits of data, representing milliseconds |                 5 |               1 |
|   10  | 20 bits of data, representing microseconds |                 7 |               7 |
|   11  | 30 bits of data, representing nanoseconds  |                 8 |               5 |

The upper bits of the base structure contain the lower bits of the year field, with the highest bit representing a "continuation" bit.

| Field                | Bits | Notes    |
| -------------------- | ---- | -------- |
| Continuation         |    1 | High bit |
| Year (low bits)      |  1-7 |          |
| Sub-seconds          | 0-30 |          |
| Month                |    4 |          |
| Day                  |    5 |          |
| Hour                 |    5 |          |
| Minute               |    6 |          |
| Second               |    6 |          |
| Sub-second Magnitude |    2 | Low bit  |

If the continuation bit is 1, then the `year` field's next higher group of 7 bits are stored in the subsequent byte as a base-128 value (where the lower 7 bits are data and the high bit is another continuation bit). This pattern continues until a byte with a cleared (`0`) continuation bit is encountered.

The year field is encoded as a [zigzag signed integer](#zigzag-integer), with a value relative to the epoch date 00:00:00 on January 1st, 2000. Dates closer to this epoch date can be stored in fewer bits.



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

The sub-second portion of this date goes to the millisecond, which requires a magnitude field of 1. Magnitude 1 implies a base structure of 40 bits, containing 1 bit of year data.

Year field: `19 (0x13)`, as zigzag: `0x26`. This doesn't fit into 1 bit, so we need 1 byte continuation data.

Initial 40 bit structure:

| Field                | Width | Value | Encoded    |
| -------------------- | ----- | ----- | ---------- |
| Continuation         |     1 |     1 |          1 |
| Year (low bits)      |     1 |     1 |          0 |
| Sub-seconds          |    10 |   180 | 0010110100 |
| Month                |     4 |     6 |       0110 |
| Day                  |     5 |    24 |      11000 |
| Hour                 |     5 |    17 |      10001 |
| Minute               |     6 |    53 |     110101 |
| Second               |     6 |     4 |     000100 |
| Sub-second Magnitude |     2 |     1 |         01 |

Continued group:

| Field                | Width | Value | Encoded |
| -------------------- | ----- | ----- | ------- |
| Continuation         |     1 |     0 |       0 |
| Year (next 7 bits)   |     7 |    19 | 0010011 |

Encoded value:

    Continued: 0 0010011
               0x13
    Base:      1 0 0010110100 0110 11000 10001 110101 000100 01
               10001011 01000110 11000100 01110101 00010001
               0x8b     0x46     0xc4     0x75     0x11

    Little endian encoded: [11 75 c4 46 8b 13]


### Example 2:

    January 7, 1998, 08:19:20

There is no sub-second portion to this date, and so we use a magnitude of 0. Magnitude 0 implies a base structure of 32 bits, containing 3 bits of year data.

Year field: `-2`, as zigzag: `0x03`. This fits into 3 bits, so we don't need continuation data.

Initial 32 bit structure:

| Field                | Width | Value | Encoded    |
| -------------------- | ----- | ----- | ---------- |
| Continuation         |     1 |     1 |          0 |
| Year (low bits)      |     3 |     3 |        011 |
| Sub-seconds          |     0 |     - |          - |
| Month                |     4 |     1 |       0001 |
| Day                  |     5 |     7 |      00111 |
| Hour                 |     5 |     8 |      01000 |
| Minute               |     6 |    19 |     010011 |
| Second               |     6 |    20 |     010100 |
| Sub-second Magnitude |     2 |     0 |         00 |

Encoded value:

    Base: 0 011 0001 00111 01000 010011 010100 00
          00110001 00111010 00010011 01010000
          0x31     0x3a     0x13     0x50

    Little endian encoded: [50 13 3a 31]



License
-------

Copyright 2019 Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/
