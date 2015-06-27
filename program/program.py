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


s = serial.Serial('/dev/ttyACM0', baudrate=9600, timeout=.5)

def r(*args, **kwargs):
    xx = s.read(*args, **kwargs)
    print('R  ' + ' '.join('{:02x}'.format(c) for c in xx))
    return xx

def w(x, *args, **kwargs):
    for b in x:
        s.write(bb(b), *args, **kwargs)
        print('W  ' + '{:02x}'.format(b))
        sleep(.5)

sleep(2)

b = r(1)
if b[0] != 0xA4:
    raise Exception('Handshake failed')
w(bb(0xB4))

w(bb(len(program) // 2))
w(bytes(program))

r(1)

w(bb(0))

r(1)
