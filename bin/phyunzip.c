#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lz77.h"

#define LZ77_VERSION_STRING "1.0"
#define PHYZIP_VERSION_STRING "1.2.3"

#define BLOCK_SIZE 65536

/* magic identifier for phyzip file */
static unsigned char phyzip_magic[8] = {'$', 'p', 'h', 'y', 'z', 'i', 'p', '$'};

int detect_magic(FILE* file)
{
	unsigned char buffer[8];
	size_t bytes_read;
	int c;

	fseek(file, SEEK_SET, 0);
	bytes_read = fread(buffer, 1, 8, file);
	fseek(file, SEEK_SET, 0);

	if (bytes_read < 8)
		return 0;

	for (c = 0; c < 8; c++)
		if (buffer[c] != phyzip_magic[c])
			return 0;

	return -1;
}

/* for Adler-32 checksum algorithm, see RFC 1950 Section 8.2 */
#define ADLER32_BASE 65521
static unsigned long update_adler32(unsigned long checksum, const void* buf, int len)
{
	const unsigned char* ptr = (const unsigned char*)buf;
	unsigned long s1 = checksum & 0xffff;
	unsigned long s2 = (checksum >> 16) & 0xffff;

	while (len > 0) {
		unsigned k = len < 5552 ? len : 5552;
		len -= k;

		while (k >= 8) {
			s1 += *ptr++;
			s2 += s1;
			s1 += *ptr++;
			s2 += s1;
			s1 += *ptr++;
			s2 += s1;
			s1 += *ptr++;
			s2 += s1;
			s1 += *ptr++;
			s2 += s1;
			s1 += *ptr++;
			s2 += s1;
			s1 += *ptr++;
			s2 += s1;
			s1 += *ptr++;
			s2 += s1;
			k -= 8;
		}

		while (k-- > 0) {
			s1 += *ptr++;
			s2 += s1;
		}

		s1 = s1 % ADLER32_BASE;
		s2 = s2 % ADLER32_BASE;
	}

	return (s2 << 16) + s1;
}

static unsigned long readU16(const unsigned char* p)
{
	return p[0] + (p[1] << 8);
}

static unsigned long readU32(const unsigned char* p)
{
	return p[0] + (p[1] << 8) + (p[2] << 16) + (p[3] << 24);
}

void read_chunk_header(FILE* file, int* id, int* options, unsigned long* size, unsigned long* checksum, unsigned long* extra)
{
	unsigned char buffer[16];
	fread(buffer, 1, 16, file);

	*id = readU16(buffer) & 0xffff;
	*options = readU16(buffer + 2) & 0xffff;
	*size = readU32(buffer + 4) & 0xffffffff;
	*checksum = readU32(buffer + 8) & 0xffffffff;
	*extra = readU32(buffer + 12) & 0xffffffff;
}

