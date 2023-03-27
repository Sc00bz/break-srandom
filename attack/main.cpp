#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdlib.h>
#include "srandom.h"
#include "xorshft.h"
#include "csprng.h"

uint64_t inverseMod2Pow64(uint64_t x)
{
	uint64_t y = x;

	for (int i = 0; i < 5; i++)
	{
		y *= 2 - x * y;
	}
	return y;
}

void show_xorshft64_getState()
{
	uint64_t invert;
	uint64_t num;

	printf("Inverse modulo 2**64 xorshft64()'s constants:\n");

	num = UINT64_C(0x94D049BB133111EB);
	invert = inverseMod2Pow64(num);
	printf("0x%016" PRIx64 " * 0x%016" PRIx64 " = 0x%016" PRIx64 "\n",   num, invert, num * invert);

	num = UINT64_C(0xBF58476D1CE4E5B9);
	invert = inverseMod2Pow64(num);
	printf("0x%016" PRIx64 " * 0x%016" PRIx64 " = 0x%016" PRIx64 "\n\n", num, invert, num * invert);

	uint64_t originalState;
	uint64_t recoveredState;
	uint64_t data;

	Csprng::get(&originalState, sizeof(originalState));

	data = xorshft64(originalState);
	recoveredState = xorshft64_getState(data);

	printf("Original state:  0x%016" PRIx64 "\n",   originalState);
	printf("Recovered state: 0x%016" PRIx64 "\n\n", recoveredState);
}

void show_xorshft128_getState()
{
	uint64_t originalState[2];
	uint64_t recoveredState[2];
	uint64_t data[2] = {0};

	Csprng::get(originalState, sizeof(originalState));
	printf("Original state: 0x%016" PRIx64 ", 0x%016" PRIx64 "\n\n", originalState[0], originalState[1]);

	for (int i = 0; i < 128; i++)
	{
		if (i < 64)
		{
			data[0] |= (xorshft128(originalState) & 1) << i;
		}
		else
		{
			data[1] |= (xorshft128(originalState) & 1) << (i - 64);
		}
	}

	xorshft128_getState(recoveredState, data, 1);

	printf("Incrementing recovered state\n");
	for (int i = 0; i < 128; i++)
	{
		xorshft128(recoveredState);
	}

	printf("Original state:  0x%016" PRIx64 ", 0x%016" PRIx64 "\n",   originalState[0],  originalState[1]);
	printf("Recovered state: 0x%016" PRIx64 ", 0x%016" PRIx64 "\n\n", recoveredState[0], recoveredState[1]);
}

#define PRNG_ARRAY_SIZE_UHS         65
#define PRNG_ARRAY_SIZE_NORM        67
#define NUMBER_OF_PRNG_ARRAYS_UHS   32
#define NUMBER_OF_PRNG_ARRAYS_NORM  16

int reset(int version)
{
	uint64_t num;

	printf("Reseting srandom state to unknown\n");
	srandom_reset();
	if (srandom_read(&num, sizeof(uint64_t), version) != sizeof(uint64_t))
	{
		return 1;
	}
	for (int i = 0, rounds = num % 1021; i < rounds; i++)
	{
		if (srandom_read(&num, 1, version) != 1)
		{
			return 1;
		}
	}

	return 0;
}

