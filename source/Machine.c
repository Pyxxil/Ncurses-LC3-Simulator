#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "LC3.h"
#include "Machine.h"
#include "Enums.h"

static char *file_name = NULL;

static const struct LC3 init_state = {
	.PC		=   0  ,
	.memory		= { 0 },
	.registers	= { 0 },
	.IR		=   0  ,
	.CC		=  'Z' ,
	.halted		= false,
	.isPaused	=  true,
};

static void init_machine(struct LC3 *simulator)
{
	*simulator = init_state;
        populate_memory(simulator, file_name);
}

static bool simulate(WINDOW *output, WINDOW *status,
                struct LC3 *simulator, enum STATE *currentState)
{
	int input;
	bool simulating = true;

	wtimeout(status, 0);
	while (simulating) {
		switch (input = wgetch(status)) {
		case 'q':
		case 'Q':
			// Quit the program.
			simulating = false;
			break;
		case  27: // 27 is the escape key scan code
		case 'b':
		case 'B':
			// Go back to the main state.
			*currentState = MAIN;
			simulator->isPaused = true;
			return simulating;
		case 'p':
		case 'P':
			simulator->isPaused = !simulator->isPaused;
			break;
		case 'r':
		case 'R':
			init_machine(simulator);
                        if (input == 'R')
                                simulator->isPaused = false;
			wclear(output);
			wrefresh(output);
                        break;
		default:
			break;
		}
		if (!simulator->isPaused && !simulator->halted) {
			execute_next(simulator, output);
			wtimeout(status,  0);
		} else {
			print_state(simulator, status);
			wtimeout(status, -1);
		}

#ifdef DEBUG
  #if (DEBUG & 0x2)
		print_state(simulator, status);
  #endif
  #if (DEBUG & 0x4)
		simulator->isPaused = true;
  #endif
#endif

	}

	return simulating;

}

static bool run_main_ui(WINDOW *status, enum STATE *currentState)
{
	int input;
	bool simulating = true;

	while (simulating) {
		switch (input = wgetch(status)) {
		case 'q':
		case 'Q':
			// Quit the program.
			return false;
		case 'd': // For file dumps.
		case 'D': // TODO
			break;
		case 's':
		case 'S':
			// Start simulating the machine.
			*currentState = SIM;
			return true;
		default:
			break;
		}
	}

	return simulating;
}

static bool view_memory(WINDOW *context, struct LC3 *simulator,
			enum STATE *currentState)
{
	int input;
	bool simulating = true;

	while (simulating) {
		switch (input = wgetch(context)) {
		case 'q':
		case 'Q':
			return false;
		case 'b':
		case 'B':
			*currentState = MAIN;
			return true;
		default:
			break;
		}
	}

	return simulating;
}

void run_machine(struct LC3 *simulator)
{
	bool simulating = true;
	WINDOW *status, *output, *context;
	enum STATE currentState = MAIN;

	initscr();

	raw();
	curs_set(0);
	noecho();
	cbreak();

	status  = newwin(6, COLS, 0, 0);
	output  = newwin((LINES - 6) / 3, COLS, 6, 0);
	context = newwin(2 * (LINES - 6) / 3 + 1, COLS,
			 (LINES - 6) / 3 + 6, 0);

	box(status,  0, 0);
	box(context, 0, 0);

	wrefresh(status);
	wrefresh(output);
	wrefresh(context);

	scrollok(output, 1);

	simulator->isPaused = false;
	print_state(simulator, status);

	while (simulating) {
		switch (currentState) {
		case MAIN:
			simulating = run_main_ui(status, &currentState);
			break;
		case SIM:
			simulating = simulate(output, status, simulator,
						 &currentState);
			break;
		case MEM:
			simulating = view_memory(context, simulator,
						 &currentState);
			break;
		case EDIT:
			break;
		default:
			break;
		}
	}

	endwin();
}

void start_machine(char *file)
{
	size_t len = strlen(file) + 1;
	file_name = (char *) malloc(len);
	strncpy(file_name, file, len);
	struct LC3 simulator = init_state;
	init_machine(&simulator);
	run_machine(&simulator);
	free(file_name);
}

