#include <gtest/gtest.h>
#include <stdio.h>
#include <vector>
#include <compact_time/compact_time.h>

static void print_buffer(const uint8_t* data, int length)
{
    for(int i = 0; i < length; i++)
    {
        printf("%02x ", data[i]);
    }
    printf("\n");
}

static void demonstrate_encode_date()
{
    ct_date date;
    date.year = 2031;
    date.month = 10;
    date.day = 30;

    uint8_t data[10];
    int bytes_encoded = ct_date_encode(&date, data, sizeof(data));
    print_buffer(data, bytes_encoded);

    // Outputs: 5e 01 3e
}

static void demonstrate_decode_date()
{
    std::vector<uint8_t> data = {0x4f, 0x00, 0x27};
    ct_date date;
    ct_date_decode(data.data(), data.size(), &date);
    printf("%04d-%02d-%02d\n", date.year, date.month, date.day);

    // Outputs: 1980-02-15
}

static void demonstrate_encode_time()
{
    ct_time time;
    time.hour = 12;
    time.minute = 30;
    time.second = 14;
    time.nanosecond = 134000000;
    time.timezone.type = CT_TZ_LATLONG;
    time.timezone.data.as_location.latitude = 5881;
    time.timezone.data.as_location.longitude = -12270;

    uint8_t data[20];
    int bytes_encoded = ct_time_encode(&time, data, sizeof(data));
    print_buffer(data, bytes_encoded);

    // Outputs: 62 9e 63 08 f3 2d 09 28 
}

static void demonstrate_decode_time()
{
    std::vector<uint8_t> data = {0x05, 0xe4, 0x23, 0x45, 0xef};
    ct_time time;
    ct_time_decode(data.data(), data.size(), &time);
    printf("%02d:%02d:%02d.%d ",
        time.hour,
        time.minute,
        time.second,
        time.nanosecond);
    switch(time.timezone.type)
    {
        case CT_TZ_ZERO:
            printf("Etc/UTC\n");
            break;
        case CT_TZ_STRING:
            printf("%s\n", time.timezone.data.as_string);
            break;
        case CT_TZ_LATLONG:
            printf("%.2f,%.2f\n",
                ((float)time.timezone.data.as_location.longitude) / 100,
                ((float)time.timezone.data.as_location.latitude) / 100
                );
            break;
    }

    // Outputs: 00:36:15.980050000 Etc/UTC
}

static void demonstrate_encode_timestamp()
{
    ct_timestamp timestamp;
    timestamp.date.year = 1998;
    timestamp.date.month = 8;
    timestamp.date.day = 30;
    timestamp.time.hour = 15;
    timestamp.time.minute = 33;
    timestamp.time.second = 14;
    timestamp.time.nanosecond = 19577323;
    timestamp.time.timezone.type = CT_TZ_ZERO;

    uint8_t data[20];
    int bytes_encoded = ct_timestamp_encode(&timestamp, data, sizeof(data));
    print_buffer(data, bytes_encoded);

    // Outputs: 3b e1 f3 b8 9e ab 12 00 07 
}

static void demonstrate_decode_timestamp()
{
    std::vector<uint8_t> data = {0x78, 0x13, 0x3a, 0x01, 0x78, 0x16, 0x4d, 0x2f, 0x56, 0x61, 0x6e, 0x63, 0x6f, 0x75, 0x76, 0x65, 0x72};
    ct_timestamp timestamp;
    ct_timestamp_decode(data.data(), data.size(), &timestamp);
    printf("%04d-%02d-%02d %02d:%02d:%02d.%d ",
        timestamp.date.year,
        timestamp.date.month,
        timestamp.date.day,
        timestamp.time.hour,
        timestamp.time.minute,
        timestamp.time.second,
        timestamp.time.nanosecond);
    switch(timestamp.time.timezone.type)
    {
        case CT_TZ_ZERO:
            printf("Etc/UTC\n");
            break;
        case CT_TZ_STRING:
            printf("%s\n", timestamp.time.timezone.data.as_string);
            break;
        case CT_TZ_LATLONG:
            printf("%.2f,%.2f\n",
                ((float)timestamp.time.timezone.data.as_location.longitude) / 100,
                ((float)timestamp.time.timezone.data.as_location.latitude) / 100
                );
            break;
    }

    // Outputs: 2030-01-07 08:19:30.0 M/Vancouver
}


TEST(Readme, example)
{
    demonstrate_encode_date();
    demonstrate_decode_date();
    demonstrate_encode_time();
    demonstrate_decode_time();
    demonstrate_encode_timestamp();
    demonstrate_decode_timestamp();
}
