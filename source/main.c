#include <string.h>
#include <getopt.h>
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

static void usage(char *program_name)
{
	printf(usage_string, program_name);
	exit(EXIT_SUCCESS);
}

static void error_usage(char *program_name)
{
	printf(usage_string, program_name);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	if (argc == 1)
		error_usage(argv[0]);

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
		if (((optind + 1) == argc) && (file[0] == '\0'))
			strcpy(file, argv[optind++]);
		else
			error_usage(argv[0]);
	}

	if (optind < argc)
		error_usage(argv[0]);

	start_machine(file);

	return 0;
}

