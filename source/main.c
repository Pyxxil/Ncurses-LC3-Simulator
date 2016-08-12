#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdio.h>

#include "Argparse.h"
#include "Machine.h"
#include "Structs.h"

static struct program prog = {
	.name 	 = NULL,
	.infile  = NULL,
	.outfile = NULL,
};


/*
 * Give an accurate error description.
 */

static void handle(int errval)
{
	if (errval & MUL_INCORRECT_OPT) {
		fprintf(stderr, "\t- An incorrect opt was supplied.\n");
	} else if (errval & INCORRECT_OPT) {
		fprintf(stderr, "\t- The following flags you wanted to use are "
				"incorrect.\n\t\t- %s\n", incorrect_opts);
	}

	if (errval & MUL_INPUT_FILES) {
		fprintf(stderr, "\t- Multiple input files were given.\n"
				"\t  - The following were interpreted as input files.\n"
				"\t\t- %s\n", input_files);
	}

	if (errval & MUL_NOT_IMPLEMENTED) {
		fprintf(stderr, "\t- The following flags you wanted to use are "
				"not fully implemented.\n\t\t%s\n",
				unimplemented_opts);
	} else if (errval & NOT_IMPLEMENTED_OPT) {
		fprintf(stderr, "\t- The %s flag is not fully implemented. \n", unimplemented_opts);
	}
}


int main(int argc, char **argv)
{
	unsigned int errval = argparse(argc, argv, &prog);

	if (!errval)
		start_machine(&prog);
	else
		printf("%#x\n", errval); //handle(errval);

	tidyup(&prog);

	return errval;
}

