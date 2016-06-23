#include <string.h>
#include <stdio.h>
#include <getopt.h>	// For getopt_long

#include "Machine.h"

const char usage[] = "Usage: %s [-fch] file.\n";

// Some prototyped functions to use.
static void prompt_for_file(char *);

int main(int argc, char **argv)
{
	int opt, index;

	const char *short_options = "hf:a:o:";

	const struct option long_options[] = {
		{"assemble", required_argument, NULL, 'a'},
		{"file",     required_argument, NULL, 'f'},
		{"outfile",  required_argument, NULL, 'o'},
		{NULL,       0,                 NULL,  0 }
	};

	char file[256] = { 0 };

	while ((opt = getopt_long(argc, argv, short_options,
			long_options, NULL)) != -1) {
		switch (opt) {
		case 'f':
			for (index = 0; argv[optind - 1][index] != '\0'; ++index)
				file[index] = optarg[index];
			break;
		case 'a':
			break;
		case 'o':
			break;
		case '?':
		case 'h':
			printf(usage, argv[0]);
			return 0;

		default:
			break;
		}
	}

	if (optind == argc) {
		if (file[0] == '\0')
			prompt_for_file(file);
	} else if (optind < argc) {
		if (((optind + 1) == argc) && (file[0] == '\0')) {
			for (index = 0; argv[optind][index] != '\0'; ++index)
				file[index] = argv[optind][index];
		}
	} else {
		printf(usage, argv[0]);
		return 1;
	}

	start_machine(file);

	return 0;
}

static void prompt_for_file(char *file)
{
	/*
	 * The file wasn't supplied, so prompt the user for it.
	 *
	 * buffer -- Where we will store the file name.
	 * size   -- The size of the provided buffer.
	 */

	printf("Enter the name of the object file: ");
	fgets(file, 256, stdin);
	file[strlen(file) - 1] = '\0';
}