int getXorshft64State(int version, uint64_t &xorshft64_state, uint64_t &z1, int print = 0)
{
	uint64_t buffer[64+4];
	uint64_t x, y, z2, z3;

	if (print)
	{
		printf("Get xorshft64() state\n");
	}

	if (srandom_read(buffer, sizeof(buffer), version) != sizeof(buffer))
	{
		return 1;
	}

	if (version == SRANDOM_VERSION_NORM_ARRAY_BUG || version == SRANDOM_VERSION_NORM)
	{
		z3 = buffer[1] ^ buffer[64 + 0] ^ buffer[64 + 3];
		xorshft64_state = xorshft64_getState(z3);
		xorshft64_skip(xorshft64_state, -3);
		z1 = xorshft64(xorshft64_state);
		z2 = xorshft64(xorshft64_state);
		xorshft64_skip(xorshft64_state, 1);
		x = buffer[3] ^ buffer[64 + 2] ^ z2;
		y = buffer[2] ^ buffer[64 + 1] ^ z1;
		if ((z1 & 1) != 0 || buffer[64 + 3] != (x ^ y ^ z3))
		{
			z1 = buffer[2] ^ buffer[64 + 1] ^ buffer[64 + 3];
			xorshft64_state = xorshft64_getState(z1);
			z2 = xorshft64(xorshft64_state);
			z3 = xorshft64(xorshft64_state);
			x = buffer[1] ^ buffer[64 + 0] ^ z2;
			y = buffer[3] ^ buffer[64 + 2] ^ z3;
			if ((z1 & 1) == 0 || buffer[64 + 3] != (x ^ y ^ z1))
			{
				return 1; // error
			}
		}
		xorshft64_skip(xorshft64_state, 3);
	}
	else
	{
		x  = buffer[64 + 0] ^ buffer[1];
		z1 = buffer[64 + 3] ^ x;
		xorshft64_state = xorshft64_getState(z1);
		if ((z1 & 1) != 0 || x != xorshft64(xorshft64_state))
		{
			x  = buffer[64 + 1] ^ buffer[2];
			z1 = buffer[64 + 3] ^ x;
			xorshft64_state = xorshft64_getState(z1);
			if ((z1 & 1) == 0 || x != xorshft64(xorshft64_state))
			{
				return 1; // error
			}
		}
		xorshft64_skip(xorshft64_state, 32);
	}
	if (print)
	{
		printf("xorshft64_state = 0x%016" PRIx64 "\n", xorshft64_state);
	}

	return 0;
}

int getXorshft128StateNorm(int version, int &arraysBufferPosition, uint64_t &xorshft64_state, uint64_t xorshft128_state[2])
{
	uint64_t buffer[256+64];
	uint64_t xorshft128_output[2] = {0};
	
	printf("\nGet xorshft128() state\n");
	printf("Getting xorshft128() output...\n");
	arraysBufferPosition++;
	if (srandom_read(buffer, sizeof(buffer), version) != sizeof(buffer))
	{
		return 1;
	}
	for (int i = 64, shift = 0; i < 256 + 64; i += 64)
	{
		uint64_t x, y, z1, z2, z3;

		z1 = xorshft64(xorshft64_state);
		z2 = xorshft64(xorshft64_state);
		z3 = xorshft64(xorshft64_state);

		if ((z1 & 1) == 0)
		{
			for (size_t j = 0; j < PRNG_ARRAY_SIZE_NORM - 4; j += 4)
			{
				x = buffer[i + j + 2] ^ buffer[i + j + 3 - 64] ^ z2;
				y = buffer[i + j + 1] ^ buffer[i + j + 2 - 64] ^ z1;
				if (shift < 64)
				{
					xorshft128_output[0] |= (x & 1) << (shift++);
					xorshft128_output[0] |= (y & 1) << (shift++);
				}
				else
				{
					xorshft128_output[1] |= (x & 1) << (shift++ - 64);
					xorshft128_output[1] |= (y & 1) << (shift++ - 64);
				}
			}
		}
		else
		{
			for (size_t j = 0; j < PRNG_ARRAY_SIZE_NORM - 4; j += 4)
			{
				x = buffer[i + j    ] ^ buffer[i + j + 1 - 64] ^ z2;
				y = buffer[i + j + 2] ^ buffer[i + j + 3 - 64] ^ z3;
				if (shift < 64)
				{
					xorshft128_output[0] |= (x & 1) << (shift++);
					xorshft128_output[0] |= (y & 1) << (shift++);
				}
				else
				{
					xorshft128_output[1] |= (x & 1) << (shift++ - 64);
					xorshft128_output[1] |= (y & 1) << (shift++ - 64);
				}
			}
		}
	}
	xorshft64_skip(xorshft64_state, 6);

	printf("Recovering state...\n");
	if (xorshft128_getState(xorshft128_state, xorshft128_output))
	{
		return 1;
	}
	printf("xorshft128_state = 0x%016" PRIx64 ", 0x%016" PRIx64 "\n\n", xorshft128_state[0], xorshft128_state[1]);

	// Incrementing xorshft128_state
	printf("Incrementing recovered state...\n");
	for (int i = 0; i < (256+64+64)/2; i++)
	{
		xorshft128(xorshft128_state);
	}
	printf("xorshft128_state = 0x%016" PRIx64 ", 0x%016" PRIx64 "\n", xorshft128_state[0], xorshft128_state[1]);

	return 0;
}

