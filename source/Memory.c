#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "Machine.h"
#include "Memory.h"
#include "Error.h"
#include "Parser.h"

#define WORD_SIZE 2

#ifndef OS_PATH
#error "No Path has been supplied for the Operating System."
#endif

#define STR(x) #x
#define OSPATH(path)  STR(path)
#define OS_OBJ_FILE OSPATH(OS_PATH) "/LC3_OS.obj"

int selected = 0;
uint16_t output_height    = 0;
uint16_t *memory_output   = NULL;
uint16_t selected_address = 0;

bool OSInstalled = false;
static bool symsInstalled = false;

static char const  *const MEMFORMAT      = "0x%04X  %s  0x%04X  %-25s %-50s";
static unsigned int const SELECTATTR     = A_REVERSE | A_BOLD;
static unsigned int const BREAKPOINTATTR = COLOR_PAIR(1) | A_REVERSE;

/*
 * For now, this is mostly pointless ... It will be more useful when proper
 * instruction handling is working (i.e. no more of the LC3xxxx functions).
 */

static void installOS(struct program *prog)
{
	uint16_t OSOrigin, tmp;

	FILE *OSFile = fopen(OS_OBJ_FILE, "r");
	if (NULL == OSFile) {
		perror("LC3-Simulator");
		exit(EXIT_FAILURE);
	}

	if (1 != fread(&OSOrigin, WORD_SIZE, 1, OSFile)) {
		fprintf(stderr, "Unable to read from the OS object file.\n");
		exit(EXIT_FAILURE);
	}

	OSOrigin = 0xffff & (OSOrigin << 8 | OSOrigin >> 8);

	while (1 == fread(&tmp, WORD_SIZE, 1, OSFile)) {
		prog->simulator.memory[OSOrigin] = (struct memory_slot) {
			.value = 0xffff & (tmp << 8 | tmp >> 8),
			.address = OSOrigin,
			.isBreakpoint = false,
		};

		OSOrigin++;
	}

	fclose(OSFile);

	if (!OSInstalled) {
		populateOSSymbols();
		OSInstalled = true;
	}
}

/*
 * Populate the memory of the supplied simulator with the contents of
 * the provided file.
 *
 * @prog: The program we want to populate the memory of, which also contains the
 *        file we will read the memory from.
 *
 * Returns: 0 on success, >0 on failure.
 */

int populate_memory(struct program *prog)
{
	uint16_t tmp_PC, instruction;

	FILE *file = fopen(prog->objectfile, "rb");
	if (NULL == file) {
		perror("LC3-Simulator");
		exit(EXIT_FAILURE);
	}

	installOS(prog);
	symsInstalled = false;

	if (NULL == file || !file)
		return 1;

	// First line in the .obj file is the starting PC.
	if (1 != fread(&tmp_PC, WORD_SIZE, 1, file)) {
		fprintf(stderr, "Unable to read from %s.\n", prog->objectfile);
		exit(EXIT_FAILURE);
	}

	prog->simulator.PC = tmp_PC = 0xffff & (tmp_PC << 8 | tmp_PC >> 8);

	while (1 == fread(&instruction, WORD_SIZE, 1, file)) {
		prog->simulator.memory[tmp_PC] = (struct memory_slot) {
			.value = 0xffff & (instruction << 8 | instruction >> 8),
			.address = tmp_PC,
		};

		tmp_PC++;
	}

	if (!feof(file)) {
		fclose(file);
		tidyup(prog);
		read_error();
		return 2;
	}

	fclose(file);
	return 0;
}

/*
 * Convert a binary instruction to characters.
 *
 * @instr:   The 16 bit binary value that we want to convert.
 * @address: The address of the supplied instruction.
 * @buff:    Where we are to store the converted instruction.
 * @prog:    The program containing all instructions/files we need.
 *
 * Returns: The buff supplied.
 */

static char *instruction(uint16_t instr, uint16_t address, char *buff,
		struct program *prog)
{
	uint16_t opcode = instr & 0xF000;
	int16_t offset;
	char immediate[5];
	struct symbol *symbol;

	if (!symsInstalled) {
		populateSymbolsFromFile(prog);
		symsInstalled = true;
	}

