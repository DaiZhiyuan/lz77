/*
 * Byte-aligned LZ77 compression library
 */

#include "lz77.h"
#include <stdint.h>
#include <string.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

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

#define MAX_COPY		32

#define HASH_LOG		13
#define HASH_SIZE		(1 << HASH_LOG)
#define HASH_MASK		(HASH_SIZE - 1)

static uint16_t lz77_hash(uint32_t v)
{
	uint32_t h = (v * 2654435769LL) >> (32 - HASH_LOG);

	return h & HASH_MASK;
}

/* special case of memcpy: at most MAX_COPY bytes */
static void lz77_smallcopy(uint8_t* dest, const uint8_t* src, uint32_t count)
{
	if (count >= 4) {
		const uint32_t* p = (const uint32_t*)src;
		uint32_t* q = (uint32_t*)dest;

		while (count > 4) {
			*q++ = *p++;
			count -= 4;
			dest += 4;
			src += 4;
		}
	}
}

/* special case of memcpy: exactly MAX_COPY bytes */
static void lz77_maxcopy(void* dest, const void* src)
{
	const uint32_t* p = (const uint32_t*)src;
	uint32_t* q = (uint32_t*)dest;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
	*q++ = *p++;
}

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

int lz77_compress(const void* input, int length, void* output)
{
	return 0;
}

int lz77_decompress(const void* input, int length, void* output, int maxout)
{
	return 0;
}

#pragma GCC diagnostic pop
