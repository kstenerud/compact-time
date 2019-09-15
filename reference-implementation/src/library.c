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

// #define KSLog_FileDesriptor STDOUT_FILENO
// #define KSLog_LocalMinLevel KSLOG_LEVEL_TRACE
#include <kslog/kslog.h>

#include "compact_time/compact_time.h"
#include <endianness/endianness.h>

#include <vlq/vlq.h>

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

static const int YEAR_BIAS = 2000;
static const int BITS_PER_YEAR_GROUP = 7;

#define FAILURE_AT_POS(A) -(A)

#define SIZE_UTC       1
#define SIZE_MAGNITUDE 2
#define SIZE_SUBSECOND 10
#define SIZE_SECOND    6
#define SIZE_MINUTE    6
#define SIZE_HOUR      5
#define SIZE_DAY       5
#define SIZE_MONTH     4

#define SIZE_LATITUDE  14
#define SIZE_LONGITUDE 15

#define SIZE_DATE_YEAR_UPPER_BITS 7


static const int BASE_SIZE_TIME = SIZE_UTC + SIZE_MAGNITUDE + SIZE_SECOND + SIZE_MINUTE + SIZE_HOUR;
static const int BASE_SIZE_TIMESTAMP = SIZE_MAGNITUDE + SIZE_SECOND + SIZE_MINUTE + SIZE_HOUR + SIZE_DAY + SIZE_MONTH;

static const int BYTE_COUNT_DATE = 2;

static const unsigned MASK_MAGNITUDE = ((1<<SIZE_MAGNITUDE)-1);
static const unsigned MASK_SECOND    = ((1<<SIZE_SECOND)-1);
static const unsigned MASK_MINUTE    = ((1<<SIZE_MINUTE)-1);
static const unsigned MASK_HOUR      = ((1<<SIZE_HOUR)-1);
static const unsigned MASK_DAY       = ((1<<SIZE_DAY)-1);
static const unsigned MASK_MONTH     = ((1<<SIZE_MONTH)-1);

static const unsigned MASK_LATITUDE  = ((1<<SIZE_LATITUDE)-1);
static const unsigned MASK_LONGITUDE = ((1<<SIZE_LONGITUDE)-1);

static const unsigned MASK_DATE_YEAR_UPPER_BITS = (1 << SIZE_DATE_YEAR_UPPER_BITS) - 1;

static const uint8_t g_timestamp_year_upper_bits[] = { 4, 2, 0, 6 };
static const unsigned g_subsec_multipliers[] = { 1, 1000000, 1000, 1 };


static int get_subsecond_magnitude(const uint32_t nanoseconds)
{
    if(nanoseconds == 0)
    {
        return 0;
    }
    if((nanoseconds % 1000) != 0)
    {
        return 3;
    }
    if((nanoseconds % 1000000) != 0)
    {
        return 2;
    }
    return 1;
}

static unsigned zigzag_encode(const int32_t value)
{
    return (value >> 31) ^ (value << 1);
}

static int zigzag_decode(const uint32_t value)
{
    return (value >> 1) ^ -(value & 1);
}

static unsigned encode_year(const int year)
{
    return zigzag_encode(year - YEAR_BIAS);
}

static unsigned encode_year_and_utc_flag(const int year, const int is_utc_timezone)
{
    return (encode_year(year) << 1) | is_utc_timezone;
}

static int decode_year(const unsigned encoded_year)
{
    return zigzag_decode(encoded_year) + YEAR_BIAS;
}

static int get_base_byte_count(int base_size, int magnitude)
{
    const int size = base_size + SIZE_SUBSECOND * magnitude;
    const int8_t remainder = size & 7;
    const int extra_byte = ((remainder | (-remainder)) >> 7) & 1;
    return size / 8 + extra_byte;
}

static int get_year_group_count(uint32_t encoded_year, int uncounted_bits)
{
    uint32_t year = encoded_year >> uncounted_bits;
    if(year == 0)
    {
        return 1;
    }

    int size = 0;
    while(year != 0)
    {
        size++;
        year >>= BITS_PER_YEAR_GROUP;
    }
    return size;
}

static int timezone_encoded_size(const ct_timezone* timezone)
{
    switch(timezone->type)
    {
        case CT_TZ_STRING:
            return strlen(timezone->data.as_string) + 1;
        case CT_TZ_LATLONG:
            return 4;
        case CT_TZ_ZERO:
            return 0;
        default:
            // TODO: Maybe panic?
            return 0;
    }
}

