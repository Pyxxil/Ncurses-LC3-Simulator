#include <string.h>
#include <getopt.h>	// For getopt_long
#include <stdlib.h>
#include <stdio.h>

#include "Machine.h"

const char usage_string[] =
	"Usage: %s [OPTION] file.\n"
	"  -f, --file file         specify the input file to use\n"
	"  -o, --outfile file      specify the outputfile to write any given\n"
	"                            assembly file's output to\n"
	"  -a, --assemble file     assemble the given file into a .obj file,\n"
	"                            .sym file, and a .bin file\n";


static void prompt_for_file(char *);

static void usage(char *program_name)
{
	printf(usage_string, program_name);
	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	if (argc == 1) {
		printf(usage_string, argv[0]);
		return 1;
	}

	int opt, index;

	const char *short_options = "hf:a:o:";

	const struct option long_options[] = {
		{"assemble", required_argument, NULL, 'a'},
		{"file",     required_argument, NULL, 'f'},
		{"outfile",  required_argument, NULL, 'o'},
		{"help",           no_argument, NULL, 'h'},
		{NULL,                       0, NULL,  0 }
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
			usage(argv[0]);
		default:
			break;
		}
	}

	if (optind < argc) {
		if (((optind + 1) == argc) && (file[0] == '\0')) {
			for (index = 0; argv[optind][index] != '\0'; ++index)
				file[index] = argv[optind][index];
			++optind;
		}
	}

	if (optind < argc)
		usage(argv[0]);

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

