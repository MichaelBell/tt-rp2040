#pragma once

#include "hardware/gpio.h"

// Mapping for TT-ETRv3 board
enum GPIOMap {
    CLK = 16,
    nRST = 14,
    CENA = 0,
    nCRST = 1,
    CINC = 2,
    IN0 = 17,
    IN1 = 18,
    IN2 = 19,
    IN3 = 20,
    IN4  = 21,
    IN5  = 22,
    IN6  = 23,
    IN7  = 24,
    OUT0 = 33,
    OUT1 = 34,
    OUT2 = 35,
    OUT3 = 36,
    OUT4 = 37,
    OUT5 = 38,
    OUT6 = 39,
    OUT7 = 40,
    UIO0 = 25,
    UIO1 = 26,
    UIO2 = 27,
    UIO3 = 28,
    UIO4 = 29,
    UIO5 = 30,
    UIO6 = 31,
    UIO7 = 32,
};

inline static void tt_set_input_byte(int val) {
    gpio_put_masked(0xFF << IN0, val << IN0);
}

inline static int tt_get_output_byte() {
    uint64_t gpio = gpio_get_all64();
    return (gpio >> OUT0) & 0xFF;
}

inline static void tt_clock_project_once() {
    gpio_xor_mask(1 << CLK);
    sleep_us(20);
    gpio_xor_mask(1 << CLK);
    sleep_us(20);
}