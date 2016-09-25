#include <stdlib.h>
#include <stdio.h>

#include "Error.h"

/*
 * Failed to read to the end of the file for some reason,
 * so we want to complain and exit the program with a failed
 * status.
 */

char *input_files       = NULL,
	*incorrect_opts     = NULL,
	*unimplemented_opts = NULL,
	*deprecated_opts    = NULL,
	*no_args_provided   = NULL;


inline void read_error()
{
	fprintf(stderr, "Error: Failed to read to end of file\n");
}


void tidyup(struct program *prog)
{
	if (prog->name != NULL)
		free(prog->name);
	if (input_files != NULL)
		free(input_files);
	if (prog->objfile != NULL)
		free(prog->objfile);
	if (prog->logfile != NULL)
		free(prog->logfile);
	if (incorrect_opts != NULL)
		free(incorrect_opts);
	if (deprecated_opts != NULL)
		free(deprecated_opts);
	if (unimplemented_opts != NULL)
		free(unimplemented_opts);
}
