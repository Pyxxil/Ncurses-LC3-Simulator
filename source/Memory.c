#include <stdio.h>

#ifdef DEBUG
#include <stdio.h>
#if (DEBUG & 0x1)
#include <stdlib.h>
#endif
#endif

#include "Memory.h"
#include "Error.h"

#define WORD_SIZE 2

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

        int instruction;

	// First line in the .obj file is the starting PC.
	fread(&instruction, WORD_SIZE, 1, file);
	simulator->PC = tmp_PC = 0xffff & (instruction << 8 | instruction >> 8);

	while (fread(&instruction, WORD_SIZE, 1, file) == 1) {
		simulator->memory[tmp_PC++] = 0xffff &
                        (instruction << 8 | instruction >> 8);

#if defined (DEBUG) && (DEBUG & 0x1)
		printf("0x%04x - 0x%04x", tmp_PC - 1, simulator->memory[tmp_PC - 1]);
		printf("   0x%04x  0x%04x\n", instruction << 8, instruction >> 8);
#endif

	}

#if defined (DEBUG) && (DEBUG & 0x1)
	exit(EXIT_SUCCESS);
#endif

	if (!feof(file)) {
		fclose(file);
		read_error();
	}

	// Close the file
	fclose(file);
}

