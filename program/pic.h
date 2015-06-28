#pragma once


#include <stdbool.h>
#include <stdint.h>


struct pic_tf {
    uint8_t mclr_n;

    bool in;
    unsigned int len;
    uint8_t* data;

    unsigned int post_delay;
};


void pic_init(struct pic_tf* buffer, struct pic_tf* end);

// returns whether to call pic_step again
//
// next_phase:
//   returns the next transfer, or NULL if none remain
// process_word:
//   returns false if the word could not be processed yet
bool pic_step(bool (* const process_word)(uint16_t data));

struct pic_tf* pic_run(struct pic_tf* tf);

struct pic_tf* pic_enter_lvp(struct pic_tf* tf);

struct pic_tf* pic_load_config(struct pic_tf* tf, uint8_t* const data);

struct pic_tf* pic_load_data(struct pic_tf* tf, uint8_t* const data);

struct pic_tf* pic_read_data(struct pic_tf* tf);

struct pic_tf* pic_inc_addr(struct pic_tf* tf);

struct pic_tf* pic_reset_addr(struct pic_tf* tf);

struct pic_tf* pic_int_timed_prgm(struct pic_tf* tf);

struct pic_tf* pic_bulk_erase(struct pic_tf* tf);

struct pic_tf* pic_row_erase(struct pic_tf* tf);
