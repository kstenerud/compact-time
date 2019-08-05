#include "cdate/cdate.h"

#include <vlq/vlq.h>

// #define KSLogger_LocalLevel TRACE
#include "kslogger.h"

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

static const int YEAR_BIAS = 2000;
static const int BITS_PER_YEAR_GROUP = 7;

#define SIZE_MAGNITUDE 2
#define SIZE_SUBSECOND 10
#define SIZE_SECOND    6
#define SIZE_MINUTE    6
#define SIZE_HOUR      5
#define SIZE_DAY       5
#define SIZE_MONTH     4

static const unsigned MASK_SECOND    = ((1<<SIZE_SECOND)-1);
static const unsigned MASK_MINUTE    = ((1<<SIZE_MINUTE)-1);
static const unsigned MASK_HOUR      = ((1<<SIZE_HOUR)-1);
static const unsigned MASK_DAY       = ((1<<SIZE_DAY)-1);
static const unsigned MASK_MONTH     = ((1<<SIZE_MONTH)-1);

static const int g_base_sizes[] = { 4, 5, 6, 8 };
static const int g_year_high_bits[] = { 4, 2, 0, 6 };
static const unsigned g_subsec_multipliers[] = { 1, 1000000, 1000, 1 };


static int get_subsecond_magnitude(const cdate* const date)
{
    if(date->nanosecond == 0)
    {
        return 0;
    }
    if((date->nanosecond % 1000) != 0)
    {
        return 3;
    }
    if((date->nanosecond % 1000000) != 0)
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

static int decode_year(const unsigned encoded_year)
{
    return zigzag_decode(encoded_year) + YEAR_BIAS;
}

static int get_base_byte_count(int magnitude)
{
    return (SIZE_MAGNITUDE
           + SIZE_SUBSECOND * magnitude
           + SIZE_SECOND
           + SIZE_MINUTE
           + SIZE_HOUR
           + SIZE_DAY
           + SIZE_MONTH
           + g_year_high_bits[magnitude])
           / 8;
}

static int get_year_group_count(uint32_t encoded_year, int subsecond_magnitude)
{
    const int extra_bit_count = g_year_high_bits[subsecond_magnitude];
    uint32_t year = encoded_year >> BITS_PER_YEAR_GROUP;
    if(year == 0)
    {
        return 1;
    }

    int size = 1;
    while(year != 0)
    {
        size++;
        year >>= BITS_PER_YEAR_GROUP;
    }

    uint32_t extra_mask = (1<<extra_bit_count)-1;
    uint32_t last_group_bits = encoded_year >> (BITS_PER_YEAR_GROUP * (size-1));
    if(last_group_bits & ~extra_mask)
    {
        return size;
    }
    return size - 1;
}



// ----------
// Public API
// ----------

const char* cdate_version()
{
    return EXPAND_AND_QUOTE(PROJECT_VERSION);
}

int cdate_encoded_size(cdate* date)
{
    const int magnitude = get_subsecond_magnitude(date);
    const int base_byte_count = get_base_byte_count(magnitude);
    const unsigned encoded_year = encode_year(date->year);
    const int year_group_count = get_year_group_count(encoded_year, magnitude);

    return base_byte_count + year_group_count;
}

int cdate_encode(const cdate* date, uint8_t* dst, int dst_length)
{
    KSLOG_DEBUG("encode");
    const int magnitude = get_subsecond_magnitude(date);
    const int base_byte_count = get_base_byte_count(magnitude);
    unsigned encoded_year = encode_year(date->year);
    const int year_group_count = get_year_group_count(encoded_year, magnitude);
    KSLOG_TRACE("m %d, b %d, y %d (%d), g %d", magnitude, base_byte_count, encoded_year, date->year, year_group_count);

    if(base_byte_count + year_group_count > dst_length)
    {
        return -1;
    }

    const uint64_t subsecond = date->nanosecond / g_subsec_multipliers[magnitude];
    const int year_group_bit_count = year_group_count*BITS_PER_YEAR_GROUP;
    const unsigned year_grouped_mask = (1<<year_group_bit_count) - 1;
    KSLOG_TRACE("subs %d, year group bits %d, year mask %x", subsecond, year_group_bit_count, year_grouped_mask);

    int b = 0;
    KSLOG_TRACE("y: %016lx %02d %d", (uint64_t)(encoded_year >> year_group_bit_count) << b, b, encoded_year >> year_group_bit_count); b += g_year_high_bits[magnitude];
    KSLOG_TRACE("M: %016lx %02d %d", (uint64_t)date->month << b, b, date->month); b += SIZE_MONTH;
    KSLOG_TRACE("d: %016lx %02d %d", (uint64_t)date->day << b, b, date->day); b += SIZE_DAY;
    KSLOG_TRACE("h: %016lx %02d %d", (uint64_t)date->hour << b, b, date->hour); b += SIZE_HOUR;
    KSLOG_TRACE("m: %016lx %02d %d", (uint64_t)date->minute << b, b, date->minute); b += SIZE_MINUTE;
    KSLOG_TRACE("s: %016lx %02d %d", (uint64_t)date->second << b, b, date->second); b += SIZE_SECOND;
    KSLOG_TRACE("S: %016lx %02d %d", (uint64_t)subsecond << b, b, subsecond); b += SIZE_SUBSECOND * magnitude;
    KSLOG_TRACE("a: %016lx %02d %d", (uint64_t)magnitude << b, b, magnitude);

    uint64_t accumulator = magnitude;
    accumulator = (accumulator << (SIZE_SUBSECOND * magnitude)) + subsecond;
    accumulator = (accumulator << SIZE_SECOND) + date->second;
    accumulator = (accumulator << SIZE_MINUTE) + date->minute;
    accumulator = (accumulator << SIZE_HOUR) + date->hour;
    accumulator = (accumulator << SIZE_DAY) + date->day;
    accumulator = (accumulator << SIZE_MONTH) + date->month;
    accumulator = (accumulator << g_year_high_bits[magnitude]) + (encoded_year >> year_group_bit_count);

    KSLOG_DEBUG("Accumulator: %016lx", (uint64_t)accumulator);

    encoded_year &= year_grouped_mask;

    int offset = 0;
    for(int i = base_byte_count-1; i >= 0; i--)
    {
        KSLOG_TRACE("Write %02x", (uint8_t)(accumulator >> (8*i)));
        dst[offset++] = (uint8_t)(accumulator >> (8*i));
    }

    KSLOG_TRACE("encoded year %d (%02x)", encoded_year, encoded_year);
    offset += rvlq_encode_32(encoded_year, dst+offset, dst_length - offset);
    return offset;
}

int cdate_decode(const uint8_t* src, int src_length, cdate* date)
{
    KSLOG_DEBUG("decode");
    if(src_length < 1)
    {
        return -1;
    }

    const int shift_magnitude = 6;
    const uint8_t mask_magnitude = (1<<shift_magnitude) - 1;
    KSLOG_TRACE("mask mag %02x", mask_magnitude);
    uint8_t next_byte = src[0];
        KSLOG_TRACE("Read %d: %02x", 0, src[0]);
    int src_index = 1;

    int magnitude = next_byte >> shift_magnitude;
    next_byte &= mask_magnitude;
    KSLOG_TRACE("next byte masked %02x: %02x", ~mask_magnitude, next_byte);

    int remaining_bytes = g_base_sizes[magnitude] - 1;
    int src_index_end = src_index + remaining_bytes;
    KSLOG_TRACE("rem bytes %d, src index %d, src length %d", remaining_bytes, src_index, src_length);
    if(src_index_end >= src_length)
    {
        return -1;
    }

    uint64_t accumulator = next_byte;
    KSLOG_TRACE("Accum %016lx", accumulator);
    while(src_index < src_index_end)
    {
        KSLOG_TRACE("Read %d: %02x", src_index, src[src_index]);
        accumulator = (accumulator << 8) | src[src_index];
        src_index++;
        KSLOG_TRACE("Accum %016lx", accumulator);
    }

    const int year_high_bits = g_year_high_bits[magnitude];
    const uint32_t year_high_bits_mask = (1<<year_high_bits) - 1;

    uint32_t year_encoded = accumulator & year_high_bits_mask;
    accumulator >>= year_high_bits;
    date->month = accumulator & MASK_MONTH;
    accumulator >>= SIZE_MONTH;
    date->day = accumulator & MASK_DAY;
    accumulator >>= SIZE_DAY;
    date->hour = accumulator & MASK_HOUR;
    accumulator >>= SIZE_HOUR;
    date->minute = accumulator & MASK_MINUTE;
    accumulator >>= SIZE_MINUTE;
    date->second = accumulator & MASK_SECOND;
    accumulator >>= SIZE_SECOND;
    date->nanosecond = accumulator * g_subsec_multipliers[magnitude];

    int decoded_group_count = rvlq_decode_32(&year_encoded, src + src_index, src_length - src_index);
    date->year = decode_year(year_encoded);

    return src_index + decoded_group_count;
}
