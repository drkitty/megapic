#!/usr/bin/env bash

set -o errexit

CFLAGS="-std=c99 -fpic -coverage -g -Wall -Wextra \
-Werror=implicit-function-declaration"

avr-gcc $CFLAGS -mmcu=avr6 -mmcu=atmega2560 -Os -DF_CPU=16000000L -o pic timer.c utils.c usart.c main.c
avr-objcopy -O binary pic pic.bin
