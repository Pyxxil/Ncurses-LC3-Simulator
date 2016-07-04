#include <stdio.h>

#include "Memory.h"
#include "Error.h"

#define WORD_SIZE 2

int selected = 0;
uint16_t output_height	  = 0;
uint16_t *memory_output	  = NULL;
uint16_t selected_address = 0;

static const char *memory_format       = "0x%04hx\t\t0x%04hx";
static const unsigned int memory_attrs = A_REVERSE;

void populate_memory(struct program *prog)
{
	/*
	 * Populate the memory of the supplied simulator with the contents of
	 * the provided file.
	 *
	 * simulator -- The simulator whose memory we want to populate.
	 * file_name -- The file we want to read the memory from.
	 */

	FILE *file = fopen(prog->infile, "rb");
	uint16_t tmp_PC, instruction;

	if (!file || file == NULL)
		unable_to_open_file(prog->infile);

	// First line in the .obj file is the starting PC.
	fread(&tmp_PC, WORD_SIZE, 1, file);
	prog->simulator.PC = tmp_PC = 0xffff & (tmp_PC << 8 | tmp_PC >> 8);

	while (fread(&instruction, WORD_SIZE, 1, file) == 1)
		prog->simulator.memory[tmp_PC++].value = 0xffff
			& (instruction << 8 | instruction >> 8);

	if (!feof(file)) {
		fclose(file);
		read_error();
	}

	fclose(file);
}

static void redraw(WINDOW *window)
{
	for (int i = 0; i < output_height; ++i) {
		if (i < selected) {
			mvwprintw(window, i + 1, 1, memory_format,
				  selected_address - selected + i, memory_output[i]);
		} else {
			mvwprintw(window, i + 1, 1, memory_format,
				  selected_address + i, memory_output[i]);
		}
	}

	wattron(window, memory_attrs);
	mvwprintw(window, selected + 1, 1, memory_format,
		  selected_address, memory_output[selected]);
	wattroff(window, memory_attrs);

	wrefresh(window);
}

/*
 * Given a currently selected index and address, populate the memory_output
 * array to contain relative values.
 */

void generate_context(WINDOW *window, struct LC3 *simulator, int _selected,
		      uint16_t _selected_address)
{
	selected	 = _selected;
	selected_address = _selected_address;

	selected = ((selected_address + (output_height - selected)) > 0xfffe) ?
		(output_height - (0xfffe - selected_address)) : selected;

	for (int i = 0; i < selected; i++)
		memory_output[i] =
			simulator->memory[selected_address - selected + i].value;
	for (int i = 0; i < (output_height - 1); i++)
		memory_output[i] = simulator->memory[selected_address + i].value;

	redraw(window);
}

void move_context(WINDOW *window, struct LC3 *simulator, enum DIRECTION direction)
{
	bool _redraw	   = false;
	int prev	   = selected;
	uint16_t prev_addr = selected_address;

	switch (direction) {
	case UP:
		selected_address = (selected_address == 0) ?
			0 : selected_address - 1;
		selected	= (selected == 0) ?
			_redraw = true, 0 : selected - 1;
		break;
	case DOWN:
		selected_address = (selected_address == 0xfffe) ?
			0xfffe : selected_address + 1;
		selected	= (selected == (output_height - 1)) ?
			_redraw = true, output_height - 1 : selected + 1;
		break;
	default:
		break;
	}

	wattron(window, memory_attrs);
	mvwprintw(window, selected + 1, 1, memory_format,
		  selected_address, memory_output[selected]);
	wattroff(window, memory_attrs);
	mvwprintw(window, prev + 1, 1, memory_format,
		  prev_addr, memory_output[prev]);

	if (_redraw) {
		create_context(window, simulator, selected, selected_address);
		redraw(window);
	}
} /* move_context */
