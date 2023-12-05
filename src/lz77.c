/*
 * Byte-aligned LZ77 compression library
 */

#include "lz77.h"

int lz77_l1_compress(const void* input, int length, void* output)
{
	return 0;
}

int lz77_l2_compress(const void* input, int length, void* output)
{
	return 0;
}

int lz77_compress(const void* input, int length, void* output)
{
	if (length < 65536)
		return lz77_l1_compress(input, length, output);

	return lz77_l2_compress(input, length, output);
}

int lz77_compress_level(int level, const void* input, int length, void* output)
{
	if (1 == level)
		return lz77_l1_compress(input, length, output);

	if (2 == level)
		return lz77_l2_compress(input, length, output);

	return 0;
}

int lz77_decompress(const void* input, int length, void* output, int maxout)
{
	return 0;
}
