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


// All keys, commands, and data are LSb first.


uint8_t config_data[] = {
    0, // start
    1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0, 1, 0,
    0, // stop
};


uint8_t send_buf[2];
unsigned int send_idx = lengthof(send_buf);


struct pic_tf buffer[20];


enum {
    PROGRAM_CONFIG,
    READ_CONFIG,
} phase = PROGRAM_CONFIG;


struct pic_tf* next_phase()
{
    static unsigned int count;

    if (phase == PROGRAM_CONFIG) {
        phase = READ_CONFIG;
        count = 0;

        struct pic_tf* buf = buffer;
        buf = pic_read_data(buf, false);
        pic_inc_addr(buf, true);
    } else if (phase == READ_CONFIG) {
        ++count;
        if (count > 0x00A)
            return NULL; // no more phases
    }
    return buffer;
}


ISR(USART0_UDRE_vect)
{
    UDR0 = send_buf[send_idx];
    if (++send_idx >= lengthof(send_buf))
        U0_ie_config(-1, -1, 0);
}


bool process_word(uint16_t word)
{
    if (send_idx < lengthof(send_buf))
        return false;
    send_buf[0] = word >> 8;
    send_buf[1] = word & 0xFF;
    send_idx = 0;
    U0_ie_config(-1, -1, 1);
    return true;
}


ISR(TIMER3_COMPA_vect)
{
    if (!pic_step(next_phase, process_word))
        T3_config(-1, cs_none);
}


int main()
{
    U0_config(0, 1, umode_async, upar_none, ustop_1, usize_8, 103);
    // 9600 baud
    U0_ie_config(1, 0, 0);

    bset(DDRB, 1<<7);
    bclr(PORTB, 1<<7);

    T3_config(wgm_ctc_ocr, cs_none);
    T3A_config(com_dc, 1);
    OCR3A = 40; // 50 kHz (period 20 us)

    {
        struct pic_tf* buf = buffer;

        buf = pic_enter_lvp(buf, false);

        buf = pic_load_config(buf, false, config_data); // data is irrelevant
        buf = pic_bulk_erase(buf, false);

        buf = pic_load_data(buf, false, config_data);
        buf = pic_inc_addr(buf, false);

        buf = pic_load_data(buf, false, config_data);
        buf = pic_inc_addr(buf, false);

        buf = pic_load_data(buf, false, config_data);
        buf = pic_inc_addr(buf, false);

        buf = pic_load_data(buf, false, config_data);

        buf = pic_int_timed_prgm(buf, false);

        buf = pic_load_config(buf, true, config_data);
    }

    pic_init(buffer);

    TCNT3 = 0;
    T3_config(-1, cs_clkio_8);
    sei();

    spin();
}
