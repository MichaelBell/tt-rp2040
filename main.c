#include <pico/stdlib.h>
#include <hardware/clocks.h>
#include <hardware/sync.h>
#include <hardware/vreg.h>
#include <pico/multicore.h>

#include <stdio.h>

#include "tt_setup.h"
#include "tt_pins.h"

#define DESIGN_NUM 876

void send_recv_data_stream(int len, uint8_t* data_a, uint8_t* data_b, uint16_t* data_out);

#define DATA_LEN 16384
static uint8_t data_a[DATA_LEN];
static uint8_t data_b[DATA_LEN];
static uint16_t data_out[DATA_LEN+2];

int main() {
    vreg_set_voltage(VREG_VOLTAGE_1_30);

    stdio_init_all();

    // Uncomment to pause until the USB is connected before continuing
    while (!stdio_usb_connected());

#if 0
    printf("Desired freq MHz (use a multiple of 4): ");
    scanf("%d", &freq);
    printf("\nSetting frequency to %dMHz\n", freq);
    freq *= 1000;
    set_sys_clock_khz(freq, true);
#endif

    sleep_ms(20);
    printf("Selecting 8b10b\n");
    sleep_ms(10);

    tt_select_design(DESIGN_NUM);
    printf("Selected\n");
    sleep_ms(10);

    gpio_set_drive_strength(CLK, GPIO_DRIVE_STRENGTH_12MA);
    gpio_set_drive_strength(IN0, GPIO_DRIVE_STRENGTH_8MA);
    gpio_set_drive_strength(IN1, GPIO_DRIVE_STRENGTH_8MA);

    int error = 0;
    for (int freq = 160000; freq <= 320000 && !error; freq += 2000) {
        set_sys_clock_khz(freq, true);

        //stdio_init_all();

        // Clock in reset and then take out of reset
        tt_clock_project_once();
        gpio_put(nRST, 1);

        // Set inputs to read decoded data
        gpio_put(IN2, 0);
        gpio_put(IN3, 0);
        gpio_put(IN4, 0);
        gpio_put(IN5, 0);
        gpio_put(IN6, 1);
        gpio_put(IN7, 1);

        for (int i = 0; i < DATA_LEN; i++) {
            data_a[i] = i & 0xFF;
            data_b[i] = (i >> 6) & 0xFF;
        }

        send_recv_data_stream(DATA_LEN, data_a, data_b, data_out);

        for (int i = 0; i < DATA_LEN && error < 10; i++) {
            if (data_out[i+2] != ((data_a[i] << 8) | data_b[i])) {
                printf("Data mismatch at index %d: expected %04X, got %04X\n", i, ((data_a[i] << 8) | data_b[i]), data_out[i+2]);
                error++;
            }
        }

        printf("Data transfer %s at freq %d\n", error ? "failed" : "complete", freq);

        gpio_put(IN2, 1);
        gpio_put(IN3, 0);
        gpio_put(IN4, 0);
        gpio_put(IN5, 0);
        gpio_put(IN6, 0);
        gpio_put(IN7, 1);
        if (((gpio_get_all64() >> OUT0) & 0xff) != 5) {
            printf("Error: Not synced at freq %d\n", freq);
        }

        if (freq >= 268000) freq += 2000;
    }

    #if 0
    for (int i = 0; i < DATA_LEN; i++) {
        printf("%04X ", data_out[i+2]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    #endif

    while(1);
}
