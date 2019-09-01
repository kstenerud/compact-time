#include <gtest/gtest.h>
#include <compact_time/compact_time.h>

// #define KSLog_LocalMinLevel KSLOG_LEVEL_TRACE
#include <kslog/kslog.h>

TEST(Library, version)
{
    const char* expected = "1.0.0";
    const char* actual = ct_version();
    ASSERT_STREQ(expected, actual);
}

static void fill_timestamp(ct_timestamp* timestamp, int year, int month, int day, int hour, int minute, int second, int nanosecond)
{
    memset(timestamp, 0, sizeof(*timestamp));
    timestamp->date.year = year;
    timestamp->date.month = month;
    timestamp->date.day = day;
    timestamp->time.hour = hour;
    timestamp->time.minute = minute;
    timestamp->time.second = second;
    timestamp->time.nanosecond = nanosecond;
}

static void fill_timezone_utc(ct_timezone* timezone)
{
    timezone->type = CT_TZ_ZERO;
}

static void fill_timezone_named(ct_timezone* timezone, const char* name)
{
    timezone->type = CT_TZ_STRING;
    strcpy(timezone->data.as_string, name);
}

static void fill_timezone_loc(ct_timezone* timezone, const int latitude, const int longitude)
{
    timezone->type = CT_TZ_LATLONG;
    timezone->data.as_location.latitude = latitude;
    timezone->data.as_location.longitude = longitude;
}

#define ASSERT_DATE_EQ(ACTUAL, EXPECTED) \
    ASSERT_EQ(EXPECTED.year, ACTUAL.year); \
    ASSERT_EQ(EXPECTED.month, ACTUAL.month); \
    ASSERT_EQ(EXPECTED.day, ACTUAL.day)

#define ASSERT_TIME_EQ(ACTUAL, EXPECTED) \
    ASSERT_EQ(EXPECTED.hour, ACTUAL.hour); \
    ASSERT_EQ(EXPECTED.minute, ACTUAL.minute); \
    ASSERT_EQ(EXPECTED.second, ACTUAL.second); \
    ASSERT_EQ(EXPECTED.nanosecond, ACTUAL.nanosecond)

#define ASSERT_TIMESTAMP_ENCODE_DECODE(ACTUAL_TIMESTAMP_NAME, ...) \
    std::vector<uint8_t> expected = __VA_ARGS__; \
    std::vector<uint8_t> actual(ct_timestamp_encoded_size(&timestamp)); \
    int bytes_encoded = ct_timestamp_encode(&timestamp, actual.data(), actual.size()); \
    ASSERT_EQ(expected, actual); \
    ASSERT_EQ(bytes_encoded, expected.size()); \
    KSLOG_ERROR("Expected size = %d", expected.size()); \
    ct_timestamp ACTUAL_TIMESTAMP_NAME; \
    memset(&ACTUAL_TIMESTAMP_NAME, 0, sizeof(ACTUAL_TIMESTAMP_NAME)); \
    int bytes_decoded = ct_timestamp_decode(actual.data(), actual.size(), &ACTUAL_TIMESTAMP_NAME); \
    ASSERT_EQ(bytes_decoded, expected.size()); \
    ASSERT_DATE_EQ(ACTUAL_TIMESTAMP_NAME.date, timestamp.date); \
    ASSERT_TIME_EQ(ACTUAL_TIMESTAMP_NAME.time, timestamp.time)

