#pragma once

#include <stdint.h>


enum {
    com_dc = 0,
    com_toggle = 1,
    com_clear = 2,
    com_set = 3
};

enum {
    wgm0_normal = 0,
    wgm0_pc_pwm_full = 1,
    wgm0_ctc = 2,
    wgm0_fast_pwm_full = 3,
    wgm0_pc_pwm_ocr = 5,
    wgm0_fast_pwm_ocr = 7
};

enum {
    wgm_normal = 0,
    wgm_pc_pwm_8 = 1,
    wgm_pc_pwm_9 = 2,
    wgm_pc_pwm_10 = 3,
    wgm_ctc_ocr = 4,
    wgm_fast_pwm_8 = 5,
    wgm_fast_pwm_9 = 6,
    wgm_fast_pwm_10 = 7,
    wgm_pfc_pwm_icr = 8,
    wgm_pfc_pwm_ocr = 9,
    wgm_pc_pwm_icr = 10,
    wgm_pc_pwm_ocr = 11,
    wgm_ctc_icr = 12,
    wgm_fast_pwm_icr = 14,
    wgm_fast_pwm_ocr = 15
};

enum {
    cs_none = 0,
    cs_clkio = 1,
    cs_clkio_8 = 2,
    cs_clkio_64 = 3,
    cs_clkio_256 = 4,
    cs_clkio_1024 = 5,
    cs_ext_falling = 6,
    cs_ext_rising = 7
};


void T0_config(int8_t wgm, int8_t cs);
void T0A_config(int8_t com, int8_t ie);

void T1_config(int8_t wgm, int8_t cs);
void T1C_config(int8_t com, int8_t ie);

void T3_config(int8_t wgm, int8_t cs);
void T3A_config(int8_t com, int8_t ie);
