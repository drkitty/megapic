#!/usr/bin/env python3


import activate
activate.activate()


from sys import stdout
from time import sleep

import serial


DEBUG = True


def bb(*args):
    return bytes(args)


program = (
    (0x00, 0x21),
    (0x11, 0x0E),
    (0x00, 0x22),
    (0x15, 0x0E),
    (0x33, 0xFF),
)


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

#for b in program:
    #r(1)
    #w(bytes(b))
    #r(2)

for x in range(4):
    r(1)
    w(bb(0x39, 0x1B))
    d()
    r(2)

r(1)
w(bb(0x00, 0x06))  # 0000 0000 0000 0110
d()
r(2)

for x in range(4):
    r(1)
    w(bb(0x39, 0x1B))
    d()
    r(2)

r(1)
w(bb(0x80))
