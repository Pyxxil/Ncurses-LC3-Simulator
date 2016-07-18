#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "Machine.h"
#include "Structs.h"
#include "Error.h"

const char usage_string[] =
	"Usage: %s [OPTION] <file>.\n"
	"  -h, --help            show this help text\n"
	"  -f, --file File       specify the input file to use\n"
	"  -o, --outFile File    specify the output file to write any given\n"
	"                          assembly file's output to\n"
	"  -a, --assemble File   assemble the given file into a .obj file,\n"
	"                          .sym file, and a .bin file\n"
	"  -l. --log-file File   specify which file to use as a log file when\n";

static void usage(FILE *file, struct program *prog)
{
	fprintf(file, usage_string, prog->name);
}

/*
 * Copy the contents of one string to another, allocating enough memory to the
 * string we want to copy to.
 */

static void strmcpy(char **to, const char *from)
{
	size_t len = strlen(from) + 1;
	*to = malloc(sizeof(char) * len);
	strncpy(*to, from, len);
}

int main(int argc, char **argv)
{
	struct program prog = {
		.name	 = NULL,
		.infile	 = NULL,
		.outfile = NULL,
	};

	int opt, ret = EXIT_SUCCESS;

	const char *short_options = "hf:a:o:l:";
	const struct option long_options[] = {
		{ "assemble", required_argument, NULL, 'a' },
		{ "file",     required_argument, NULL, 'f' },
		{ "outfile",  required_argument, NULL, 'o' },
		{ "logfile",  required_argument, NULL, 'l' },
		{ "help",     no_argument,	 NULL, 'h' },
		{ NULL,			      0, NULL,	 0 },
	};

	while ((opt = getopt_long(argc, argv, short_options,
				  long_options, NULL)) != -1) {
		switch (opt) {
		case 'f':
			if (prog.infile != NULL) {
				fprintf(stderr, "Error: Multiple input files given.\n");
				tidyup(&prog);
				return EXIT_FAILURE;
			} else {
				strmcpy(&(prog.infile), optarg);
			}
			break;
		case 'a': // FALLTHROUGH
		case 'o': // FALLTHROUGH
		case 'l':
			break;
		case '?':
		case 'h':
			usage(stdin, &prog);
			tidyup(&prog);
			return EXIT_SUCCESS;
		default:
			break;
		}
	}

	// Assume the last argument is the file name.
	if ((argc > 1) && ((optind + 1) == argc) && (prog.infile == NULL)) {
		strmcpy(&(prog.infile), argv[optind]);
		optind++;
	}

	if (optind < argc) {
		usage(stderr, &prog);
		ret = EXIT_FAILURE;
	} else {
		run_machine(&prog);
	}

	tidyup(&prog);

	return ret;
}
