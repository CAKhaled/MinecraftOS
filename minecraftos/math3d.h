#ifndef MATH3D_H
#define MATH3D_H

#include <stdint.h>

extern const int16_t sin_table[256];

static inline int16_t get_sin(uint8_t angle) { return sin_table[angle]; }
static inline int16_t get_cos(uint8_t angle) { return sin_table[(uint8_t)(angle + 64)]; }

#endif
