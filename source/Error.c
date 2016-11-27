#include <stdlib.h>
#include <stdio.h>

#include "Error.h"

char *input_files        = NULL;
char *incorrect_opts     = NULL;
char *deprecated_opts    = NULL;
char *no_args_provided   = NULL;
char *unimplemented_opts = NULL;

/*
 * Failed to read to the end of the file for some reason,
 * so we want to complain and exit the program with a failed
 * status.
 */

inline void read_error(void)
{
	fprintf(stderr, "Error: Failed to read to end of file\n");
	exit(EXIT_FAILURE);
}


void tidyup(struct program *prog)
{
	if (NULL != prog->name)
		free(prog->name);
	if (NULL != input_files)
		free(input_files);
	if (NULL != prog->objectfile)
		free(prog->objectfile);
	if (NULL != prog->assemblyfile)
		free(prog->assemblyfile);
	if (NULL != prog->symbolfile)
		free(prog->symbolfile);
	if (NULL != prog->hexoutfile)
		free(prog->hexoutfile);
	if (NULL != prog->binoutfile)
		free(prog->binoutfile);
	if (NULL != prog->logfile)
		free(prog->logfile);
	if (NULL != incorrect_opts)
		free(incorrect_opts);
	if (NULL != deprecated_opts)
		free(deprecated_opts);
	if (NULL != unimplemented_opts)
		free(unimplemented_opts);
}
