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



Specifications
--------------

This specification is part of the [Specifications Project](https://github.com/kstenerud/specifications)

* [Compact Time Format Specification](compact-time-specification.md)



Implementations
---------------

* [C implementation](https://github.com/kstenerud/c-compact-time)
* [Go implementation](https://github.com/kstenerud/go-compact-time)



License
-------

Copyright Karl Stenerud. All rights reserved.

Specifications released under [Creative Commons Attribution 4.0 International Public License](LICENSE.md).
