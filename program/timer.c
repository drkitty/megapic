#include "timer.h"

#include <avr/io.h>


void T0_config(int8_t wgm, int8_t cs)
{
    uint8_t a = TCCR0A;
    uint8_t b = TCCR0B;
    if (wgm >= 0) {
        a &= 0b11111100;
        a |= (uint8_t)wgm & 0b11;
        b &= 0b11110111;
        b |= ((uint8_t)wgm & 0b100) << 1;
    };
    if (cs >= 0) {
        b &= 0b11111000;
        b |= 0b111 & (uint8_t)cs;
    };
    TCCR0A = a;
    TCCR0B = b;
}


void T0A_config(int8_t com, int8_t ie)
{
    uint8_t a = TCCR0A;
    uint8_t m = TIMSK0;
    if (com >= 0) {
        a &= 0b00111111;
        a |= ((uint8_t)com & 0b11) << 6;
    };
    if (ie >= 0) {
        m &= 0b11111101;
        m |= ((uint8_t)ie & 0b1) << 1;
    };
    TCCR0A = a;
    TIMSK0 = m;
}


void T1_config(int8_t wgm, int8_t cs)
{
    uint8_t a = TCCR1A;
    uint8_t b = TCCR1B;
    if (wgm >= 0) {
        a &= 0b11111100;
        a |= (uint8_t)wgm & 0b11;
        b &= 0b11100111;
        b |= ((uint8_t)wgm & 0b1100) << 1;
    };
    if (cs >= 0) {
        b &= 0b11111000;
        b |= 0b111 & (uint8_t)cs;
    };
    TCCR1A = a;
    TCCR1B = b;
}


void T1C_config(int8_t com, int8_t ie)
{
    uint8_t a = TCCR1A;
    uint8_t m = TIMSK1;
    if (com >= 0) {
        a &= 0b11110011;
        a |= ((uint8_t)com & 0b11) << 2;
    };
    if (ie >= 0) {
        m &= 0b11110111;
        m |= ((uint8_t)ie & 0b1) << 3;
    };
    TCCR1A = a;
    TIMSK1 = m;
}


void T3_config(int8_t wgm, int8_t cs)
{
    uint8_t a = TCCR3A;
    uint8_t b = TCCR3B;
    if (wgm >= 0) {
        a &= 0b11111100;
        a |= (uint8_t)wgm & 0b11;
        b &= 0b11100111;
        b |= ((uint8_t)wgm & 0b1100) << 1;
    };
    if (cs >= 0) {
        b &= 0b11111000;
        b |= 0b111 & (uint8_t)cs;
    };
    TCCR3A = a;
    TCCR3B = b;
}


void T3A_config(int8_t com, int8_t ie)
{
    uint8_t a = TCCR3A;
    uint8_t m = TIMSK3;
    if (com >= 0) {
        a &= 0b00111111;
        a |= ((uint8_t)com & 0b11) << 6;
    };
    if (ie >= 0) {
        m &= 0b11111101;
        m |= ((uint8_t)ie & 0b1) << 1;
    };
    TCCR3A = a;
    TIMSK3 = m;
}