	switch (opcode) {
	case AND:
	case ADD:
		strcpy(buff, AND == opcode ? "AND R" : "ADD R");
		buff[5] = ((instr >> 9) & 0x7) + 0x30;
		strcat(buff, ", R");
		buff[9] = ((instr >> 6) & 0x7) + 0x30;
		strcat(buff, ", ");
		if (instr & 0x20) {
			strcat(buff, "#");
			snprintf(immediate, 5, "%hd",
				(int16_t) (((int16_t) (instr << 11)) >> 11));
			strcat(buff, immediate);
		} else {
			strcat(buff, "R");
			buff[13] = (instr & 0x7) + 0x30;
		}
		break;
	case JMP:
		if (0xC1C0 == instr) {
			strcpy(buff, "RET");
		} else {
			strcpy(buff, "JMP R");
			buff[5] = ((instr >> 9) & 0x7) + 0x30;
		}
		break;
	case BR:
		strcpy(buff, "BR");
		if (!(instr & 0x0E00)) {
			strcpy(buff, "NOP");
			break;
		}

		if (instr & 0x0800) {
			strcat(buff, "n");
		}
		if (instr & 0x0400) {
			strcat(buff, "z");
		}
		if (instr & 0x0200) {
			strcat(buff, "p");
		}
		strcat(buff, " ");
		if (instr & 0x0100) {
			offset = ((int16_t) (instr << 7)) >> 7;
			symbol = findSymbolByAddress((uint16_t) address +
					(uint16_t) offset);
		} else {
			symbol = findSymbolByAddress(address + (instr & 0x00FF));
		}
		if (NULL != symbol) {
			strcat(buff, symbol->name);
		} else {
			// It should be pretty safe to assume that if we can't
			// find the symbol, it doesn't exist and the
			// 'instruction' is just a value in memory.
			strcpy(buff, "NOP");
		}
		break;
	case JSR:
		strcpy(buff, "JSR ");

		if (instr & 0x0400) {
			offset = ((int16_t) (instr << 5)) >> 5;
			symbol = findSymbolByAddress((uint16_t) address +
					(uint16_t) offset);
		} else {
			symbol = findSymbolByAddress(address + (instr & 0x03FF));
		}
		if (NULL != symbol) {
			strcat(buff, symbol->name);
		} else {
			strcpy(buff, "NOP");
		}
		break;
	case LEA:
	case LD:
	case LDI:
	case ST:
	case STI:
		switch (opcode) {
		case LEA:
			strcpy(buff, "LEA R");
			break;
		case LD:
			strcpy(buff, "LD R");
			break;
		case LDI:
			strcpy(buff, "LDI R");
			break;
		case ST:
			strcpy(buff, "ST R");
			break;
		case STI:
			strcpy(buff, "STI R");
			break;
		default:
			break;
		}

		buff[strlen(buff)] = ((instr >> 9) & 0x7) + 0x30;
		strcat(buff, ", ");
		if (instr & 0x0100) {
			offset = ((int16_t) (instr << 7)) >> 7;
			symbol = findSymbolByAddress((uint16_t) address +
					(uint16_t) offset);
		} else {
			symbol = findSymbolByAddress((uint16_t) address +
					(uint16_t) (instr & 0x00FF));
		}
		if (NULL != symbol) {
			strcat(buff, symbol->name);
		} else {
			strcpy(buff, "NOP");
		}
		break;
	case LDR:
	case STR:
		strcpy(buff, STR == opcode ? "STR R" : "LDR R");
		buff[5] = ((instr >> 9) & 0x7) + 0x30;
		strcat(buff, ", R");
		buff[9] = ((instr >> 6) & 0x7) + 0x30;
		strcat(buff, ", #");
		snprintf(immediate, 5, "%hd",
				(int16_t) (((int16_t) (instr << 10)) << 10));
		strcat(buff, immediate);
		break;
	case NOT:
		strcpy(buff, "NOT R");
		buff[5] = ((instr >> 9) & 0x7) + 0x30;
		strcat(buff, ", R");
		buff[9] = ((instr >> 6) & 0x7) + 0x30;
		break;
	case TRAP:
		switch (instr & 0x00FF) {
		case 0x25:
			strcpy(buff, "HALT");
			break;
		case 0x24:
			strcpy(buff, "PUTSP");
			break;
		case 0x23:
			strcpy(buff, "IN");
			break;
		case 0x22:
			strcpy(buff, "PUTS");
			break;
		case 0x21:
			strcpy(buff, "OUT");
			break;
		case 0x20:
			strcpy(buff, "GETC");
			break;
		default:
			strcpy(buff, "NOP");
			break;
		}
		break;
	default:
		break;
	}

