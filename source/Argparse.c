#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "Argparse.h"
#include "Error.h"

static const char errprefix[] = "\n\t";
static size_t prefixsize;

static unsigned int errcount, errvalue;

static const char _usage[] =
	"Usage: %s [OPTION] <file>.                                           \n"
	"  -h, --help           show this help text.                          \n"
	"  -a, --assemble File  assemble a file into a .obj file, a .sym file,\n"
	"                       a .hex file, and a .bin file.                 \n"
	"  -l. --log-file File  specify which file to use as a log file when. \n"
	"  -o, --objfile        specify the object file to read from.         \n"
;


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
	*to = (char *) malloc(sizeof(char) * len);
	strncpy(*to, from, len);
}

/*
 * When we hit an error, we want to update the error count, the error value, and
 * the string we'll print when we throw an error.
 */

static void ERROR(unsigned long long error, unsigned int mul, char **errorString,
		char const *arg)
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
		fprintf(stderr, "ERROR: There were some error's while parsing "
				"your options.\n");
	} else {
		fprintf(stderr, "ERROR: There was an error while parsing your "
				"options.\n");
	}


	if (errvalue & MUL_INPUT_FILES) {
		fprintf(stderr, "\nThe following files were seen as input files "
				"and couldn't be decided upon:");
		fprintf(stderr, "%s%s",   errprefix, prog->objfile);
		fprintf(stderr, "%s%s\n", errprefix, input_files);
	}

	if (errvalue & MUL_INCORRECT_OPT) {
		fprintf(stderr, "\nThe following options weren't recognised:");
		fprintf(stderr, "%s%s\n", errprefix, incorrect_opts);
	} else if (errvalue & INCORRECT_OPT) {
		fprintf(stderr, "\n%s was not recognised.\n", incorrect_opts);
	}

	if (errvalue & MUL_NO_ARG_PROVIDED) {
		fprintf(stderr, "\nThe following options require arguments:");
		fprintf(stderr, "%s%s\n", errprefix, no_args_provided);
	} else if (errvalue & NO_ARG_PROVIDED) {
		fprintf(stderr, "\n%s requires an argument.\n",
			no_args_provided);
	}

	//if (errvalue & MUL_WARN_DEPRECATED) {
	//} else if (errvalue & WARN_DEPRECATED) {
	//}

	if (errvalue & MUL_WARN_UNIMPLEMENTED) {
		fprintf(stderr, "\nThe following options aren't fully "
				"implemented:");
		fprintf(stderr, "%s%s\n", errprefix, unimplemented_opts);
	} else if (errvalue & WARN_UNIMPLEMENTED) {
		fprintf(stderr, "\n%s is not fully implemented.\n",
			unimplemented_opts);
	}
}


static void addFile(char **file, char const *from, char const *flag)
{
	if (*file == NULL)
		strmcpy(file, from);
	else
		ERROR(MUL_INPUT_FILES, MUL_INPUT_FILES, &input_files, flag);
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

	// First argument is program name.
	strmcpy(&prog->name, argvals[0]);

	char const *arg = (char const *) NULL;

	while (argindex < argcount) {
		arg = argvals[argindex++];

		if (!strcmp(arg, "--help") || !strcmp(arg, "-h")) {
			usage(prog->name);
			exit(EXIT_SUCCESS);
		} else if (!strcmp(arg, "--assemble") || !strcmp(arg, "-a")) {
			// TODO: Implement this opt
			ERROR(WARN_UNIMPLEMENTED, MUL_WARN_UNIMPLEMENTED,
				&unimplemented_opts, arg);
		} else if (!strcmp(arg, "--objfile") || !strcmp(arg, "-o")) {
			if (argindex >= argcount || *argvals[argindex] == '-') {
				ERROR(NO_ARG_PROVIDED, MUL_NO_ARG_PROVIDED,
					&no_args_provided, arg);
			} else {
				addFile(&prog->objfile, argvals[argindex], arg);
				argindex++;
			}
		} else if (!strcmp(arg, "--logfile") || !strcmp(arg, "-l")) {
			if (argindex >= argcount || *argvals[argindex] == '-') {
				ERROR(NO_ARG_PROVIDED, MUL_NO_ARG_PROVIDED,
					&no_args_provided, arg);
			} else {
				addFile(&prog->logfile, argvals[argindex], arg);
				argindex++;
			}
		} else {
			ERROR(INCORRECT_OPT, MUL_INCORRECT_OPT,
				&incorrect_opts, arg);
		}
	}

	return errvalue;
}

