#include <stdbool.h>
#include <stdint.h>

#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "timer.h"
#include "usart.h"
#include "utils.h"


#define MCLR_N 0
#define DAT 1
#define CLK 2


unsigned int lvp_key[] = {
    0, 0, 0, 0,
    1, 0, 1, 0,
    0, 0, 0, 1,
    0, 0, 1, 0,
    1, 1, 0, 0,
    0, 0, 1, 0,
    1, 0, 1, 1,
    0, 0, 1, 0,
    0, // don't care
};

unsigned int config_cmd[] = {
    0, 0, 0, 0, 0, 0,
};

unsigned int config_data[] = {
    0, // start
    0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0,
    0, // stop
};

unsigned int inc_addr_cmd[] = {
    0, 1, 1, 0, 0, 0,
};

unsigned int read_data_cmd[] = {
    0, 0, 1, 0, 0, 0,
};


struct state {
    const uint8_t clr;
    const uint8_t set;

    unsigned int d;
    const bool in;
    const unsigned int len;
    unsigned int* const data;
    const unsigned int post_delay;
};


struct state phase_begin[] = {
    // initial
    {
        .len = 0,
    },

    // run
    {
        .set = 1<<MCLR_N,
        .len = 0,
        .post_delay = 4,
    },

    // reset
    {
        .clr = 1<<MCLR_N,
        .len = 0,
        .post_delay = 4,
    },

    // send LVP key
    {
        .len = lengthof(lvp_key),
        .data = lvp_key,
    },

    // load configuration (command)
    {
        .len = lengthof(config_cmd),
        .data = config_cmd,
    },

    // load configuration (data)
    {
        .len = lengthof(config_data),
        .data = config_data,
    },
};

struct state phase_inc_addr[] = {
    // increment address
    {
        .len = lengthof(inc_addr_cmd),
        .data = inc_addr_cmd,
    },
};

struct state phase_read_device_id[] = {
    // read data from program memory
    {
        .in = true,
        .len = lengthof(read_data_cmd),
        .data = read_data_cmd,
    }
};


struct phase {
    unsigned int s;
    const unsigned int len;
    struct state* const states;

    unsigned int i;
    const unsigned int times;
} phases[] = {
    {
        .len = lengthof(phase_begin),
        .states = phase_begin,
        .times = 1,
    },
    {
        .len = lengthof(phase_inc_addr),
        .states = phase_inc_addr,
        .times = 6, // move to address 0x8006
    },
    {
        .len = lengthof(phase_read_device_id),
        .states = phase_read_device_id,
        .times = 1,
    },
};



/*
 *ISR(USART0_RX_vect)
 *{
 *    uint8_t c = UDR0;
 *    (void)c;
 *}
 */


unsigned int phase_idx = 0;


void advance(struct phase* phase, struct state* state)
{
    // advance data index
    ++state->d;

    if (state->d >= state->len) {
        if (state->d >= state->len + state->post_delay) {
            // reinitialize current state
            state->d = 0;
            // advance state
            ++phase->s;

            if (phase->s >= phase->len) {
                // go back to initial state
                phase->s = 0;
                // advance iteration
                ++phase->i;

                if (phase->i >= phase->times) {
                    // reinitialize current phase
                    phase->i = 0;
                    // advance phase
                    ++phase_idx;
                    PORTB ^= 1<<7;

                    if (phase_idx >= lengthof(phases)) {
                        // done
                        T3_config(-1, cs_none);
                        return;
                    }

                    phase = &phases[phase_idx];
                }
            }

            state = &phase->states[phase->s];
            bclror(DDRA, 1<<DAT, (state->in ? 0 : 1)<<DAT);
            bclr(PORTA, state->clr);
            bset(PORTA, state->set);
        }
        // else delay (do nothing)
    }
}


ISR(TIMER3_COMPA_vect)
{
    struct phase* phase = &phases[phase_idx];
    struct state* state = &phase->states[phase->s];

    if (state->d < state->len) {
        // send or receive data
        if (PORTA & 1<<CLK) {
            // falling edge
            bclr(PORTA, 1<<CLK);
            if (state->in)
                state->data[state->d] = PORTA & 1<<DAT ? 1 : 0;
            advance(phase, state);
        } else {
            // rising edge
            bset(PORTA, 1<<CLK);
            if (!state->in)
                bclror(PORTA, 1<<DAT, state->data[state->d]<<DAT);
        }
    } else {
        advance(phase, state);
    }
}


int main()
{
    bset(DDRA, 1<<MCLR_N | 1<<DAT | 1<<CLK);
    bset(DDRB, 1<<7);

    bclror(PORTA, 1<<MCLR_N | 1<<DAT | 1<<CLK, 1<<MCLR_N);

    /*U0_ie_config(1, 0, 0);*/
    /*U0_config(1, 1, umode_async, upar_none, ustop_1, usize_8, 103);*/
    // 9600 baud

    /*
     *T1_config(wgm_ctc_ocr, cs_none);
     *T1C_config(com_toggle, 0);
     *OCR1A = 3906; // rising edge at 2 Hz
     */

    T3_config(wgm_ctc_ocr, cs_none);
    T3A_config(com_dc, 1);
    OCR3A = 7813; // 2 Hz

    TCNT3 = 0;
    T3_config(-1, cs_clkio_1024);
    sei();

    spin();
}
