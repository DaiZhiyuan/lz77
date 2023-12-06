#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lz77.h"

#define MAX_FILE_SIZE (100 * 1024 * 1024)
#define LOG
#undef LOG

int compare(const char* name, const uint8_t* a, const uint8_t* b, int size)
{
	int bad = 0;
	int i;

	for (i = 0; i < size; ++i) {
		if (a[i] != b[i]) {
			bad = 1;
			printf("Error on %s!\n", name);
			printf("Different at index %d: expecting %02x,actual %02x\n", i, a[i], b[i]);
			break;
		}
	}

	return bad;
}

void test_roundtrip_lz77(const char* name, const char* file_name)
{
#ifdef LOG
	printf("Processing %s...\n", name);
#endif
	FILE* f = fopen(file_name, "rb");
	if (!f) {
		printf("Error: can not open %s!\n", file_name);
		exit(1);
	}
	fseek(f, 0L, SEEK_END);
	long file_size = ftell(f);
	rewind(f);

#ifdef LOG
	printf("Size is %ld bytes.\n", file_size);
#endif
	if (file_size > MAX_FILE_SIZE) {
		fclose(f);
		printf("%25s %10ld [skipped, file too big]\n", name, file_size);
		return;
	}

	uint8_t* file_buffer = malloc(file_size);
	long read = fread(file_buffer, 1, file_size, f);
	fclose(f);
	if (read != file_size) {
		free(file_buffer);
		printf("Error: only read %ld bytes!\n", read);
		exit(1);
	}

#ifdef LOG
	printf("Compressing. Please wait...\n");
#endif
	uint8_t* compressed_buffer = malloc(1.05 * file_size);
	int compressed_size = lz77_compress(file_buffer, file_size, compressed_buffer);
	double ratio = (100.0 * compressed_size) / file_size;
#ifdef LOG
	printf("Compressing was completed: %ld -> %d (%.2f%%)\n", file_size, compressed_size, ratio);
#endif

#ifdef LOG
	printf("Decompressing. Please wait...\n");
#endif
	uint8_t* uncompressed_buffer = malloc(file_size);
	if (uncompressed_buffer == NULL) {
		printf("%25s %10ld  -> %10d  (%.2f%%)  skipped, can't decompress\n", name, file_size, compressed_size, ratio);
		return;
	}
	memset(uncompressed_buffer, '-', file_size);
	lz77_decompress(compressed_buffer, compressed_size, uncompressed_buffer, file_size);
#ifdef LOG
	printf("Comparing. Please wait...\n");
#endif
	int result = compare(file_name, file_buffer, uncompressed_buffer, file_size);
	if (result == 1) {
		free(uncompressed_buffer);
		exit(1);
	}

	free(file_buffer);
	free(compressed_buffer);
	free(uncompressed_buffer);
#ifdef LOG
	printf("OK.\n");
#else
	printf("%25s %10ld  -> %10d  (%.2f%%)\n", name, file_size, compressed_size, ratio);
#endif
	return;
}

int main(int argc, char** argv)
{
	const char* default_prefix = "../dataset/";
	const char* names[] = {"canterbury/alice29.txt",
		"canterbury/asyoulik.txt",
		"canterbury/cp.html",
		"canterbury/fields.c",
		"canterbury/grammar.lsp",
		"canterbury/kennedy.xls",
		"canterbury/lcet10.txt",
		"canterbury/plrabn12.txt",
		"canterbury/ptt5",
		"canterbury/sum",
		"canterbury/xargs.1",
		"silesia/dickens",
		"silesia/mozilla",
		"silesia/mr",
		"silesia/nci",
		"silesia/ooffice",
		"silesia/osdb",
		"silesia/reymont",
		"silesia/samba",
		"silesia/sao",
		"silesia/webster",
		"silesia/x-ray",
		"silesia/xml",
		"enwik/enwik8.txt"};

	const char* prefix = (argc == 2) ? argv[1] : default_prefix;

	const int count = sizeof(names) / sizeof(names[0]);
	int i;

	printf("Test round-trip for lz77\n\n");
	for (i = 0; i < count; ++i) {
		const char* name = names[i];
		char* filename = malloc(strlen(prefix) + strlen(name) + 1);
		strcpy(filename, prefix);
		strcat(filename, name);
		test_roundtrip_lz77(name, filename);
		free(filename);
	}
	printf("\n");

	return 0;
}
