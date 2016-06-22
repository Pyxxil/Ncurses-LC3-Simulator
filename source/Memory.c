#include <stdio.h>

#ifdef DEBUG
#include <stdio.h>
#if (DEBUG & 0x1)
#include <stdlib.h>
#endif
#endif

#include "Memory.h"
#include "Error.h"

void populate_memory(struct LC3 *simulator, char *file_name)
{
	/*
	 * Populate the memory of the supplied simulator with the contents of
	 * the file provided.
	 *
	 * simulator -- The simulator whose memory we want to populate.
	 * file_name -- The file we want to read the memory from.
	 */

	FILE *file = fopen(file_name, "rb");

	if (!file || file == NULL)
		unable_to_open_file(file_name);

	uint16_t tmp_PC;

	char buffer[2];

	// First line in the .obj file is the starting PC.
	fread(buffer, sizeof(buffer), 1, file);
	simulator->PC = tmp_PC = buffer[0] << 8 | buffer[1];

	while (fread(buffer, sizeof(buffer), 1, file) == 1)
#ifdef DEBUG
	{
#endif
		simulator->memory[tmp_PC++] = (buffer[0] & 0xff) << 8 |
						(buffer[1] & 0xff);
#ifdef DEBUG
		printf("0x%04x - 0x%04x", tmp_PC - 1, simulator->memory[tmp_PC - 1]);
		printf("   0x%04x  0x%04x\n", buffer[0] & 0xff, buffer[1] & 0xff);
	}
#if (DEBUG & 0x1)
	exit(EXIT_SUCCESS);
#endif
#endif

	if (!feof(file)) {
		fclose(file);
		read_error();
	}

	// Close the file
	fclose(file);
}