static int timezone_encode(const ct_timezone* timezone, uint8_t* dst, int dst_length)
{
    switch(timezone->type)
    {
        case CT_TZ_ZERO:
            return 0;
        case CT_TZ_STRING:
        {
            const int string_length = strlen(timezone->data.as_string);
            if(string_length + 1 > dst_length)
            {
                return FAILURE_AT_POS(string_length + 1);
            }
            dst[0] = string_length << 1;
            memcpy(dst+1, timezone->data.as_string, string_length);
            return string_length + 1;
        }
        case CT_TZ_LATLONG:
        {
            uint32_t value = timezone->data.as_location.longitude & MASK_LONGITUDE;
            value <<= SIZE_LATITUDE;
            value |= timezone->data.as_location.latitude & MASK_LATITUDE;
            value <<= 1;
            value |= 1;
            int length = sizeof(value);
            if(length > dst_length)
            {
                return FAILURE_AT_POS(length);
            }
            write_uint32_le(value, dst);
            return length;
        }
        default:
            return 0;
    }
}

static int timezone_decode(ct_timezone* timezone, const uint8_t* src, int src_length, bool timezone_is_utc)
{
    if(timezone_is_utc)
    {
        timezone->type = CT_TZ_ZERO;
        return 0;
    }

    if(src_length < 1)
    {
        return FAILURE_AT_POS(1);
    }

    bool is_latlong = src[0] & 1;
    if(is_latlong)
    {
        int size = sizeof(uint32_t);
        if(size > src_length)
        {
            return FAILURE_AT_POS(size);
        }
        timezone->type = CT_TZ_LATLONG;
        uint32_t latlong = read_uint32_le(src) >> 1;
        timezone->data.as_location.latitude = latlong & MASK_LATITUDE;
        latlong >>= SIZE_LATITUDE;
        timezone->data.as_location.longitude = latlong & MASK_LONGITUDE;
        return size;
    }

    int offset = 0;
    const int length = src[offset] >> 1;
    offset++;
    if(offset + length > src_length)
    {
        return FAILURE_AT_POS(offset + length);
    }
    timezone->type = CT_TZ_STRING;
    memcpy(timezone->data.as_string, src+offset, length);
    timezone->data.as_string[length] = 0;
    offset += length;
    return offset;
}



// ----------
// Public API
// ----------

const char* ct_version()
{
    return EXPAND_AND_QUOTE(PROJECT_VERSION);
}

int ct_date_encoded_size(const ct_date* date)
{
    const unsigned encoded_year = encode_year(date->year);
    return BYTE_COUNT_DATE + get_year_group_count(encoded_year, SIZE_DATE_YEAR_UPPER_BITS);
}

int ct_time_encoded_size(const ct_time* time)
{
    const int magnitude = get_subsecond_magnitude(time->nanosecond);
    const int base_byte_count = get_base_byte_count(BASE_SIZE_TIME, magnitude);

    return base_byte_count + timezone_encoded_size(&time->timezone);
}

int ct_timestamp_encoded_size(const ct_timestamp* timestamp)
{
    const int magnitude = get_subsecond_magnitude(timestamp->time.nanosecond);
    const int base_byte_count = get_base_byte_count(BASE_SIZE_TIMESTAMP, magnitude);
    const unsigned encoded_year = encode_year(timestamp->date.year);
    const int year_group_count = get_year_group_count(encoded_year<<1, g_timestamp_year_upper_bits[magnitude]);

    return base_byte_count + year_group_count + timezone_encoded_size(&timestamp->time.timezone);
}

int ct_date_encode(const ct_date* date, uint8_t* dst, int dst_length)
{
    const unsigned encoded_year = encode_year(date->year);
    const int year_group_count = get_year_group_count(encoded_year, SIZE_DATE_YEAR_UPPER_BITS);
    const int year_group_bit_count = year_group_count * BITS_PER_YEAR_GROUP;
    const unsigned year_grouped_mask = (1<<year_group_bit_count) - 1;

    uint16_t accumulator = encoded_year >> year_group_bit_count;
    accumulator = (accumulator << SIZE_MONTH) | date->month;
    accumulator = (accumulator << SIZE_DAY) | date->day;

    int offset = 0;
    const int accumulator_size = BYTE_COUNT_DATE;
    if(accumulator_size > dst_length)
    {
        return FAILURE_AT_POS(accumulator_size);
    }
    write_uint16_le(accumulator, dst + offset);
    offset += accumulator_size;
    const int rvlq_byte_count = rvlq_encode_32(encoded_year & year_grouped_mask, dst+offset, dst_length - offset);
    if(rvlq_byte_count <= 0)
    {
        return FAILURE_AT_POS(offset) + rvlq_byte_count;
    }
    offset += rvlq_byte_count;

    return offset;
}

