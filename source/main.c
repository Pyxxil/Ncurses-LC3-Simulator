#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

#include "Machine.h"
#include "Structs.h"

static const char usage_string[] =
	"Usage: %s [OPTION] inFile.\n"
	"  -h, --help            show this help text\n"
	"  -f, --file File       specify the input file to use\n"
	"  -o, --outFile File    specify the output file to write any given\n"
	"                          assembly file's output to\n"
	"  -a, --assemble File   assemble the given file into a .obj file,\n"
	"                          .sym file, and a .bin file\n";

static void usage(struct program *prog)
{
	printf(usage_string, prog->name);
}

static void prompt_flag_not_implemented(const char flag)
{
	printf("''%c' is currently not currently implemented.", flag);
}

int main(int argc, char **argv)
{
	struct program prog = {
		.name	 = *argv,
		.infile	 = NULL,
		.outfile = NULL,
	};

	if (argc == 1) {
		prog.errno = EXIT_FAILURE;
		usage(&prog);
		return prog.errno;
	}

	int opt;
	size_t len;

	const char *short_options = "hf:a:o:";
	const struct option long_options[] = {
		{ "assemble", required_argument, NULL, 'a' },
		{ "file",     required_argument, NULL, 'f' },
		{ "outfile",  required_argument, NULL, 'o' },
		{ "help",     no_argument,	 NULL, 'h' },
		{ NULL,			      0, NULL,	 0 },
	};

	while ((opt = getopt_long(argc, argv, short_options,
				  long_options, NULL)) != -1)
	{
		switch (opt) {
		case 'f':
			len	    = strlen(optarg);
			prog.infile = (char *) malloc(len + 1);
			strncpy(prog.infile, optarg, len);
			break;
		case 'a':
			prompt_flag_not_implemented('a');
			break;
		case 'o':
			prompt_flag_not_implemented('o');
			break;
		case '?':
		case 'h':
			prog.errno = EXIT_SUCCESS;
			usage(&prog);
			return prog.errno;

		default:
			break;
		}
	}

	if (((optind + 1) == argc) && (prog.infile == NULL)) {
	// Assume the last argument is the inFile name
		len	    = strlen(argv[optind]);
		prog.infile = (char *) malloc(len + 1);
		strncpy(prog.infile, argv[optind], len);
		++optind;
	} else if (prog.infile == NULL) {
		prog.errno = EXIT_FAILURE;
		usage(&prog);
		return prog.errno;
	}

	if (optind < argc) {
		free(prog.infile);
		prog.errno = EXIT_FAILURE;
		usage(&prog);
		return prog.errno;
	}

	start_machine(&prog);

	free(prog.infile);

	return prog.errno;
} /* main */