#define TEST_TIMESTAMP_TZ_UTC(SIGN, YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, NANOSECOND, ...) \
TEST(CDate, timestamp_utc_ ## YEAR ## _ ## MONTH ## _ ## DAY ## _ ## HOUR ## _ ## MINUTE ## _ ## SECOND ## _ ## NANOSECOND) \
{ \
    ct_timestamp timestamp; \
    fill_timestamp(&timestamp, SIGN YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, NANOSECOND); \
    fill_timezone_utc(&timestamp.time.timezone); \
    ASSERT_TIMESTAMP_ENCODE_DECODE(actual_timestamp, __VA_ARGS__); \
}

#define TEST_TIMESTAMP_TZ_NAMED(SIGN, YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, NANOSECOND, TZ, ...) \
TEST(CDate, timestamp_named_ ## YEAR ## _ ## MONTH ## _ ## DAY ## _ ## HOUR ## _ ## MINUTE ## _ ## SECOND ## _ ## NANOSECOND) \
{ \
    ct_timestamp timestamp; \
    fill_timestamp(&timestamp, SIGN YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, NANOSECOND); \
    fill_timezone_named(&timestamp.time.timezone, TZ); \
    ASSERT_TIMESTAMP_ENCODE_DECODE(actual_timestamp, __VA_ARGS__); \
    ASSERT_STREQ(actual_timestamp.time.timezone.data.as_string, TZ); \
}

#define TEST_TIMESTAMP_TZ_LOC(SIGN, YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, NANOSECOND, LAT, LONG, ...) \
TEST(CDate, timestamp_loc_ ## YEAR ## _ ## MONTH ## _ ## DAY ## _ ## HOUR ## _ ## MINUTE ## _ ## SECOND ## _ ## NANOSECOND ## _ ## LAT ## _ ## LONG) \
{ \
    ct_timestamp timestamp; \
    fill_timestamp(&timestamp, SIGN YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, NANOSECOND); \
    fill_timezone_loc(&timestamp.time.timezone, LAT, LONG); \
    ASSERT_TIMESTAMP_ENCODE_DECODE(actual_timestamp, __VA_ARGS__); \
    ASSERT_EQ(timestamp.time.timezone.data.as_location.latitude, LAT); \
    ASSERT_EQ(timestamp.time.timezone.data.as_location.longitude, LONG); \
}


// ----------
// Timestamps
// ----------

TEST_TIMESTAMP_TZ_NAMED( , 2000,1,1,0,0,0,0, "Europe/Berlin", {0x00, 0x00, 0x08, 0x01, 00, 0x1a, 'E','u','r','o','p','e','/','B','e','r','l','i','n'})
TEST_TIMESTAMP_TZ_LOC( , 2000,1,1,1,0,0,0,100,200, {0x00, 0x40, 0x08, 0x01, 0x00, 0xc9, 0x00, 0x64, 0x00})

TEST_TIMESTAMP_TZ_UTC( ,  2000,1,1,0,0,0,        0, {0x00, 0x00, 0x08, 0x01, 0x01})
TEST_TIMESTAMP_TZ_UTC( ,  2000,1,1,1,0,0,        0, {0x00, 0x40, 0x08, 0x01, 0x01})
TEST_TIMESTAMP_TZ_UTC( ,  2000,1,1,0,1,0,        0, {0x00, 0x01, 0x08, 0x01, 0x01})
TEST_TIMESTAMP_TZ_UTC( ,  2000,1,1,0,0,1,        0, {0x04, 0x00, 0x08, 0x01, 0x01})
TEST_TIMESTAMP_TZ_UTC( ,  2000,1,1,0,0,0,  1000000, {0x01, 0x00, 0x08, 0x11, 0x00, 0x01})
TEST_TIMESTAMP_TZ_UTC( ,  2000,1,1,0,0,0,999000000, {0x01, 0x00, 0x08, 0x71, 0x3e, 0x01})
TEST_TIMESTAMP_TZ_UTC( ,  2000,1,1,0,0,0,   999000, {0x02, 0x00, 0x08, 0x71, 0x3e, 0x00, 0x01})
TEST_TIMESTAMP_TZ_UTC( ,  2000,1,1,0,0,0,      999, {0x03, 0x00, 0x08, 0x71, 0x3e, 0x00, 0x00, 0x00, 0x01})
TEST_TIMESTAMP_TZ_UTC( ,  2009,1,1,0,0,0,        0, {0x00, 0x00, 0x08, 0x01, 0x25})
TEST_TIMESTAMP_TZ_UTC( ,  3009,1,1,0,0,0,        0, {0x00, 0x00, 0x08, 0x01, 0x9f, 0x45})
TEST_TIMESTAMP_TZ_UTC(-, 50000,1,1,0,0,0,        0, {0x00, 0x00, 0x08, 0xc1, 0xd8, 0x7f})


// -------------
// Spec Examples
// -------------

// June 24, 2019, 17:53:04.180
TEST_TIMESTAMP_TZ_UTC(, 2019,6,24,17,53,4,180000000, {0x11, 0x75, 0xc4, 0x46, 0x0b, 0x4d})

// January 7, 1998, 08:19:20, Europe/Rome
TEST_TIMESTAMP_TZ_NAMED(, 1998,1,7,8,19,20,0, "e/rome", {0x50, 0x13, 0x3a, 0x01, 0x06, 0x0c, 'e', '/', 'r', 'o', 'm', 'e'})

// August 31, 3190, 00:54:47.394129, location 59.94, 10.71
TEST_TIMESTAMP_TZ_LOC(, 3190,8,31,0,54,47,394129000, 5994, 1071, {0xbe, 0x36, 0xf8, 0x18, 0x39, 0x60, 0xa5, 0x18, 0xd5, 0xae, 0x17, 0x02})
