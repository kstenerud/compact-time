Reference Implementation for Compact Time
=========================================

A C implementation to demonstrate compact time.



Usage
-----

```cpp
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
```

Output:

    c1 2a b9 eb 3a 17 fa 00 28


```cpp
static void demonstrate_decode()
{
    std::vector<uint8_t> data = {0x14, 0x4d, 0x07, 0x10, 0x03};
    cdate date;
    cdate_decode(data.data(), data.size(), &date);
    printf("%04d-%02d-%02d %02d:%02d:%02d.%d\n",
        date.year, date.month, date.day,
        date.hour, date.minute, date.second, date.nanosecond);
}
```

Output:

    1998-01-07 08:19:20.0



Requirements
------------

  * Meson 0.49 or newer
  * Ninja 1.8.2 or newer
  * A C compiler
  * A C++ compiler (for the tests)



Building
--------

    meson build
    ninja -C build



Running Tests
-------------

    ninja -C build test

For the full report:

    ./build/run_tests



Installing
----------

    ninja -C build install
