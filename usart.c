#include "usart.h"

#include <avr/io.h>


void U0_config(int8_t rxen, int8_t txen, int8_t mode, int8_t parity,
        int8_t stop, int8_t size, int16_t baud_rate)
{
    uint8_t b = UCSR0B;
    uint8_t c = UCSR0C;
    if (rxen >= 0)
        b = (b & ~(1<<RXEN0)) | (uint8_t)rxen<<RXEN0;
    if (txen >= 0)
        b = (b & ~(1<<TXEN0)) | (uint8_t)txen<<TXEN0;
    if (mode >= 0)
        c = (c & 0b00111111) | (uint8_t)mode<<6;
    if (parity >= 0)
        c = (c & 0b11001111) | (uint8_t)parity<<4;
    if (stop >= 0)
        c = (c & ~(1<<USBS0)) | (uint8_t)stop<<USBS0;
    if (size >= 0) {
        b = (b & ~(1<<UCSZ02)) | (size & 0b100)<<UCSZ02;
        c = (c & 0b11111001) | (size & 0b11)<<1;
    };
    UCSR0B = b;
    UCSR0C = c;
    if (baud_rate >= 0) {
        UBRR0H = (uint16_t)baud_rate>>8;
        UBRR0L = (uint16_t)baud_rate & 0xFF;
    };
}


void U0_ie_config(int8_t rxcie, int8_t txcie, int8_t udrie)
{
    uint8_t b = UCSR0B;
    if (rxcie >= 0)
        b = (b & ~(1<<RXCIE0)) | (uint8_t)rxcie<<RXCIE0;
    if (txcie >= 0)
        b = (b & ~(1<<TXCIE0)) | (uint8_t)txcie<<TXCIE0;
    if (udrie >= 0)
        b = (b & ~(1<<UDRIE0)) | (uint8_t)udrie<<UDRIE0;
    UCSR0B = b;
}
