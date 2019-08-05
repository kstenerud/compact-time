#include <gtest/gtest.h>
#include <cdate/cdate.h>

// #define KSLogger_LocalLevel DEBUG
#include "kslogger.h"

TEST(Library, version)
{
    const char* expected = "1.0.0";
    const char* actual = cdate_version();
    ASSERT_STREQ(expected, actual);
}

#define TEST_ENCODED_SIZE(YEAR, NANOSECOND, EXPECTED_SIZE) \
TEST(CDate, size_ ## YEAR ## _ ## NANOSECOND) \
{ \
    int expected_size = EXPECTED_SIZE; \
    cdate date; \
    memset(&date, 0, sizeof(date)); \
    date.year = YEAR; \
    date.nanosecond = NANOSECOND; \
    int actual_size = cdate_encoded_size(&date); \
    ASSERT_EQ(expected_size, actual_size); \
}

#define TEST_ENCODE(YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, NANOSECOND, ...) \
TEST(CDate, encode_ ## YEAR ## _ ## MONTH ## _ ## DAY ## _ ## HOUR ## _ ## MINUTE ## _ ## SECOND ## _ ## NANOSECOND) \
{ \
    std::vector<uint8_t> expected = __VA_ARGS__; \
    cdate date; \
    memset(&date, 0, sizeof(date)); \
    date.year = YEAR; \
    date.month = MONTH; \
    date.day = DAY; \
    date.hour = HOUR; \
    date.minute = MINUTE; \
    date.second = SECOND; \
    date.nanosecond = NANOSECOND; \
    std::vector<uint8_t> actual(cdate_encoded_size(&date)); \
    int bytes_encoded = cdate_encode(&date, actual.data(), actual.size()); \
    ASSERT_GT(bytes_encoded, 0); \
    ASSERT_EQ(expected, actual); \
    \
    cdate actual_date; \
    memset(&actual_date, 0, sizeof(actual_date)); \
    int bytes_decoded = cdate_decode(actual.data(), actual.size(), &actual_date); \
    ASSERT_GT(bytes_decoded, 0); \
    ASSERT_EQ(date.year, actual_date.year); \
    ASSERT_EQ(date.month, actual_date.month); \
    ASSERT_EQ(date.day, actual_date.day); \
    ASSERT_EQ(date.hour, actual_date.hour); \
    ASSERT_EQ(date.minute, actual_date.minute); \
    ASSERT_EQ(date.second, actual_date.second); \
    ASSERT_EQ(date.nanosecond, actual_date.nanosecond); \
}

#define TEST_ENCODE_NYEAR(YEAR, MONTH, DAY, HOUR, MINUTE, SECOND, NANOSECOND, ...) \
TEST(CDate, encode_ ## YEAR ## _ ## MONTH ## _ ## DAY ## _ ## HOUR ## _ ## MINUTE ## _ ## SECOND ## _ ## NANOSECOND) \
{ \
    std::vector<uint8_t> expected = __VA_ARGS__; \
    cdate date; \
    memset(&date, 0, sizeof(date)); \
    date.year = -YEAR; \
    date.month = MONTH; \
    date.day = DAY; \
    date.hour = HOUR; \
    date.minute = MINUTE; \
    date.second = SECOND; \
    date.nanosecond = NANOSECOND; \
    std::vector<uint8_t> actual(cdate_encoded_size(&date)); \
    int bytes_encoded = cdate_encode(&date, actual.data(), actual.size()); \
    ASSERT_GT(bytes_encoded, 0); \
    ASSERT_EQ(expected, actual); \
    \
    cdate actual_date; \
    memset(&actual_date, 0, sizeof(actual_date)); \
    int bytes_decoded = cdate_decode(actual.data(), actual.size(), &actual_date); \
    ASSERT_GT(bytes_decoded, 0); \
    ASSERT_EQ(date.year, actual_date.year); \
    ASSERT_EQ(date.month, actual_date.month); \
    ASSERT_EQ(date.day, actual_date.day); \
    ASSERT_EQ(date.hour, actual_date.hour); \
    ASSERT_EQ(date.minute, actual_date.minute); \
    ASSERT_EQ(date.second, actual_date.second); \
    ASSERT_EQ(date.nanosecond, actual_date.nanosecond); \
}

TEST_ENCODED_SIZE(2000, 0, 5)
TEST_ENCODED_SIZE(1999, 0, 5)
TEST_ENCODED_SIZE(2001, 0, 5)
TEST_ENCODED_SIZE(1996, 0, 5)
TEST_ENCODED_SIZE(2003, 0, 5)
TEST_ENCODED_SIZE(1995, 0, 5)
TEST_ENCODED_SIZE(2004, 0, 5)

TEST_ENCODED_SIZE(2000, 10000000, 6)
TEST_ENCODED_SIZE(1999, 10000000, 6)
TEST_ENCODED_SIZE(2001, 10000000, 6)
TEST_ENCODED_SIZE(1998, 10000000, 6)

TEST_ENCODED_SIZE(2000, 10000, 7)
TEST_ENCODED_SIZE(1999, 10000, 7)
TEST_ENCODED_SIZE(2001, 10000, 7)
TEST_ENCODED_SIZE(1936, 10000, 7)
TEST_ENCODED_SIZE(2063, 10000, 7)
TEST_ENCODED_SIZE(1935, 10000, 8)
TEST_ENCODED_SIZE(2064, 10000, 8)

TEST_ENCODED_SIZE(2000, 10, 9)
TEST_ENCODED_SIZE(1999, 10, 9)
TEST_ENCODED_SIZE(2001, 10, 9)
TEST_ENCODED_SIZE(1984, 10, 9)
TEST_ENCODED_SIZE(2015, 10, 9)
TEST_ENCODED_SIZE(1983, 10, 9)
TEST_ENCODED_SIZE(7000, 10, 10)

TEST_ENCODE(2000,1,1,0,0,0,0, {0x00, 0x00, 0x01, 0x10, 00})
TEST_ENCODE(2000,1,1,1,0,0,0, {0x00, 0x00, 0x21, 0x10, 0x00})
TEST_ENCODE(2000,1,1,0,1,0,0, {0x00, 0x04, 0x01, 0x10, 0x00})
TEST_ENCODE(2000,1,1,0,0,1,0, {0x01, 0x00, 0x01, 0x10, 0x00})
TEST_ENCODE(2000,1,1,0,0,0,1000000, {0x40, 0x10, 0x00, 0x00, 0x44, 0x00})
TEST_ENCODE(2000,1,1,0,0,0,999000000, {0x7e, 0x70, 0x00, 0x00, 0x44, 0x00})
TEST_ENCODE(2000,1,1,0,0,0,999000, {0x80, 0x0f, 0x9c, 0x00, 0x00, 0x11, 0x00})
TEST_ENCODE(2000,1,1,0,0,0,999, {0xc0, 0x00, 0x03, 0xe7, 0x00, 0x00, 0x04, 0x40, 0x00})
TEST_ENCODE(2009,1,1,0,0,0,0, {0x00, 0x00, 0x01, 0x10, 0x12})
TEST_ENCODE(3009,1,1,0,0,0,0, {0x00, 0x00, 0x01, 0x1f, 0x62})
TEST_ENCODE_NYEAR(50000,1,1,0,0,0,0, {0x00, 0x00, 0x01, 0x16, 0xac, 0x3f})


// Spec Examples

// June 24, 2019, 17:53:04.180
TEST_ENCODE(2019,6,24,17,53,4,180000000, {0x4b, 0x41, 0x35, 0x8e, 0x18, 0x26})

// January 7, 1998, 08:19:20
TEST_ENCODE(1998,1,7,8,19,20,0, {0x14, 0x4d, 0x07, 0x10, 0x03})

// August 31, 3190, 00:54:47.394129
TEST_ENCODE(3190,8,31,0,54,47,394129000, {0x98, 0x0e, 0x46, 0xfd, 0x81, 0xf8, 0x92, 0x4c})
