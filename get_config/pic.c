#include "pic.h"

#include "timer.h"
#include "utils.h"

#include <avr/io.h>

#include <stdlib.h>


#define MCLR_N 0
#define DAT 1
#define CLK 2


// All keys, commands, and data are LSb first.

uint8_t lvp_key[] = {
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

uint8_t config_cmd[] = {
    0, 0, 0, 0, 0, 0,
};

uint8_t inc_addr_cmd[] = {
    0, 1, 1, 0, 0, 0,
};

uint8_t read_data_cmd[] = {
    0, 0, 1, 0, 0, 0,
};


uint8_t read_data_data[16];


static struct pic_tf* tf = NULL;


void tf_init(struct pic_tf* tf)
{
    bclror(DDRA, 1<<DAT, (tf->in ? 0 : 1)<<DAT);
    bclror(PORTB, 1<<7, (tf->in ? 0 : 1)<<7);
    bclror(PORTA, 1<<MCLR_N, tf->mclr_n<<MCLR_N);
}


void pic_init(struct pic_tf* buffer)
{
    tf = buffer;
    bset(DDRA, 1<<MCLR_N | 1<<DAT | 1<<CLK);
    tf_init(tf);
}


bool pic_step(struct pic_tf* (* const next_phase)(),
        bool (* const process_word)(uint16_t data))
{
    enum {
        WAITING = -1,
        DONE = -2,
    };
    static unsigned int b = 0;
    static int16_t word = WAITING;

    if (b < tf->len) {
        //
        // send or receive data
        //

        if (PORTA & 1<<CLK) {
            // falling edge
            bclr(PORTA, 1<<CLK);
            if (tf->in)
                tf->data[b] = (PINA & 1<<DAT) ? 1 : 0;
            ++b;
        } else {
            // rising edge
            if (!tf->in)
                bclror(PORTA, 1<<DAT, tf->data[b]<<DAT);
            bset(PORTA, 1<<CLK);
        }
    } else if (b < tf->len + tf->post_delay) {
        //
        // delay
        //

        ++b;
    }

    if (tf->in && b >= tf->len) {
        if (word == WAITING) {
            //
            // assemble word
            //

            word = 0;
            for (int i = 14; i >= 1; --i)
                word = (word << 1) | tf->data[i];
        }

        // try to process word
        if (word >= 0 && process_word(word))
            word = DONE;
    }

    if (b >= tf->len + tf->post_delay && (!tf->in || word == DONE)) {
        //
        // advance
        //

        if (tf->final) {
            tf = next_phase();
            if (tf == NULL)
                return false;
        } else {
            ++tf;
        }

        b = 0;
        word = WAITING;
        tf_init(tf);
    }

    return true;
}


struct pic_tf* pic_enter_lvp(struct pic_tf* tf, bool final)
{
    // run
    *(tf++) = (struct pic_tf){
        .mclr_n = 1,
        .post_delay = 1, // 20 us (>100 ns)
    };

    // reset
    *(tf++) = (struct pic_tf){
        .post_delay = 20, // 400 us (>250 us)
    };

    // send LVP key
    *(tf++) = (struct pic_tf){
        .len = lengthof(lvp_key),
        .data = lvp_key,
        .final = final,
    };

    return tf;
}


struct pic_tf* pic_load_config(struct pic_tf* tf, const bool final,
        uint8_t* const config_data)
{
    // load configuration (command)
    *(tf++) = (struct pic_tf){
        .len = lengthof(config_cmd),
        .data = config_cmd,
        .post_delay = 1, // 20 us (>1 us)
    };

    // load configuration (data)
    *(tf++) = (struct pic_tf){
        .len = 16,
        .data = config_data,
        .post_delay = 1, // 20 us (>1 us)
        .final = final,
    };

    return tf;
}


struct pic_tf* pic_read_data(struct pic_tf* tf, const bool final)
{
    // read data from program memory (command)
    *(tf++) = (struct pic_tf){
        .len = lengthof(read_data_cmd),
        .data = read_data_cmd,
        .post_delay = 1, // 20 us (>1 us)
    };

    // read data from program memory (data)
    *(tf++) = (struct pic_tf){
        .in = true,
        .len = lengthof(read_data_data),
        .data = read_data_data,
        .post_delay = 1, // 20 us (>1 us)
        .final = final,
    };

    return tf;
}


struct pic_tf* pic_inc_addr(struct pic_tf* tf, const bool final)
{
    // increment address
    *(tf++) = (struct pic_tf){
        .len = lengthof(inc_addr_cmd),
        .data = inc_addr_cmd,
        .post_delay = 1, // 20 us (>1 us)
        .final = final,
    };

    return tf;
}
