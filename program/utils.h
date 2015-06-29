#pragma once


#define lengthof(x) (sizeof(x) / sizeof(x[0]))

#define bset(x, c) ((x) |= (c))
#define bclr(x, c) ((x) &= ~(c))
#define bclror(x, c, o) ((x) = ((x) & ~(c)) | (o))
#define btst(x, c) ((x) & (c))


void spin();