int ct_time_encode(const ct_time* time, uint8_t* dst, int dst_length)
{
    const int magnitude = get_subsecond_magnitude(time->nanosecond);
    const uint64_t subsecond = time->nanosecond / g_subsec_multipliers[magnitude];

    uint64_t accumulator = subsecond;
    accumulator = (accumulator << SIZE_SECOND) + time->second;
    accumulator = (accumulator << SIZE_MINUTE) + time->minute;
    accumulator = (accumulator << SIZE_HOUR) + time->hour;
    accumulator = (accumulator << SIZE_MAGNITUDE) + magnitude;
    accumulator = (accumulator << 1) + (time->timezone.type == CT_TZ_ZERO ? 1 : 0);

    int offset = 0;
    const int accumulator_size = get_base_byte_count(BASE_SIZE_TIME, magnitude);
    if(accumulator_size > dst_length)
    {
        return FAILURE_AT_POS(accumulator_size);
    }
    copy_le(&accumulator, dst + offset, accumulator_size);
    offset += accumulator_size;

    const int timezone_byte_count = timezone_encode(&time->timezone, dst+offset, dst_length-offset);
    if(timezone_byte_count < 0)
    {
        return FAILURE_AT_POS(offset) + timezone_byte_count;
    }
    offset += timezone_byte_count;

    return offset;
}

int ct_timestamp_encode(const ct_timestamp* timestamp, uint8_t* dst, int dst_length)
{
    const int magnitude = get_subsecond_magnitude(timestamp->time.nanosecond);
    const uint64_t subsecond = timestamp->time.nanosecond / g_subsec_multipliers[magnitude];
    const unsigned encoded_year = encode_year_and_utc_flag(timestamp->date.year, timestamp->time.timezone.type == CT_TZ_ZERO);
    const int year_group_count = get_year_group_count(encoded_year, g_timestamp_year_upper_bits[magnitude]);
    const int year_group_bit_count = year_group_count * BITS_PER_YEAR_GROUP;
    const unsigned year_grouped_mask = (1<<year_group_bit_count) - 1;

    uint64_t accumulator = encoded_year >> year_group_bit_count;
    accumulator = (accumulator << (SIZE_SUBSECOND * magnitude)) + subsecond;
    accumulator = (accumulator << SIZE_MONTH) + timestamp->date.month;
    accumulator = (accumulator << SIZE_DAY) + timestamp->date.day;
    accumulator = (accumulator << SIZE_HOUR) + timestamp->time.hour;
    accumulator = (accumulator << SIZE_MINUTE) + timestamp->time.minute;
    accumulator = (accumulator << SIZE_SECOND) + timestamp->time.second;
    accumulator = (accumulator << SIZE_MAGNITUDE) + magnitude;

    int offset = 0;
    const int accumulator_size = get_base_byte_count(BASE_SIZE_TIMESTAMP, magnitude);
    if(accumulator_size > dst_length)
    {
        return FAILURE_AT_POS(accumulator_size);
    }
    copy_le(&accumulator, dst + offset, accumulator_size);
    offset += accumulator_size;

    const int rvlq_byte_count = rvlq_encode_32(encoded_year & year_grouped_mask, dst+offset, dst_length - offset);
    if(rvlq_byte_count <= 0)
    {
        return FAILURE_AT_POS(offset) + rvlq_byte_count;
    }
    offset += rvlq_byte_count;

    const int timezone_byte_count = timezone_encode(&timestamp->time.timezone, dst+offset, dst_length-offset);
    if(timezone_byte_count < 0)
    {
        return FAILURE_AT_POS(offset) + timezone_byte_count;
    }
    offset += timezone_byte_count;

    return offset;
}

