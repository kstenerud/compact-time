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
 * Supports IANA time zones and latitude/longitude time zones.



Contents
--------

* [General Structure](#general-structure)
* [Compact Date](#compact-date)
* [Compact Time](#compact-time)
* [Compact Timestamp](#compact-timestamp)
* [Sub-second Magnitude](#sub-second-magnitude)
* [Year Encoding](#year-encoding)
  - [Zigzag Integer](#zigzag-integer)
* [Proleptic Gregorian Calendar](#proleptic-gregorian-calendar)
* [Time Zone](#time-zone)
  - [Area-Location](#area-location)
    - [Abbreviated Areas](#abbreviated-areas)
    - [Special Areas](#special-areas)
  - [Latitude-longitude](#latitude-longitude)
  - [Comparison of Forms](#comparison-of-forms)
* [Invalid Encoding](#invalid-encoding)
  - [Zero Value](#zero-value)
* [Examples](#examples)
* [How to Keep Time](#how-to-keep-time)
* [License](#license)



General Structure
-----------------

All date and time structures are composed of bitfields, which are either encoded as fixed width data, or a combined fixed width and variable width portion:

| Type                            | Encoding         |
| ------------------------------- | ---------------- |
| [Date](#compact-date)           | Fixed + Variable |
| [Time](#compact-time)           | Fixed            |
| [Timestamp](#compact-timestamp) | Fixed + Variable |

Fixed width data is encoded as a series of octets in little endian byte order, and variable width data is encoded as an [unsigned LEB128](https://en.wikipedia.org/wiki/LEB128).



Compact Date
------------

A compact date's fields are laid out as follows:

| Field                | Bits | Min | Max | Notes                                |
| -------------------- | ---- | --- | --- | ------------------------------------ |
| Year                 |  14+ |   * |   * | See: [year encoding](#year-encoding) |
| Month                |   4  |   1 |  12 |                                      |
| Day                  |   5  |   1 |  31 |                                      |

The fixed portion is 16 bits wide, and the variable portion uses a minimum of 1 octet.


#### Example: Dec 31, 3000

| Field | Width | Value | Encoded          | Notes                                                      |
| ----- | ----- | ----- | ---------------- | ---------------------------------------------------------- |
| Year  |    14 |  2000 | `00011111010000` | (Year 3000) - (bias of 2000) = 1000, encoded zigzag = 2000 |
| Month |     4 |    12 | `          1100` |                                                            |
| Day   |     5 |    31 | `         11111` |                                                            |

Layout:

    | Var | | --- Fixed --- |
    0001111 10100001 10011111

Encoded: `[9f a1 0f]`



Compact Time
------------

A compact time's fields are laid out as follows:

| Field                | Bits | Min | Max       | Notes                                                |
| -------------------- | ---- | --- | --------- | ---------------------------------------------------- |
| RESERVED             |  0-6 |   0 |         0 | Brings the base structure to a multiple of 8 bits    |
| Hour                 |    5 |   0 |        23 |                                                      |
| Minute               |    6 |   0 |        59 |                                                      |
| Second               |    6 |   0 |        60 | 60 to support leap seconds                           |
| Sub-seconds          | 0-30 |   0 | 999999999 | Width and units determined by [sub-second magnitude](#sub-second-magnitude) |
| Sub-second Magnitude |    2 |   0 |         3 | Determines [sub-second width](#sub-second-magnitude) |
| Time Zone Present    |    1 |   0 |         1 | If 1, a time zone structure follows                  |

The structure will be anywhere from 24 to 56 bits wide (determined by the [`sub-second magnitude`](#sub-second-magnitude)).

| Magnitude | Total Width | Sub-Second Width | Reserved Field Width |
| --------- | ----------- | ---------------- | -------------------- |
|     0     |      24     |         0        |           4          |
|     1     |      32     |        10        |           2          |
|     2     |      40     |        20        |           0          |
|     3     |      56     |        30        |           6          |

If the `Time Zone Present` flag is 1, the time structure is followed by a [time zone structure](#time-zone). If the flag is 0, the time zone is UTC.

The RESERVED field must always be set to all 1 bits.


#### Example: 23:59:59 UTC

| Field                | Width | Value | Encoded  |
| -------------------- | ----- | ----- | -------- |
| RESERVED             |     4 |     * | `  1111` |
| Hour                 |     5 |    23 | ` 10111` |
| Minute               |     6 |    59 | `111011` |
| Second               |     6 |    59 | `111011` |
| Sub-seconds          |     0 |     0 |          |
| Sub-second Magnitude |     2 |     0 | `    00` |
| Time Zone Present    |     1 |     0 | `     0` |

Layout:

    | ------- Fixed -------- |
    11111011 11110111 11011000

Encoded: `[d8 f7 fb]`



Compact Timestamp
-----------------

A compact timestamp's fields are laid out as follows:

| Field                | Bits | Min | Max       | Notes                                                |
| -------------------- | ---- | --- | --------- | ---------------------------------------------------- |
| Year                 |   8+ |   * |         * | See: [year encoding](#year-encoding)                 |
| Month                |    4 |   1 |        12 |                                                      |
| Day                  |    5 |   1 |        31 |                                                      |
| Hour                 |    5 |   0 |        23 |                                                      |
| Minute               |    6 |   0 |        59 |                                                      |
| Second               |    6 |   0 |        60 | Max 60 to support leap seconds                       |
| Sub-seconds          | 0-30 |   0 | 999999999 | Width and units determined by [sub-second magnitude](#sub-second-magnitude) |
| Sub-second Magnitude |    2 |   0 |         3 | Determines [sub-second width](#sub-second-magnitude) |
| Time Zone Present    |    1 |   0 |         1 | If 1, a time zone structure follows                  |

The fixed portion will be anywhere from 32 to 64 bits wide (determined by the [`sub-second magnitude`](#sub-second-magnitude)), and the variable portion uses a minimum of 1 octet.

| Magnitude | Fixed Width | Sub-Second Width | Min Year Width |
| --------- | ----------- | ---------------- | -------------- |
|     0     |      32     |         0        |       10       |
|     1     |      40     |        10        |        8       |
|     2     |      56     |        20        |       14       |
|     3     |      64     |        30        |       12       |

If the `Time Zone Present` flag is 1, the time structure is followed by a [time zone structure](#time-zone). If the flag is 0, the time zone is UTC.


#### Example: Dec 31, 2000, 23:59:59

| Field                | Width | Value | Encoded      | Notes                                                |
| -------------------- | ----- | ----- | ------------ | ---------------------------------------------------- |
| Year                 |    10 |     0 | `0000000000` | (Year 2000) - (bias of 2000) = 0, encoded zigzag = 0 |
| Month                |     4 |    12 | `      1100` |                                                      |
| Day                  |     5 |    31 | `     11111` |                                                      |
| Hour                 |     5 |    23 | `     10111` |                                                      |
| Minute               |     6 |    59 | `    111011` |                                                      |
| Second               |     6 |    59 | `    111011` |                                                      |
| Sub-seconds          |     0 |     0 |              |                                                      |
| Sub-second Magnitude |     2 |     0 | `        00` |                                                      |
| Time Zone Present    |     1 |     0 | `         0` |                                                      |

Layout:

    | Var | | ------------ Fixed ------------ |
    0000000 00011001 11111011 11110111 11011000

Encoded: `[d8 f7 fb 19 00]`



Sub-second Magnitude
--------------------

The `sub-second magnitude` field determines how many bits of `sub-second` data are present, and the units:

| Magnitude | Sub-second Bits | Units        | Min | Max       |
| --------- | --------------- | ------------ | --- | --------- |
|     0     |         0       | -            |   - |         - |
|     1     |        10       | milliseconds |   0 |       999 |
|     2     |        20       | microseconds |   0 |    999999 |
|     3     |        30       | nanoseconds  |   0 | 999999999 |



Year Encoding
-------------

The year field can be any number of digits, and can be positive (representing AD dates) or negative (representing BC dates).

Note: The Anno Domini system has no zero year (there is no 0 BC or 0 AD), thus year value `0` is invalid. Although many date systems internally use the value 0 to represent 1 BC and offset all BC dates by 1 for mathematical continuity, it's preferable in interchange formats to avoid potential confusion from such tricks.

Years are encoded as [zigzag signed integers](#zigzag-integer) representing the number of years relative to the epoch date 2000-01-01 00:00:00. Dates closer to this epoch can be stored in fewer bits.


### Zigzag Integer

[Zigzag encoding](https://en.wikipedia.org/wiki/Variable-length_quantity#Zigzag_encoding) is a scheme that encodes signed integers such that both positive and negative values have their upper bits cleared. This helps with variable-length encodings, which typically compress high order zero bits.

In zigzag encoding, the sign is encoded into the low bit rather than the high bit of the value, such that incrementing the encoded binary representation alternates between negative and positive values.

| Original | Bits       | Encoded     |
| -------- | ---------- | ----------- |
|        0 | `00000000` | `0000000 0` |
|       -1 | `11111111` | `0000000 1` |
|        1 | `00000001` | `0000001 0` |
|       -2 | `11111110` | `0000001 1` |
|        2 | `00000010` | `0000010 0` |

Assuming a signed integer size of 32, values would be encoded like so:

    encoded_value = (signed_value << 1) ^ (signed_value >> 31)

Where `<<` and `>>` are arithmetic bit shifts, and `^` is the logical XOR operator.



Proleptic Gregorian Calendar
-----------------------------

Dates prior to the introduction of the Gregorian Calendar in 1582 must be stored according to the proleptic Gregorian calender.



Time Zone
---------

A time zone can take one of two forms: `area-location` or `latitude-longitude`. The form is determined by the `lat-long form` flag.


### Area-Location

The area-location form makes use of time zone identifiers from the [IANA time zone database](https://www.iana.org/time-zones). A time zone is encoded as a length-delimited string in the form `Area/Location`.

| Field                | Bits | Min | Max |
| -------------------- | ---- | --- | --- |
| Length               |    7 |   1 | 127 |
| lat-long form        |    1 |   0 |   0 |

Followed by:

| Field                | Bits | Min | Max | Notes                             |
| -------------------- | ---- | --- | --- | --------------------------------- |
| String               |    * |   * |   * | [IANA time zone identifier](https://www.iana.org/time-zones) |


#### Abbreviated Areas

Since there are only a limited number of areas in the database, the following area abbreviations can be used to save space in the area portion of the time zone:

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

The following special values can also be used. They do not contain a location component.

| Area    | Abbreviation | Meaning            |
| ------- | ------------ | ------------------ |
| `Zero`  | `Z`          | Alias to `Etc/UTC` |
| `Local` | `L`          | "Local" time zone, meaning that the accompanying time value is to be interpreted as if in the time zone of the observer. |


### Latitude-Longitude

The latitude and longitude values are encoded into a 32-bit structure, stored in little endian byte order:

| Field         | Bits | Min     | Max    |
| ------------- | ---- | ------- | ------ |
| Longitude     |   16 | -180.00 | 180.00 |
| Latitude      |   15 |  -90.00 |  90.00 |
| lat-long form |    1 |       1 |      1 |

Latitude and longitude are stored as two's complement signed integers representing hundredths of degrees. This gives a resolution of roughly 1 kilometer at the equator, which is enough to uniquely locate a time zone.

The location, combined with an associated date, refers to the time zone that the location falls under on that particular date. Location data should ideally be within the boundaries of a politically notable region whenever possible.

Note: Time zone values that contain different longitude/latitude values, but still refer to the same time zone at their particular time, are considered equal. For example: `[48.85, 2.32] on Dec 10, 2010`, and `[48.90, 2.28] on Jan 1, 2000`: both refer to Europe/Paris in the same daylight savings mode.


### Comparison of Forms

There are benefits and drawbacks to consider when choosing which form to use for time zones.

Area-Location:

  - More widely implemented.
  - More recognizable to humans.

Latitude-Longitude:

  - Smaller size.
  - Impervious to the effects of changing names or boundaries over time.



Invalid Encoding
----------------

If any field contains values outside of its allowed range, the entire time/date/timestamp is invalid.

RESERVED fields must contain all one bits. If a RESERVED field contains any `0` bits, the time/date/timestamp is invalid.


### Zero Values

Values made of all zero bits are guaranteed to be invalid:

* Date value [`00 00 00`] has 0 for month and day, which is invalid.
* Time value [`00 00 00`] has 0 bits in the 4-bit reserved section, which is invalid.
* Timestamp value [`00 00 00 00 00`] has 0 for month and day, which is invalid.

These invalid encodings can be useful as a marker to represent unset or missing time values.



Examples
--------

### Example: June 24, 2019, 17:53:04.180 UTC

Year is calculated as `2000` + `19 (0x13)`, encoded as zigzag: `0x26` (`100110`).

| Field                | Width | Value | Encoded      |
| -------------------- | ----- | ----- | ------------ |
| Year                 |     8 |  0x26 | `  00100110` |
| Month                |     4 |     6 | `      0110` |
| Day                  |     5 |    24 | `     11000` |
| Hour                 |     5 |    17 | `     10001` |
| Minute               |     6 |    53 | `    110101` |
| Second               |     6 |     4 | `    000100` |
| Sub-seconds          |    10 |   180 | `0010110100` |
| Sub-second Magnitude |     2 |     1 | `        01` |
| Time Zone Present    |     1 |     0 | `         0` |

    Data:    00100110 0110 11000 10001 110101 000100 0010110100 01 0

    Split:   | Var |  | ----------------- Fixed ---------------- |
             0010011  00110110 00100011 10101000 10000101 10100010

    Encoded: [a2 85 a8 23 36 13]


### Example: January 7, 40000

Year is calculated as `2000` + `38000` (`0x9470`), encoded as zigzag: `0x128e0` (`10010100011100000`).

| Field | Width | Value  | Encoded             |
| ----- | ----- | ------ | ------------------- |
| Year  |     7 | 0x9470 | `10010100011100000` |
| Month |     4 |      1 | `             0001` |
| Day   |     5 |      7 | `            00111` |

    Data:    10010100011100000 0001 00111

    Split:   |   Var  |  | --- Fixed --- |
             1001010001  11000000 00100111

    ULEB128: | ---- Var ---- |  | --- Fixed --- |
             00000100 11010001  11000000 00100111

    Encoded: [27 c0 d1 04]


### Example: 00:54:47.394129115, Europe/Paris

The sub-second portion goes to the nanosecond, which requires a magnitude field of 3.

#### Time:

| Field                | Width | Value     | Encoded                          |
| -------------------- | ----- | --------- | -------------------------------- |
| RESERVED             |     6 |         * | `                        111111` |
| Hour                 |     5 |        00 | `                         00000` |
| Minute               |     6 |        54 | `                        110110` |
| Second               |     6 |        47 | `                        101111` |
| Sub-seconds          |    30 | 394129115 | `010111011111011110111011011011` |
| Sub-second Magnitude |     2 |         3 | `                            11` |
| Time Zone Present    |     1 |         1 | `                             1` |

    Layout:  111111 00000 110110 101111 010111011111011110111011011011 11 1
             11111100 00011011 01011110 10111011 11101111 01110110 11011111

    Encoded: [df 76 ef bb 5e 1b fc]

#### Time Zone (if using area/location):

Time Zone:

| Field           | Width | Value     | Encoded                  |
| --------------- | ----- | --------- | ------------------------ |
| Length          |     7 |         7 |                `0000111` |
| lat-long form   |     1 |         0 |                 `     0` |

Followed by string contents "E/Paris"

    Encoded: [0e 45 2f 50 61 72 69 73]

#### Time Zone (if using latitude/longitude):

| Field         | Width | Value | Encoded            |
| ------------- | ----- | ----- | ------------------ |
| Longitude     |    16 |  2.32 | `0000000011101000` |
| Latitude      |    15 | 48.85 | ` 001001100010101` |
| lat-long form |     1 |     1 | `               1` |

    Layout:  0000000011101000 001001100010101 1
             00000000 11101000 00100110 00101011

    Encoded: [2b 26 e8 00]



How to Keep Time
----------------

Time is one of the most difficult data types to get right. Aside from issues of synchronization, leap seconds, data container limitations and such, it's important to choose the correct **kind** of time to store, and the right kind depends on what the purpose of recording the time is.

There are three main kinds of time:

#### Absolute Time

Absolute time is a time that is fixed relative to UTC (or relative to an offset from UTC). It is not affected by daylight savings time, nor will it ever change if an area's time zone changes for political reasons. Absolute time is best recorded in the UTC time zone, and is mostly useful for events in the past (because the time zone is now fixed at the time of the event, so it probably no longer matters what specific time zone was in effect).

#### Fixed Time

Fixed time is a time that is fixed to a particular place, and that place has a time zone associated with it (but the time zone might change for political reasons in the future,for example with daylight savings). If the venue changes, only the time zone data needs to be updated. An example would be an appointment in London this coming October 12th at 10:30.

#### Floating Time

Floating (or local) time is always relative to the time zone of the observer. If you travel and change time zones, floating time changes zones with you. If you and another observer are in different time zones and observe the same floating time value, the absolute times you calculate will be different. An example would be your 8:00 morning workout.


### When to Use Each Kind

Use whichever kind of time most succinctly and completely handles your time needs. Don't depend on time zone information as a proxy for a location; that's depending on a side effect, which is always brittle. Always store location information separately if it's important.

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
