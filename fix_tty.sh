#!/usr/bin/env bash

set -o nounset

stty -F $1 cs8 9600 ignbrk -brkint -icrnl -imaxbel -opost -onlcr -isig \
	-icanon -iexten -echo -echoe -echok -echoctl -echoke noflsh -ixon -crtscts
