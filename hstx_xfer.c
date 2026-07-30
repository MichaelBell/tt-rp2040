#include <pico/stdlib.h>
#include <stdio.h>
#include "hardware/structs/hstx_ctrl.h"
#include "hardware/structs/hstx_fifo.h"
#include "hardware/dma.h"

#include "tt_pins.h"
#include "8b10b.h"

#include "recv.pio.h"

#define MAX_DATA_LEN 16384
static uint32_t encoded[MAX_DATA_LEN+2];

static void encodeData(uint8_t* data_a, uint8_t* data_b, int len)
{
    unsigned char b, lu;
    int i, x, y, code6, code4;
    int rdp_a = 0, rdp_b = 0;
    uint16_t enc_a, enc_b;
    uint32_t enc;

    encoded[0] = 0b11001100000000001111;  // Sync word to start the transfer

    for (i=0; i < len; i++)
    {
        b = data_a[i];
        x = b & 0x1F;   //HGFEDCBA => EDCBA
        lu = data5b6b[x];
        code6 = RD_CODE6(lu, rdp_a); //EDCBA => abcdei
        rdp_a = RD_NEXT(lu, rdp_a);

        y = b >> 5;     //HGFEDBCA => HGF
        if (y==7 &&
           ((rdp_a && (x == 11 || x == 13 || x == 14)) ||  //RD=+1
            (!rdp_a && (x == 17 || x == 18 || x == 20))))   //RD=-1
            lu = data3b4b[A7];  //use alternate code to prevent runs of 5
        else
            lu = data3b4b[y];

        code4 = RD_CODE4(lu, rdp_a);  //HGF => fghj
        rdp_a = RD_NEXT(lu, rdp_a);

        enc_a = code4 << 6 | code6;

        b = data_b[i];
        x = b & 0x1F;   //HGFEDCBA => EDCBA
        lu = data5b6b[x];
        code6 = RD_CODE6(lu, rdp_b); //EDCBA => abcdei
        rdp_b = RD_NEXT(lu, rdp_b);

        y = b >> 5;     //HGFEDBCA => HGF
        if (y==7 &&
           ((rdp_b && (x == 11 || x == 13 || x == 14)) ||  //RD=+1
            (!rdp_b && (x == 17 || x == 18 || x == 20))))   //RD=-1
            lu = data3b4b[A7];  //use alternate code to prevent runs of 5
        else
            lu = data3b4b[y];

        code4 = RD_CODE4(lu, rdp_b);  //HGF => fghj
        rdp_b = RD_NEXT(lu, rdp_b);

        enc_b = code4 << 6 | code6;
        enc_b <<= 1;
        
        enc = 0;
        for (int j = 0; j < 10; j++) {
            enc |= (((enc_a >> j) & 1) | ((enc_b >> j) & 2)) << (j * 2);
        }
        encoded[i+1] = enc;
    }
    encoded[len+1] = encoded[1];
}

static bool pio_initialized;

void send_recv_data_stream(int len, uint8_t* data_a, uint8_t* data_b, uint16_t* data_out)
{
    if (len > MAX_DATA_LEN) {
        printf("Data length %d exceeds maximum allowed length of %d\n", len, MAX_DATA_LEN);
        return;
    }

    PIO pio = pio0;
    uint sm = 0;
    static uint offset;
    if (!pio_initialized) {
        pio->gpiobase = 16;
        offset = pio_add_program(pio, &recv_data_program);
        recv_data_program_init(pio, sm, offset);
        pio_initialized = true;
    }

    encodeData(data_a, data_b, len);

    hstx_ctrl_hw->bit[CLK - 12] = 0x20000;
    hstx_ctrl_hw->bit[IN0 - 12] = 0;
    hstx_ctrl_hw->bit[IN1 - 12] = 0x101;
    hstx_ctrl_hw->csr = (1 << HSTX_CTRL_CSR_CLKDIV_LSB) | (0 << HSTX_CTRL_CSR_CLKPHASE_LSB) | (10 << HSTX_CTRL_CSR_N_SHIFTS_LSB) | (2 << HSTX_CTRL_CSR_SHIFT_LSB);
    
    gpio_set_function(CLK, GPIO_FUNC_HSTX);
    gpio_set_function(IN0, GPIO_FUNC_HSTX);
    gpio_set_function(IN1, GPIO_FUNC_HSTX);

    hstx_ctrl_hw->csr |= HSTX_CTRL_CSR_EN_BITS;

    // Setup the transfer.  First send some zeros.
    hstx_fifo_hw->fifo = 0;

    // We will use the sync word that starts with a 1, so that the receiving PIO can trigger off that to start receiving data.
    pio_sm_restart(pio, sm);
    pio_sm_exec(pio, sm, pio_encode_jmp(offset));
    pio_sm_exec(pio, sm, pio_encode_wait_gpio(1, IN0-16));
    pio_sm_clear_fifos(pio, sm);
    pio_sm_set_enabled(pio, sm, true);

    uint recv_ch = dma_claim_unused_channel(true);
    dma_channel_config c;
    c = dma_channel_get_default_config(recv_ch);
    channel_config_set_dreq(&c, DREQ_PIO0_RX0);
    channel_config_set_read_increment(&c, false);
    channel_config_set_write_increment(&c, true);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_16);
    dma_channel_configure(
        recv_ch,
        &c,
        data_out,
        &pio->rxf[sm],
        len+2,
        true
    );

    uint dma_ch = dma_claim_unused_channel(true);
    c = dma_channel_get_default_config(dma_ch);
    channel_config_set_dreq(&c, DREQ_HSTX);
    dma_channel_configure(
        dma_ch,
        &c,
        &hstx_fifo_hw->fifo,
        encoded,
        len+2,
        true
    );

    dma_channel_wait_for_finish_blocking(dma_ch);
    dma_channel_unclaim(dma_ch);

    while (!(hstx_fifo_hw->stat & HSTX_FIFO_STAT_EMPTY_BITS))
		;

    sleep_us(100);
    hstx_ctrl_hw->csr &= ~HSTX_CTRL_CSR_EN_BITS;

    dma_channel_unclaim(recv_ch);
    pio_sm_set_enabled(pio, sm, false);

    gpio_set_function(CLK, GPIO_FUNC_SIO);
}
