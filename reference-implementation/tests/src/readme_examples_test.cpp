#include <gtest/gtest.h>
#include <stdio.h>
#include <vector>
#include <cdate/cdate.h>

static void print_buffer(const uint8_t* data, int length)
{
    for(int i = 0; i < length; i++)
    {
        printf("%02x ", data[i]);
    }
    printf("\n");
}

static void demonstrate_encode()
{
    cdate date;
    date.year = 2020;
    date.month = 8;
    date.day = 30;
    date.hour = 15;
    date.minute = 33;
    date.second = 14;
    date.nanosecond = 19577323;
    uint8_t data[10];
    int bytes_encoded = cdate_encode(&date, data, sizeof(data));
    print_buffer(data, bytes_encoded);
}

static void demonstrate_decode()
{
    std::vector<uint8_t> data = {0x50, 0x13, 0x3a, 0x31};
    cdate date;
    cdate_decode(data.data(), data.size(), &date);
    printf("%04d-%02d-%02d %02d:%02d:%02d.%d\n",
        date.year, date.month, date.day,
        date.hour, date.minute, date.second, date.nanosecond);
}

TEST(Readme, example)
{
	demonstrate_encode();
    demonstrate_decode();
}
