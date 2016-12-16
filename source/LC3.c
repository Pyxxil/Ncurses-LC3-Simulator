#include <stdbool.h> // Much nicer to use true/false

#include "Enums.h"
#include "LC3.h"

static const uint16_t KBSR = 0xFE00;
static const uint16_t KBDR = 0xFE02;
static const uint16_t DSR  = 0xFE04;
static const uint16_t DDR  = 0xFE06;
static const uint16_t MCR  = 0xFFFE;

/*
 * Change the Condition Code based on the value last put into
 * a register.
 *
 */

static void setcc(uint16_t *last_result, unsigned char *CC)
{
	if (!*last_result) *CC = 'Z';
	else if (((int16_t) *last_result) < 0) *CC = 'N';
	else *CC = 'P';
}

/*
 * Print the current state of the simulator to the window provided.
 */

void print_state(struct LC3 *simulator, WINDOW *window)
{
	size_t index = 0;

	// Clear, and reborder, the window.
	wclear(window);
	box(window, 0, 0);

	// Print the first four registers.
	for (; index < 4; ++index) {
		mvwprintw(window, (int) index + 1, 3, "R%d 0x%04X %hd", index,
			simulator->registers[index],
			simulator->registers[index]);
	}

	// Print the last 4 registers.
	for (; index < 8; ++index) {
		mvwprintw(window, (int) index - 3, 20, "R%d 0x%04X %hd", index,
			simulator->registers[index],
			simulator->registers[index]);
	}

	// Print the PC, IR, and CC.
	mvwprintw(window, 1, 37, "PC 0x%04X %hd", simulator->PC, simulator->PC);
	mvwprintw(window, 2, 37, "IR 0x%04X %hd", simulator->IR, simulator->IR);
	mvwprintw(window, 3, 37, "CC %C        ", simulator->CC);
	wrefresh(window);
}

/*
 * Execute the next instruction of the given simulator.
 */

