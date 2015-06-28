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
uint8_t uart_buf[3];
uint8_t uart_idx;

enum {
    C_DATA = 0,
    C_CONFIG = 1,
    C_RUN = 2,
    // 3 is reserved
} cmd;

uint16_t prgm_word;
// start bit, data (LSb first), stop bit
uint8_t prgm_buf[16] = {0};


struct pic_tf tfs[50];


uint8_t sync = 0xEA;


enum {
    PH_START,
    PH_HANDSHAKE_SEND,
    PH_HANDSHAKE_RECV,
    PH_PREP,
    PH_READY,
    PH_CMD_RECV,
    PH_CMD_EXEC,
    PH_DATA_SEND,
    PH_DIE,
} phase = PH_START;


void die()
{
    cli();
    disable_timer();
    U0_config(0, 0, -1, -1, -1, -1, -1);
    spin();
}


void next_phase()
{
    // clean up and advance

    if (phase == PH_START) {
        phase = PH_HANDSHAKE_SEND;
    } else if (phase == PH_HANDSHAKE_SEND) {
        U0_ie_config(-1, -1, 0);
        phase = PH_HANDSHAKE_RECV;
    } else if (phase == PH_HANDSHAKE_RECV) {
        U0_ie_config(0, -1, -1);
        phase = PH_PREP;
    } else if (phase == PH_PREP) {
        disable_timer();
        phase = PH_READY;
    } else if (phase == PH_READY) {
        U0_ie_config(-1, -1, 0);
        phase = PH_CMD_RECV;
    } else if (phase == PH_CMD_RECV) {
        U0_ie_config(0, -1, -1);
        phase = PH_CMD_EXEC;
    } else if (phase == PH_CMD_EXEC) {
        disable_timer();
        if (cmd == C_RUN) {
            phase = PH_DIE;
        } else {
            phase = PH_DATA_SEND;
        }
    } else if (phase == PH_DATA_SEND) {
        phase = PH_READY;
    }

    // initialize

    if (phase == PH_HANDSHAKE_SEND) {
        U0_ie_config(-1, -1, 1);
    } else if (phase == PH_HANDSHAKE_RECV) {
        U0_ie_config(1, -1, -1);
    } else if (phase == PH_PREP) {
        struct pic_tf* t = tfs;
        t = pic_enter_lvp(t, false);
        t = pic_load_config(t, false, prgm_buf);
        t = pic_bulk_erase(t, false);
        t = pic_reset_addr(t, true);

        pic_init(tfs);
        enable_timer();
    } else if (phase == PH_READY) {
        U0_ie_config(-1, -1, 1);
        bset(PORTB, 1<<7);
    } else if (phase == PH_CMD_RECV) {
        uart_idx = 0;
        U0_ie_config(1, -1, -1);
    } else if (phase == PH_CMD_EXEC) {
        if (cmd == C_DATA || cmd == C_CONFIG) {
            uint16_t word = (uart_buf[0] << 8) | uart_buf[1];
            prgm_word = word;
            for (uint8_t i = 1; i <= 14; ++i) {
                prgm_buf[i] = word & 1;
                word >>= 1;
            }
        }

        struct pic_tf* t = tfs;
        if (cmd == C_DATA) {
            t = pic_load_data(t, false, prgm_buf);
            t = pic_int_timed_prgm(t, false);
            t = pic_read_data(t, false);
            t = pic_inc_addr(t, true);
        } else if (cmd == C_CONFIG) {
            t = pic_load_config(t, false, prgm_buf);
            for (uint8_t i = 0; i < uart_buf[2]; ++i)
                t = pic_inc_addr(t, false);
            t = pic_int_timed_prgm(t, false);
            t = pic_read_data(t, true);
        } else if (cmd == C_RUN) {
            t = pic_run(t, true);
        } else {
            // illegal command
            die();
        }

        pic_init(tfs);
        enable_timer();
    } else if (phase == PH_DATA_SEND) {
        uart_idx = 0;
        U0_ie_config(-1, -1, 1);
    } else if (phase == PH_DIE) {
        die();
    }
}


ISR(USART0_RX_vect)
{
    if (phase == PH_HANDSHAKE_RECV) {
        if (UDR0 != 0xB4) {
            die();
        }
        next_phase();
    } else if (phase == PH_CMD_RECV) {
        uart_buf[uart_idx] = UDR0;
        if (uart_idx == 0) {
            cmd = uart_buf[0] >> 6;
            uart_buf[0] &= 0x3F;
        }

        ++uart_idx;
        if ((cmd == C_DATA && uart_idx >= 2) ||
                (cmd == C_CONFIG && uart_idx >= 3) ||
                (cmd == C_RUN))
            next_phase();
    } else {
        die();
    }
}


ISR(USART0_UDRE_vect)
{
    if (phase == PH_HANDSHAKE_SEND) {
        UDR0 = 0xA4;
        next_phase();
    } else if (phase == PH_READY) {
        bclr(PORTB, 1<<7);
        UDR0 = sync++;
        next_phase();
    } else if (phase == PH_DATA_SEND) {
        UDR0 = uart_buf[uart_idx];
        if (++uart_idx >= 2)
            next_phase();
    } else {
        die();
    }
}


bool process_word(uint16_t word)
{
    uart_buf[0] = word >> 8;
    uart_buf[1] = word & 0xFF;
    return true;
}


ISR(TIMER3_COMPA_vect)
{
    if (!pic_step(process_word))
        next_phase();
}


int main()
{
    U0_ie_config(0, 0, 0);
    U0_config(0, 0, -1, -1, -1, -1, -1);
    U0_config(1, 1, umode_async, upar_none, ustop_1, usize_8, 103);
    // 9600 baud

    bset(DDRB, 1<<7);
    bclr(PORTB, 1<<7);

    T3_config(wgm_ctc_ocr, cs_none);
    T3A_config(com_dc, 1);
    OCR3A = 40; // 50 kHz (period 20 us)

    next_phase();

    sei();

    spin();
}
