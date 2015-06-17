#pragma once

#include <stdint.h>


enum {
    umode_async = 0,
    umode_sync = 1,
    umode_master_spi = 2
};

enum {
    upar_none = 0,
    upar_even = 2,
    upar_odd = 3
};

enum {
    ustop_1 = 0,
    ustop_2 = 1
};

enum {
    usize_5 = 0,
    usize_6 = 1,
    usize_7 = 2,
    usize_8 = 3,
    usize_9 = 7
};

void U0_config(int8_t rxen, int8_t txen, int8_t mode, int8_t parity,
        int8_t stop, int8_t size, int16_t baud_rate);
void U0_ie_config(int8_t rxcie, int8_t txcie, int8_t udrie);
