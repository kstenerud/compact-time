Compact Date Format
===================

Compact date format is an encoding scheme to store a complete date, down to the nanosecond, in as few bytes as possible.



Features
--------

 * Encodes a complete date & time into as few as 32 bits.
 * Maintenance-free (no leap second tables to update).
 * Easily converts to human readable fields.
 * Supports all years in both directions, in perpetuity.
 * Supports time units to the nanosecond.
 * Supports leap years and leap seconds.
 * Dates are in UTC by default.



Specifications
--------------

This specification is part of the [Specification Project](https://github.com/kstenerud/specifications)

* [Compact Date Format Specification](compact-date-specification.md)



Implementations
---------------

* [C implementation](reference-implementation)



License
-------

Copyright 2019 Karl Stenerud

Specification released under Creative Commons Attribution 4.0 International Public License https://creativecommons.org/licenses/by/4.0/

Reference implementation released under MIT License https://opensource.org/licenses/MIT