int unpack_file(const char *input_file)
{
	FILE *in, *out;
	unsigned long fsize;
	size_t pos;
	int chunk_id;
	int chunk_options;
	unsigned long chunk_size;
	unsigned long chunk_checksum;
	unsigned long chunk_extra;
	unsigned char buffer[BLOCK_SIZE];
	unsigned long checksum;
	unsigned long decompressed_size;
	unsigned long total_extracted;
	unsigned long compressed_bufsize = 0;
	unsigned long decompressed_bufsize = 0;
	unsigned char* compressed_buffer;
	unsigned char* decompressed_buffer;
	int file_name_length;
	char* output_file_name;
	int c;
	unsigned long remaining;

	/* sanity check */
	in = fopen(input_file, "rb");
	if (!in) {
		printf("Error: could not open %s\n", input_file);
		return -1;
	}

	/* find size of the file */
	fseek(in, 0, SEEK_END);
	fsize = ftell(in);
	fseek(in, 0, SEEK_SET);

	/* not a phyzip archive */
	if (!detect_magic(in)) {
		fclose(in);
		printf("Error: file %s is not a phyzip archive!\n", input_file);
		return -1;
	}

	/* position of first chunk */
	fseek(in, 8, SEEK_SET);

	while (1) {
		pos = ftell(in);
		if (pos >= fsize)
			break;

		read_chunk_header(in, &chunk_id, &chunk_options, &chunk_size, &chunk_checksum, &chunk_extra);

		if ((chunk_id == 1) && (chunk_size > 10) && (chunk_size < BLOCK_SIZE)) {
			fread(buffer, 1, chunk_size, in);
			checksum = update_adler32(1L, buffer, chunk_size);

			if (checksum != chunk_checksum) {
				printf("\nError: checksum mismatch!\n");
				printf("Got %08lX Expecting %08lX\n", checksum, chunk_checksum);
				fclose(in);
				return -1;
			}

			total_extracted = 0;

			decompressed_size = readU32(buffer);

			file_name_length = (int)readU16(buffer + 8);
			if (file_name_length > (int)chunk_size - 10)
				file_name_length = chunk_size - 10;

			output_file_name = (char*)malloc(file_name_length + 1);
			memset(output_file_name, 0, file_name_length + 1);
			for (c = 0; c < file_name_length; c++)
				output_file_name[c] = buffer[10 + c];

			/* check if already exists */
			out = fopen(output_file_name, "rb");
			if (out) {
				printf("File %s already exists. Skipped.\n", output_file_name);
				fclose(out);
				return -1;
			} else {
				/* create the file */
				out = fopen(output_file_name, "wb");
				if (!out) {
					printf("Can't create file %s. Skipped.\n", output_file_name);
					return -1;
				}
			}
		}

		if ((chunk_id == 17) && out && output_file_name && decompressed_size) {
			/* enlarge input buffer if necessary */
			if (chunk_size > compressed_bufsize) {
				compressed_bufsize = chunk_size;
				free(compressed_buffer);
				compressed_buffer = (unsigned char*)malloc(compressed_bufsize);
			}

			/* enlarge output buffer if necessary */
			if (chunk_extra > decompressed_bufsize) {
				decompressed_bufsize = chunk_extra;
				free(decompressed_buffer);
				decompressed_buffer = (unsigned char*)malloc(decompressed_bufsize);
			}

			/* read and check checksum */
			fread(compressed_buffer, 1, chunk_size, in);
			checksum = update_adler32(1L, compressed_buffer, chunk_size);
			total_extracted += chunk_extra;

			/* verify that the chunk data is correct */
			if (checksum != chunk_checksum) {
				printf("\nError: checksum mismatch. Skipped.\n");
				printf("Got %08lX Expecting %08lX\n", checksum, chunk_checksum);
				return -1;
			} else {
				/* decompress and verify */
				remaining = lz77_decompress(compressed_buffer, chunk_size, decompressed_buffer, chunk_extra);
				if (remaining != chunk_extra) {
					printf("\nError: decompression failed. Skipped.\n");
					return -1;
				} else {
					fwrite(decompressed_buffer, 1, chunk_extra, out);
				}
			}
		}

		/* position of next chunk */
		fseek(in, pos + 16 + chunk_size, SEEK_SET);
	}

	/* free allocated stuff */
	free(compressed_buffer);
	free(decompressed_buffer);
	free(output_file_name);

	/* close working files */
	if (out)
		fclose(out);
	fclose(in);

	return 0;
}

void usage(void)
{
	printf("phyunzip: uncompress phyzip archive\n");
	printf("\n");
	printf("Usage: phyunzip archive-file\n");
	printf("\n");
}

int main(int argc, char **argv)
{
	int i;
	const char* archive_file;

	if (argc == 1) {
		usage();
		return 0;
	}

	for (i = 1; i <= argc; i++) {
		char* argument = argv[i];

		if (!argument)
			continue;

		if (!strcmp(argument, "-h") || !strcmp(argument, "--help")) {
			usage();
			return 0;
		}

		if (!strcmp(argument, "-v") || !strcmp(argument, "--version")) {
			printf("phyunzip: high-speed file compression tool\n");
			printf("Version %s (using LZ77 %s)\n", PHYZIP_VERSION_STRING, LZ77_VERSION_STRING);
			printf("\n");
			return 0;
		}

		/* unknown option */
		if (argument[0] == '-') {
			printf("Error: unknown option %s\n\n", argument);
			printf("To get help on usage:\n");
			printf("  phyunzip --help\n\n");
			return -1;
		}

	}

	/* needs at least two arguments */
	if (argc <= 1) {
		usage();
		return 0;
	}

	archive_file = argv[1];

	return unpack_file(archive_file);
}
