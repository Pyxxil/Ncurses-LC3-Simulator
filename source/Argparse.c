#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "Argparse.h"
#include "Error.h"

static const char errprefix[] = "\n\t\t- ";

#define ERR(DO_MUL, ON, errc, errv, errstring, arg) \
	do { 			\
		if ((DO_MUL)) { 						\
			*errv |= MUL_##ON; 					\
		} else { 							\
			(*(errc))++; 						\
			*(errv) |= (ON); 					\
		} 								\
		if (*(errstring) == NULL) {					\
			strmcpy(errstring, errprefix);				\
			*(errstring) = (char *) realloc(*errstring,		\
				sizeof(char) * (strlen(errprefix) + 1 + 	\
				strlen(arg)));					\
			strcat(*(errstring), arg);				\
		} else { 							\
			*(errstring) = (char *) realloc(*(errstring), 		\
				sizeof(char) * (strlen(*(errstring)) + 1 + 	\
				strlen(errprefix) + strlen(arg))); 		\
			strcat(*(errstring), errprefix); 			\
			strcat(*(errstring), arg); 				\
		} 								\
	} while (0)


static const char _usage[] =
	"Usage: %s [OPTION] <file>.\n"
	"  -h, --help           show this help text.                          \n"
	"  -f, --infile File    specify the input file to use.                \n"
	"  -o, --outfile File   specify the prefix of the output file to write\n"
	"                       any given assembly file's output to.          \n"
	"                       If this is not supplied, the prefix of the    \n"
	"                       input file is used.                           \n"
	"                       E.g. -o out results in the following files    \n"
	"                       being written:                                \n"
	"                           out.obj (The file used to run the program)\n"
	"                           out.bin (A file full of binary values that\n"
	"                                    correspond to the value at that  \n"
	"                                    address in the program)          \n"
	"                           out.hex (Same as the bin file, but in hex)\n"
	"                           out.sym (A file with labels and their     \n"
	"                                    corresponding address)           \n"
	"  -a, --assemble File  assemble the given file into a .obj file,     \n"
	"                       a .sym file, a .hex file, and a .bin file.    \n"
	"  -l. --log-file File  specify which file to use as a log file when. \n";


/*
 * This should only be printed when the user passes -h as a flag.
 */

static void usage(const char *prog_name)
{
	printf(_usage, prog_name);
}


/*
 * Copy the contents of one string to another, allocating enough memory to the
 * string we want to copy to.
 */

static inline void strmcpy(char **to, const char *from)
{
	size_t len = strlen(from) + 1;
	*to = (char *) malloc(sizeof(char) * len);
	strncpy(*to, from, len);
}


/*
 * Given the argument count, and each argument, go through each argument and
 * compare it with ones we want, and if so, do some pre-defined operation
 * involving that value.
 *
 * Returns:
 * 	- 0 if no errors were found
 * 	- A bitmask of the error value if one (or more) was found.
 */

unsigned int argparse(int argcount, char **argvals, struct program *prog)
{
	int argindex = 1;

	unsigned int errcount = 0,
		errvalue = 0;

	// First argument is program name.
	strmcpy(&prog->name, argvals[0]);

	char const *currentarg = (char const *) NULL;

	while (argindex < argcount) {
		currentarg = argvals[argindex];

		if (!strcmp(currentarg, "--help") || !strcmp(currentarg, "-h")) {
			errvalue |= HELP_FLAG;
			return errvalue;
		} else if (!strcmp(currentarg, "--assemble") || !strcmp(currentarg, "-a")) {
			if (argindex + 1 == argcount || argvals[argindex + 1][0] == '-') {
				ERR(errvalue & NO_ARG_PROVIDED, NO_ARG_PROVIDED,
					&errcount, &errvalue, &no_args_provided, currentarg);
				argindex++;
			} else {
				// TODO: Implement this opt
				argindex += 2;
			}
		} else if (!strcmp(currentarg, "--infile")   || !strcmp(currentarg, "-f")) {
			if (argindex + 1 == argcount || argvals[argindex + 1][0] == '-') {
				ERR(errvalue & NO_ARG_PROVIDED, NO_ARG_PROVIDED, \
					&errcount, &errvalue, &no_args_provided, currentarg);
				argindex++;
			} else {
				if (prog->infile == NULL) {
					strmcpy(&prog->infile, argvals[argindex + 1]);
				} else {
					ERR(0, MUL_INPUT_FILES, &errcount, &errvalue,
						&input_files, currentarg);
				}
				argindex += 2;
			}

		} else if (!strcmp(currentarg, "--outfile")  || !strcmp(currentarg, "-o")) {

		} else if (!strcmp(currentarg, "--logfile")  || !strcmp(currentarg, "-l")) {

		} else {
			argindex++;
			if (currentarg[0] != '-') {
				if (prog->infile == NULL) {
					strmcpy(&prog->infile, currentarg);
				} else {
					ERR(0, MUL_INPUT_FILES, &errcount, &errvalue,
						&input_files, currentarg);
				}
			} else if (errvalue & INCORRECT_OPT) {
				errvalue |= MUL_INCORRECT_OPT;
			} else {
				errvalue |= INCORRECT_OPT;
				errcount++;
			}
		}
	}

	return errvalue;
}

