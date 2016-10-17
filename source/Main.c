#include <stdlib.h>
#include <stdio.h>

#include "Argparse.h"
#include "Machine.h"
#include "Structs.h"
#include "Parser.h"
#include "Error.h"

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

	unsigned long long errval = argparse(argc, argv, &prog);

	if (errval & (0xFFFFll << 32)) {
		errhandle(&prog);
	} else {
		if ((errval & ASSEMBLE) && !parse(&prog) ||
				(errval & ASSEMBLE_ONLY)) {
			/* NO OP */;
		} else {
			start_machine(&prog);
		}
	}

	freeTable(&tableHead);
	tidyup(&prog);

	return errval;
}