int ct_date_decode(const uint8_t* src, int src_length, ct_date* date)
{
    if(BYTE_COUNT_DATE >= src_length)
    {
        return FAILURE_AT_POS(BYTE_COUNT_DATE);
    }

    uint16_t accumulator = read_uint16_le(src);
    int offset = BYTE_COUNT_DATE;

    date->day = accumulator & MASK_DAY;
    accumulator >>= SIZE_DAY;
    date->month = accumulator & MASK_MONTH;
    accumulator >>= SIZE_MONTH;
    uint32_t year_encoded = accumulator & MASK_DATE_YEAR_UPPER_BITS;

    const int decoded_group_count = rvlq_decode_32(&year_encoded, src + offset, src_length - offset);
    if(decoded_group_count < 1)
    {
        return FAILURE_AT_POS(offset) + decoded_group_count;
    }
    offset += decoded_group_count;
    date->year = decode_year(year_encoded);

    return offset;
}

int ct_time_decode(const uint8_t* src, int src_length, ct_time* time)
{
    if(src_length < 1)
    {
        return FAILURE_AT_POS(1);
    }

    const bool timezone_is_utc = src[0] & 1;
    const int magnitude = (src[0] >> 1) & MASK_MAGNITUDE;
    const int subsecond_multiplier = g_subsec_multipliers[magnitude];
    const int size_subsecond = SIZE_SUBSECOND * magnitude;
    const unsigned mask_subsecond = (1 << size_subsecond) - 1;

    int offset = get_base_byte_count(BASE_SIZE_TIME, magnitude);
    if(offset > src_length)
    {
        return FAILURE_AT_POS(offset);
    }

    uint64_t accumulator = 0;
    copy_le(src, &accumulator, offset);

    accumulator >>= 1;
    accumulator >>= SIZE_MAGNITUDE;
    time->hour = accumulator & MASK_HOUR;
    accumulator >>= SIZE_HOUR;
    time->minute = accumulator & MASK_MINUTE;
    accumulator >>= SIZE_MINUTE;
    time->second = accumulator & MASK_SECOND;
    accumulator >>= SIZE_SECOND;
    time->nanosecond = (accumulator & mask_subsecond) * subsecond_multiplier;

    int timezone_byte_count = timezone_decode(&time->timezone, src + offset, src_length - offset, timezone_is_utc);
    if(timezone_byte_count < 0)
    {
        return FAILURE_AT_POS(offset) + timezone_byte_count;
    }
    offset += timezone_byte_count;

    return offset;
}

int ct_timestamp_decode(const uint8_t* src, int src_length, ct_timestamp* timestamp)
{
    if(src_length < 1)
    {
        return FAILURE_AT_POS(1);
    }

    const int magnitude = src[0] & MASK_MAGNITUDE;
    const int subsecond_multiplier = g_subsec_multipliers[magnitude];
    const int size_subsecond = SIZE_SUBSECOND * magnitude;
    const unsigned mask_subsecond = (1 << size_subsecond) - 1;

    int offset = get_base_byte_count(BASE_SIZE_TIMESTAMP, magnitude);
    if(offset >= src_length)
    {
        return FAILURE_AT_POS(offset);
    }

    uint64_t accumulator = 0;
    copy_le(src, &accumulator, offset);

    accumulator >>= SIZE_MAGNITUDE;
    timestamp->time.second = accumulator & MASK_SECOND;
    accumulator >>= SIZE_SECOND;
    timestamp->time.minute = accumulator & MASK_MINUTE;
    accumulator >>= SIZE_MINUTE;
    timestamp->time.hour = accumulator & MASK_HOUR;
    accumulator >>= SIZE_HOUR;
    timestamp->date.day = accumulator & MASK_DAY;
    accumulator >>= SIZE_DAY;
    timestamp->date.month = accumulator & MASK_MONTH;
    accumulator >>= SIZE_MONTH;
    timestamp->time.nanosecond = (accumulator & mask_subsecond) * subsecond_multiplier;
    accumulator >>= size_subsecond;
    uint32_t year_encoded = (uint32_t)accumulator;

    const int decoded_group_count = rvlq_decode_32(&year_encoded, src + offset, src_length - offset);
    if(decoded_group_count < 1)
    {
        return FAILURE_AT_POS(offset) + decoded_group_count;
    }
    offset += decoded_group_count;

    uint32_t timezone_is_utc = year_encoded & 1;
    year_encoded >>= 1;
    timestamp->date.year = decode_year(year_encoded);

    int timezone_byte_count = timezone_decode(&timestamp->time.timezone, src + offset, src_length - offset, timezone_is_utc);
    if(timezone_byte_count < 0)
    {
        return FAILURE_AT_POS(offset) + timezone_byte_count;
    }
    offset += timezone_byte_count;

    return offset;
}
