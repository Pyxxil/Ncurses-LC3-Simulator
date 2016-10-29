#include <stdlib.h>
#include <stdio.h>

#include "Argparse.h"
#include "Machine.h"
#include "Structs.h"
#include "Parser.h"
#include "Error.h"

struct program* program = NULL;

void _exit(void)
{
	if (NULL != program)
		tidyup(program);

	freeTable(&tableHead);
}

int main(int argc, char **argv)
{
	struct program prog = {
		.name         = NULL,
		.logfile      = NULL,
		.objectfile   = NULL,
		.assemblyfile = NULL,
		.symbolfile   = NULL,
		.hexoutfile   = NULL,
		.binoutfile   = NULL,
		.objectfile   = NULL,
		.verbosity    = 0,
	};

	program = &prog;
	atexit(_exit);

	unsigned long long errval = argparse(argc, argv, &prog);

	if (errval & (0xFFFFll << 32)) {
		errhandle(program);
	} else {
		if ((errval & ASSEMBLE) && (!parse(program) ||
				(errval & ASSEMBLE_ONLY))) {
			/* NO OP */;
		} else {
			start_machine(program);
		}
	}

	return errval;
}

