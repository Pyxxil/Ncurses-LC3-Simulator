#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "Memory.h"
#include "Machine.h"

static char *file_name = NULL;

static WINDOW *status, *output, *context;


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

static void print_view(enum STATE *currentState)
{
	mvprintw(0, 0, "Currently Viewing: %- 15s",
			(*currentState == MEM)  ? "Memory View"	:
			(*currentState == MAIN) ? "Main View"	:
			(*currentState == EDIT) ? "Editor"	:
			(*currentState == SIM)  ? "Simulator"	:
						"Unknown");
	refresh();

	wrefresh(status);
	wrefresh(output);
	wrefresh(context);
}

static bool simulate(WINDOW *output, WINDOW *status,
		struct LC3 *simulator, enum STATE *currentState)
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
			simulator->isPaused = true;
			return simulating;
		case 'p':
		case 'P':
			simulator->isPaused = !simulator->isPaused;
			break;
		case 's':
		case 'S':
		case 'r':
		case 'R':
			init_machine(simulator);
			if (input == 'R' || input == 'S')
				simulator->isPaused = false;
			if (input == 'R' || input == 'r') {
				wclear(output);
				wrefresh(output);
			}
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
}

static bool view_memory(WINDOW *window, struct LC3 *simulator,
			enum STATE *currentState)
{
	int input;
	bool simulating = true;

	print_view(currentState);

	simulator->isPaused = true;

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
			create_context(window, simulator, 0, 0x0000);
			break;
		case 'J':
			create_context(window, simulator, output_height - 1, 0xfffe);
			break;
		case KEY_UP:
		case 'w':
		case 'W':
			move_context(window, simulator, UP);
			break;
		case KEY_DOWN:
		case 's':
		case 'S':
			move_context(window, simulator, DOWN);
			break;
		default:
			break;
		}
	}

	return simulating;
}

static void popup_window(const char *message)
{
	int lines = 5;
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

void run_machine(struct LC3 *simulator)
{
	bool simulating = true;
	enum STATE currentState = MAIN;

	initscr();

	raw();
	curs_set(0);
	noecho();
	cbreak();
	start_color();

	status  = newwin(6, COLS, 1, 0);
	output  = newwin((LINES - 6) / 3, COLS, 7, 0);
	context = newwin(2 * (LINES - 6) / 3, COLS,
			(LINES - 6) / 3 + 7, 0);

	box(status,  0, 0);
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
			simulating = simulate(output, status, simulator,
						&currentState);
			break;
		case MEM:
			create_context(context, simulator, 0, simulator->PC);
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
	free(memory_output);
}

void start_machine(const char *file)
{
	size_t len = strlen(file) + 1;
	file_name = (char *) malloc(len);
	strncpy(file_name, file, len);
	struct LC3 simulator = init_state;
	init_machine(&simulator);
	run_machine(&simulator);
	free(file_name);
}

