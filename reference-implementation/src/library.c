#include "cdate/cdate.h"

// #define KSLogger_LocalLevel TRACE
#include "kslogger.h"

ANSI_EXTENSION typedef unsigned __int128 uint128_ct;

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

enum
{
    SHIFT_SECOND =  2,
    SHIFT_MINUTE =  8,
    SHIFT_HOUR   = 14,
    SHIFT_DAY    = 19,
    SHIFT_MONTH  = 24,
    SHIFT_SUBSEC = 28,
};

static const unsigned MASK_MAGNITUDE = ((1<<2)-1);
static const unsigned MASK_SECOND    = ((1<<6)-1);
static const unsigned MASK_MINUTE    = ((1<<6)-1);
static const unsigned MASK_HOUR      = ((1<<5)-1);
static const unsigned MASK_DAY       = ((1<<5)-1);
static const unsigned MASK_MONTH     = ((1<<4)-1);

static const int g_base_sizes[] = { 4, 5, 7, 8 };
static const int g_year_bits[] = { 3, 1, 7, 5 };
static const int g_year_shifts[] = { 28, 38, 48, 58 };
static const unsigned g_subsec_masks[] = { 0, (1<<10)-1, (1<<20)-1, (1<<30)-1 };
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
    return zigzag_encode(year - 2000);
}

static int decode_year(const unsigned encoded_year)
{
    return zigzag_decode(encoded_year) + 2000;
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
    int size = g_base_sizes[magnitude];
    unsigned year = encode_year(date->year) >> g_year_bits[magnitude];
    KSLOG_DEBUG("Mag %d, size %d, year %d (%d), bits %d. Result year %d",
        magnitude, size, date->year, encode_year(date->year), g_year_bits[magnitude], year);
    while(year > 0)
    {
        year = year >> 7;
        size++;
    }
    return size;
}

int cdate_encode(const cdate* date, uint8_t* dst, int dst_length)
{
    KSLOG_DEBUG("encode");
    const int magnitude = get_subsecond_magnitude(date);
    uint128_ct accumulator = magnitude |
        ((uint64_t)date->second << SHIFT_SECOND) |
        ((uint64_t)date->minute << SHIFT_MINUTE) |
        ((uint64_t)date->hour << SHIFT_HOUR) |
        ((uint64_t)date->day << SHIFT_DAY) |
        ((uint64_t)date->month << SHIFT_MONTH) |
        ((uint64_t)encode_year(date->year) << g_year_shifts[magnitude]);

    uint64_t subsecond = date->nanosecond / g_subsec_multipliers[magnitude];
    accumulator |= (subsecond << SHIFT_SUBSEC);

    KSLOG_TRACE("Magnitude:   %016lx", magnitude);
    KSLOG_TRACE("Seconds:     %016lx", ((uint64_t)date->second << SHIFT_SECOND));
    KSLOG_TRACE("Minutes:     %016lx", ((uint64_t)date->minute << SHIFT_MINUTE));
    KSLOG_TRACE("Hours:       %016lx", ((uint64_t)date->hour << SHIFT_HOUR));
    KSLOG_TRACE("Days:        %016lx", ((uint64_t)date->day << SHIFT_DAY));
    KSLOG_TRACE("Months:      %016lx", ((uint64_t)date->month << SHIFT_MONTH));
    KSLOG_TRACE("Subseconds:  %016lx", (subsecond << SHIFT_SUBSEC));
    KSLOG_TRACE("Years:       %016lx: y %x, s %d", ((uint64_t)encode_year(date->year) << g_year_shifts[magnitude]), encode_year(date->year), g_year_shifts[magnitude]);
    KSLOG_DEBUG("Accumulator: %016lx", (uint64_t)accumulator);

    // (base size - 1) because the first continuation bit occurs in the last
    // byte of the base encoded structure.
    const int base_size = g_base_sizes[magnitude] - 1;
    if(base_size >= dst_length)
    {
        return -1;
    }
    int offset = 0;
    for(; offset < base_size; offset++)
    {
        dst[offset] = (uint8_t)accumulator;
        accumulator >>= 8;
    }

    while(accumulator > 0)
    {
        if(offset >= dst_length)
        {
            return -1;
        }
        uint8_t next_byte = accumulator & 0x7f;
        accumulator >>= 7;
        if(accumulator > 0)
        {
            next_byte |= 0x80;
        }
        dst[offset++] = next_byte;
    }

    return offset;
}



int cdate_decode(const uint8_t* src, int src_length, cdate* date)
{
    KSLOG_DEBUG("decode");
    if(src_length < 1)
    {
        return -1;
    }
    uint128_ct accumulator = *src;
    const int magnitude = accumulator & MASK_MAGNITUDE;

    const int base_size = g_base_sizes[magnitude] - 1;
    if(base_size >= src_length)
    {
        return -1;
    }
    int offset = 1;
    KSLOG_DEBUG("Accum start %02x", (uint8_t)accumulator);
    for(; offset < base_size; offset++)
    {
        accumulator |= ((uint128_ct)src[offset]) << ((offset)*8);
        KSLOG_DEBUG("Accum add %02x %016lx",
            src[offset],
            (uint64_t)(((uint128_ct)src[offset]) << ((offset)*8)));
    }

    int shift_amount = (offset)*8;
    uint8_t next_byte = 0;
    do
    {
        if(offset >= src_length)
        {
            return -1;
        }
        next_byte = src[offset++];
        accumulator |= ((uint128_ct)next_byte&0x7f) << shift_amount;
        KSLOG_DEBUG("Accum add %02x %016lx",
            next_byte&0x7f,
            (uint64_t)(((uint128_ct)next_byte&0x7f) << shift_amount));
        shift_amount += 7;
    } while(next_byte & 0x80);

    KSLOG_DEBUG("Accumulator: %016lx", (uint64_t)accumulator);

    date->second = (accumulator>>SHIFT_SECOND) & MASK_SECOND;
    date->minute = (accumulator>>SHIFT_MINUTE) & MASK_MINUTE;
    date->hour = (accumulator>>SHIFT_HOUR) & MASK_HOUR;
    date->day = (accumulator>>SHIFT_DAY) & MASK_DAY;
    date->month = (accumulator>>SHIFT_MONTH) & MASK_MONTH;
    date->nanosecond = ((unsigned)(accumulator>>SHIFT_SUBSEC) & g_subsec_masks[magnitude]) * g_subsec_multipliers[magnitude];
    date->year = decode_year(accumulator>>g_year_shifts[magnitude]);

    KSLOG_TRACE("Magnitude:   %d", magnitude);
    KSLOG_TRACE("Seconds:     %d", date->second);
    KSLOG_TRACE("Minutes:     %d", date->minute);
    KSLOG_TRACE("Hours:       %d", date->hour);
    KSLOG_TRACE("Days:        %d", date->day);
    KSLOG_TRACE("Months:      %d", date->month);
    KSLOG_TRACE("Subseconds:  %d", date->nanosecond);
    KSLOG_TRACE("Years:       %d", date->year);

    KSLOG_TRACE("subsec shifted %x, mask %x, result %d",
        (unsigned)(accumulator>>SHIFT_SUBSEC),
        g_subsec_masks[magnitude],
        (unsigned)(accumulator>>SHIFT_SUBSEC) & g_subsec_masks[magnitude]);

    return offset;
}
