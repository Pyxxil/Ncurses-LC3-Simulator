#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "Error.h"
#include "Parser.h"
#include "Machine.h"
#include "Structs.h"
#include "OptParse.h"

static struct program *program = NULL;

__attribute__((noreturn)) static void close(int sig)
{
	(void) sig;

	if (NULL != program)
		tidyup(program);

	freeTable(&tableHead);

	exit(EXIT_FAILURE);
}

__attribute__((noreturn)) static void usage(char const *const name)
{
	printf(
		"Usage: %s [options]                                       \n\n"
		"Options:                                                    \n"
		"  -a [--assemble] file   Assemble the given file.           \n"
		"  -v [--verbose] <level> Set the verbosity of the assembler.\n"
		"  -o [--assemble-only]   Only assemble the given program.   \n",
		name
	);

	exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
	struct program prog = {
		.name         = NULL,
		.logfile      = NULL,
		.assemblyfile = NULL,
		.symbolfile   = NULL,
		.hexoutfile   = NULL,
		.binoutfile   = NULL,
		.objectfile   = NULL,
		.verbosity    = 0,
	};

	program = &prog;

	signal(SIGINT, close);

	int err = 0;

	options _options[] = {
		{
			.long_option = "assemble",
			.short_option = 'a',
			.option = REQUIRED,
		},
		{
			.long_option = "assemble-only",
			.short_option = 'o',
			.option = NONE,
		},
		{
			.long_option = "verbose",
			.short_option = 'v',
			.option = OPTIONAL,
		},
		{
			.long_option = "help",
			.short_option = 'h',
			.option = NONE,
		},
		{
			NULL, '\0', NONE,
		},
	};

	int option = 0;

	while ((option = parse_options(_options, argc, argv)) != 0) {
		switch (option) {
		case 'a':
			program->assemblyfile = strdup(returned_option.long_option);
			err |= ASSEMBLE;
			break;
		case 'o':
			err |= ASSEMBLE_ONLY;
			break;
		case 'v':
			if (returned_option.option == OPTIONAL) {
				char *end = NULL;
				program->verbosity = (int) strtol(returned_option.long_option, &end, 10);
				if (*end) {
					printf("Invalid argument supplied for verbosity: %s\n",
							returned_option.long_option);
					exit(0);
				}
			} else {
				program->verbosity++;
			}
			break;
		case 'h':
			usage(argv[0]);
			break;
		default:
			break;
		}
	}

	if (err & ASSEMBLE && !parse(program)) {
		// NO_OPT
	} else if (!(err & ASSEMBLE_ONLY)) {
		start_machine(&prog);
	}

	tidyup(&prog);
	freeTable(&tableHead);

	return 0;
}

