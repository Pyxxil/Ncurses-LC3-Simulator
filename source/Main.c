#include <stdlib.h>

#include "Argparse.h"
#include "Machine.h"
#include "Structs.h"


int main(int argc, char **argv)
{
	struct program prog = {
		.name    = NULL,
		.objfile = NULL,
		.logfile = NULL,
	};

	unsigned long long errval = argparse(argc, argv, &prog);

	if (errval & (0xFFFFll << 32)) {
		errhandle(&prog);
	} else {
		start_machine(&prog);
	}

	tidyup(&prog);

	return errval;
}

