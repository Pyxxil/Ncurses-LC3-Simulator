#ifdef __linux__
#define _XOPEN_SOURCE 500
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Error.h"
#include "Parser.h"
#include "Machine.h"
#include "OptParse.h"

static struct program *program = NULL;

__attribute__((noreturn)) static void close(int sig)
{
	(void) sig;

	if (NULL != program) {
		tidy_up(program);
	}

	freeTable(&tableHead);

	exit(EXIT_FAILURE);
}

__attribute__((noreturn)) static void usage(char const *const name)
{
	printf("Usage: %s [options]                                       \n\n"
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

	int opts = 0;

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
			.long_option = "objectfile",
			.short_option = 'f',
			.option = REQUIRED,
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
			if (returned_option.option == NONE) {
				fprintf(stderr, "Option --assemble requires a file.\n");
				exit(EXIT_FAILURE);
			}

			program->assemblyfile = strdup(returned_option.long_option);
			if (NULL == program->assemblyfile) {
				perror(argv[0]);
				exit(EXIT_FAILURE);
			}

			opts |= ASSEMBLE;
			break;
		case 'f':
			if (returned_option.option == NONE) {
				fprintf(stderr, "Option --objectfile requires a file.\n");
				exit(EXIT_FAILURE);
			}

			program->objectfile = strdup(returned_option.long_option);
			if (NULL == program->objectfile) {
				perror(argv[0]);
				exit(EXIT_FAILURE);
			}
			break;
		case 'o':
			opts |= ASSEMBLE_ONLY;
			break;
		case 'v':
			if (returned_option.option == OPTIONAL) {
				char *end = NULL;
				program->verbosity = (int) strtol(
					returned_option.long_option, &end, 10);
				if (*end) {
					printf("Invalid verbosity level: %s\n",
					       returned_option.long_option);
					exit(0);
				}
			} else {
				program->verbosity++;
			}
			break;
		case 'h':
			usage(argv[0]);
		default:
			if (returned_option.long_option != NULL) {
				fprintf(stderr, "Invalid opt: %s\n",
					returned_option.long_option);
			} else {
				fprintf(stderr, "Invalid opt: -%c\n",
					returned_option.short_option);
			}
			exit(EXIT_FAILURE);
		}
	}

	if (opts & ASSEMBLE && !parse(program)) {
		// NO_OPT
	} else if (!(opts & ASSEMBLE_ONLY)) {
		start_machine(&prog);
	}

	tidy_up(&prog);
	freeTable(&tableHead);

	return 0;
}

