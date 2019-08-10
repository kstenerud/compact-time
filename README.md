Compact Date Format
===================

Compact date format is an encoding scheme to store a complete date, down to the nanosecond, in as few bytes as possible.



Features
--------

 * Encodes a complete date & time into as few as 5 bytes.
 * Maintenance-free (no leap second tables to update).
 * Efficient conversion to/from human readable fields (no multiplication or division).
 * Supports unlimited positive and negative years.
 * Supports time units down to the nanosecond.
 * Supports leap years and leap seconds.
 * Dates are relative to UTC by default.



Specifications
--------------

This specification is part of the [Specification Project](https://github.com/kstenerud/specifications)

* [Compact Date Format Specification](compact-date-specification.md)



Implementations
---------------

* [C implementation](reference-implementation)
* [Go implementation](https://github.com/kstenerud/go-compact-date)



License
-------

Copyright 2019 Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/

Reference implementation released under MIT License https://opensource.org/licenses/MIT
