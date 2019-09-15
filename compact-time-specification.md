Compact Time Format
===================

The compact time formats are encoding schemes to store a complete time, date, or timestamp in as few bytes as possible for data transmission.

Any Gregorian or proleptic Gregorian date, time, or timestamp can be recorded down to the nanosecond using this encoding.



Features
--------

 * Encodes a date into as few as 3 bytes.
 * Encodes a time into as few as 4 bytes.
 * Encodes a timestamp into as few as 5 bytes.
 * Supports unlimited positive and negative year values.
 * Supports time units down to the nanosecond.
 * Supports leap years and leap seconds.
 * Maintenance-free (no leap second tables to update).
 * Efficient conversion to/from human readable fields (no multiplication or division).
 * Time zones are location-based.



Contents
--------

* [Compact Date](#compact-date)
* [Compact Time](#compact-time)
* [Compact Timestamp](#compact-timestamp)
* [Sub-second Magnitude](#sub-second-magnitude)
* [Time Zone](#time-zone)
  - [Latitude-longitude](#latitude-longitude)
  - [Area-Location](#area-location)
    - [Abbreviated Areas](#abbreviated-areas)
    - [Special Areas](#special-areas)
  - [Comparison of Forms](#comparison-of-forms)
* [Year Encoding](#year-encoding)
* [Zigzag Integer](#zigzag-integer)
* [Proleptic Gregorian Calendar](#proleptic-gregorian-calendar)
* [Invalid Encoding](#invalid-encoding)
* [Examples](#examples)
* [How to Keep Time](#wow-to-keep-time)
* [License](#license)



Compact Date
------------

The compact date structure is composed of the following components:

| Component      | Required |
| -------------- | -------- |
| Base structure |     Y    |
| RVLQ Extension |     Y    |


### Base Structure

The base structure is 2 bytes wide, and is stored in little endian byte order.

The [`year` field](#year-encoding) is split across the `base structure` (upper bits) and the `RVLQ extension` (lower bits) to allow compression of leading zeros.

| Field                | Bits | Min | Max | Notes                                |
| -------------------- | ---- | --- | --- | ------------------------------------ |
| Year (upper bits)    |    7 |   * |   * | See: [year encoding](#year-encoding) |
| Month                |    4 |   1 |  12 |                                      |
| Day                  |    5 |   1 |  31 |                                      |


### RVLQ Extension

The RVLQ extension contains the lower bits of the `year` field, and is encoded as an [RVLQ](https://github.com/kstenerud/vlq/blob/master/vlq-specification.md).

| Field                | Bits | Min | Max | Notes                                 |
| -------------------- | ---- | --- | --- | ------------------------------------- |
| Year (lower bits)    |  7-* |   * |   * |  See: [year encoding](#year-encoding) |



Compact Time
------------

The compact time structure is composed of the following components:

| Component      | Required |
| -------------- | -------- |
| Base structure |     Y    |
| Time Zone      |     N    |


### Base Structure

The base structure is from 3 to 7 bytes wide (depending on the [`sub-second magnitude`](#sub-second-magnitude)), and is stored in little endian byte order.

| Field                | Bits | Min | Max       | Notes                                                |
| -------------------- | ---- | --- | --------- | ---------------------------------------------------- |
| RESERVED             |  0-6 |   0 |         0 | Brings the base structure to a multiple of 8 bits    |
| Sub-seconds          | 0-30 |   0 | 999999999 | Width and units determined by [sub-second magnitude](#sub-second-magnitude) |
| Second               |    6 |   0 |        60 | 60 to support leap seconds                           |
| Minute               |    6 |   0 |        59 |                                                      |
| Hour                 |    5 |   0 |        23 |                                                      |
| Sub-second Magnitude |    2 |   0 |         3 | Determines [sub-second width](#sub-second-magnitude) |
| Time Zone is UTC     |    1 |   0 |         1 | If 1, no time zone structure follows                 |

If the `time zone is utc` flag is 0, the base structure is followed by a [time zone](#time-zone).



Compact Timestamp
-----------------

The compact timestamp structure is composed of the following components:

| Component      | Required |
| -------------- | -------- |
| Base structure |     Y    |
| RVLQ Extension |     Y    |
| Time Zone      |     N    |


### Base Structure

The base structure is from 4 to 8 bytes wide (depending on the [`sub-second magnitude`](#sub-second-magnitude)), and is stored in little endian byte order.

The [`year` field](#year-encoding) is split across the `base structure` (upper bits) and the `RVLQ extension` (lower bits) to allow compression of leading zeros.

| Field                | Bits | Min | Max       | Notes                                                |
| -------------------- | ---- | --- | --------- | ---------------------------------------------------- |
| Year (upper bits)    |  0-6 |   * |         * | Brings the base structure to a multiple of 8 bits    |
| Sub-seconds          | 0-30 |   0 | 999999999 | Width and units determined by [sub-second magnitude](#sub-second-magnitude) |
| Month                |    4 |   1 |        12 |                                                      |
| Day                  |    5 |   1 |        31 |                                                      |
| Hour                 |    5 |   0 |        23 |                                                      |
| Minute               |    6 |   0 |        59 |                                                      |
| Second               |    6 |   0 |        60 | Max 60 to support leap seconds                       |
| Sub-second Magnitude |    2 |   0 |         3 | [Determines sub-second width](#sub-second-magnitude) |


### RVLQ Extension

The RVLQ extension contains the lower bits of the `year` field, as well as the `time zone is utc` flag, and is stored as an [RVLQ](https://github.com/kstenerud/vlq/blob/master/vlq-specification.md).

| Field                | Bits | Min | Max | Notes                                   |
| -------------------- | ---- | --- | --- | --------------------------------------- |
| Year (lower bits)    |  6-* |   * |   * | See: [year encoding](#year-encoding)    |
| Time Zone is UTC     |    1 |   0 |   1 | If 1, no time zone structure follows    |

If the `time zone is utc` flag is 0, the RVLQ extension is followed by a [time zone](#time-zone).



Sub-second Magnitude
--------------------

The `sub-second magnitude` field determines how many bits of sub-second data are present, and the units:

| Magnitude | Sub-second Bits | Units        | Min | Max       |
| --------- | --------------- | ------------ | --- | --------- |
|     0     |         0       | -            |   - |         - |
|     1     |        10       | milliseconds |   0 |       999 |
|     2     |        20       | microseconds |   0 |    999999 |
|     3     |        30       | nanoseconds  |   0 | 999999999 |



Time Zone
---------

A time zone can take one of two forms: `latitude-longitude`, or `area-location`.


### Latitude-Longitude

The latitude and longitude values are encoded into a 32-bit structure, stored in little endian byte order:

| Field         | Bits | Min     | Max    |
| ------------- | ---- | ------- | ------ |
| RESERVED      |    2 |       0 |      0 |
| Longitude     |   15 | -179.99 | 179.99 |
| Latitude      |   14 |  -90.00 |  90.00 |
| lat-long form |    1 |       1 |      1 |

Latitude and longitude are stored as two's complement signed integers representing hundredths of degrees. This gives a resolution of roughly 1 kilometer at the equator, which is enough to uniquely locate a time zone.

The location, combined with an associated date, refers to the time zone that the location falls under on that particular date. Location data should ideally be within the boundaries of a politically notable region whenever possible.

Note: Time zone values that contain different longitude/latitude values, but still refer to the same time zone at their particular time (for example, [48.85, 2.32] on Dec 10, 2010, and [48.90, 2.28] on Jan 1, 2000, which both refer to Europe/Paris in the same daylight savings mode), are considered equal.


### Area-Location

The area-location form makes use of time zone identifiers from the [IANA time zone database](https://www.iana.org/time-zones). A time zone is encoded as a length-delimited string in the form `Area/Location`.

| Field                | Bits | Min | Max | Notes                             |
| -------------------- | ---- | --- | --- | --------------------------------- |
| Length               |    7 |   1 | 127 |                                   |
| lat-long form        |    1 |   0 |   0 |                                   |

Followed by:

| Field                | Bits | Min | Max | Notes                             |
| -------------------- | ---- | --- | --- | --------------------------------- |
| String               |    * |   * |   * | [IANA time zone identifier](https://www.iana.org/time-zones) |


#### Abbreviated Areas

Since there are only a limited number of areas in the database, the following abbreviations may be used to save space in the area portion of the time zone:

| Area         | Abbreviation |
| ------------ | ------------ |
| `Africa`     | `F`          |
| `America`    | `M`          |
| `Antarctica` | `N`          |
| `Arctic`     | `R`          |
| `Asia`       | `S`          |
| `Atlantic`   | `T`          |
| `Australia`  | `U`          |
| `Etc`        | `C`          |
| `Europe`     | `E`          |
| `Indian`     | `I`          |
| `Pacific`    | `P`          |

#### Special Areas

The following special values may also be used. They do not contain a location component.

| Area    | Abbreviation | Meaning            |
| ------- | ------------ | ------------------ |
| `Zero`  | `Z`          | Alias to `Etc/UTC` |
| `Local` | `L`          | "Local" time zone, meaning that the accompanying time value is to be interpreted as if in the time zone of the observer. |


### Comparison of Forms

There are benefits and drawbacks to consider when choosing which form to use for time zones.

#### Latitude-Longitude

  - Smaller size.
  - Impervious to the effects of changing names or boundaries over time.

#### Area-Location

  - More widely implemented.
  - Less complex decoding procedure.
  - Human decodable without a database.



Year Encoding
-------------

Years are encoded as [zigzag signed integers](#zigzag-integer) representing the number of years relative to the epoch date 2000-01-01 00:00:00. Dates closer to this epoch can be stored in fewer bits.

A `year` field will be split across two structures, with the upper bits in the fixed length structure, and the lower bits in the variable length structure. This allows an unlimited year range while keeping the leading zero bits compressible.



Zigzag Integer
--------------

A zigzag signed integer is an encoding scheme which encodes positive and negative values in a manner that ensures the upper bits remain cleared in order to support leading zero bit compression. It's most commonly used in conjunction with variable-length encodings, for example in [Google's protocol buffers](https://developers.google.com/protocol-buffers/docs/encoding#signed-integers).

In zigzag encoding, the sign is encoded into the low bit rather than the high bit of the value, such that incrementing the encoded binary representation alternates between negative and positive values.

| Original | Encoded | Bits    |
| -------- | ------- | ------- |
|        0 |       0 | `000 0` |
|       -1 |       1 | `000 1` |
|        1 |       2 | `001 0` |
|       -2 |       3 | `001 1` |
|        2 |       4 | `010 0` |

Assuming a signed integer size of 32, values would be encoded like so:

    encoded_value = (signed_value << 1) ^ (signed_value >> 31)

Where `<<` and `>>` are arithmetic bit shifts, and `^` is the logical XOR operator.



Proleptic Gregorian Calendar
-----------------------------

Dates prior to the introduction of the Gregorian Calendar in 1582 must be stored according to the proleptic Gregorian calender.



Invalid Encoding
----------------

If any field contains values outside of its allowed range, the entire time/date/timestamp is invalid.

RESERVED fields must contain all zero bits. If a RESERVED field contains any `1` bits, the time/date/timestamp is invalid.



Examples
--------

### Example 1:

    June 24, 2019, 17:53:04.180 UTC

Year is calculated as `2000` + `19 (0x13)`, encoded as zigzag: `0x26`.

The sub-second portion of this date goes to the millisecond, which requires a magnitude field of 1. Magnitude 1 implies a base structure of 40 bits, containing 2 high bits of year data to bring the structure to a multiple of 8 bits.

Base structure (40 bits):

| Field                | Width | Value | Encoded      |
| -------------------- | ----- | ----- | ------------ |
| Year (high bits)     |     2 |     0 | `        00` |
| Sub-seconds          |    10 |   180 | `0010110100` |
| Month                |     4 |     6 | `      0110` |
| Day                  |     5 |    24 | `     11000` |
| Hour                 |     5 |    17 | `     10001` |
| Minute               |     6 |    53 | `    110101` |
| Second               |     6 |     4 | `    000100` |
| Sub-second Magnitude |     2 |     1 | `        01` |

RVLQ:

| Field                | Width | Value | Encoded  |
| -------------------- | ----- | ----- | -------- |
| Continuation         |     1 |     0 | `     0` |
| Year (next 7 bits)   |     6 |  0x26 | `100110` |
| Time Zone is UTC     |     1 |     1 | `     1` |

Encoded value:

    Base:          00 0010110100 0110 11000 10001 110101 000100 01
                   00001011 01000110 11000100 01110101 00010001
                   0x0b     0x46     0xc4     0x75     0x11
    Little Endian: 0x11, 0x75, 0xc4, 0x46, 0x0b

    RVLQ:          0 100110 1
                   01001101
                   0x4d

    Encoded: [11 75 c4 46 0b 4d]


### Example 2:

    January 7, 5000


Year is calculated as `2000` + `3000`, encoded as zigzag: `0x1770`.

     0101110 1110000

Base structure:

| Field                | Width | Value | Encoded   |
| -------------------- | ----- | ----- | --------- |
| Day                  |     5 |     7 | `  00111` |
| Month                |     4 |     1 | `   0001` |
| Year (upper bits)    |     7 |  0x2e | `0101110` |

RVLQ:

| Field                | Width | Value | Encoded   |
| -------------------- | ----- | ----- | --------- |
| Continuation         |     1 |     0 | `      0` |
| Year (next 7 bits)   |     7 |  0x70 | `1110000` |

Encoded value:

    Base:          00111 0001 0101110
                   00111000 10101110
                   0x38     0xae
    Little Endian: 0xae 0x38

    RVLQ:          0 1110000
                   01110000
                   0x70

    Encoded: [ae 38 70]


### Example 3:

    00:54:47.394129115, Europe/Paris

The sub-second portion of this date goes to the nanosecond, which requires a magnitude field of 3. Magnitude 3 implies a base structure of 56 bits, containing 6 RESERVED bits.

Base structure (56 bits):

| Field                | Width | Value     | Encoded                          |
| -------------------- | ----- | --------- | -------------------------------- |
| RESERVED             |     6 |         0 | `                        000000` |
| Sub-seconds          |    30 | 394129115 | `010111011111011110111011011011` |
| Second               |     6 |        47 | `                        101111` |
| Minute               |     6 |        54 | `                        110110` |
| Hour                 |     5 |        00 | `                         00000` |
| Sub-second Magnitude |     2 |         3 | `                            11` |
| Time Zone is UTC     |     1 |         0 | `                             0` |

    Base:          000000 010111011111011110111011011011 101111 110110 00000 11 0
                   00000001 01110111 11011110 11101101 10111011 11110110 00000110
                   0x01     0x77     0xde     0xed     0xbb     0xf6     0x06
    Little Endian: 0x06 0xf6 0xbb 0xed 0xde 0x77 0x01

#### If using latitude-longitide:

Time Zone:

| Field         | Width | Value | Encoded           |
| ------------- | ----- | ----- | ----------------- |
| RESERVED      |     2 |     0 | `             00` |
| Longitude     |    15 |  2.32 | `000000011101000` |
| Latitude      |    14 | 48.85 | ` 01001100010101` |
| lat-long form |     1 |     1 | `              1` |


    Time Zone:     00 000000011101000 01001100010101 1
                   00000000 01110100 00100110 00101011
                   0x00     0x74     0x26     0x2b
    Little Endian: 0x2b 0x26 0x74 0x00

    Encoded: [06 f6 bb ed de 77 01 2b 26 74 00]


#### If using area/location:

Time Zone:

| Field           | Width | Value     | Encoded                  |
| --------------- | ----- | --------- | ------------------------ |
| Length          |     7 |         7 |                `0000111` |
| lat-long form   |     1 |         0 |                `      0` |
| string contents |    56 | "E/Paris" | `[45 2f 50 61 72 69 73]` |

    Time Zone: 0x0e [45 2f 50 61 72 69 73]

    Encoded: [06 f6 bb ed de 77 01 0e 45 2f 50 61 72 69 73]



How to Keep Time
----------------

Time is one of the most difficult data types to get right. Aside from issues of synchronization, leap seconds, data container limitations and such, it's important to choose the correct **kind** of time to store, and the right kind depends on what the purpose of recording the time is.

There are three main kinds of time:

#### Absolute Time

Absolute time is a time that is fixed relative to UTC (or relative to an offset from UTC). It is not affected by daylight savings time, nor will it ever change if an area's time zone changes for political reasons. Absolute time is best recorded in the UTC time zone, and is mostly useful for events in the past (because the time zone is now fixed at the time of the event, so it no longer matters what specific time zone was in effect).

#### Fixed Time

Fixed time is a time that is fixed to a particular place, and that place has a time zone associated with it (but the time zone may change for political reasons in the future). If the venue changes, only the time zone data needs to be updated. An example would be an appointment in London this coming October 12th at 10:30.

#### Floating Time

Floating (or local) time is always relative to the time zone of the observer. If you travel and change time zones, floating time changes zones with you. If someone else looks at a floating time, it will have a different absolute value from yours if you happen to be in different time zones. An example would be an 8:00 morning workout.


### When to Use Each Kind

Use whichever kind of time most succinctly and completely handles your time needs. Don't depend on time zone information as a proxy for a location; that's depending on a side effect, which is always brittle. Store location information separately if it's important.

| Situation           | Kind                                            |
| ------------------- | ----------------------------------------------- |
| Recording an event  | Absolute                                        |
| Log entries         | Absolute                                        |
| An appointment      | Fixed                                           |
| Your daily schedule | Floating                                        |
| Deadlines           | Usually fixed time, but possibly absolute time. |



License
-------

Copyright 2019 Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/
