#pragma once

#include <stdint.h>

uint64_t xorshft64         (uint64_t &state);
uint64_t xorshft64_getState(uint64_t  output);
void     xorshft64_skip    (uint64_t &state, int64_t count);

uint64_t xorshft128         (uint64_t state[2]);
int      xorshft128_getState(uint64_t state[2], uint64_t leastSignificantBitsOutput[2], int showWork = 0, int saveFrames = 0);
void     xorshft128_undo    (uint64_t state[2]);
