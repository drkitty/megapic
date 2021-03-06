#!/usr/bin/env bash

set -o errexit

CFLAGS="-std=c99 -g -Wall -Wextra -Werror"

avr-gcc $CFLAGS -mmcu=avr6 -mmcu=atmega2560 -Os -DF_CPU=16000000L \
	-o program pic.c timer.c utils.c usart.c main.c
avr-objcopy -O binary program program.bin
