/*
 * Compact Time
 * ============
 *
 *
 * License
 * -------
 *
 * Copyright 2019 Karl Stenerud
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
#ifndef KS_compact_time_H
#define KS_compact_time_H

#ifndef COMPACT_TIME_PUBLIC
    #if defined _WIN32 || defined __CYGWIN__
        #define COMPACT_TIME_PUBLIC __declspec(dllimport)
    #else
        #define COMPACT_TIME_PUBLIC
    #endif
#endif


#ifdef __cplusplus 
extern "C" {
#endif

#include <stdint.h>


// ---
// API
// ---

typedef enum
{
    CT_TZ_ZERO,
    CT_TZ_STRING,
    CT_TZ_LATLONG,
} ct_tz_type;

typedef struct
{
    int longitude;
    int latitude;
} ct_location;

typedef struct
{
    int32_t year;
    uint8_t month;
    uint8_t day;
} ct_date;

typedef struct
{
    union
    {
        ct_location as_location;
        char as_string[128];
    } data;
    ct_tz_type type;
} ct_timezone;

typedef struct
{
    ct_timezone timezone;
    uint32_t nanosecond;
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
} ct_time;

typedef struct 
{
    ct_date date;
    ct_time time;
} ct_timestamp;

/**
 * Get the current library version as a semantic version (e.g. "1.5.2").
 *
 * @return The library version.
 */
COMPACT_TIME_PUBLIC const char* ct_version();

/**
 * Calculate the number of bytes that would be occupied by this date when
 * encoded.
 */
COMPACT_TIME_PUBLIC int ct_date_encoded_size(const ct_date* date);

/**
 * Calculate the number of bytes that would be occupied by this time when
 * encoded.
 */
COMPACT_TIME_PUBLIC int ct_time_encoded_size(const ct_time* time);

/**
 * Calculate the number of bytes that would be occupied by this timestamp when
 * encoded.
 */
COMPACT_TIME_PUBLIC int ct_timestamp_encoded_size(const ct_timestamp* timestamp);

/**
 * Encode a date to a destination buffer.
 *
 * Returns the number of bytes written to encode the object, or 0 if there
 * wasn't enough room.
 */
COMPACT_TIME_PUBLIC int ct_date_encode(const ct_date* date, uint8_t* dst, int dst_length);

/**
 * Encode a time to a destination buffer.
 *
 * Returns the number of bytes written to encode the object, or 0 if there
 * wasn't enough room.
 */
COMPACT_TIME_PUBLIC int ct_time_encode(const ct_time* time, uint8_t* dst, int dst_length);

/**
 * Encode a timestamp to a destination buffer.
 *
 * Returns the number of bytes written to encode the object, or 0 if there
 * wasn't enough room.
 */
COMPACT_TIME_PUBLIC int ct_timestamp_encode(const ct_timestamp* timestamp, uint8_t* dst, int dst_length);

/**
 * Decode a date from a source buffer.
 *
 * Returns the number of bytes read to decode the object, or 0 if there wasn't
 * enough data.
 */
COMPACT_TIME_PUBLIC int ct_date_decode(const uint8_t* src, int src_length, ct_date* date);

/**
 * Decode a time from a source buffer.
 *
 * Returns the number of bytes read to decode the object, or 0 if there wasn't
 * enough data.
 */
COMPACT_TIME_PUBLIC int ct_time_decode(const uint8_t* src, int src_length, ct_time* time);

/**
 * Decode a timestamp from a source buffer.
 *
 * Returns the number of bytes read to decode the object, or 0 if there wasn't
 * enough data.
 */
COMPACT_TIME_PUBLIC int ct_timestamp_decode(const uint8_t* src, int src_length, ct_timestamp* timestamp);


#ifdef __cplusplus 
}
#endif

#endif // KS_compact_time_H
