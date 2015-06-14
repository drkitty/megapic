#!/usr/bin/env bash

set -o errexit

avrdude -p m2560 -c stk500v2 -P /dev/ttyACM0 -U flash:w:pic.bin
