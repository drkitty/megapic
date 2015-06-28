#include "pic.h"
#include "timer.h"
#include "usart.h"
#include "utils.h"

#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


#define enable_timer() do { T3_config(-1, cs_clkio_8); } while (false)
#define disable_timer() do { T3_config(-1, cs_none); TCNT3 = 0; } while (false)


// MSB first (network byte order)
uint8_t recv_buf[2 * 16];
uint8_t recv_idx;

// LSb first with "start" and "stop" bits
uint8_t prgm_buf[16 * 16];
uint8_t prgm_len; // in words
uint8_t prgm_pos; // in words


uint8_t zeroes[16];


struct pic_tf tfs[50];


enum {
    PH_HANDSHAKE_SEND,
    PH_HANDSHAKE_RECV,
    PH_PREP,
    PH_RECV_LEN,
    PH_RECV_DATA,
    PH_WRITE_DATA,
    PH_ACK_DATA,
    PH_RUN,
} phase = PH_HANDSHAKE_SEND;


void die()
{
    disable_timer();
    U0_config(0, 0, -1, -1, -1, -1, -1);
    spin();
}


void next_phase()
{
    // clean up and advance

    if (phase == PH_HANDSHAKE_SEND) {
        U0_ie_config(-1, -1, 0);
        phase = PH_HANDSHAKE_RECV;
    } else if (phase == PH_HANDSHAKE_RECV) {
        U0_ie_config(0, -1, -1);
        phase = PH_PREP;
    } else if (phase == PH_PREP) {
        disable_timer();
        phase = PH_RECV_LEN;
    } else if (phase == PH_RECV_LEN) {
        if (prgm_len == 0)
            phase = PH_RUN;
        else if (prgm_len > 16)
            spin();
        else
            phase = PH_RECV_DATA;
    } else if (phase == PH_RECV_DATA) {
        U0_ie_config(0, -1, -1);

        for (recv_idx = 0; recv_idx < prgm_len * 2; recv_idx += 2) {
            // MSB first
            uint16_t word = recv_buf[recv_idx] << 8 | recv_buf[recv_idx + 1];
            uint16_t p = prgm_pos * 16;
            prgm_buf[p++] = 0;
            for (uint8_t i = 0; i < 14; ++i) {
                prgm_buf[p++] = word & 1;
                word >>= 1;
            }
            prgm_buf[p++] = 0;
        }

        phase = PH_WRITE_DATA;
    } else if (phase == PH_WRITE_DATA) {
        disable_timer();
        phase = PH_ACK_DATA;
    } else if (phase == PH_ACK_DATA) {
        U0_ie_config(-1, -1, 0);
        phase = PH_RECV_LEN;
    } else if (phase == PH_RUN) {
        die();
    }

    // initialize

    if (phase == PH_HANDSHAKE_RECV) {
        U0_ie_config(1, -1, -1);
    } else if (phase == PH_PREP) {
        struct pic_tf* t = tfs;
        t = pic_enter_lvp(t, false);
        pic_bulk_erase(t, true);

        pic_init(tfs);
        enable_timer();
    } else if (phase == PH_RECV_LEN) {
        U0_ie_config(1, -1, -1);
    } else if (phase == PH_RECV_DATA) {
        recv_idx = 0;
        prgm_pos = 0;
    } else if (phase == PH_WRITE_DATA) {
        prgm_pos = 0;

        struct pic_tf* t = tfs;
        for (uint16_t i = 0; i < prgm_len * 16; i += 16) {
            t = pic_load_data(t, false, &prgm_buf[i]);
            t = pic_inc_addr(t, false);
        }
        pic_int_timed_prgm(t, true);

        pic_init(tfs);
        enable_timer();
    } else if (phase == PH_ACK_DATA) {
        U0_ie_config(-1, -1, 1);
    } else if (phase == PH_RUN) {
        pic_run(tfs, true);

        pic_init(tfs);
        enable_timer();
        bset(PORTB, 1<<7);
    }
}


ISR(USART0_RX_vect)
{
    if (phase == PH_HANDSHAKE_RECV) {
        if (UDR0 != 0xB4) {
            die();
        }
        next_phase();
    } else if (phase == PH_RECV_LEN) {
        prgm_len = UDR0;
        next_phase();
    } else if (phase == PH_RECV_DATA) {
        recv_buf[recv_idx] = UDR0;
        while ((UCSR0A & 1<<5) == 0)
            __asm("");
        UDR0 = recv_idx;
        if (++recv_idx >= prgm_len * 2)
            next_phase();
    }
}


ISR(USART0_UDRE_vect)
{
    if (phase == PH_HANDSHAKE_SEND) {
        UDR0 = 0xA4;
        next_phase();
    } else if (phase == PH_ACK_DATA) {
        UDR0 = 0xFE;
        next_phase();
    }
}


bool process_word(uint16_t word)
{
    (void)word;
    return true;
}


ISR(TIMER3_COMPA_vect)
{
    if (!pic_step(process_word))
        next_phase();
}


int main()
{
    U0_ie_config(0, 0, 1);
    U0_config(0, 0, -1, -1, -1, -1, -1);
    U0_config(1, 1, umode_async, upar_none, ustop_1, usize_8, 103);
    // 9600 baud

    bset(DDRB, 1<<7);
    bclr(PORTB, 1<<7);

    T3_config(wgm_ctc_ocr, cs_none);
    T3A_config(com_dc, 1);
    OCR3A = 40; // 50 kHz (period 20 us)

    sei();

    spin();
}
