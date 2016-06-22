#include <stdio.h>

#include "Simulator.h"
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
		simulator->memory[tmp_PC++] = buffer[0] << 8 | buffer[1];

	if (!feof(file)) {
		fclose(file);
		read_error();
	}

	// Close the file
	fclose(file);
}

