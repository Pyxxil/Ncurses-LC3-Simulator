#include <stdio.h>

#include "Machine.h"
#include "Memory.h"
#include "Error.h"

#define WORD_SIZE 2

uint16_t selected	  = 0;
uint16_t output_height	  = 0;
uint16_t selected_address = 0;
uint16_t *memory_output	  = NULL;

static char const *const MEMFORMAT = "0x%04X\t\t%s\t\t0x%04X";
static unsigned int const SELECTATTR = A_REVERSE | A_BOLD;
static unsigned int const BREAKPOINTATTR = COLOR_PAIR(1) | A_REVERSE;

/*
 * For now, this is mostly pointless ... It will be more useful when proper
 * instruction handling is working (i.e. no more of the LC3xxxx functions).
 */

static void installOS(struct program *prog)
{
	FILE *OSFile = fopen("LC3_OS.obj", "r");
	uint16_t OSOrigin, tmp;

	fread(&OSOrigin, WORD_SIZE, 1, OSFile);
	OSOrigin = 0xffff & (OSOrigin << 8 | OSOrigin >> 8);

	while (fread(&tmp, WORD_SIZE, 1, OSFile) == 1) {
		prog->simulator.memory[OSOrigin] = (struct memory_slot) {
			.value = 0xffff & (tmp << 8 | tmp >> 8),
			.address = OSOrigin,
			.isBreakpoint = false,
		};

		OSOrigin++;
	}

	fclose(OSFile);
}

/*
 * Populate the memory of the supplied simulator with the contents of
 * the provided file.
 *
 * prog -- The program we want to populate the memory of, which also contains the
 *         file we will read the memory from.
 */

int populate_memory(struct program *prog)
{
	installOS(prog);
	FILE *file = fopen(prog->objectfile, "rb");
	uint16_t tmp_PC, instruction;

	if (file == NULL || !file)
		return 1;

	// First line in the .obj file is the starting PC.
	fread(&tmp_PC, WORD_SIZE, 1, file);
	prog->simulator.PC = tmp_PC = 0xffff & (tmp_PC << 8 | tmp_PC >> 8);

	while (fread(&instruction, WORD_SIZE, 1, file) == 1) {
		prog->simulator.memory[tmp_PC] = (struct memory_slot) {
			.value = 0xffff & (instruction << 8 | instruction >> 8),
			.address = tmp_PC,
		};

		tmp_PC++;
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

static void wprint(WINDOW *window, struct LC3 *simulator, size_t address,
		int y, int x)
{
	char binary[] = "0000000000000000";
	for (int i = 15, bit = 1; i >= 0; i--, bit <<= 1) {
		binary[i] = simulator->memory[address].value & bit ? '1' : '0';
	}

	mvwprintw(window, y, x, MEMFORMAT, address, binary,
			simulator->memory[address].value);
	wrefresh(window);
}

void update(WINDOW *window, struct LC3 *simulator)
{
	memory_output[selected] = simulator->memory[selected_address].value;
	wattron(window, SELECTATTR);
	wprint(window, simulator, selected_address, selected + 1, 1);
	wattroff(window, SELECTATTR);
}

/*
 * Redraw the memory view. Called after every time the user moves up / down in
 * the area.
 */

static void redraw(WINDOW *window, struct LC3 *simulator)
{
	for (int i = 0; i < output_height; ++i) {
		if (i < selected) {
			if (simulator->memory[selected_address - selected + i]
					.isBreakpoint) {
				wattron(window, BREAKPOINTATTR);
			}

			wprint(window, simulator,
				selected_address - selected + i, i + 1, 1);

			if (simulator->memory[selected_address - selected + i]
					.isBreakpoint) {
				wattroff(window, BREAKPOINTATTR);
			}
		} else {
			if (simulator->memory[selected_address + i]
					.isBreakpoint) {
				wattron(window, BREAKPOINTATTR);
			}

			wprint(window, simulator, selected_address + i,
				i + 1, 1);

			if (simulator->memory[selected_address + i]
					.isBreakpoint) {
				wattroff(window, BREAKPOINTATTR);
			}
		}
	}

	wattron(window, SELECTATTR);
	wprint(window, simulator, selected_address, selected + 1, 1);
	wattroff(window, SELECTATTR);
}


/*
 * Given a currently selected index and address, populate the memory_output
 * array to contain relative values.
 */

void generate_context(WINDOW *window, struct LC3 *simulator, int _selected,
		uint16_t _selected_address)
{
	selected = _selected;
	selected_address = _selected_address;

	selected =
		((selected_address + (output_height - 1 - selected)) > 0xfffe) ?
		(output_height - (0xfffe - selected_address)) - 1 : selected;

	int i = 0;

	for (; i < selected; i++)
		memory_output[i] =
			simulator->memory[selected_address - selected + i].value;
	for (; i < output_height; i++)
		memory_output[i] = simulator->memory[selected_address + i].value;

	redraw(window, simulator);
	mem_populated = selected_address;
}

void move_context(WINDOW *window, struct LC3 *simulator,
		enum DIRECTION direction)
{
	bool _redraw = false;
	int prev = selected;
	uint16_t prev_addr = selected_address;

	switch (direction) {
	case UP:
		selected_address -= (selected_address == 0) ? 0 : 1;
		selected = (selected == 0) ? _redraw = true, 0 : selected - 1;
		break;
	case DOWN:
		selected_address += (selected_address == 0xfffe) ? 0 : 1;
		selected = (selected == (output_height - 1)) ? _redraw = true,
				(output_height - 1) : selected + 1;
		break;
	}

	wattron(window, SELECTATTR);
	wprint(window, simulator, selected_address, selected + 1, 1);
	wattroff(window, SELECTATTR);

	if (simulator->memory[prev_addr].isBreakpoint) {
		wattron(window, BREAKPOINTATTR);
	}
	wprint(window, simulator, prev_addr, prev + 1, 1);
	if (simulator->memory[prev_addr].isBreakpoint) {
		wattroff(window, BREAKPOINTATTR);
	}

	if (_redraw)
		generate_context(window, simulator, selected, selected_address);
}

