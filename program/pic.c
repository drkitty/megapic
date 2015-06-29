#include "pic.h"

#include "timer.h"
#include "utils.h"

#include <avr/io.h>

#include <stdlib.h>


#define MCLR_N 0
#define DAT 1
#define CLK 2


#define T_ENTS 1 // 20 us (>100 ns)
#define T_ENTH 25 // 500 us (>250 us)
#define T_DLY 1 // 20 us (>1 us)
#define T_PINT 500 // 10 ms (>5 ms)
#define T_ERAB 500 // 10 ms (>5 ms)
#define T_ERAR 250 // 5 ms (>2.5 ms)


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

uint8_t load_config_cmd[] = {
    0, 0, 0, 0, 0, 0,
};

uint8_t load_data_cmd[] = {
    0, 1, 0, 0, 0, 0,
};

uint8_t read_data_cmd[] = {
    0, 0, 1, 0, 0, 0,
};

uint8_t inc_addr_cmd[] = {
    0, 1, 1, 0, 0, 0,
};

uint8_t reset_addr_cmd[] = {
    0, 1, 1, 0, 1, 0,
};

uint8_t int_timed_prgm_cmd[] = {
    0, 0, 0, 1, 0, 0,
};

uint8_t bulk_erase_cmd[] = {
    1, 0, 0, 1, 0, 0,
};

uint8_t row_erase_cmd[] = {
    1, 0, 0, 0, 1, 0,
};


uint8_t read_data_data[16];


static struct pic_tf* tf = NULL;
static struct pic_tf* end = NULL;


void tf_init(struct pic_tf* tf)
{
    bclror(DDRA, 1<<DAT, (tf->in ? 0 : 1)<<DAT);
    bclror(PORTA, 1<<MCLR_N, tf->mclr_n<<MCLR_N);
}


void pic_init(struct pic_tf* buffer, struct pic_tf* end_)
{
    tf = buffer;
    end = end_;
    bset(DDRA, 1<<MCLR_N | 1<<DAT | 1<<CLK);
    tf_init(tf);
}


bool pic_step(bool (* const process_word)(uint16_t data),
        void (* const debug)(uint8_t val))
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

        if (debug != NULL)
            debug(PORTA);
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

        ++tf;

        if (tf >= end) {
            if (debug != NULL)
                debug(0xFF);
            return false;
        }

        b = 0;
        word = WAITING;
        tf_init(tf);
    }

    return true;
}


struct pic_tf* pic_run(struct pic_tf* tf)
{
    *(tf++) = (struct pic_tf){
        .mclr_n = 1,
        .post_delay = T_ENTS,
    };

    return tf;
}


struct pic_tf* pic_enter_lvp(struct pic_tf* tf)
{
    // run
    tf = pic_run(tf);

    // reset
    *(tf++) = (struct pic_tf){
        .post_delay = T_ENTH,
    };

    // send LVP key
    *(tf++) = (struct pic_tf){
        .len = lengthof(lvp_key),
        .data = lvp_key,
        .post_delay = T_DLY,
    };

    return tf;
}


// Load configuration
struct pic_tf* pic_load_config(struct pic_tf* tf, uint8_t* const data)
{
    *(tf++) = (struct pic_tf){
        .len = lengthof(load_config_cmd),
        .data = load_config_cmd,
        .post_delay = T_DLY,
    };

    *(tf++) = (struct pic_tf){
        .len = 16,
        .data = data,
        .post_delay = T_DLY,
    };

    return tf;
}


// Load data for program memory
struct pic_tf* pic_load_data(struct pic_tf* tf, uint8_t* const data)
{
    *(tf++) = (struct pic_tf){
        .len = lengthof(load_data_cmd),
        .data = load_data_cmd,
        .post_delay = T_DLY,
    };

    *(tf++) = (struct pic_tf){
        .len = 16,
        .data = data,
        .post_delay = T_DLY,
    };

    return tf;
}


// Read data from program memory
struct pic_tf* pic_read_data(struct pic_tf* tf)
{
    *(tf++) = (struct pic_tf){
        .len = lengthof(read_data_cmd),
        .data = read_data_cmd,
        .post_delay = T_DLY,
    };

    *(tf++) = (struct pic_tf){
        .in = true,
        .len = lengthof(read_data_data),
        .data = read_data_data,
        .post_delay = T_DLY,
    };

    return tf;
}


// Increment address
struct pic_tf* pic_inc_addr(struct pic_tf* tf)
{
    *(tf++) = (struct pic_tf){
        .len = lengthof(inc_addr_cmd),
        .data = inc_addr_cmd,
        .post_delay = T_DLY,
    };

    return tf;
}


// Reset address
struct pic_tf* pic_reset_addr(struct pic_tf* tf)
{
    *(tf++) = (struct pic_tf){
        .len = lengthof(reset_addr_cmd),
        .data = reset_addr_cmd,
        .post_delay = T_DLY,
    };

    return tf;
}


// Begin internally timed programming
struct pic_tf* pic_int_timed_prgm(struct pic_tf* tf)
{
    *(tf++) = (struct pic_tf){
        .len = lengthof(int_timed_prgm_cmd),
        .data = int_timed_prgm_cmd,
        .post_delay = T_PINT,
    };

    return tf;
}


// Bulk erase program memory
struct pic_tf* pic_bulk_erase(struct pic_tf* tf)
{
    *(tf++) = (struct pic_tf){
        .len = lengthof(bulk_erase_cmd),
        .data = bulk_erase_cmd,
        .post_delay = T_ERAB,
    };

    return tf;
}


// Row erase program memory
struct pic_tf* pic_row_erase(struct pic_tf* tf)
{
    *(tf++) = (struct pic_tf){
        .len = lengthof(row_erase_cmd),
        .data = row_erase_cmd,
        .post_delay = T_ERAR,
    };

    return tf;
}
