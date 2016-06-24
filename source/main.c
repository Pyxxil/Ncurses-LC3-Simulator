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

static void usage(char const *const program_name)
{
	printf(usage_string, program_name);
	exit(EXIT_SUCCESS);
}

static void error_usage(char const *const program_name)
{
	printf(usage_string, program_name);
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	if (argc == 1)
		error_usage(argv[0]);

	int opt;
	size_t len;
	char *file = NULL;

	const char *short_options = "hf:a:o:";
	const struct option long_options[] = {
		{"assemble", required_argument, NULL, 'a'},
		{"file",     required_argument, NULL, 'f'},
		{"outfile",  required_argument, NULL, 'o'},
		{"help",           no_argument, NULL, 'h'},
		{NULL,                       0, NULL,  0 }
	};

	while ((opt = getopt_long(argc, argv, short_options,
			long_options, NULL)) != -1) {
		switch (opt) {
		case 'f':
			len = strlen(optarg);
			file = (char *) malloc(len + 1);
			strncpy(file, optarg, len);
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

	if (((optind + 1) == argc) && (file == NULL)) {
		// Assume the last argument is the file name
		len = strlen(argv[optind]);
		file = (char *) malloc(len + 1);
		strncpy(file, argv[optind], len);
		++optind;
	} else if (file == NULL) {
		error_usage(argv[0]);
	}

	if (optind < argc) {
		free(file);
		error_usage(argv[0]);
	}

	start_machine(file);

	free(file);

	return 0;
}