void execute_next(struct LC3 *simulator, WINDOW *output)
{
	int16_t PCoffset;
	uint16_t *DR, SR1, SR2, opcode;

	// The PC is incremented at the beginning of each instruction.
	simulator->IR = simulator->memory[simulator->PC++].value;

	// The opcode is in the first 4 bits of the instruction.
	opcode = simulator->IR & 0xF000;

	switch (opcode) {
	case TRAP:
		simulator->registers[7] = simulator->PC;
		simulator->PC = simulator->memory[simulator->IR & 0xFF].value;
		break;
	case LEA:
		// LEA takes a destination register in bits[11:9].
		DR = &(simulator->registers[(simulator->IR >> 9) & 7]);
		// It also gives a signed PC offset in bits[8:0].
		PCoffset = ((int16_t) ((simulator->IR & 0x1FF) << 7)) >> 7;
		// Set the register to equal the PC + SEXT(PCoffset).
		*DR = (uint16_t) ((int16_t) simulator->PC + PCoffset);
		// We then flag for the condition code to be set.
		setcc(DR, &simulator->CC);
		break;
	case LDI:
		// LDI takes a destination register in bits[11:9].
		DR = &(simulator->registers[(simulator->IR >> 9) & 7]);
		// It also gives a signed PC offset in bits[8:0].
		PCoffset = ((int16_t) ((simulator->IR & 0x1FF) << 7)) >> 7;
		// Then we want to find what is stored at that address in
		// memory, and then load the value stored at that address into
		// the destination register.
		if (KBDR == simulator->memory[simulator->PC + PCoffset].value) {
			wtimeout(output, -1);
			*DR = (uint16_t) wgetch(output);
			wtimeout(output, 0);
		} else {
			*DR = simulator->memory[
					simulator->memory[
					simulator->PC + PCoffset].value].value;
		}
		// We then flag for the condition code to be set.
		setcc(DR, &simulator->CC);
		break;
	case NOT:
		// NOT takes a destination register in bits[11:9].
		DR = &(simulator->registers[(simulator->IR >> 9) & 7]);
		// It also takes a source register in bits[8:6].
		SR1 = simulator->registers[(simulator->IR >> 6) & 7];
		// We then bitwise NOT the two registers together, and store the
		// result in the destination register.
		*DR = (uint16_t) ~SR1;
		// We then flag for the condition code to be set.
		setcc(DR, &simulator->CC);
		break;
	case LD:
		// LD takes a destination register in bits[11:9].
		DR = &(simulator->registers[(simulator->IR >> 9) & 7]);
		// It also takes a signed 9 bit PC offset, so erase the top 7.
		// bits, then sign extend the number.
		PCoffset = ((int16_t) ((simulator->IR & 0x1FF) << 7)) >> 7;
		// We then retrieve this value in memory, and store the value
		// into the destination register.
		*DR = simulator->memory[simulator->PC + PCoffset].value;
		// We then flag for the condition code to be set.
		setcc(DR, &simulator->CC);
		break;
	case ADD:
	case AND:
		// ADD/AND takes a destination register in bits[11:9].
		DR = &(simulator->registers[(simulator->IR >> 9) & 7]);
		// It also takes a source register in bits[8:6].
		SR1 = simulator->registers[(simulator->IR >> 6) & 7];
		// There are two possible second operands. Either bit[5] == 1,
		// meaning we have a signed 5 bit integer as the second operand,
		// or we have another source register as the second operand.
		if (simulator->IR & 0x0020) {
			SR2 = (uint16_t) ((simulator->IR & 0x1F)
				| ((simulator->IR & 0x10) ? 0xFFE0 : 0));
		} else {
			SR2 = simulator->registers[simulator->IR & 7];
		}
		// We then ADD/AND the two source registers together, and store
		// that result in the destination register.
		*DR = (ADD == opcode) ? SR1 + SR2 : SR1 & SR2;
		// We then flag for the condition code to be set.
		setcc(DR, &simulator->CC);
		break;
	case BR:
		// BR takes 3 potential conditions, and checks if that condition
		// is set.
		if ((((simulator->IR >> 11) & 1) && 'N' == simulator->CC) ||
			(((simulator->IR >> 10) & 1) && 'Z' == simulator->CC) ||
			(((simulator->IR >> 9) & 1) && 'P' == simulator->CC)) {
			// If that condition is set, then we want the signed 9
			// bit PCoffset provided.
			PCoffset = ((int16_t) ((simulator->IR & 0x1FF) << 7)) >> 7;
			// Which is then added to the current program counter.
			simulator->PC =
				(uint16_t) ((int16_t) simulator->PC + PCoffset);
		}
		break;
	case LDR:
		DR = &simulator->registers[(simulator->IR >> 9) & 7];
		SR1 = simulator->registers[(simulator->IR >> 6) & 7];
		PCoffset = ((int16_t) ((simulator->IR & 0x3F) << 9)) >> 9;
		*DR = simulator->memory[SR1 + PCoffset].value;
		setcc(DR, &simulator->CC);
		break;
	case ST:
		SR1 = simulator->registers[(simulator->IR >> 9) & 7];
		PCoffset = ((int16_t) ((simulator->IR & 0x1FF) << 7)) >> 7;
		simulator->memory[simulator->PC + PCoffset].value = SR1;
		break;
	case STR:
		SR1 = simulator->registers[(simulator->IR >> 9) & 7];
		SR2 = simulator->registers[(simulator->IR >> 6) & 7];
		PCoffset = ((int16_t) ((simulator->IR & 0x3F) << 9)) >> 9;
		simulator->memory[SR2 + PCoffset].value = SR1;
		break;
	case STI:
		SR1 = simulator->registers[(simulator->IR >> 9) & 7];
		PCoffset = ((int16_t) ((simulator->IR & 0x1FF) << 7)) >> 7;
		simulator->memory[
			simulator->memory[
				simulator->PC + PCoffset].value].value = SR1;
		if (MCR == simulator->memory[simulator->PC + PCoffset].value) {
			simulator->isHalted = true;
		}
		break;
	case JMP:
		SR1 = simulator->registers[(simulator->IR >> 6) & 7];
		simulator->PC = SR1;
		break;
	case JSR:
		simulator->registers[7] = simulator->PC;
		if (simulator->IR & 0x800) {
			PCoffset =
				((int16_t) ((simulator->IR & 0x7FF) << 5)) >> 5;
			simulator->PC =
				(uint16_t) ((int16_t) simulator->PC + PCoffset);
		} else {
			SR1 = simulator->registers[(simulator->IR >> 6) & 7];
			simulator->PC = SR1;
		}
		break;
	default:
		break;
	}

	if (simulator->memory[DDR].value) {
		wechochar(output, simulator->memory[0xFE06].value & 0xFF);
		simulator->memory[DDR].value = 0x0;
	}

	simulator->memory[KBSR].value = 0x8000;
	simulator->memory[DSR].value  = 0x8000;
}

