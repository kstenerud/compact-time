/*
 * Compact Date
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
#ifndef KS_cdate_H
#define KS_cdate_H

#ifndef CDATE_PUBLIC
    #if defined _WIN32 || defined __CYGWIN__
        #define CDATE_PUBLIC __declspec(dllimport)
    #else
        #define CDATE_PUBLIC
    #endif
#endif


#ifdef __cplusplus 
extern "C" {
#endif

#include <stdint.h>


// ---
// API
// ---

typedef struct
{
    uint32_t nanosecond;
    int32_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} cdate;

/**
 * Get the current library version as a semantic version (e.g. "1.5.2").
 *
 * @return The library version.
 */
CDATE_PUBLIC const char* cdate_version();

/**
 * Calculate the number of bytes that would be occupied by this date when
 * encoded.
 */
CDATE_PUBLIC int cdate_encoded_size(const cdate* date);

/**
 * Encode a date to a destination buffer.
 *
 * Returns the number of bytes written to encode the date, or 0 if there wasn't
 * enough room.
 */
CDATE_PUBLIC int cdate_encode(const cdate* date, uint8_t* dst, int dst_length);

/**
 * Decode a date from a source buffer.
 *
 * Returns the number of bytes read to decode the date, or 0 if there wasn't
 * enough data.
 */
CDATE_PUBLIC int cdate_decode(const uint8_t* src, int src_length, cdate* date);


#ifdef __cplusplus 
}
#endif

#endif // KS_cdate_H
