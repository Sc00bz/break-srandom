/*
 * Copyright (C) 2015 Jonathan Senkerik
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
 
#include <stdio.h>
#include <inttypes.h>
#include "srandom.h"
#include "csprng.h"

// srandom 1.41.1

#define PRNG_ARRAY_SIZE_UHS         65 // Must be at least 64. (actual size used will be 64, anything greater is thrown away).
#define PRNG_ARRAY_SIZE_NORM        67 // Must be at least 64. (actual size used will be 64, anything greater is thrown away). Recommended multible of 4.
#define NUMBER_OF_PRNG_ARRAYS_UHS   32 // Number of 512 byte arrays (Must be power of 2)
#define NUMBER_OF_PRNG_ARRAYS_NORM  16 // Number of 512 byte arrays (Must be power of 2)

static uint64_t xorshft64 (uint64_t &state);
static uint64_t xorshft128(uint64_t state[2]);

static uint64_t xorshft64_state[4];
static uint64_t xorshft128_state[8];
static int      arraysBufferPositions[4];

static uint64_t (*prngArraysNorm         )[PRNG_ARRAY_SIZE_NORM          ] = NULL;
static uint64_t (*prngArraysNorm_arrayBug)[NUMBER_OF_PRNG_ARRAYS_NORM + 1] = NULL;
static uint64_t (*prngArraysUhs          )[PRNG_ARRAY_SIZE_UHS           ] = NULL;
static uint64_t (*prngArraysUhs_arrayBug )[NUMBER_OF_PRNG_ARRAYS_UHS  + 1] = NULL;

void srandom_reset()
{
	const size_t PRNG_ARRAYS_SIZE_NORM = (NUMBER_OF_PRNG_ARRAYS_NORM + 1) * PRNG_ARRAY_SIZE_NORM * sizeof(uint64_t);
	const size_t PRNG_ARRAYS_SIZE_UHS  = (NUMBER_OF_PRNG_ARRAYS_UHS  + 1) * PRNG_ARRAY_SIZE_UHS  * sizeof(uint64_t);

	if (prngArraysNorm == NULL)
	{
		prngArraysNorm          = (uint64_t (*)[PRNG_ARRAY_SIZE_NORM          ]) malloc(PRNG_ARRAYS_SIZE_NORM);
		prngArraysNorm_arrayBug = (uint64_t (*)[NUMBER_OF_PRNG_ARRAYS_NORM + 1]) malloc(PRNG_ARRAYS_SIZE_NORM);
		prngArraysUhs           = (uint64_t (*)[PRNG_ARRAY_SIZE_UHS           ]) malloc(PRNG_ARRAYS_SIZE_UHS);
		prngArraysUhs_arrayBug  = (uint64_t (*)[NUMBER_OF_PRNG_ARRAYS_UHS  + 1]) malloc(PRNG_ARRAYS_SIZE_UHS);

		if (
			prngArraysNorm == NULL || prngArraysNorm_arrayBug == NULL ||
			prngArraysUhs  == NULL || prngArraysUhs_arrayBug  == NULL)
		{
			fprintf(stderr, "srandom_reset(): malloc failed\n");
			exit(1);
		}
	}

	arraysBufferPositions[0] = 0;
	arraysBufferPositions[1] = 0;
	arraysBufferPositions[2] = 0;
	arraysBufferPositions[3] = 0;

	// Seed with real random... unless srandom is installed
	Csprng::get(xorshft64_state,         sizeof(xorshft64_state));
	Csprng::get(xorshft128_state,        sizeof(xorshft128_state));
	Csprng::get(prngArraysNorm,          PRNG_ARRAYS_SIZE_NORM);
	Csprng::get(prngArraysNorm_arrayBug, PRNG_ARRAYS_SIZE_NORM);
	Csprng::get(prngArraysUhs,           PRNG_ARRAYS_SIZE_UHS);
	Csprng::get(prngArraysUhs_arrayBug,  PRNG_ARRAYS_SIZE_UHS);
}

size_t srandom_read(void *buffer, size_t bufferSize, uint64_t *prngArrays, uint64_t &xorshft64_state, uint64_t xorshft128_state[2], int &arraysBufferPosition, int version)
{
	uint64_t  numPrngArrays;
	uint64_t *prngArray;
	uint8_t  *tmpBuffer;

	if (version < 0 || version > 3)
	{
		return 0;
	}

	tmpBuffer = (uint8_t*) malloc((bufferSize + 512) * sizeof(uint8_t));
	if (!tmpBuffer)
	{
		return 0;
	}

	// Select a RND array
	if (version == SRANDOM_VERSION_NORM_ARRAY_BUG)
	{
		prngArray = prngArraysNorm_arrayBug[NUMBER_OF_PRNG_ARRAYS_NORM];
		numPrngArrays = NUMBER_OF_PRNG_ARRAYS_NORM;
	}
	else if (version == SRANDOM_VERSION_NORM)
	{
		prngArray = prngArraysNorm[NUMBER_OF_PRNG_ARRAYS_NORM];
		numPrngArrays = NUMBER_OF_PRNG_ARRAYS_NORM;
	}
	else if (version == SRANDOM_VERSION_UHS_ARRAY_BUG)
	{
		prngArray = prngArraysUhs_arrayBug[NUMBER_OF_PRNG_ARRAYS_UHS];
		numPrngArrays = NUMBER_OF_PRNG_ARRAYS_UHS;
	}
	else
	{
		prngArray = prngArraysUhs[NUMBER_OF_PRNG_ARRAYS_UHS];
		numPrngArrays = NUMBER_OF_PRNG_ARRAYS_UHS;
	}
	int arraysPosition = nextbuffer(prngArray, numPrngArrays, xorshft64_state, xorshft128_state, arraysBufferPosition);
	if (version == SRANDOM_VERSION_NORM_ARRAY_BUG)
	{
		prngArray = ((uint64_t (*)[NUMBER_OF_PRNG_ARRAYS_NORM + 1]) prngArrays)[arraysPosition];
	}
	else if (version == SRANDOM_VERSION_NORM)
	{
		prngArray = ((uint64_t (*)[PRNG_ARRAY_SIZE_NORM          ]) prngArrays)[arraysPosition];
	}
	else if (version == SRANDOM_VERSION_UHS_ARRAY_BUG)
	{
		prngArray = ((uint64_t (*)[NUMBER_OF_PRNG_ARRAYS_UHS  + 1]) prngArrays)[arraysPosition];
	}
	else
	{
		prngArray = ((uint64_t (*)[PRNG_ARRAY_SIZE_UHS           ]) prngArrays)[arraysPosition];
	}

	// Send the Array of RND to USER
	for (int block = 0; block <= bufferSize / 512; block++)
	{
		memcpy(tmpBuffer + 512 * block, prngArray, 512);
		if (version < 2)
		{
			update_sarray(prngArray, xorshft64_state, xorshft128_state);
		}
		else
		{
			update_sarray_uhs(prngArray, xorshft64_state);
		}
	}

	memcpy(buffer, tmpBuffer, bufferSize);
	free(tmpBuffer);

	return bufferSize;
}

size_t srandom_read(void *buffer, size_t bufferSize, int version)
{
	uint64_t  numPrngArrays;
	uint64_t *prngArray;
	uint8_t  *tmpBuffer;

	if (version < 0 || version > 3)
	{
		return 0;
	}
	if (prngArraysNorm == NULL)
	{
		srandom_reset();
	}

	tmpBuffer = (uint8_t*) malloc((bufferSize + 512) * sizeof(uint8_t));
	if (!tmpBuffer)
	{
		return 0;
	}

	// Select a RND array
	if (version == SRANDOM_VERSION_NORM_ARRAY_BUG)
	{
		prngArray = prngArraysNorm_arrayBug[NUMBER_OF_PRNG_ARRAYS_NORM];
		numPrngArrays = NUMBER_OF_PRNG_ARRAYS_NORM;
	}
	else if (version == SRANDOM_VERSION_NORM)
	{
		prngArray = prngArraysNorm[NUMBER_OF_PRNG_ARRAYS_NORM];
		numPrngArrays = NUMBER_OF_PRNG_ARRAYS_NORM;
	}
	else if (version == SRANDOM_VERSION_UHS_ARRAY_BUG)
	{
		prngArray = prngArraysUhs_arrayBug[NUMBER_OF_PRNG_ARRAYS_UHS];
		numPrngArrays = NUMBER_OF_PRNG_ARRAYS_UHS;
	}
	else
	{
		prngArray = prngArraysUhs[NUMBER_OF_PRNG_ARRAYS_UHS];
		numPrngArrays = NUMBER_OF_PRNG_ARRAYS_UHS;
	}
	int arraysPosition = nextbuffer(prngArray, numPrngArrays, xorshft64_state[version], xorshft128_state + 2 * version, arraysBufferPositions[version]);

	if (version == SRANDOM_VERSION_NORM_ARRAY_BUG)
	{
		prngArray = prngArraysNorm_arrayBug[arraysPosition];
	}
	else if (version == SRANDOM_VERSION_NORM)
	{
		prngArray = prngArraysNorm[arraysPosition];
	}
	else if (version == SRANDOM_VERSION_UHS_ARRAY_BUG)
	{
		prngArray = prngArraysUhs_arrayBug[arraysPosition];
	}
	else
	{
		prngArray = prngArraysUhs[arraysPosition];
	}

	// Send the Array of RND to USER
	for (int block = 0; block <= bufferSize / 512; block++)
	{
		memcpy(tmpBuffer + 512 * block, prngArray, 512);
		if (version < 2)
		{
			update_sarray(prngArray, xorshft64_state[version], xorshft128_state + 2 * version);
		}
		else
		{
			update_sarray_uhs(prngArray, xorshft64_state[version]);
		}
	}

	memcpy(buffer, tmpBuffer, bufferSize);
	free(tmpBuffer);

	return bufferSize;
}

void update_sarray(uint64_t *prngArray, uint64_t &xorshft64_state, uint64_t xorshft128_state[2])
{
	uint64_t x, y, z1, z2, z3;

	z1 = xorshft64(xorshft64_state);
	z2 = xorshft64(xorshft64_state);
	z3 = xorshft64(xorshft64_state);

	if ((z1 & 1) == 0)
	{
		for (size_t i = 0; i < PRNG_ARRAY_SIZE_NORM - 4; i += 4)
		{
			x = xorshft128(xorshft128_state);
			y = xorshft128(xorshft128_state);
			prngArray[i    ] = prngArray[i + 1] ^ x ^ y;
			prngArray[i + 1] = prngArray[i + 2] ^ y ^ z1;
			prngArray[i + 2] = prngArray[i + 3] ^ x ^ z2;
			prngArray[i + 3] = x ^ y ^ z3;
		}
	}
	else
	{
		for (size_t i = 0; i < PRNG_ARRAY_SIZE_NORM - 4; i += 4)
		{
			x = xorshft128(xorshft128_state);
			y = xorshft128(xorshft128_state);
			prngArray[i    ] = prngArray[i + 1] ^ x ^ z2;
			prngArray[i + 1] = prngArray[i + 2] ^ x ^ y;
			prngArray[i + 2] = prngArray[i + 3] ^ y ^ z3;
			prngArray[i + 3] = x ^ y ^ z1;
		}
	}
}

void update_sarray_uhs(uint64_t *prngArray, uint64_t &xorshft64_state)
{
	uint64_t x, z1;

	z1 = xorshft64(xorshft64_state);
	if ((z1 & 1) == 0)
	{
		for (size_t i = 0; i < PRNG_ARRAY_SIZE_UHS - 4; i += 4)
		{
			x = xorshft64(xorshft64_state);
			prngArray[i    ] = prngArray[i + 1] ^ x;
			prngArray[i + 1] = prngArray[i + 2] ^ x ^ z1;
			prngArray[i + 2] = prngArray[i + 3] ^ x ^ z1;
			prngArray[i + 3] = x ^ z1;
		}
	}
	else
	{
		for (size_t i = 0; i < PRNG_ARRAY_SIZE_UHS - 4; i += 4)
		{
			x = xorshft64(xorshft64_state);
			prngArray[i    ] = prngArray[i + 1] ^ x ^ z1;
			prngArray[i + 1] = prngArray[i + 2] ^ x;
			prngArray[i + 2] = prngArray[i + 3] ^ x ^ z1;
			prngArray[i + 3] = x ^ z1;
		}
	}
}

uint64_t xorshft64(uint64_t &state)
{
	uint64_t z = (state += UINT64_C(0x9E3779B97F4A7C15));
	z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
	z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
	return z ^ (z >> 31);
}

uint64_t xorshft128(uint64_t state[2])
{
	uint64_t s0 = state[0];
	uint64_t s1 = state[1];

	s0 = s0 ^ (s0 << 23);
	s0 = s0 ^ (s0 >> 17) ^ s1 ^ (s1 >> 26);

	state[0] = s1;
	state[1] = s0;
	return s0 + s1;
}

int nextbuffer(uint64_t *prngArray, uint64_t numPrngArrays, uint64_t &xorshft64_state, uint64_t xorshft128_state[2], int &arraysBufferPosition)
{
	int       position = arraysBufferPosition / 16;
	int       roll     = arraysBufferPosition % 16;
	int       arrayIndex;

	arrayIndex = (int) ((prngArray[position] >> (roll * 4)) & (numPrngArrays - 1));

	arraysBufferPosition++;
	if (arraysBufferPosition >= 1021)
	{
		arraysBufferPosition = 0;
		update_sarray(prngArray, xorshft64_state, xorshft128_state);
	}

	return arrayIndex;
}
