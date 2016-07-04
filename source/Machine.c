#include <string.h> // strlen is helpful.
#include <stdlib.h> // uint16_t.

#include "Machine.h"
#include "Memory.h"
#include "LC3.h"

static WINDOW *status, *output, *context;

static const struct LC3 init_state = {
	.PC	   =	 0,
	.memory	   = { { 0 } },
	.registers = { 0 },
	.IR	   =	 0,
	.CC	   = 'Z',
	.isHalted  = false,
	.isPaused  = true,
};

static void init_machine(struct program *prog)
{
	prog->simulator = init_state;
	populate_memory(prog);
}

static void print_view(enum STATE *currentState)
{
	mvprintw(0, 0, "Currently Viewing: %- 15s",
		 (*currentState == MEM) ? "Memory View" :
		 (*currentState == MAIN) ? "Main View" :
		 (*currentState == EDIT) ? "Editor" :
		 (*currentState == SIM) ? "Simulator" :
		 "Unknown");
	refresh();

	wrefresh(status);
	wrefresh(output);
	wrefresh(context);
}

static bool simulate(WINDOW *output, WINDOW *status,
		     struct program *prog, enum STATE *currentState)
{
	int input;
	bool simulating = true;

	print_view(currentState);

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
			prog->simulator.isPaused = true;
			return simulating;
		case 'p':
		case 'P':
			prog->simulator.isPaused = !(prog->simulator.isPaused);
			break;
		case 's':
		case 'S':
		case 'r':
		case 'R':
			init_machine(prog);
			if (input == 'R' || input == 'S')
				prog->simulator.isPaused = false;
			if (input == 'R' || input == 'r') {
				wclear(output);
				wrefresh(output);
			}
			break;
		default:
			break;
		}
		if (!(prog->simulator.isPaused) && !(prog->simulator.isHalted)) {
			execute_next(&(prog->simulator), output);
			wtimeout(status, 0);
		} else {
			print_state(&(prog->simulator), status);
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
} /* simulate */

static bool run_main_ui(WINDOW *status, enum STATE *currentState)
{
	int input;
	bool simulating = true;

	print_view(currentState);

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
		case 'm':
		case 'M':
			*currentState = MEM;
			return true;
		default:
			break;
		}
	}

	return simulating;
} /* run_main_ui */

static bool view_memory(WINDOW *window, struct program *prog,
			enum STATE *currentState)
{
	int input;
	bool simulating = true;

	print_view(currentState);

	prog->simulator.isPaused = true;

	while (simulating) {
		switch (input = wgetch(window)) {
		case 'q':
		case 'Q':
			return false;
		case 'b':
		case 'B':
			*currentState = MAIN;
			return true;
		case 'j':
			generate_context(window, &(prog->simulator), 0, 0x0000);
			break;
		case 'J':
			generate_context(window, &(prog->simulator),
					 output_height - 1, 0xfffe);
			break;
		case KEY_UP:
		case 'w':
		case 'W':
			move_context(window, &(prog->simulator), UP);
			break;
		case KEY_DOWN:
		case 's':
		case 'S':
			move_context(window, &(prog->simulator), DOWN);
			break;
		default:
			break;
		}
	}

	return simulating;
} /* view_memory */

static void popup_window(const char *message)
{
	int lines   = 5;
	int columns = COLS / 2;
	char string[7];

	WINDOW *popup = newwin(lines, columns, 10, 10);

	box(popup, 0, 0);
	echo();

	mvwaddstr(popup, 2, 1, message);
	wgetstr(popup, string);
	string[6] = '\0';

	noecho();

	wrefresh(popup);
}

void run_machine(struct program *prog)
{
	bool simulating		= true;
	enum STATE currentState = MAIN;

	initscr();

	raw();
	curs_set(0);
	noecho();
	cbreak();
	start_color();

	status	= newwin(6, COLS, 1, 0);
	output	= newwin((LINES - 6) / 3, COLS, 7, 0);
	context = newwin(2 * (LINES - 6) / 3, COLS,
			 (LINES - 6) / 3 + 7, 0);

	box(status, 0, 0);
	box(context, 0, 0);

	keypad(context, 1);

	scrollok(output, 1);

	output_height = 2 * (LINES - 6) / 3 - 2;
	memory_output = (uint16_t *) malloc(output_height);

	popup_window("Enter a string: ");

	bkgdset(COLOR_BLACK);
	wbkgdset(status, COLOR_BLACK);
	wbkgdset(context, COLOR_BLACK);

	refresh();
	wrefresh(status);
	wrefresh(context);

	while (simulating) {
		switch (currentState) {
		case MAIN:
			simulating = run_main_ui(status, &currentState);
			break;
		case SIM:
			simulating = simulate(output, status, prog, &currentState);
			break;
		case MEM:
			generate_context(context, &(prog->simulator), 0,
					 prog->simulator.PC);
			simulating = view_memory(context, prog, &currentState);
			break;
		case EDIT:
			break;
		default:
			break;
		}
	}

	endwin();
	free(memory_output);
} /* run_machine */

void start_machine(struct program *prog)
{
	prog->simulator = init_state;
	init_machine(prog);
	run_machine(prog);
}
