#include <stdio.h>
#include <string.h>

#define LZ77_VERSION_STRING "1.0"
#define PHYZIP_VERSION_STRING "1.2.3"

#define BLOCK_SIZE (2 * 64 * 1024)

/* magic identifier for phyzip file */
static unsigned char phyzip_magic[8] = {'$', 'p', 'h', 'y', 'z', 'i', 'p', '$'};

void write_magic(FILE* file)
{
	fwrite(phyzip_magic, 8, 1, file);
}

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

void write_chunk_header(FILE* file, int id, int options, unsigned long size, unsigned long checksum, unsigned long extra)
{
	unsigned char buffer[16];

	buffer[0] = id & 255;
	buffer[1] = id >> 8;
	buffer[2] = options & 255;
	buffer[3] = options >> 8;
	buffer[4] = size & 255;
	buffer[5] = (size >> 8) & 255;
	buffer[6] = (size >> 16) & 255;
	buffer[7] = (size >> 24) & 255;
	buffer[8] = checksum & 255;
	buffer[9] = (checksum >> 8) & 255;
	buffer[10] = (checksum >> 16) & 255;
	buffer[11] = (checksum >> 24) & 255;
	buffer[12] = extra & 255;
	buffer[13] = (extra >> 8) & 255;
	buffer[14] = (extra >> 16) & 255;
	buffer[15] = (extra >> 24) & 255;

	fwrite(buffer, 16, 1, file);
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

int pack_file_compressed(const char* input_file, FILE* output_file)
{
	FILE *in;
	unsigned long fsize;
	const char* shown_name;
	unsigned char buffer[BLOCK_SIZE];
	unsigned long checksum;
	unsigned long total_compressed;

	in = fopen(input_file, "rb");
	if (!in) {
		printf("Error: could not open %s\n", input_file);
		return -1;
	}

	fseek(in, 0, SEEK_END);
	fsize = ftell(in);
	fseek(in, 0, SEEK_SET);

	if (detect_magic(in)) {
		printf("Error: file %s is already a phyzip archive!\n", input_file);
		fclose(in);
		return -1;
	}

	/* truncate directory prefix, e.g. "/path/to/FILE.txt" becomes "FILE.txt" */
	shown_name = input_file + strlen(input_file) - 1;
	while (shown_name > input_file)
		if (*(shown_name - 1) == '/')
			break;
		else
			shown_name--;

	printf("file name: %s\n", shown_name);

	buffer[0] = fsize & 255;
	buffer[1] = (fsize >> 8) & 255;
	buffer[2] = (fsize >> 16) & 255;
	buffer[3] = (fsize >> 24) & 255;
	buffer[4] = (fsize >> 32) & 255;
	buffer[5] = (fsize >> 40) & 255;
	buffer[6] = (fsize >> 48) & 255;
	buffer[7] = (fsize >> 56) & 255;
	buffer[8] = (strlen(shown_name) + 1) & 255;
	buffer[9] = (strlen(shown_name) + 1) >> 8;

	checksum = 1L;
	checksum = update_adler32(checksum, buffer, 10);
	printf("checksum: %lu\n", checksum);
	checksum = update_adler32(checksum, shown_name, strlen(shown_name) + 1);
	printf("checksum: %lu\n", checksum);
	write_chunk_header(output_file, 1, 0, 10 + strlen(shown_name) + 1, checksum, 0);
	fwrite(buffer, 10, 1, output_file);
	fwrite(shown_name, strlen(shown_name) + 1, 1, output_file);
	total_compressed = 16 + 10 + strlen(shown_name) + 1;

    /*
	 * 00000000  24 70 68 79 7a 69 70 24  01 00 00 00 0f 00 00 00  |$phyzip$........|
	 * 00000010  a5 01 21 06 00 00 00 00  09 00 00 00 00 00 00 00  |..!.............|
	 * 00000020  05 00 4e 6f 74 65 00                              |..Note.|
	 */
	printf("total compressed: %lu\n", total_compressed);

	return 0;
}

int pack_file(const char *input_file, const char *output_file)
{
	FILE *file;
	int result;

	file = fopen(output_file, "rb");
	if (file) {
		printf("Error: file %s already exists. Aborted.\n\n", output_file);
		fclose(file);
		return -1;
	}

	file = fopen(output_file, "wb");
	if (!file) {
		printf("Error: could not create %s. Aborted.\n\n", output_file);
		return -1;
	}

	write_magic(file);
	result = pack_file_compressed(input_file, file);
	fclose(file);

	return result;
}

void usage(void)
{
	printf("phyzip: high-speed file compression tool\n");
	printf("\n");
	printf("Usage: phyzip [options] input-file output-file\n");
	printf("\n");
	printf("Options:\n");
	printf("  -v    show program version\n");
	printf("\n");
}

int main(int argc, char **argv)
{
	int i;
	char *input_file = NULL;
	char *output_file = NULL;

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
			printf("phyzip: high-speed file compression tool\n");
			printf("Version %s (using LZ77 %s)\n", PHYZIP_VERSION_STRING, LZ77_VERSION_STRING);
			printf("\n");
			return 0;
		}

		/* unknown option */
		if (argument[0] == '-') {
			printf("Error: unknown option %s\n\n", argument);
			printf("To get help on usage:\n");
			printf("  phyzip --help\n\n");
			return -1;
		}

		/* first specified file is input */
		if (!input_file) {
			input_file = argument;
			continue;
		}

		/* next specified file is output */
		if (!output_file) {
			output_file = argument;
			continue;
		}
	}

	return pack_file(input_file, output_file);
}
