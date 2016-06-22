#include <string.h>
#include <stdio.h>
#include <getopt.h>	// For getopt

#include "Machine.h"

// Some prototyped functions to use.
static void prompt_for_file(char *);

int main(int argc, char **argv)
{
	int opt, index;

	char file[256] = { 0 };

	while ((opt = getopt(argc, argv, "f:")) != -1) {
		switch (opt) {
		case 'f':
			for (index = 0; argv[optind - 1][index] != '\0'; ++index)
				file[index] = optarg[index];
			break;
		default:
			break;
		}
	}

	if (optind == argc) {
		if (strlen(file) == 0)
			prompt_for_file(file);
	} else if (optind < argc) {
		if (((optind + 1) == argc) && (strlen(file) == 0)) {
			for (index = 0; argv[optind][index] != '\0'; ++index)
				file[index] = argv[optind][index];
		}
	}

	start_machine(file);

	return 0;
}

static void prompt_for_file(char *file)
{
	/*
	 * The file wasn't supplied, so prompt the user for it.
	 *
	 * buffer -- Where we will store the file name.
	 * size   -- The size of the provided buffer.
	 */

	printf("Enter the name of the object file: ");
	fgets(file, 256, stdin);
	file[strlen(file) - 1] = '\0';
}