int getXorshft128StateUhs(uint64_t z1s[5], uint64_t z2s[5], uint64_t z3s[5], uint64_t buffer[256+64], uint64_t xorshft128_state[2])
{
	uint64_t xorshft128_output[2] = {0};

	printf("\nGet xorshft128() state\n");
	printf("Using output...\n");
	for (int i = 64, shift = 0; i < 256 + 64; i += 64)
	{
		uint64_t x, y, z1, z2, z3;

		z1 = z1s[i / 64];
		z2 = z2s[i / 64];
		z3 = z3s[i / 64];

		if ((z1 & 1) == 0)
		{
			for (size_t j = 0; j < PRNG_ARRAY_SIZE_NORM - 4; j += 4)
			{
				x = buffer[i + j + 2] ^ buffer[i + j + 3 - 64] ^ z2;
				y = buffer[i + j + 1] ^ buffer[i + j + 2 - 64] ^ z1;
				if (shift < 64)
				{
					xorshft128_output[0] |= (x & 1) << (shift++);
					xorshft128_output[0] |= (y & 1) << (shift++);
				}
				else
				{
					xorshft128_output[1] |= (x & 1) << (shift++ - 64);
					xorshft128_output[1] |= (y & 1) << (shift++ - 64);
				}
			}
		}
		else
		{
			for (size_t j = 0; j < PRNG_ARRAY_SIZE_NORM - 4; j += 4)
			{
				x = buffer[i + j    ] ^ buffer[i + j + 1 - 64] ^ z2;
				y = buffer[i + j + 2] ^ buffer[i + j + 3 - 64] ^ z3;
				if (shift < 64)
				{
					xorshft128_output[0] |= (x & 1) << (shift++);
					xorshft128_output[0] |= (y & 1) << (shift++);
				}
				else
				{
					xorshft128_output[1] |= (x & 1) << (shift++ - 64);
					xorshft128_output[1] |= (y & 1) << (shift++ - 64);
				}
			}
		}
	}

	printf("Recovering state...\n");
	if (xorshft128_getState(xorshft128_state, xorshft128_output))
	{
		return 1;
	}
	printf("xorshft128_state = 0x%016" PRIx64 ", 0x%016" PRIx64 "\n\n", xorshft128_state[0], xorshft128_state[1]);

	// Incrementing xorshft128_state
	printf("Incrementing recovered state...\n");
	for (int i = 0; i < (256+64+64)/2; i++)
	{
		xorshft128(xorshft128_state);
	}
	printf("xorshft128_state = 0x%016" PRIx64 ", 0x%016" PRIx64 "\n", xorshft128_state[0], xorshft128_state[1]);

	return 0;
}

int makeArraysBufferPositionZero(int version, int &arraysBufferPosition, uint64_t &xorshft64_state)
{
	printf("Make arraysBufferPosition = 0\n");

	for (int i = 0; i < 1021; i++)
	{
		uint64_t z1_ = xorshft64(xorshft64_state);
		uint64_t z1;

		if (getXorshft64State(version, xorshft64_state, z1)) return 1;

		if (z1 != z1_)
		{
			arraysBufferPosition = 0;
			break;
		}
	}
	if (arraysBufferPosition != 0)
	{
		return 1; // error
	}
	return 0;
}

