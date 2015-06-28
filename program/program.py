#!/usr/bin/env python3


import activate
activate.activate()


from time import sleep

import serial


def bb(*args):
    return bytes(args)


program = (
    0x00, 0x21,
    0x11, 0x0E,
    0x00, 0x22,
    0x15, 0x0E,
    0x33, 0xFF,
)


s = serial.Serial('/dev/ttyACM0', baudrate=9600)

def r(*args, **kwargs):
    xx = s.read(*args, **kwargs)
    print('R  ' + ' '.join('{:02x}'.format(c) for c in xx))
    return xx

def w(x, *args, **kwargs):
    xx = s.write(x, *args, **kwargs)
    print('W  ' + ' '.join('{:02x}'.format(c) for c in x))
    return xx

b = r(1)
if b[0] != 0xA4:
    raise Exception('Handshake failed')
w(bb(0xB4))

w(bb(len(program) // 2))
for b in program:
    w(bb(b))
    r(1)

r(1)

w(bb(0))
