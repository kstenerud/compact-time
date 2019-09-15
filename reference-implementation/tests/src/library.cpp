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

static void fill_date(ct_date* date, int year, int month, int day)
{
    memset(date, 0, sizeof(*date));
    date->year = year;
    date->month = month;
    date->day = day;
}

static void fill_time(ct_time* time, int hour, int minute, int second, int nanosecond)
{
    memset(time, 0, sizeof(*time));
    time->hour = hour;
    time->minute = minute;
    time->second = second;
    time->nanosecond = nanosecond;
}

static void fill_timestamp(ct_timestamp* timestamp, int year, int month, int day, int hour, int minute, int second, int nanosecond)
{
    memset(timestamp, 0, sizeof(*timestamp));
    fill_date(&timestamp->date, year, month, day);
    fill_time(&timestamp->time, hour, minute, second, nanosecond);
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

#define ASSERT_DATE_ENCODE_DECODE(EXPECTED_DATE, ACTUAL_DATE, ...) \
    std::vector<uint8_t> expected = __VA_ARGS__; \
    std::vector<uint8_t> actual(ct_date_encoded_size(&EXPECTED_DATE)); \
    int bytes_encoded = ct_date_encode(&EXPECTED_DATE, actual.data(), actual.size()); \
    ASSERT_EQ(expected, actual); \
    ASSERT_EQ(bytes_encoded, expected.size()); \
    ct_date ACTUAL_DATE; \
    memset(&ACTUAL_DATE, 0, sizeof(ACTUAL_DATE)); \
    int bytes_decoded = ct_date_decode(actual.data(), actual.size(), &ACTUAL_DATE); \
    ASSERT_EQ(bytes_decoded, expected.size()); \
    ASSERT_DATE_EQ(ACTUAL_DATE, EXPECTED_DATE)

#define ASSERT_TIME_ENCODE_DECODE(EXPECTED_TIME, ACTUAL_TIME, ...) \
    std::vector<uint8_t> expected = __VA_ARGS__; \
    std::vector<uint8_t> actual(ct_time_encoded_size(&EXPECTED_TIME)); \
    int bytes_encoded = ct_time_encode(&EXPECTED_TIME, actual.data(), actual.size()); \
    ASSERT_EQ(expected, actual); \
    ASSERT_EQ(bytes_encoded, expected.size()); \
    ct_time ACTUAL_TIME; \
    memset(&ACTUAL_TIME, 0, sizeof(ACTUAL_TIME)); \
    int bytes_decoded = ct_time_decode(actual.data(), actual.size(), &ACTUAL_TIME); \
    ASSERT_EQ(bytes_decoded, expected.size()); \
    ASSERT_TIME_EQ(ACTUAL_TIME, EXPECTED_TIME)

#define ASSERT_TIMESTAMP_ENCODE_DECODE(EXPECTED_TIMESTAMP, ACTUAL_TIMESTAMP, ...) \
    std::vector<uint8_t> expected = __VA_ARGS__; \
    std::vector<uint8_t> actual(ct_timestamp_encoded_size(&EXPECTED_TIMESTAMP)); \
    int bytes_encoded = ct_timestamp_encode(&EXPECTED_TIMESTAMP, actual.data(), actual.size()); \
    ASSERT_EQ(expected, actual); \
    ASSERT_EQ(bytes_encoded, expected.size()); \
    ct_timestamp ACTUAL_TIMESTAMP; \
    memset(&ACTUAL_TIMESTAMP, 0, sizeof(ACTUAL_TIMESTAMP)); \
    int bytes_decoded = ct_timestamp_decode(actual.data(), actual.size(), &ACTUAL_TIMESTAMP); \
    ASSERT_EQ(bytes_decoded, expected.size()); \
    ASSERT_DATE_EQ(ACTUAL_TIMESTAMP.date, EXPECTED_TIMESTAMP.date); \
    ASSERT_TIME_EQ(ACTUAL_TIMESTAMP.time, EXPECTED_TIMESTAMP.time)


#define TEST_DATE(SIGN, YEAR, MONTH, DAY, ...) \
TEST(CDate, date_utc_ ## YEAR ## _ ## MONTH ## _ ## DAY) \
{ \
    ct_date date; \
    fill_date(&date, SIGN YEAR, MONTH, DAY); \
    ASSERT_DATE_ENCODE_DECODE(date, actual_date, __VA_ARGS__); \
}

#define TEST_TIME_TZ_UTC(HOUR, MINUTE, SECOND, NANOSECOND, ...) \
TEST(CDate, time_utc_ ## HOUR ## _ ## MINUTE ## _ ## SECOND ## _ ## NANOSECOND) \
{ \
    ct_time time; \
    fill_time(&time, HOUR, MINUTE, SECOND, NANOSECOND); \
    fill_timezone_utc(&time.timezone); \
    ASSERT_TIME_ENCODE_DECODE(time, actual_time, __VA_ARGS__); \
}

#define TEST_TIME_TZ_NAMED(HOUR, MINUTE, SECOND, NANOSECOND, TZ, ...) \
TEST(CDate, time_named_ ## HOUR ## _ ## MINUTE ## _ ## SECOND ## _ ## NANOSECOND) \
{ \
    ct_time time; \
    fill_time(&time, HOUR, MINUTE, SECOND, NANOSECOND); \
    fill_timezone_named(&time.timezone, TZ); \
    ASSERT_TIME_ENCODE_DECODE(time, actual_time, __VA_ARGS__); \
    ASSERT_STREQ(actual_time.timezone.data.as_string, TZ); \
}

#define TEST_TIME_TZ_LOC(HOUR, MINUTE, SECOND, NANOSECOND, LAT, LONG, ...) \
TEST(CDate, time_loc_ ## HOUR ## _ ## MINUTE ## _ ## SECOND ## _ ## NANOSECOND) \
{ \
    ct_time time; \
    fill_time(&time, HOUR, MINUTE, SECOND, NANOSECOND); \
    fill_timezone_loc(&time.timezone, LAT, LONG); \
    ASSERT_TIME_ENCODE_DECODE(time, actual_time, __VA_ARGS__); \
    ASSERT_EQ(time.timezone.data.as_location.latitude, LAT); \
    ASSERT_EQ(time.timezone.data.as_location.longitude, LONG); \
}

#define TEST_TIMESTAMP_TZ_UTC(SIGN, YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, NANOSECOND, ...) \
TEST(CDate, timestamp_utc_ ## YEAR ## _ ## MONTH ## _ ## DAY ## _ ## HOUR ## _ ## MINUTE ## _ ## SECOND ## _ ## NANOSECOND) \
{ \
    ct_timestamp timestamp; \
    fill_timestamp(&timestamp, SIGN YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, NANOSECOND); \
    fill_timezone_utc(&timestamp.time.timezone); \
    ASSERT_TIMESTAMP_ENCODE_DECODE(timestamp, actual_timestamp, __VA_ARGS__); \
}

#define TEST_TIMESTAMP_TZ_NAMED(SIGN, YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, NANOSECOND, TZ, ...) \
TEST(CDate, timestamp_named_ ## YEAR ## _ ## MONTH ## _ ## DAY ## _ ## HOUR ## _ ## MINUTE ## _ ## SECOND ## _ ## NANOSECOND) \
{ \
    ct_timestamp timestamp; \
    fill_timestamp(&timestamp, SIGN YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, NANOSECOND); \
    fill_timezone_named(&timestamp.time.timezone, TZ); \
    ASSERT_TIMESTAMP_ENCODE_DECODE(timestamp, actual_timestamp, __VA_ARGS__); \
    ASSERT_STREQ(actual_timestamp.time.timezone.data.as_string, TZ); \
}

#define TEST_TIMESTAMP_TZ_LOC(SIGN, YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, NANOSECOND, LAT, LONG, ...) \
TEST(CDate, timestamp_loc_ ## YEAR ## _ ## MONTH ## _ ## DAY ## _ ## HOUR ## _ ## MINUTE ## _ ## SECOND ## _ ## NANOSECOND ## _ ## LAT ## _ ## LONG) \
{ \
    ct_timestamp timestamp; \
    fill_timestamp(&timestamp, SIGN YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, NANOSECOND); \
    fill_timezone_loc(&timestamp.time.timezone, LAT, LONG); \
    ASSERT_TIMESTAMP_ENCODE_DECODE(timestamp, actual_timestamp, __VA_ARGS__); \
    ASSERT_EQ(timestamp.time.timezone.data.as_location.latitude, LAT); \
    ASSERT_EQ(timestamp.time.timezone.data.as_location.longitude, LONG); \
}


// ----
// Date
// ----

TEST_DATE( , 2000,1,1, {0x21, 0x00, 0x00})
TEST_DATE(-, 2000,12,21, {0x95, 0x7d, 0x3f})



// ----
// Time
// ----

TEST_TIME_TZ_UTC(8, 41, 05, 999999999, {0x47, 0x69, 0xf1, 0x9f, 0xac, 0xb9, 0x03})
TEST_TIME_TZ_UTC(14, 18, 30, 43000000, {0x73, 0x92, 0xb7, 0x02})
TEST_TIME_TZ_UTC(23, 6, 55, 8000, {0xbd, 0xc6, 0x8d, 0x00, 0x00})
TEST_TIME_TZ_NAMED(10, 10, 10, 0, "S/Tokyo", {0x50, 0x8a, 0x02, 0x0e, 'S','/','T','o','k','y','o'})
TEST_TIME_TZ_LOC(7, 45, 0, 1000000, 3876, -2730, {0x3a, 0x2d, 0x10, 0x00, 0x49, 0x1e, 0xab, 0x3a})



// ---------
// Timestamp
// ---------

TEST_TIMESTAMP_TZ_NAMED( , 2000,1,1,0,0,0,0, "Europe/Berlin", {0x00, 0x00, 0x08, 0x01, 00, 0x1a, 'E','u','r','o','p','e','/','B','e','r','l','i','n'})
TEST_TIMESTAMP_TZ_NAMED( , 2020,8,30,15,33,14,19577323, "S/Singapore", {0x3b, 0xe1, 0xf3, 0xb8, 0x9e, 0xab, 0x12, 0x00, 0x50, 0x16, 'S', '/', 'S', 'i', 'n', 'g', 'a', 'p', 'o', 'r', 'e'})

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
TEST_TIMESTAMP_TZ_NAMED(, 1998,1,7,8,19,20,0, "E/Rome", {0x50, 0x13, 0x3a, 0x01, 0x06, 0x0c, 'E', '/', 'R', 'o', 'm', 'e'})

// August 31, 3190, 00:54:47.394129, location 59.94, 10.71
TEST_TIMESTAMP_TZ_LOC(, 3190,8,31,0,54,47,394129000, 5994, 1071, {0xbe, 0x36, 0xf8, 0x18, 0x39, 0x60, 0xa5, 0x18, 0xd5, 0xae, 0x17, 0x02})
