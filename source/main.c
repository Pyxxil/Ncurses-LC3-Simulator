#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "Machine.h"
#include "Structs.h"

const char usage_string[] =
	"Usage: %s [OPTION] inFile.\n"
	"  -h, --help            show this help text\n"
	"  -f, --file File       specify the input file to use\n"
	"  -o, --outFile File    specify the output file to write any given\n"
	"                          assembly file's output to\n"
	"  -a, --assemble File   assemble the given file into a .obj file,\n"
	"                          .sym file, and a .bin file\n"
	"  -l. --log-file File   specify which file to use as a log file when\n";

static void usage(struct program *prog)
{
	printf(usage_string, prog->name);
}

int main(int argc, char **argv)
{
	struct program prog = {
		.name	 = *argv,
		.infile	 = NULL,
		.outfile = NULL,
	};

	int opt;
	size_t len;

	const char *short_options = "hf:a:o:l:";
	const struct option long_options[] = {
		{ "assemble", required_argument, NULL, 'a' },
		{ "file",     required_argument, NULL, 'f' },
		{ "outfile",  required_argument, NULL, 'o' },
		{ "help",     no_argument,	 NULL, 'h' },
		{ "logfile",  required_argument, NULL, 'l' },
		{ NULL,			      0, NULL,	 0 },
	};

	while ((opt = getopt_long(argc, argv, short_options,
				  long_options, NULL)) != -1) {
		switch (opt) {
		case 'f':
			len	    = strlen(optarg);
			prog.infile = (char *) malloc(sizeof(char) * (len + 1));
			strncpy(prog.infile, optarg, len);
			break;
		case 'a':
			break;
		case 'o':
			break;
		case 'l':
			break;
		case '?':
		case 'h':
			usage(&prog);
			return EXIT_SUCCESS;
		default:
			break;
		}
	}

	if ((argc != 1) && ((optind + 1) == argc) && (prog.infile == NULL)) {
	// Assume the last argument is the file name.
		len	    = strlen(argv[optind]);
		prog.infile = (char *) malloc(sizeof(char) * (len + 1));
		strncpy(prog.infile, argv[optind], len);
		++optind;
	}

	if (optind < argc) {
		free(prog.infile);
		usage(&prog);
		return EXIT_FAILURE;
	}

	run_machine(&prog);

	free(prog.infile);

	return 0;
}
