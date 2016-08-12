#include <stdlib.h>

#include "Argparse.h"
#include "Machine.h"
#include "Structs.h"


int main(int argc, char **argv)
{
	struct program prog = {
		.name 	 = NULL,
		.infile  = NULL,
		.outfile = NULL,
	};

	unsigned int errval = argparse(argc, argv, &prog);

	if (!errval)
		start_machine(&prog);

	tidyup(&prog);

	return errval;
}

