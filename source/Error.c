#include <stdlib.h>
#include <stdio.h>
#include "Error.h"

void usage(char *program)
{
	/*
	 *  The user didn't give enough arguments, so we want
	 *  to complain and exit the program with a failed status.
	 *
	 *  program -- The program name used when running the program.
	 */

	fprintf(stderr, "Usage: %s <file>\n", program);
	exit(EXIT_FAILURE);
}

void unable_to_open_file(char *file)
{
	/*
	 * The file failed to open for some reason, so we want
	 * to complain and exit the program with a failed status.
	 *
	 * file -- The file we tried to open
	 */

	fprintf(stderr, "Error: Failed to load file \"%s\"\n", file);
	exit(EXIT_FAILURE);
}

void read_error()
{
	/*
	 * Failed to read to the end of the file for some reason,
	 * so we want to complain and exit the program with a failed
	 * status.
	 */

	fprintf(stderr, "Error: Failed to read to end of file\n");
	exit(EXIT_FAILURE);
}
