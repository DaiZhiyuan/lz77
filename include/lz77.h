/*
 * Byte-aligned LZ77 compression library
 */

 #ifndef __LZ77_H__
 #define __LZ77_H__

int lz77_compress(const void* input, int length, void* output);
int lz77_compress_level(int level, const void* input, int length, void* output);
int lz77_decompress(const void* input, int length, void* output, int maxout);

 #endif
