#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

#include "Argparse.h"
#include "Error.h"

static const char errprefix[] = "\n\t";
static size_t prefixsize;

static unsigned long long errcount, errvalue;

static const char _usage[] =
	"Usage: %s [OPTION] <file>.                                           \n"
	"  -h, --help           show this help text.                          \n"
	"  -a, --assemble File  assemble a file into a .obj file, a .sym file,\n"
	"                       a .hex file, and a .bin file.                 \n"
	"  -l. --log-file File  specify which file to use as a log file when. \n"
	"  -o, --objectfile     specify the object file to read from.         \n"
;

static void ERR(char const *const string, ...)
{
	va_list args;
	va_start(args, string);

	fprintf(stderr, "\n");
	fprintf(stderr, string, args);
	fprintf(stderr, "\n");

	va_end(args);
}

/*
 * This should only be printed when the user passes -h or --help as a flag.
 */

static inline void usage(char const *prog_name)
{
	printf(_usage, prog_name);
}

/*
 * Copy the contents of one string to another, allocating enough memory to the
 * string we want to copy to.
 */

static inline void strmcpy(char **to, char const *from)
{
	size_t len = strlen(from) + 1;
	*to = calloc(len, sizeof(char));
	strncpy(*to, from, len);
}

/*
 * When we hit an error, we want to update the error count, the error value, and
 * the string we'll print when we throw an error.
 */

static void error(unsigned long long error, unsigned long long mul,
		char **errorString, char const *arg)
{
	errvalue |= errvalue & error ? mul : error;
	errcount ++;

	if (*errorString == NULL) {
		strmcpy(errorString, arg);
		return;
	}

	*errorString = (char *) realloc(*errorString, sizeof(char) * \
		(strlen(*errorString) + 1 + prefixsize + strlen(arg)));
	strcat(*errorString, errprefix);
	strcat(*errorString, arg);
}

void errhandle(struct program const *prog)
{
	if (errcount > 1) {
		ERR("There were some error's while parsing your options.", 0);
	} else {
		ERR("There was an error while parsing your options.");
	}

	if (errvalue & MUL_INPUT_FILES) {
		ERR("The following files were seen as input files and couldn't "
			"be decided upon: %s%s %s%s",
			errprefix, prog->objectfile, errprefix, input_files);
	}

	if (errvalue & MUL_INCORRECT_OPT) {
		ERR("The following options weren't recognised: %s%s", errprefix,
			incorrect_opts);
	} else if (errvalue & INCORRECT_OPT) {
		ERR("%s was not recognised.", incorrect_opts);
	}

	if (errvalue & MUL_NO_ARG_PROVIDED) {
		ERR("The following options require arguments: %s%s", errprefix,
			no_args_provided);
	} else if (errvalue & NO_ARG_PROVIDED) {
		ERR("%s requires an argument.", no_args_provided);
	}
}

static void addFile(char **file, char const *from, char const *flag)
{
	if (*file == NULL)
		strmcpy(file, from);
	else
		error(MUL_INPUT_FILES, MUL_INPUT_FILES, &input_files, flag);
}

/*
 * Given the argument count, and each argument, go through each argument and
 * compare it with ones we want, and if so, do some pre-defined operation
 * involving that value.
 *
 * Returns:
 *    - 0 if no errors were found
 *    - A bitmask of the error value if one (or more) was found.
 */

unsigned long long argparse(int argcount, char **argvals, struct program *prog)
{
	prefixsize = strlen(errprefix);
	int argindex = 1;

	strmcpy(&prog->name, argvals[0]);

	char const *arg = (char const *) NULL;

	while (argindex < argcount) {
		arg = argvals[argindex++];

		if (!strcmp(arg, "--help") || !strcmp(arg, "-h")) {
			usage(prog->name);
			exit(EXIT_SUCCESS);
		} else if (!strcmp(arg, "--assemble") || !strcmp(arg, "-a")) {
			if (argindex >= argcount || *argvals[argindex] == '-') {
				error(NO_ARG_PROVIDED, MUL_NO_ARG_PROVIDED,
					&no_args_provided, arg);
			} else {
				errvalue |= ASSEMBLE;
				addFile(&prog->assemblyfile, argvals[argindex],
					arg);
				argindex++;
			}
		} else if (!strcmp(arg, "--objectfile") || !strcmp(arg, "-o")) {
			if (argindex >= argcount || *argvals[argindex] == '-') {
				error(NO_ARG_PROVIDED, MUL_NO_ARG_PROVIDED,
					&no_args_provided, arg);
			} else {
				addFile(&prog->objectfile, argvals[argindex],
					arg);
				argindex++;
			}
		} else if (!strcmp(arg, "--logfile") || !strcmp(arg, "-l")) {
			if (argindex >= argcount || *argvals[argindex] == '-') {
				error(NO_ARG_PROVIDED, MUL_NO_ARG_PROVIDED,
					&no_args_provided, arg);
			} else {
				addFile(&prog->logfile, argvals[argindex], arg);
				argindex++;
			}
		} else if (!strcmp(arg, "--assemble-only")) {
			errvalue |= ASSEMBLE_ONLY;
		} else if (!strcmp(arg, "--verbose") || !strcmp(arg, "-v")) {
			// Should we check for verbosity first before gathering
			// all information?
			char *end = NULL;
			if (argindex < argcount && *argvals[argindex] != '-') {
				int verboseLevel = strtol(argvals[argindex++],
						&end, 10);
				if (*end) {
					error(INVALID_VERBOSE_LEVEL,
						MUL_INVALID_VERBOSE_LEVEL,
						&incorrect_opts, arg);
					continue;
				} else {
					prog->verbosity = verboseLevel;
				}
			} else {
				prog->verbosity++;
			}
			if (prog->verbosity > 3) {
				printf("Note: A verbosity greater than 3 is "
					"superfluous.\n");
			}
		} else {
			error(INCORRECT_OPT, MUL_INCORRECT_OPT,
				&incorrect_opts, arg);
		}
	}

	return errvalue;
}

