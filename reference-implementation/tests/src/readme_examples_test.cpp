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
}

static void demonstrate_decode_timestamp()
{
    std::vector<uint8_t> data = {0x78, 0x13, 0x3a, 0x01, 0x78, 0x16, 0x6d, 0x2f, 0x76, 0x61, 0x6e, 0x63, 0x6f, 0x75, 0x76, 0x65, 0x72};
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
            printf("UTC\n");
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
}

TEST(Readme, example)
{
    demonstrate_encode_timestamp();
    demonstrate_decode_timestamp();
}
