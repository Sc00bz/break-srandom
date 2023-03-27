#pragma once

#include <stdint.h>

const int SRANDOM_VERSION_NORM_ARRAY_BUG = 0;
const int SRANDOM_VERSION_NORM           = 1;
const int SRANDOM_VERSION_UHS_ARRAY_BUG  = 2;
const int SRANDOM_VERSION_UHS            = 3;

void   srandom_reset();
size_t srandom_read(void *buffer, size_t bufferSize, int version);

// For breaking
size_t srandom_read(void *buffer, size_t bufferSize, uint64_t *prngArrays, uint64_t &xorshft64_state, uint64_t xorshft128_state[2], int &arraysBufferPosition, int version);

// Private but used for breaking
int    nextbuffer       (uint64_t *prngArray, uint64_t numPrngArrays, uint64_t &xorshft64_state, uint64_t xorshft128_state[2], int &arraysBufferPosition);
void   update_sarray    (uint64_t *prngArray, uint64_t &xorshft64_state, uint64_t xorshft128_state[2]);
void   update_sarray_uhs(uint64_t *prngArray, uint64_t &xorshft64_state);
