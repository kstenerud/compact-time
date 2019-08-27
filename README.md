Compact Time Format
===================

The compact time formats are encoding schemes to store a complete time, date, or timestamp in as few bytes as possible for data transmission.

Any Gregorian or proleptic Gregorian date can be recorded to the nanosecond using this encoding.



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
 * Time zones are location-based, not offset-based.



Specifications
--------------

This specification is part of the [Specification Project](https://github.com/kstenerud/specifications)

* [Compact Time Format Specification](compact-time-specification.md)



Implementations
---------------

* [C implementation](reference-implementation)
* [Go implementation](https://github.com/kstenerud/go-compact-date)



License
-------

Copyright 2019 Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/

Reference implementation released under MIT License https://opensource.org/licenses/MIT
