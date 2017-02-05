#include <stdlib.h>
#include <stdio.h>

#include "Error.h"

/*
 * Failed to read to the end of the file for some reason,
 * so we want to complain and exit the program with a failed
 * status.
 */

__attribute__((noreturn)) inline void read_error(void)
{
        fprintf(stderr, "Error: Failed to read to end of file\n");
        exit(EXIT_FAILURE);
}


void tidyUp(struct program *program)
{
        if (NULL != program->name) {
                free(program->name);
        }
        if (NULL != program->objectfile) {
                free(program->objectfile);
        }
        if (NULL != program->assemblyfile) {
                free(program->assemblyfile);
        }
        if (NULL != program->symbolfile) {
                free(program->symbolfile);
        }
        if (NULL != program->hexoutfile) {
                free(program->hexoutfile);
        }
        if (NULL != program->binoutfile) {
                free(program->binoutfile);
        }
        if (NULL != program->logfile) {
                free(program->logfile);
        }
}