int show_srandom_normArrayBug()
{
	const int VERSION = SRANDOM_VERSION_NORM_ARRAY_BUG;
	const size_t PRNG_ARRAYS_SIZE_NORM = (NUMBER_OF_PRNG_ARRAYS_NORM + 1) * PRNG_ARRAY_SIZE_NORM;

	uint64_t (*prngArrays)[NUMBER_OF_PRNG_ARRAYS_NORM + 1] = (uint64_t (*)[NUMBER_OF_PRNG_ARRAYS_NORM + 1]) new uint64_t[PRNG_ARRAYS_SIZE_NORM];
	uint64_t xorshft64_state = 0;
	uint64_t xorshft128_state[2] = {0};
	int      arraysBufferPosition = -1;
	uint64_t x, y, z1, z2, z3;
	uint64_t buffer;

	// Make state unknown
	if (reset(VERSION)) return 1;

	// Get xorshft64() state
	if (getXorshft64State(VERSION, xorshft64_state, z1, 1)) return 1;

	// Make arraysBufferPosition = 0
	if (makeArraysBufferPositionZero(VERSION, arraysBufferPosition, xorshft64_state)) return 1;

	// Get xorshft128() state
	if (getXorshft128StateNorm(VERSION, arraysBufferPosition, xorshft64_state, xorshft128_state)) return 1;

	// Reset prngArrays[NUMBER_OF_PRNG_ARRAYS_NORM]
	printf("Reseting prngArrays[NUMBER_OF_PRNG_ARRAYS_NORM]...\n");
	for (int i = 0; i < 4; i++)
	{
		while (++arraysBufferPosition < 1021)
		{
			if (srandom_read(&buffer, 1, VERSION) != 1) return 1;
			update_sarray(prngArrays[0], xorshft64_state, xorshft128_state);
		}

		if (srandom_read(&buffer, 1, VERSION) != 1) return 1;
		arraysBufferPosition = 0;
		update_sarray(prngArrays[NUMBER_OF_PRNG_ARRAYS_NORM], xorshft64_state, xorshft128_state);
		update_sarray(prngArrays[0], xorshft64_state, xorshft128_state);
	}

	// Get state
	printf("Get prngArrays...\n(I didn't keep track of them so just get them now)\n");

	// ############################################
	// #### Start of overlap array bug changes ####
	// ############################################
	// Just advance nextbuffer() to known output
	//
	// Technically this can fail but because this gives the next loop 269 tries to fill all of the arrays.
	// But if array indexes 13, 14, 15 then it gets 208, 480, infinite more tries respectively.
	// fuckit: added a reset
fuckit:
	while (arraysBufferPosition++ < 16 * (64 - (NUMBER_OF_PRNG_ARRAYS_NORM + 1)))
	{
		if (srandom_read(&buffer, 1, VERSION) != 1) return 1;
		update_sarray(prngArrays[0], xorshft64_state, xorshft128_state);
	}
	arraysBufferPosition--;
	// ##########################################
	// #### End of overlap array bug changes ####
	// ##########################################

	int filledArrays = 0;
	while (filledArrays != 0xffff)
	{
		int index = nextbuffer(prngArrays[NUMBER_OF_PRNG_ARRAYS_NORM], NUMBER_OF_PRNG_ARRAYS_NORM, xorshft64_state, xorshft128_state, arraysBufferPosition);
		filledArrays |= 1 << index;

		if (srandom_read(prngArrays[index], 512, VERSION) != 512) return 1;
		update_sarray(prngArrays[index], xorshft64_state, xorshft128_state);
		update_sarray(prngArrays[index], xorshft64_state, xorshft128_state);

		// ############################################
		// #### Start of overlap array bug changes ####
		// ############################################
		if (arraysBufferPosition == 0 && (filledArrays & 0x8000) == 0)
		{
			goto fuckit; // 1 in 34,651,867 chance
		}
		// ##########################################
		// #### End of overlap array bug changes ####
		// ##########################################
	}

	printf("\nFull state of srandom recovered:\n");
	printf("srandom:\n");
	for (int i = 0; i < 4; i++)
	{
		printf("   ");
		for (int j = 0; j < 4; j++)
		{
			if (srandom_read(&buffer, sizeof(buffer), VERSION) != sizeof(buffer)) return 1;
			printf(" %016" PRIx64, buffer);
		}
		printf("\n");
	}
	printf("\n");
	printf("srandom recovered:\n");
	for (int i = 0; i < 4; i++)
	{
		printf("   ");
		for (int j = 0; j < 4; j++)
		{
			if (srandom_read(&buffer, sizeof(buffer), (uint64_t*) prngArrays, xorshft64_state, xorshft128_state, arraysBufferPosition, VERSION) != sizeof(buffer)) return 1;
			printf(" %016" PRIx64, buffer);
		}
		printf("\n");
	}
	printf("\n");

	delete [] prngArrays;
	return 0;
}

