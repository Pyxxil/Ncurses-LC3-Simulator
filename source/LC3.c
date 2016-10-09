#include <stdbool.h> // Much nicer to use true/false

#include "Enums.h"
#include "LC3.h"

static void LC3out(struct LC3 *simulator, WINDOW *window)
{
	wechochar(window, (unsigned char) simulator->registers[0]);
}

static void LC3halt(WINDOW *window)
{
	waddstr(window, "\n----- Halting the processor -----\n");
	wrefresh(window);
}

static void LC3puts(struct LC3 *simulator, WINDOW *window)
{
	uint16_t tmp = simulator->registers[0],
		orig = simulator->registers[0];
	while (simulator->memory[tmp].value != 0x0) {
		simulator->registers[0] = simulator->memory[tmp++].value;
		LC3out(simulator, window);
	}
	simulator->registers[0] = orig;
}

static void LC3getc(struct LC3 *simulator, WINDOW *window)
{
	wtimeout(window, -1);
	simulator->registers[0] = wgetch(window);
	wtimeout(window, 0);
}

static void LC3in(struct LC3 *simulator, WINDOW *window)
{
	waddstr(window, "\nInput a character> ");
	wrefresh(window);
	LC3getc(simulator, window);
}

/*
 * Change the Condition Code based on the value last put into
 * a register.
 *
 */

