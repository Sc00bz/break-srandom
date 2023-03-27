#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include "xorshft.h"

uint64_t xorshft64(uint64_t &state)
{
	uint64_t z = (state += UINT64_C(0x9E3779B97F4A7C15));
	z = (z ^ (z >> 30)) * UINT64_C(0xBF58476D1CE4E5B9);
	z = (z ^ (z >> 27)) * UINT64_C(0x94D049BB133111EB);
	return z ^ (z >> 31);
}

uint64_t xorshft64_getState(uint64_t z)
{
	z = (z ^ (z >> 31) ^ (z >> (2*31))) * UINT64_C(0x319642b2d24d8ec3);
	z = (z ^ (z >> 27) ^ (z >> (2*27))) * UINT64_C(0x96de1b173f119089);
	z =  z ^ (z >> 30) ^ (z >> (2*30));
	return z;
}

void xorshft64_skip(uint64_t &state, int64_t count)
{
	state += (uint64_t) (INT64_C(0x9E3779B97F4A7C15) * count);
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

void xorshft128_undo(uint64_t state[2])
{
	uint64_t s1 = state[0];
	uint64_t s0 = state[1];

	s0 ^= s1 ^ (s1 >> 26);
	s0  = s0 ^ (s0 >> 17) ^ (s0 >> (2*17)) ^ (s0 >> (3*17));
	s0  = s0 ^ (s0 << 23) ^ (s0 << (2*23));

	state[0] = s0;
	state[1] = s1;
}



// #########################################
// #########################################
// ###                                   ###
// ###         Now the fun part          ###
// ###  Solving the system of equations  ###
// ###                                   ###
// #########################################
// #########################################


// #####################################
// ## Follow the bits in xorshft128() ##
// #####################################

struct bits128
{
	uint64_t lo;
	uint64_t hi;
};

struct uint64_bits
{
	bits128 bits[64];
};

bits128 operator^(const bits128 &a, const bits128 &b)
{
	bits128 ret;

	ret.lo = a.lo ^ b.lo;
	ret.hi = a.hi ^ b.hi;
	return ret;
}

uint64_bits operator^(const uint64_bits &a, const uint64_bits &b)
{
	uint64_bits ret;

	for (int i = 0; i < 64; i++)
	{
		ret.bits[i] = a.bits[i] ^ b.bits[i];
	}
	return ret;
}

uint64_bits operator>>(const uint64_bits &a, int shift)
{
	uint64_bits ret = {0};

	for (int i = 0; i < 64 - shift; i++)
	{
		ret.bits[i] = a.bits[i + shift];
	}
	return ret;
}

uint64_bits operator<<(const uint64_bits &a, int shift)
{
	uint64_bits ret = {0};

	for (int i = 63 - shift; i >= 0; i--)
	{
		ret.bits[i + shift] = a.bits[i];
	}
	return ret;
}

void xorshft128(uint64_bits &s0_, uint64_bits &s1)
{
	uint64_bits s0 = s0_;

	s0 = s0 ^ (s0 << 23);
	s0 = s0 ^ (s0 >> 17) ^ s1 ^ (s1 >> 26);

	s0_ = s1;
	s1  = s0;
}


// ###################################
// ## System of equations functions ##
// ###################################

int getBit(bits128 bits, int bit)
{
	int ret;

	if (bit < 64)
	{
		ret = (int) (bits.lo >> bit);
	}
	else
	{
		ret = (int) (bits.hi >> (bit - 64));
	}
	return ret & 1;
}

void setBit(bits128 &bits, int bit, int value)
{
	if (bit < 64)
	{
		uint64_t tmp = bits.lo;

		tmp &= ~(UINT64_C(1)          << bit);
		tmp |= ((uint64_t) value & 1) << bit;
		bits.lo = tmp;
	}
	else
	{
		uint64_t tmp = bits.hi;

		tmp &= ~(UINT64_C(1)          << (bit - 64));
		tmp |= ((uint64_t) value & 1) << (bit - 64);
		bits.hi = tmp;
	}
}

void swapRows(bits128 matrix[128], bits128 &answers, int row0, int row1)
{
	bits128 tmp;

	tmp = matrix[row0];
	matrix[row0] = matrix[row1];
	matrix[row1] = tmp;

	int bit0, bit1;
	bit0 = getBit(answers, row0);
	bit1 = getBit(answers, row1);
	setBit(answers, row0, bit1);
	setBit(answers, row1, bit0);
}

void xorRow(bits128 matrix[128], bits128 &answers, bits128 matrixRow, int answerBit, int row)
{
	matrix[row].lo ^= matrixRow.lo;
	matrix[row].hi ^= matrixRow.hi;
	if (row < 64)
	{
		answers.lo ^= ((uint64_t) answerBit) << row;
	}
	else
	{
		answers.hi ^= ((uint64_t) answerBit) << (row - 64);
	}
}

int nextRowWithBitSet(const bits128 matrix[128], int bit, int currentRow)
{
	int row;

	for (row = currentRow; row < 128; row++)
	{
		if (getBit(matrix[row], bit))
		{
			break;
		}
	}

	// Not found
	if (row == 128)
	{
		row = -1;
	}

	return row;
}


// #################################
// ## Display system of equations ##
// #################################

void printSystemOfEquations(const bits128 matrix[128], bits128 answers)
{
	for (int row = 0; row < 128; row++)
	{
		printf("%016" PRIx64 "%016" PRIx64 " = %d\n", matrix[row].hi, matrix[row].lo, getBit(answers, row));
	}
	printf("\n");
}

int saveBmp(const bits128 matrix[128], bits128 answers, int frame)
{
	const size_t   pixelSize   = 1;
	const size_t   width       = 130 * pixelSize;
	const size_t   height      = 128 * pixelSize;
	const size_t   rowDataSize = (3 * pixelSize * (128 + 2) + 3) & ~(size_t) 3;
	const uint8_t  zero[3]     = {0x00, 0x00, 0x00};
	const uint8_t  one[3]      = {0xff, 0xff, 0xff};
	const uint8_t  equal[3]    = {0x00, 0x00, 0x00};

	uint8_t header[54] = {
		0x42, 0x4d, 0x36, 0xc4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00,
		width  & 0xff, (width  >> 8) & 0xff, (width  >> 16) & 0xff, (width  >> 24) & 0xff,
		height & 0xff, (height >> 8) & 0xff, (height >> 16) & 0xff, (height >> 24) & 0xff,
		0x01, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	char           fname[256];
	FILE          *fout;
	uint8_t       *rowData = new uint8_t[rowDataSize];
	const uint8_t *pixel;
	int            bit;

	for (size_t i = rowDataSize - 3; i < rowDataSize; i++)
	{
		rowData[i] = 0;
	}

	sprintf(fname, "%04u.bmp", frame);
	fout = fopen(fname, "wb");
	if (fout == NULL)
	{
		fprintf(stderr, "fopen failed\n");
		return 1;
	}
	if (fwrite(header, sizeof(header), 1, fout) != 1)
	{
		fprintf(stderr, "fwrite failed\n");
		return 1;
	}

	for (int row = 127; row >= 0; row--)
	{
		for (size_t i = 0; i < 128; i++)
		{
			bit = getBit(matrix[row], 127 - (int) i);
			pixel = zero;
			if (bit)
			{
				pixel = one;
			}
			for (size_t j = 0; j < 3 * pixelSize; j++)
			{
				rowData[3 * pixelSize * i + j] = pixel[j % 3];
			}
		}
		for (size_t j = 0; j < 3 * pixelSize; j++)
		{
			rowData[3 * pixelSize * 128 + j] = equal[j % 3];
		}
		bit = getBit(answers, row);
		pixel = zero;
		if (bit)
		{
			pixel = one;
		}
		for (size_t j = 0; j < 3 * pixelSize; j++)
		{
			rowData[3 * pixelSize * 129 + j] = pixel[j % 3];
		}
		for (size_t j = 0; j < pixelSize; j++)
		{
			if (fwrite(rowData, rowDataSize, 1, fout) != 1)
			{
				fprintf(stderr, "fwrite failed\n");
				return 1;
			}
		}
	}
	fclose(fout);

	return 0;
}


// ###################################
// ## Solve the system of equations ##
// ###################################

int xorshft128_getState(uint64_t state[2], uint64_t leastSignificantBitsOutput[2], int showWork, int saveFrames)
{
	bits128     matrix[128] = {0};
	bits128     answers     = {0};
	uint64_bits s0          = {0};
	uint64_bits s1          = {0};
	int         frame       = 0;

	// Init bits
	for (int i = 0; i < 64; i++)
	{
		s0.bits[i].lo |= UINT64_C(1) << i;
		s1.bits[i].hi |= UINT64_C(1) << i;
	}

	if (showWork)
	{
		printf(
			"Getting state from xorshft128 output\n"
			"Generate system of equations matrix...\n");
	}

	// Generate system of equations matrix
	if (saveFrames > 0)
	{
		if (saveBmp(matrix, answers, frame++))
		{
			saveFrames = 0;
		}
	}
	for (int i = 0; i < 128; i++)
	{
		xorshft128(s0, s1);
		matrix[i].lo = s0.bits[0].lo ^ s1.bits[0].lo;
		matrix[i].hi = s0.bits[0].hi ^ s1.bits[0].hi;

		if (saveFrames > 0 && i % saveFrames == saveFrames - 1)
		{
			if (saveBmp(matrix, answers, frame++))
			{
				saveFrames = 0;
			}
		}
	}
	if (saveFrames > 0 && 127 % saveFrames != saveFrames - 1)
	{
		if (saveBmp(matrix, answers, frame++))
		{
			saveFrames = 0;
		}
	}

	// Set answers
	for (int i = 0; i < 64; i++)
	{
		int bit = (leastSignificantBitsOutput[0] >> i) & 1;
		setBit(answers, i, bit);
	}
	for (int i = 64; i < 128; i++)
	{
		int bit = (leastSignificantBitsOutput[1] >> (i - 64)) & 1;
		setBit(answers, i, bit);
	}
	if (saveFrames > 0)
	{
		if (saveBmp(matrix, answers, frame++))
		{
			saveFrames = 0;
		}
	}

	if (showWork)
	{
		printSystemOfEquations(matrix, answers);
		printf("Breaking...\n");
	}

	// Solve system of equations
	for (int i = 0; i < 128; i++)
	{
		int row = nextRowWithBitSet(matrix, i, i);

		// Row not found
		if (row < 0)
		{
			fprintf(stderr, "Bad system of equations\n");
			return 1;
		}
		swapRows(matrix, answers, i, row);

		// Remove variable from all other equations
		int answerBit = getBit(answers, i);
		for (int j = 0; j < 128; j++)
		{
			if (i != j && getBit(matrix[j], i))
			{
				xorRow(matrix, answers, matrix[i], answerBit, j);
			}
		}

		if (saveFrames > 0 && i % saveFrames == saveFrames - 1)
		{
			if (saveBmp(matrix, answers, frame++))
			{
				saveFrames = 0;
			}
		}
	}
	if (saveFrames > 0 && 127 % saveFrames != saveFrames - 1)
	{
		if (saveBmp(matrix, answers, frame++))
		{
			saveFrames = 0;
		}
	}

	if (showWork)
	{
		printSystemOfEquations(matrix, answers);
		printf("Recovered state: 0x%016" PRIx64 ", 0x%016" PRIx64 "\n", answers.lo, answers.hi);
	}

	state[0] = answers.lo;
	state[1] = answers.hi;
	return 0;
}
