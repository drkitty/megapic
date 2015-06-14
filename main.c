#include <stdbool.h>
#include <stdint.h>

#include <avr/cpufunc.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#include "timer.h"
#include "usart.h"
#include "utils.h"


ISR(USART0_RX_vect)
{
    uint8_t c = UDR0;
    (void)c;
}


ISR(TIMER3_COMPA_vect)
{
    static int8_t inc = 0x8;
    uint16_t pw = OCR1C + inc;
    if (pw < 0x20 || pw >= 0x100) {
        inc = -inc;
        pw += inc;
    }
    OCR1C = pw;

    PORTA ^= 0x07;
}


int main()
{
    DDRA = 0xFF;
    DDRB |= 1<<DDB7;
    _NOP();

    PORTA = 0x05;

    /*U0_ie_config(1, 0, 0);*/
    /*U0_config(1, 1, umode_async, upar_none, ustop_1, usize_8, 103);*/
    // 9600 baud

    T1_config(wgm_ctc_ocr, cs_none);
    T1C_config(com_toggle, 0);
    OCR1A = 0x3820;

    T3_config(wgm_ctc_ocr, cs_none);
    T3A_config(com_dc, 1);
    OCR3A = 0x1000;

    T1_config(-1, cs_clkio_1024);
    T3_config(-1, cs_clkio_1024);

    sei();
    spin();
}
