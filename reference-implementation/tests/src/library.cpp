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

TEST_ENCODED_SIZE(2000, 0, 4)
TEST_ENCODED_SIZE(1999, 0, 4)
TEST_ENCODED_SIZE(2001, 0, 4)
TEST_ENCODED_SIZE(1996, 0, 4)
TEST_ENCODED_SIZE(2003, 0, 4)
TEST_ENCODED_SIZE(1995, 0, 5)
TEST_ENCODED_SIZE(2004, 0, 5)

TEST_ENCODED_SIZE(2000, 10000000, 5)
TEST_ENCODED_SIZE(1999, 10000000, 5)
TEST_ENCODED_SIZE(2001, 10000000, 6)
TEST_ENCODED_SIZE(1998, 10000000, 6)

TEST_ENCODED_SIZE(2000, 10000, 7)
TEST_ENCODED_SIZE(1999, 10000, 7)
TEST_ENCODED_SIZE(2001, 10000, 7)
TEST_ENCODED_SIZE(1936, 10000, 7)
TEST_ENCODED_SIZE(2063, 10000, 7)
TEST_ENCODED_SIZE(1935, 10000, 8)
TEST_ENCODED_SIZE(2064, 10000, 8)

TEST_ENCODED_SIZE(2000, 10, 8)
TEST_ENCODED_SIZE(1999, 10, 8)
TEST_ENCODED_SIZE(2001, 10, 8)
TEST_ENCODED_SIZE(1984, 10, 8)
TEST_ENCODED_SIZE(2015, 10, 8)
TEST_ENCODED_SIZE(1983, 10, 9)
TEST_ENCODED_SIZE(2016, 10, 9)

TEST_ENCODE(2000,1,1,0,0,0,0, {0x00, 0x00, 0x08, 0x01})
TEST_ENCODE(2000,1,1,1,0,0,0, {0x00, 0x40, 0x08, 0x01})
TEST_ENCODE(2000,1,1,0,1,0,0, {0x00, 0x01, 0x08, 0x01})
TEST_ENCODE(2000,1,1,0,0,1,0, {0x04, 0x00, 0x08, 0x01})
TEST_ENCODE(2000,1,1,0,0,0,1000000, {0x01, 0x00, 0x08, 0x11, 0x00})
TEST_ENCODE(2000,1,1,0,0,0,999000000, {0x01, 0x00, 0x08, 0x71, 0x3e})
TEST_ENCODE(2000,1,1,0,0,0,999000, {0x02, 0x00, 0x08, 0x71, 0x3e, 0x00, 0x00})
TEST_ENCODE(2000,1,1,0,0,0,999, {0x03, 0x00, 0x08, 0x71, 0x3e, 0x00, 0x00, 0x00})
TEST_ENCODE(2009,1,1,0,0,0,0, {0x00, 0x00, 0x08, 0xa1, 0x02})
TEST_ENCODE(3009,1,1,0,0,0,0, {0x00, 0x00, 0x08, 0xa1, 0xfc, 01})
TEST_ENCODE_NYEAR(50000,1,1,0,0,0,0, {0x00, 0x00, 0x08, 0xf1, 0xc7, 0x65})


// Spec Examples

// June 24, 2019, 17:53:04.180
TEST_ENCODE(2019,6,24,17,53,4,180000000, {0x11, 0x75, 0xc4, 0x46, 0x8b, 0x13})

// January 7, 1998, 08:19:20
TEST_ENCODE(1998,1,7,8,19,20,0, {0x50, 0x13, 0x3a, 0x31})
