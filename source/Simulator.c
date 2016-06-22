#include <stdio.h>

#include "Simulator.h"
#include "Error.h"

extern char *file;

#define WORD_SIZE 2	// Words in the LC3 are 16 bits/2 bytes

static short convert_bin_line(char *buffer)
{
	/*
	 * Convert a line of binary into a readable number.
	 *
	 * buffer -- A pointer to an array populated with the line we want
	 * to convert.
	 */

	short  number = 0,	// Where we will store the value of the
					// line.
                        index  = 0;	// An index to gor through each value in
					// the buffer.

	for (; index < WORD_SIZE; ++index) {
		// Make sure every number is in its correct position.
		number <<= 8;
		number += buffer[index];
	}

	return number;
}

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

	short tmp_PC, value;
	tmp_PC = value = 0;

	char buffer[2];

	// First line in the .obj file is the starting PC.
	fread(buffer, sizeof(buffer), 1, file);
	simulator->PC = tmp_PC = convert_bin_line(buffer);

	while (1) {
		if(fread(buffer, sizeof(buffer), 1, file) != 1)
			break;
		value = convert_bin_line(buffer);
		simulator->memory[tmp_PC++] = value;
	}

	if (!feof(file)) {
		// Didn't read to end of the file.
		fclose(file);
		read_error();
	}

	// Close the file
	fclose(file);
}

