#include <stdio.h>

#include "Machine.h"
#include "Memory.h"
#include "Error.h"

#define WORD_SIZE 2

uint16_t selected	  = 0;
uint16_t output_height	  = 0;
uint16_t selected_address = 0;
uint16_t *memory_output	  = NULL;

static const char *MEMFORMAT	     = "0x%04hx\t\t0x%04hx";
static const unsigned int SELECTATTR = A_REVERSE | A_BOLD;

/*
 * Populate the memory of the supplied simulator with the contents of
 * the provided file.
 *
 * prog -- The program we want to populate the memory of, which also contains the
 *         file we will read the memory from.
 */

int populate_memory(struct program *prog)
{
	FILE *file = fopen(prog->infile, "rb");
	uint16_t tmp_PC, instruction;

	if (file == NULL || !file)
		return 1;

	// First line in the .obj file is the starting PC.
	fread(&tmp_PC, WORD_SIZE, 1, file);
	prog->simulator.PC = tmp_PC = 0xffff & (tmp_PC << 8 | tmp_PC >> 8);

	while (fread(&instruction, WORD_SIZE, 1, file) == 1) {
		prog->simulator.memory[tmp_PC++].value = 0xffff
			& (instruction << 8 | instruction >> 8);
	}

	if (!feof(file)) {
		fclose(file);
		read_error();
		tidyup(prog);
		return 2;
	}

	fclose(file);
	return 0;
}

/*
 * Redraw the memory view. Called after every time the user moves up / down in
 * the area.
 */

static void redraw(WINDOW *window)
{
	for (int i = 0; i < output_height; ++i) {
		if (i < selected) {
			mvwprintw(window, i + 1, 1, MEMFORMAT,
				  selected_address - selected + i, memory_output[i]);
		} else {
			mvwprintw(window, i + 1, 1, MEMFORMAT,
				  selected_address + i, memory_output[i]);
		}
	}

	wattron(window, SELECTATTR);
	mvwprintw(window, selected + 1, 1, MEMFORMAT, selected_address,
		  memory_output[selected]);
	wattroff(window, SELECTATTR);

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

	selected = ((selected_address + (output_height - 1 - selected)) > 0xfffe) ?
		(output_height - (0xfffe - selected_address)) - 1 : selected;

	int i = 0;

	for (; i < selected; i++)
		memory_output[i] =
			simulator->memory[selected_address - selected + i].value;
	for (; i < output_height; i++)
		memory_output[i] = simulator->memory[selected_address + i].value;

	redraw(window);
	mem_populated = selected_address;
}

void move_context(WINDOW *window, struct LC3 *simulator, enum DIRECTION direction)
{
	bool _redraw	   = false;
	int prev	   = selected;
	uint16_t prev_addr = selected_address;

	switch (direction) {
	case UP:
		selected_address -= (selected_address == 0) ? 0 : 1;
		selected	  = (selected == 0) ? _redraw = true, 0 : selected - 1;
		break;
	case DOWN:
		selected_address += (selected_address == 0xfffe) ? 0 : 1;
		selected	  = (selected == (output_height - 1)) ?
			_redraw	  = true, (output_height - 1) : selected + 1;
		break;
	default:
		break;
	}

	wattron(window, SELECTATTR);
	mvwprintw(window, selected + 1, 1, MEMFORMAT, selected_address, memory_output[selected]);
	wattroff(window, SELECTATTR);
	mvwprintw(window, prev + 1, 1, MEMFORMAT, prev_addr, memory_output[prev]);

	if (_redraw)
		generate_context(window, simulator, selected, selected_address);
}
