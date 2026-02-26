#include <pico/stdlib.h>
#include <hardware/gpio.h>

#include "tt_pins.h"

void tt_select_design(int idx)
{
    // Ensure all pins are set to safe defaults.
    gpio_set_dir_all_bits64(0);
    for (uint i = 0; i < NUM_BANK0_GPIOS; i++) {
        gpio_init(i);
    }

    // Enable clk, reset, and ctrl signals
    gpio_put(CLK, 0);
    gpio_put(nRST, 0);
    gpio_put(CINC, 0);
    gpio_put(CENA, 0);
    gpio_put(nCRST, 1);
    gpio_set_dir_all_bits((1 << CLK) | (1 << nRST) | (1 << CENA) | (1 << nCRST) | (1 << CINC));
    sleep_us(10);

    // Mux control reset
    gpio_put(nCRST, 0);
    sleep_us(100);
    gpio_put(nCRST, 1);
    sleep_us(100);

    // Mux select
    for (int i = 0; i < idx; ++i) {
        gpio_put(CINC, 1);
        sleep_us(10);
        gpio_put(CINC, 0);
        sleep_us(10);
    }

    // Enable design
    sleep_us(20);
    gpio_put(CENA, 1);
    sleep_us(20);

    // Set reset and project inputs to RP2040 outputs
    gpio_set_dir_all_bits((1 << CLK) | (1 << nRST) | (1 << CENA) | (1 << nCRST) | (1 << CINC) |
                          (0xFF << IN0));

    // Leave design in reset
    sleep_us(10);
}