int show_srandom_norm()
{
	const int VERSION = SRANDOM_VERSION_NORM;
	const size_t PRNG_ARRAYS_SIZE_NORM = (NUMBER_OF_PRNG_ARRAYS_NORM + 1) * PRNG_ARRAY_SIZE_NORM;

	uint64_t (*prngArrays)[PRNG_ARRAY_SIZE_NORM] = (uint64_t (*)[PRNG_ARRAY_SIZE_NORM]) new uint64_t[PRNG_ARRAYS_SIZE_NORM];
	uint64_t xorshft64_state = 0;
	uint64_t xorshft128_state[2] = {0};
	int      arraysBufferPosition = -1;
	uint64_t x, y, z1, z2, z3;
	uint64_t buffer;

	// Make state unknown
	if (reset(VERSION)) return 1;

	// Get xorshft64() state
	if (getXorshft64State(VERSION, xorshft64_state, z1, 1)) return 1;

	// Make arraysBufferPosition = 0
	if (makeArraysBufferPositionZero(VERSION, arraysBufferPosition, xorshft64_state)) return 1;

	// Get xorshft128() state
	if (getXorshft128StateNorm(VERSION, arraysBufferPosition, xorshft64_state, xorshft128_state)) return 1;

	// Reset prngArrays[NUMBER_OF_PRNG_ARRAYS_NORM]
	printf("Reseting prngArrays[NUMBER_OF_PRNG_ARRAYS_NORM]...\n");
	for (int i = 0; i < 4; i++)
	{
		while (++arraysBufferPosition < 1021)
		{
			if (srandom_read(&buffer, 1, VERSION) != 1) return 1;
			update_sarray(prngArrays[0], xorshft64_state, xorshft128_state);
		}

		if (srandom_read(&buffer, 1, VERSION) != 1) return 1;
		arraysBufferPosition = 0;
		update_sarray(prngArrays[NUMBER_OF_PRNG_ARRAYS_NORM], xorshft64_state, xorshft128_state);
		update_sarray(prngArrays[0], xorshft64_state, xorshft128_state);
	}

	// Get state
	printf("Get prngArrays...\n(I didn't keep track of them so just get them now)\n");
	int filledArrays = 0;
	while (filledArrays != 0xffff)
	{
		int index = nextbuffer(prngArrays[NUMBER_OF_PRNG_ARRAYS_NORM], NUMBER_OF_PRNG_ARRAYS_NORM, xorshft64_state, xorshft128_state, arraysBufferPosition);
		filledArrays |= 1 << index;

		if (srandom_read(prngArrays[index], 512, VERSION) != 512) return 1;
		update_sarray(prngArrays[index], xorshft64_state, xorshft128_state);
		update_sarray(prngArrays[index], xorshft64_state, xorshft128_state);
	}

	printf("\nFull state of srandom recovered:\n");
	printf("srandom:\n");
	for (int i = 0; i < 4; i++)
	{
		printf("   ");
		for (int j = 0; j < 4; j++)
		{
			if (srandom_read(&buffer, sizeof(buffer), VERSION) != sizeof(buffer)) return 1;
			printf(" %016" PRIx64, buffer);
		}
		printf("\n");
	}
	printf("\n");
	printf("srandom recovered:\n");
	for (int i = 0; i < 4; i++)
	{
		printf("   ");
		for (int j = 0; j < 4; j++)
		{
			if (srandom_read(&buffer, sizeof(buffer), (uint64_t*) prngArrays, xorshft64_state, xorshft128_state, arraysBufferPosition, VERSION) != sizeof(buffer)) return 1;
			printf(" %016" PRIx64, buffer);
		}
		printf("\n");
	}
	printf("\n");

	delete [] prngArrays;
	return 0;
}

// not done
int show_srandom_uhsArrayBug()
{
	return 0;
}

// not done
int show_srandom_uhs()
{
	return 0;
}

int main()
{
	show_xorshft64_getState();
	printf("--------------------------------------\n");

	show_xorshft128_getState();
	printf("--------------------------------------\n");

	show_srandom_normArrayBug();
	printf("--------------------------------------\n");

	show_srandom_norm();
	printf("--------------------------------------\n");

	//todo: show_srandom_uhsArrayBug();
	//todo: show_srandom_uhs();

	return 0;
}
