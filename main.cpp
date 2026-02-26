#include <pico/stdlib.h>
#include <hardware/clocks.h>
#include <hardware/sync.h>
#include <hardware/vreg.h>
#include <pico/multicore.h>

#include <array>
#include <cstdio>

#include "tt_setup.h"
#include "tt_pins.h"

#define DESIGN_NUM 1

extern "C" void hstx_send_clocks(int num, int div);

int main() {
    vreg_set_voltage(VREG_VOLTAGE_1_20);

    int freq = 100000;
    set_sys_clock_khz(freq, true);

    stdio_init_all();

    // Uncomment to pause until the USB is connected before continuing
    while (!stdio_usb_connected());

    printf("Desired freq MHz (use a multiple of 4): ");
    scanf("%d", &freq);
    printf("\nSetting frequency to %dMHz\n", freq);
    freq *= 1000;
    set_sys_clock_khz(freq, true);

    sleep_ms(20);
    printf("Selecting Factory Test\n");
    sleep_ms(10);

    tt_select_design(DESIGN_NUM);
    printf("Selected\n");
    sleep_ms(10);

    // Clock in reset and then take out of reset
    tt_clock_project_once();
    gpio_put(nRST, 1);

    // Factory test counts if in0 is high
    gpio_put(IN0, 1);

    gpio_set_drive_strength(CLK, GPIO_DRIVE_STRENGTH_12MA);

    // Initial clock doesn't register?
    tt_clock_project_once();

#if 0
    int last_val = tt_get_output_byte();
    while (1) {
        //bool error = false;
        for (int i = 0; i < 20; ++i) {
            hstx_send_clocks(8 * 5000 + 1, 1);  // Sends 32 * (8 * 5000 + 1) clocks, so we expect the counter to count up by 32.
            sleep_ms(100);
            int val = tt_get_output_byte();
            int diff = (val - last_val) & 0xFF;
            if (diff != 32 /* && diff != 31 */) {
                printf("Error: ");
                //error = true;
            }
            printf("%d %d\n", diff, val);
            last_val = val;
        }

        freq += 4000;

        if (/*error || */ freq > 300000) break;

        set_sys_clock_khz(freq, true);
        printf("\nFreq now: %dMHz\n", freq / 1000);
    }
#endif

    hstx_send_clocks(0xffffffff, 1);

    while(1);
}