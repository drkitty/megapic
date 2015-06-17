#pragma once


#include <stdbool.h>
#include <stdint.h>


struct pic_tf {
    uint8_t mclr_n;

    bool in;
    unsigned int len;
    uint8_t* data;

    unsigned int post_delay;

    bool final;
};


void pic_init(struct pic_tf* buffer);

// returns whether to call pic_step again
//
// next_phase:
//   returns the next transfer, or NULL if none remain
// process_word:
//   returns false if the word could not be processed yet
bool pic_step(struct pic_tf* (* const next_phase)(),
        bool (* const process_word)(uint16_t data));

struct pic_tf* pic_enter_lvp(struct pic_tf* tf, const bool final);

struct pic_tf* pic_load_config(struct pic_tf* tf, const bool final,
        uint8_t* const config_data);

struct pic_tf* pic_read_data(struct pic_tf* tf, const bool final);

struct pic_tf* pic_inc_addr(struct pic_tf* tf, const bool final);
