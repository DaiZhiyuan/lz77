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
#define MAX_LEN			264 /* 256 + 8 */
#define MAX_DISTANCE	8192

#define HASH_LOG		13
#define HASH_SIZE		(1 << HASH_LOG)
#define HASH_MASK		(HASH_SIZE - 1)

#define LZ77_BOUND_CHECK(cond) \
	if (unlikely(!(cond))) return 0;

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

static uint8_t* lz77_match(uint32_t len, uint32_t distance, uint8_t* op)
{
	--distance;

	if (unlikely(len > MAX_LEN - 2)) {
		while (len > MAX_LEN - 2) {
			*op++ = (7 << 5) + (distance >> 8);
			*op++ = MAX_LEN - 2 - 7 - 2;
			*op++ = (distance & 255);
			len -= MAX_LEN - 2;
		}
	}

	if (len < 7) {
		*op++ = (len << 5) + (distance >> 8);
		*op++ = (distance & 255);
	} else {
		*op++ = (7 << 5) + (distance >> 8);
		*op++ = len - 7;
		*op++ = (distance & 255);
	}

	return op;
}

static uint8_t* lz77_literals(uint32_t runs, const uint8_t* src, uint8_t* dest)
{
	while (runs >= MAX_COPY) {
		*dest++ = MAX_COPY - 1;
		lz77_maxcopy(dest, src);
		src += MAX_COPY;
		dest += MAX_COPY;
		runs -= MAX_COPY;
	}

	if (runs > 0) {
		*dest++ = runs - 1;
		lz77_smallcopy(dest, src, runs);
		dest += runs;
	}

	return dest;
}

int lz77_compress(const void* input, int length, void* output)
{
	const uint8_t* ip = (const uint8_t*)input;
	const uint8_t* ip_start = ip;
	const uint8_t* ip_bound = ip + length - 4; /* because readU32 */
	const uint8_t* ip_limit = ip + length - 12 - 1;
	uint8_t* op = (uint8_t*)output;

	uint32_t htab[HASH_SIZE];
	uint32_t seq, hash;

	/* initializes hash table */
	for (hash = 0; hash < HASH_SIZE; ++hash)
		htab[hash] = 0;

	/* we start with literal copy */
	const uint8_t* anchor = ip;
	ip += 2;

	/* main loop */
	while (likely(ip < ip_limit)) {
		const uint8_t* ref;
		uint32_t distance, cmp;

		/* find potential match */
		do {
			seq = lz77_readu32(ip) & 0xffffff;
			hash = lz77_hash(seq);
			ref = ip_start + htab[hash];
			htab[hash] = ip - ip_start;
			distance = ip - ref;
			cmp = likely(distance < MAX_DISTANCE) ? lz77_readu32(ref) & 0xffffff : 0x1000000;

			if (unlikely(ip >= ip_limit))
				break;

			++ip;
		} while (seq != cmp);

		if (unlikely(ip >= ip_limit))
			break;

		--ip;

		if (likely(ip > anchor)) {
			op = lz77_literals(ip - anchor, anchor, op);
		}

		uint32_t len = lz77_memcmp(ref + 3, ip + 3, ip_bound);
		op = lz77_match(len, distance, op);

		/* update the hash at match boundary */
		ip += len;
		seq = lz77_readu32(ip);
		hash = lz77_hash(seq & 0xffffff);
		htab[hash] = ip++ - ip_start;
		seq >>= 8;
		hash = lz77_hash(seq);
		htab[hash] = ip++ - ip_start;

		anchor = ip;
	}

	uint32_t copy = (uint8_t*)input + length - anchor;
	op = lz77_literals(copy, anchor, op);

	return op - (uint8_t*)output;
}

int lz77_decompress(const void* input, int length, void* output, int maxout)
{
	const uint8_t* ip = (const uint8_t*)input;
	const uint8_t* ip_limit = ip + length;
	const uint8_t* ip_bound = ip_limit - 2;
	uint8_t* op = (uint8_t*)output;
	uint8_t* op_limit = op + maxout;
	uint32_t ctrl = (*ip++) & 31;

	while (1) {
		if (ctrl >= 32) {
			uint32_t len = (ctrl >> 5) - 1;
			uint32_t ofs = (ctrl & 31) << 8;
			const uint8_t* ref = op - ofs - 1;

			if (len == 7 - 1) {
				LZ77_BOUND_CHECK(ip <= ip_bound);
				len += *ip++;
			}

			ref -= *ip++;
			len += 3;
			LZ77_BOUND_CHECK(op + len <= op_limit);
			LZ77_BOUND_CHECK(ref >= (uint8_t*)output);
			lz77_memmove(op, ref, len);
			op += len;
		} else {
			ctrl++;
			LZ77_BOUND_CHECK(op + ctrl <= op_limit);
			LZ77_BOUND_CHECK(ip + ctrl <= ip_limit);
			lz77_memcpy(op, ip, ctrl);
			ip += ctrl;
			op += ctrl;
		}

		if (unlikely(ip > ip_bound))
			break;

		ctrl = *ip++;
	}

	return op - (uint8_t*)output;
}

#pragma GCC diagnostic pop
