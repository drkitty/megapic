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


volatile enum {
    INIT,
    RUN,
    RESET,
    LVP_KEY_DAT,
    LVP_KEY_CLK,
    ERASE,
    DATA_LOAD_DAT,
    DATA_LOAD_CLK,
    DATA_WRITE,
    DATA_
} state = INIT;


int counter;

int key_idx = 0;
int key[] = {
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


ISR(USART0_RX_vect)
{
    uint8_t c = UDR0;
    (void)c;
}


ISR(TIMER3_COMPA_vect)
{
    if (state == INIT) {
        state = RUN;
        counter = 0;
    } else if (state == RUN && ++counter == 2) {
        state = RESET;
        counter = 0;
    } else if (state == RESET && ++counter == 2) {
        state = LVP_KEY_DAT;
    } else if (state == LVP_KEY_DAT) {
        state = LVP_KEY_CLK;
    } else if (state == LVP_KEY_CLK) {
        if (key_idx == lengthof(key))
            state = ERASE;
        else
            state = LVP_KEY_DAT;
    } else {
        return;
    }

    //
    // Handle new states.
    //

    if (state == RUN) {
        TCNT1 = 0;
        T1C_FOC();
        T1_config(-1, cs_clkio_1024);
    } else if (state == RESET) {
        bclr(PORTA, 1<<MCLR_N);
    } else if (state == LVP_KEY_DAT) {
        bclror(PORTA, 1<<DAT, 1<<CLK | key[key_idx]<<DAT);
    } else if (state == LVP_KEY_CLK) {
        bclr(PORTA, 1<<CLK);
        ++key_idx;
    }
}


int main()
{
    bset(DDRA, 1<<MCLR_N | 1<<DAT | 1<<CLK);
    bset(DDRB, 1<<DDB7);

    bclror(PORTA, 1<<MCLR_N | 1<<DAT | 1<<CLK, 1<<MCLR_N);
    /*bclr(PORTB, 1<<7);*/

    /*U0_ie_config(1, 0, 0);*/
    /*U0_config(1, 1, umode_async, upar_none, ustop_1, usize_8, 103);*/
    // 9600 baud

    T1_config(wgm_ctc_ocr, cs_none);
    T1C_config(com_toggle, 0);
    OCR1A = 7813; // 2 Hz

    T3_config(wgm_ctc_ocr, cs_none);
    T3A_config(com_dc, 1);
    OCR3A = 15625; // 1 Hz

    TCNT3 = 0;
    T3_config(-1, cs_clkio_1024);
    sei();

    spin();
}
