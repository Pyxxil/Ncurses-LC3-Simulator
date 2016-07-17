#include <stdlib.h>
#include <stdio.h>

#include "Error.h"

/*
 * Failed to read to the end of the file for some reason,
 * so we want to complain and exit the program with a failed
 * status.
 */

void read_error()
{
	fprintf(stderr, "Error: Failed to read to end of file\n");
}

void tidyup(struct program *prog)
{
	if (prog->infile != NULL)
		free(prog->infile);
	if (prog->outfile != NULL)
		free(prog->outfile);
	if (prog->name != NULL)
		free(prog->name);
}
