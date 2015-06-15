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


// All keys, commands, and data are LSb first.

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

unsigned int read_data_data[16] = {0};


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
        .post_delay = 1, // 20 us (>100 ns)
    },

    // reset
    {
        .clr = 1<<MCLR_N,
        .len = 0,
        .post_delay = 20, // 400 us (>250 us)
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
        .post_delay = 1, // 20 us (>1 us)
    },

    // load configuration (data)
    {
        .len = lengthof(config_data),
        .data = config_data,
        .post_delay = 1, // 20 us (>1 us)
    },
};

struct state phase_read_config[] = {
    // read data from program memory (command)
    {
        .len = lengthof(read_data_cmd),
        .data = read_data_cmd,
        .post_delay = 1, // 20 us (>1 us)
    },

    // read data from program memory (data)
    {
        .in = true,
        .len = lengthof(read_data_data),
        .data = read_data_data,
        .post_delay = 1, // 20 us (>1 us)
    },

    // increment address
    {
        .len = lengthof(inc_addr_cmd),
        .data = inc_addr_cmd,
        .post_delay = 1, // 20 us (>1 us)
    },
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
        .len = lengthof(phase_read_config),
        .states = phase_read_config,
        .times = 11, // 0x8000 to 0x800A
    },
};



unsigned int phase_idx = 0;

uint8_t send_buf[2];
unsigned int send_idx = lengthof(send_buf);


ISR(USART0_UDRE_vect)
{
    UDR0 = send_buf[send_idx];
    if (++send_idx == lengthof(send_buf))
        U0_ie_config(-1, -1, 0);
}


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
            // delay if USART data is still pending
            if (send_idx < lengthof(send_buf))
                return;

            // falling edge
            bclr(PORTA, 1<<CLK);
            if (state->in) {
                state->data[state->d] = (PINA & 1<<DAT) ? 1 : 0;
                if (state->d == 15) {
                    send_buf[0] = 0;
                    for (int i = 14; i >= 9; --i)
                        send_buf[0] = (send_buf[0] << 1) | state->data[i];
                    for (int i = 8; i >= 1; --i)
                        send_buf[1] = (send_buf[1] << 1) | state->data[i];
                    send_idx = 0;
                    U0_ie_config(-1, -1, 1);
                }
            }
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

    U0_config(0, 1, umode_async, upar_none, ustop_1, usize_8, 103);
    // 9600 baud
    U0_ie_config(0, 0, 0);

    T3_config(wgm_ctc_ocr, cs_none);
    T3A_config(com_dc, 1);
    OCR3A = 40; // 50 kHz (period 20 us)

    TCNT3 = 0;
    T3_config(-1, cs_clkio_8);
    sei();

    spin();
}
