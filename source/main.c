#include <stdbool.h>	// For boolean values.
#include <string.h>	// For strlen
#include <unistd.h>	// For getopts

#include "Enums.h"
#include "Error.h"
#include "LC3.h"

#define WORD_SIZE 2 // Word's in the LC3 are 16 bits (2 bytes).

// Some prototyped functions to use.
static unsigned short convert_bin_line(char *);

static void populate_memory(LC3 *, char *);
static void prompt_for_file(char *,   int);


int main(int argc, char **argv)
{
	char file[256] = { 0 };
	bool simulating = true;
	int input,
	    index = 0;

	// Our initial state that we can reset the simulator to.
	static const LC3 initialState = {
		.memory		= { 0 }, // All memory locations are zeroed.
		.registers	= { 0 }, // All registers are zeroed.
		.PC		=   0  , // The PC starts at 0.
		.IR		=   0  , // The IR starts at 0.
		.CC		=  'Z' , // The CC starts at Z.
		.halted		= false, // We haven't hit the HALT instruction
					 // yet.
		.isPaused	= true , // The simulator starts paused.
	};

	// Set the simulator to it's initial state.
	LC3 simulator = initialState;

	WINDOW *context, // What's happening in the memory.
	       *status,  // What's happening in the simulator.
	       *output;  // What's happening in the output.

	// Set the current state.
	state currentState = MAIN;

	int arg;

	while ((arg = getopt(argc, argv, "f:")) != -1) {
		switch (arg) {
		case 'f':
			for (; argv[optind - 1][index] != '\0'; ++index)
				file[index] = optarg[index];
			break;
		default:
			break;
		}
	}

	if (optind == argc) {
		if (strlen(file) == 0) {
			prompt_for_file(file, 256);
		}
	} else if (optind < argc) {
		if (((optind + 1) == argc) && (strlen(file) == 0)) {
			for (index = 0; argv[optind][index] != '\0'; ++index)
				file[index] = argv[optind][index];
		}
	}

	// Populate the memory with the contents of the file.
	populate_memory(&simulator, file);


	// Start curses mode.
	initscr();

	// Use some essential parts of curses.
	raw();
	curs_set(0);
	noecho();
	cbreak();

	// Create the windows we'll use.
	status  = newwin(6, COLS, 0, 0);
	output  = newwin((LINES - 6) / 3, COLS, 6, 0);
	context = newwin(2 * (LINES - 6) / 3 + 1, COLS, (LINES - 6) / 3 + 6, 0);

	// Put a border around some of the windows.
	box(status, 0, 0);
	box(context, 0, 0);

	// Refresh the windows.
	wrefresh(status);
	wrefresh(output);
	wrefresh(context);

	scrollok(output, 1);


	while (simulating) {
		switch (currentState) {
		case MAIN:
			switch (input = wgetch(status)) {
			case 'q': case 'Q':
				// Quit the program.
				simulating = false;
				break;
			case 's': case 'S':
				// Start simulating the machine.
				currentState = SIMULATING;
				simulator.isPaused = false;
				print_state(&simulator, status);
				break;
			default:	// Just in case.
				break;
			}
			break;
		case SIMULATING:
			wtimeout(status, 0);
			while ((currentState == SIMULATING) && simulating) {
				switch (input = wgetch(status)) {
				case 'q': case 'Q':
					// Quit the program.
					simulating = false;
					break;
				case 27: case 'b': case 'B':
					// Go back to the main state.
					// 27 is the escape key scan code
					currentState = MAIN;
					simulator.isPaused = true;
					break;
				case 'p': case 'P':
					// Toggle the paused state of the
					// simulator.
					simulator.isPaused =
						!simulator.isPaused;
					break;
				case 'r': case 'R':
					// Reset the simulator, and clear the
					// output.
					simulator = initialState;
					populate_memory(&simulator, file);
					print_state(&simulator, status);
					wclear(output);
					wtimeout(status, -1);
					break;
				default:	// Just in case.
					break;
				}
				if (!simulator.isPaused && !simulator.halted) {
					// If the simulator isn't paused and the
					// simulator isn't halted, then execute
					// the next instruction.
					simulate(&simulator, output);
					wtimeout(status,  0);
				} else if (simulator.isPaused) {
					// If the simulator is paused, then
					// print the current state of the
					// simulator to the screen.
					print_state(&simulator, status);
					wtimeout(status, -1);
                                } else if (simulator.halted) {
					// If the simulator is halted, then
					// print the current state of the
					// simulator to the screen.
					wtimeout(status, -1);
					print_state(&simulator, status);
				}
#ifdef DEBUG
				print_state(&simulator, status);
				simulator.isPaused = true;
#endif
			}
			break;
		case MEM:
			switch (input = wgetch(context)) {
			case 'q': case 'Q':
				// Quit the program.
				simulating = false;
				break;
			default:	// Just in case.
				break;
			}
			break;
		default:
			break;
		}
	}

	// Delete all of the windows, and return to normal mode.
	endwin();

	return 0;
}


static void populate_memory(LC3 *simulator, char *file_name)
{
	/*
	 *  Populate the memory of the supplied simulator
	 *  with the contents of the file provided.
	 *
	 */

	FILE *file = fopen(file_name, "rb");

	// If the file failed to open, complain and exit.
	if (!file || file == NULL)
		unable_to_open_file(file_name);

        unsigned short  tmp_PC = 0,	// A temporary PC used to store the
                                        // values in memory.
			value  = 0;	// What will be used to get the
					// instruction on the line.

	char buffer[2];

        // The first line of the file is where to start the PC at,
        // so let's read it.
	fread(buffer, sizeof(buffer), 1, file);
	simulator->PC = tmp_PC = convert_bin_line(buffer);

	// Read the file until its done
	while (fread(buffer, sizeof(buffer), 1, file) == 1) {
		value = convert_bin_line(buffer);
		simulator->memory[tmp_PC++] = value;
	}

	// If we couldn't read until the end of the file for
	// some reason, then complain and exit.
	if (!feof(file)) {
		// But, of course, close the file first.
		fclose(file);
		read_error();
	}

	//FILE *out = fopen("out.txt", "w");
	//for (; simulator->PC < tmp_PC; ++simulator->PC)
	//	fprintf(out, "%x\n", simulator->memory[simulator->PC]);
	//read_error();

	// Close the file
	fclose(file);
}

static void prompt_for_file(char *buffer, int size)
{
	/*
	 *  The file wasn't supplied as a parameter,
	 *  so prompt the user for it.
	 *
	 */

	printf("Enter the name of the object file: ");
	fgets(buffer, size, stdin);
	buffer[strlen(buffer) - 1] = '\0';
}

static unsigned short convert_bin_line(char *buffer)
{
	/*
	 * Convert a line of binary into a readable number.
	 *
	 * buffer -- A pointer to an array populated with the line we want
	 *		   to convert.
	 */

	unsigned short  number = 0,	// Where we will store the value of the
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

