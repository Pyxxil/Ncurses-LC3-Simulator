#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include "Error.h"
#include "Parser.h"
#include "Machine.h"
#include "Structs.h"
#include "Argparse.h"

static struct program *program = NULL;

 __attribute__((noreturn)) static void close(int sig)
{
	(void) sig;

	if (NULL != program)
		tidyup(program);

	freeTable(&tableHead);

	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	unsigned long long errval;

	struct program prog = {
		.name         = NULL,
		.logfile      = NULL,
		.assemblyfile = NULL,
		.symbolfile   = NULL,
		.hexoutfile   = NULL,
		.binoutfile   = NULL,
		.objectfile   = NULL,
		.verbosity    = 0,
	};

	program = &prog;

	errval = argparse(argc, argv, &prog);
	signal(SIGINT, close);

	if (errval & (0xFFFFll << 32)) {
		errhandle(&prog);
	} else {
		if ((errval & ASSEMBLE) && (!parse(&prog) ||
				(errval & ASSEMBLE_ONLY))) {
			/* NO OP */;
		} else {
			start_machine(&prog);
		}
	}

	tidyup(&prog);
	freeTable(&tableHead);

	return 0;
}

