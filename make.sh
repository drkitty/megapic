#!/usr/bin/env bash

set -o errexit

CFLAGS="-std=c99 -g -Wall -Wextra -Werror=implicit-function-declaration"

avr-gcc $CFLAGS -mmcu=avr6 -mmcu=atmega2560 -Os -DF_CPU=16000000L \
	-o get_dev_id timer.c utils.c usart.c main.c
avr-objcopy -O binary get_dev_id get_dev_id.bin