	return buff;
}

static void wprint(WINDOW *window, struct program *prog, size_t address, int y,
		int x)
{
	struct symbol *symbol = findSymbolByAddress((uint16_t) address);
	char instr[100] = { 0 };
	char label[100] = { 0 };

	char binary[] = "0000000000000000";
	for (int i = 15, bit = 1; i >= 0; i--, bit <<= 1) {
		binary[i] = prog->simulator.memory[address].value & bit ?
			'1' : '0';
	}

	instruction(prog->simulator.memory[address].value, (uint16_t) (address + 1),
			instr, prog);

	symbol = findSymbolByAddress((uint16_t) address);
	if (NULL != symbol) {
		strcpy(label, symbol->name);
	}

	mvwprintw(window, y, x, MEMFORMAT, address, binary,
			prog->simulator.memory[address].value, label, instr);
	wrefresh(window);
}

void update(WINDOW *window, struct program *prog)
{
	memory_output[selected] = prog->simulator.memory[selected_address].value;
	wattron(window, SELECTATTR);
	wprint(window, prog, selected_address, selected + 1, 1);
	wattroff(window, SELECTATTR);
}

/*
 * Redraw the memory view. Called after every time the user moves up / down in
 * the area.
 */

static void redraw(WINDOW *window, struct program *prog)
{
	for (int i = 0; i < output_height; ++i) {
		if (i < selected) {
			if (prog->simulator
					.memory[selected_address - selected + i]
					.isBreakpoint) {
				wattron(window, BREAKPOINTATTR);
			}

			wprint(window, prog,
				(size_t) selected_address - (size_t) selected +
				(size_t) i, i + 1, 1);

			if (prog->simulator
					.memory[selected_address - selected + i]
					.isBreakpoint) {
				wattroff(window, BREAKPOINTATTR);
			}
		} else {
			if (prog->simulator.memory[selected_address + i]
					.isBreakpoint) {
				wattron(window, BREAKPOINTATTR);
			}

			wprint(window, prog, (size_t) selected_address +
					(size_t) i, i + 1, 1);

			if (prog->simulator.memory[selected_address + i]
					.isBreakpoint) {
				wattroff(window, BREAKPOINTATTR);
			}
		}
	}

	wattron(window, SELECTATTR);
	wprint(window, prog, selected_address, selected + 1, 1);
	wattroff(window, SELECTATTR);
}


/*
 * Given a currently selected index and address, populate the memory_output
 * array to contain relative values.
 */

void generate_context(WINDOW *window, struct program *prog, int _selected,
		uint16_t _selected_address)
{
	int i = 0;

	selected = _selected;
	selected_address = _selected_address;

	selected =
		((selected_address + (output_height - 1 - selected)) > 0xfffe) ?
		(output_height - (0xfffe - selected_address)) - 1 : selected;

	for (; i < selected; i++)
		memory_output[i] =
			prog->simulator
				.memory[selected_address - selected + i].value;
	for (; i < output_height; i++)
		memory_output[i] = prog->simulator
			.memory[selected_address + i].value;

	redraw(window, prog);
	mem_populated = selected_address;
}

void move_context(WINDOW *window, struct program *prog, enum DIRECTION direction)
{
	bool _redraw = false;
	int prev = selected;
	uint16_t prev_addr = selected_address;

	switch (direction) {
	case UP:
		selected_address -= !!selected_address;
		selected = !selected ? _redraw = true, 0 : selected - 1;
		break;
	case DOWN:
		selected_address += (0xFFFE == selected_address) ? 0 : 1;
		selected = ((output_height - 1) == selected) ? _redraw = true,
				(output_height - 1) : selected + 1;
		break;
	default:
		break;
	}

	wattron(window, SELECTATTR);
	wprint(window, prog, selected_address, selected + 1, 1);
	wattroff(window, SELECTATTR);

	if (prog->simulator.memory[prev_addr].isBreakpoint) {
		wattron(window, BREAKPOINTATTR);
	}
	wprint(window, prog, prev_addr, prev + 1, 1);
	if (prog->simulator.memory[prev_addr].isBreakpoint) {
		wattroff(window, BREAKPOINTATTR);
	}

	if (_redraw)
		generate_context(window, prog, selected, selected_address);
}

