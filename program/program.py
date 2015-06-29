#!/usr/bin/env python3


import activate
activate.activate()


import sys
from sys import stdout
from time import sleep

import serial


DEBUG = False


def bb(*args):
    return bytes(args)


def program(name):
    s = serial.Serial('/dev/ttyACM0', baudrate=9600)

    def r(*args, **kwargs):
        xx = s.read(*args, **kwargs)
        if not DEBUG:
            print('R  ' + ' '.join('{:02x}'.format(c) for c in xx))
        return xx

    def w(x, *args, **kwargs):
        xx = s.write(x, *args, **kwargs)
        if not DEBUG:
            print('W  ' + ' '.join('{:02x}'.format(c) for c in x))
        return xx

    def d(*args, **kwargs):
        if not DEBUG:
            return

        while True:
            (c,) = s.read(1, *args, **kwargs)
            if c == 0xFF:
                return
            elif (c & 0x04) == 0:
                continue
            if DEBUG:
                stdout.write(str(int(c & 0x02 > 0)))
            else:
                print('D  ' + str(int(c & 0x02 > 0)))

    if r(1)[0] != 0xA4:
        raise Exception('Handshake failed')
    w(bb(0xB4))
    d()

    # Parse the INHX8M hex format.
    with open(name) as f:
        for line in f:
            assert line[0] == ':'
            length = int(line[1:3], 16)
            if line[7:9] == '01':
                break
            assert line[7:9] == '00'
            for i in range(length // 2):
                r(1)
                w(bb(
                    int(line[9 + 4*i + 2 : 9 + 4*i + 4], 16),
                    int(line[9 + 4*i : 9 + 4*i + 2], 16),
                ))
                d()
                r(2)

    r(1)
    w(bb(0x80))


if __name__ == '__main__':
    program(sys.argv[1])
