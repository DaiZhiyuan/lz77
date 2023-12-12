#include <stdio.h>
#include <string.h>

#define LZ77_VERSION_STRING "1.0"
#define PHYZIP_VERSION_STRING "1.2.3"

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

	printf("Input file: %s, Output file: %s\n", input_file, output_file);

	return 0;
}
