/*
 * Byte-aligned LZ77 compression library
 */

#include "lz77.h"
#include <stdint.h>
#include <string.h>

/*
 * Give hints to the compiler for branch prediction optimization.
 */
#if defined(__GNUC__)
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#else
#define likely(x)		(x)
#define unlikely(x)		(x)
#endif

static void lz77_memmove(uint8_t* dest, const uint8_t* src, uint32_t count)
{
	if ((count > 4) && (dest >= src + count)) {
		memmove(dest, src, count);
	} else {
		switch (count) {
			default:
				do {
					*dest++ = *src++;
				} while (--count);
				break;
			case 3:
				*dest++ = *src++;
			case 2:
				*dest++ = *src++;
			case 1:
				*dest++ = *src++;
			case 0:
				break;
		}
	}
}

static void lz77_memcpy(uint8_t* dest, const uint8_t* src, uint32_t count)
{
	memcpy(dest, src, count);
}

static uint32_t lz77_readu32(const void* p)
{
	return *(const uint32_t*)p;
}

static uint32_t lz77_memcmp(const uint8_t* p, const uint8_t* q, const uint8_t* len)
{
	const uint8_t* start = p;

	if (lz77_readu32(p) == lz77_readu32(q)) {
		p += 4;
		q += 4;
	}

	while (q < len) {
		if (*p++ != *q++)
			break;
	}

	return p - start;
}

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