static void setcc(uint16_t *last_result, unsigned char *CC)
{
	if (*last_result == 0)  *CC = 'Z';
	else if (*last_result >= 0x8000)  *CC = 'N';
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
		mvwprintw(window, index + 1, 3, "R%d 0x%04X %hd", index,
			simulator->registers[index],
			simulator->registers[index]);
	}

	// Print the last 4 registers.
	for (; index < 8; ++index) {
		mvwprintw(window, index - 3, 20, "R%d 0x%04X %hd", index,
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
	unsigned char opcode;
	uint16_t *DR, SR1, SR2;

	// Increment the PC when we fetch the next instruction.
	simulator->IR = simulator->memory[simulator->PC++].value;

	// The opcode is in the first 4 bits of the instruction.
	opcode = (simulator->IR & 0xf000) >> 12;

	// We only want the first 4 bits of the instruction for the opcode.
	switch (opcode) {
	case TRAP:
		switch (simulator->IR & 0x00ff) {
		case GETC:
			// Get a character from the keyboard without
			// echoing it to the display, storing it in
			// register 0.
			LC3getc(simulator, output);
			break;
		case OUT:
			// Display a character stored in register 0.
			LC3out(simulator, output);
			break;
		case PUTS:
			// With the address supplied in register 0,
			// display a string.
			LC3puts(simulator, output);
			break;
		case IN:
			// Give a generic prompt, and then retrieve
			// one character from the display, store it
			// in register 0, and display it to the screen.
			LC3in(simulator, output);
			break;
		case PUTSP:
			break;
		case HALT:
			// Display a string to the display that tells
			// the user that the simulator has halted.
			LC3halt(output);
			// Because the HALT instruction has been called, we want
			// to stop the execution of the machine.
			simulator->isHalted = true;
			break;
		default:
			break;
		}
		//simulator->registers[7] = simulator->PC;
		//simulator->PC = simulator->memory[simulator->IR & 0xff].value;
		break;
	case LEA:
		// LEA takes a destination register in bits[11:9].
		DR = &(simulator->registers[(simulator->IR >> 9) & 7]);
		// It also gives a signed PC offset in bits[8:0].
		PCoffset = ((int16_t) ((simulator->IR & 0x1ff) << 7)) >> 7;
		// Set the register to equal the PC + SEXT(PCoffset).
		*DR = simulator->PC + PCoffset;
		// We then flag for the condition code to be set.
		setcc(DR, &simulator->CC);
		break;
	case LDI:
		// LDI takes a destination register in bits[11:9].
		DR = &(simulator->registers[(simulator->IR >> 9) & 7]);
		// It also gives a signed PC offset in bits[8:0].
		PCoffset = ((int16_t) ((simulator->IR & 0x1ff) << 7)) >> 7;
		// Then we want to find what is stored at that address in
		// memory, and then load the value stored at that address into
		// the destination register.
		*DR = simulator->memory[
				simulator->memory[
					simulator->PC + PCoffset].value].value;
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
		*DR = ~SR1;
		// We then flag for the condition code to be set.
		setcc(DR, &simulator->CC);
		break;
	case LD:
		// LD takes a destination register in bits[11:9].
		DR = &(simulator->registers[(simulator->IR >> 9) & 7]);
		// It also takes a signed 9 bit PC offset, so erase the top 7.
		// bits, then sign extend the number.
		PCoffset = ((int16_t) ((simulator->IR & 0x1ff) << 7)) >> 7;
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
			SR2 = (simulator->IR & 0x1f)
				| ((simulator->IR & 0x10) ? 0xffe0 : 0);
		} else {
			SR2 = simulator->registers[simulator->IR & 7];
		}
		// We then ADD/AND the two source registers together, and store
		// that result in the destination register.
		*DR = opcode == ADD ? SR1 + SR2 : SR1 & SR2;
		// We then flag for the condition code to be set.
		setcc(DR, &simulator->CC);
		break;
	case BR:
		// BR takes 3 potential conditions, and checks if that condition
		// is set.
		if ((((simulator->IR >> 11) & 1) && simulator->CC == 'N') ||
			(((simulator->IR >> 10) & 1) && simulator->CC == 'Z') ||
			(((simulator->IR >> 9) & 1) && simulator->CC == 'P')) {
			// If that condition is set, then we want the signed 9
			// bit PCoffset provided.
			PCoffset = ((int16_t) ((simulator->IR & 0x1ff) << 7)) >> 7;
			// Which is then added to the current program counter.
			simulator->PC = simulator->PC + PCoffset;
		}
		break;
	case LDR:
		DR = &simulator->registers[(simulator->IR >> 9) & 7];
		SR1 = simulator->registers[(simulator->IR >> 6) & 7];
		PCoffset = ((int16_t) ((simulator->IR & 0x3f) << 9)) >> 9;
		*DR = simulator->memory[SR1 + PCoffset].value;
		setcc(DR, &simulator->CC);
		break;
	case ST:
		SR1 = simulator->registers[(simulator->IR >> 9) & 7];
		PCoffset = ((int16_t) (simulator->IR & 0x1ff) << 7) >> 7;
		simulator->memory[simulator->PC + PCoffset].value = SR1;
		break;
	case STR:
		SR1 = simulator->registers[(simulator->IR >> 9) & 7];
		SR2 = simulator->registers[(simulator->IR >> 6) & 7];
		PCoffset = ((int16_t) ((simulator->IR & 0x003f) << 9)) >> 9;
		simulator->memory[SR2 + PCoffset].value = SR1;
		break;
	case STI:
		SR1 = simulator->registers[(simulator->IR >> 9) & 7];
		PCoffset = ((int16_t) (simulator->IR & 0x01ff) << 7) >> 7;
		simulator->memory[
			simulator->memory[
				simulator->PC + PCoffset].value].value = SR1;
		break;
	case JMP:
		SR1 = simulator->registers[(simulator->IR >> 6) & 7];
		simulator->PC = SR1;
		break;
	case JSR:
		simulator->registers[7] = simulator->PC;
		if ((simulator->IR & 0x800) == 0x800) {
			PCoffset = ((int16_t) ((simulator->IR & 0x7ff) << 5)) >> 5;
			simulator->PC = simulator->PC + (int16_t) PCoffset;
		} else {
			SR1 = simulator->registers[(simulator->IR >> 6) & 7];
			simulator->PC = SR1;
		}
		break;
	default:
		break;
	}

	//simulator->memory[0xFE04].value = 0x8000;
	//if (simulator->memory[0xFE06].value) {
	//	waddch(output, simulator->memory[0xFE06].value & 0xff);
	//	wrefresh(output);
	//	simulator->memory[0xFE06].value = 0x0;
	//}
}

