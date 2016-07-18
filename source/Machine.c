#include <string.h> // strlen is helpful.
#include <stdlib.h> // uint16_t.

#include "Machine.h"
#include "Memory.h"
#include "LC3.h"

static WINDOW *status, *output, *context;
static int MSGWIDTH, MSGHEIGHT;
int mem_populated = -1;

static const struct LC3 init_state = {
	.PC	   =	 0,
	.memory	   = { { 0 } },
	.registers = { 0 },
	.IR	   =	 0,
	.CC	   = 'Z',
	.isHalted  = false,
	.isPaused  = true,
};

static void prompt(const char *err, const char *message, char *file)
{
	int msgwidth = MSGWIDTH + strlen(message);

	WINDOW *popup = newwin(MSGHEIGHT, msgwidth, (LINES - MSGHEIGHT) / 2,
			       (COLS - msgwidth) / 2);

	box(popup, 0, 0);
	echo();

	if (err != NULL) // We have an error message to show the user.
		mvwaddstr(popup, MSGHEIGHT / 2 - 1, (MSGWIDTH - strlen(err)) / 2, err);

	mvwaddstr(popup, MSGHEIGHT / 2, 1, message);
	wgetstr(popup, file);
	noecho();
}

static int init_machine(struct program *prog)
{
	int ret;
	prog->simulator = init_state;
	while ((ret = populate_memory(prog)) == 1)
		prompt("Invalid file name.", "Enter the .obj file: ", prog->infile);
	return ret;
}

/*
 * Print the current state at the top of the screen.
 */

static void sstate(enum STATE *currentState)
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

static int popup_window(const char *message)
{
	int ret, msgwidth = MSGWIDTH + strlen(message);
	char string[7];

	WINDOW *popup = newwin(MSGHEIGHT, msgwidth, (LINES - MSGHEIGHT) / 2,
			       (COLS - msgwidth) / 2);

	box(popup, 0, 0);
	echo();

	mvwaddstr(popup, 2, 1, message);
	wgetstr(popup, string);

	string[6] = '\0';
	ret	  = (int) strtol(string, (char **) NULL, 16);

	noecho();
	delwin(popup);

	return ret;
}

static bool simview(WINDOW *output, WINDOW *status, struct program *prog,
		    enum STATE *currentState)
{
	int input;

	sstate(currentState);

	wtimeout(status, 0);
	while (1) {
		switch (input = wgetch(status)) {
		case 'q': // FALLTHROUGH
		case 'Q': // Quit the program.
			return false;
		case  27: // 27 is the escape key scan code
		case 'b': // FALLTHROUGH
		case 'B': // Go back to the main state.
			*currentState = MAIN;
			prog->simulator.isPaused = true;
			return true;
		case 'p':
		case 'P':
			prog->simulator.isPaused = !(prog->simulator.isPaused);
			break;
		case 's': // FALLTHROUGH
		case 'S': // FALLTHROUGH
		case 'r': // FALLTHROUGH
		case 'R': // Reset the machine to it's original state.
			init_machine(prog);
			prog->simulator.isPaused = input == 'R' || input == 'S';
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
	}
}

static bool mainview(WINDOW *status, enum STATE *currentState)
{
	int input;

	sstate(currentState);

	while (1) {
		switch (input = wgetch(status)) {
		case 'q': // FALLTHROUGH
		case 'Q': // Quit the program.
			return false;
		case 'd': // For file dumps.
		case 'D': // TODO
			break;
		case 's': // FALLTHROUGH
		case 'S': // Start simulating the machine.
			*currentState = SIM;
			return true;
		case 'm': // FALLTHROUGH
		case 'M': // View the memory contents.
			*currentState = MEM;
			return true;
		default:
			break;
		}
	}
}

static bool memview(WINDOW *window, struct program *prog,
		    enum STATE *currentState)
{
	int input, jump_addr;

	sstate(currentState);

	prog->simulator.isPaused = true;

	while (1) {
		switch (input = wgetch(window)) {
		case 'q': // FALLTHROUGH
		case 'Q': // Quit the program.
			return false;
		case 'b': // FALLTHROUGH
		case 'B': // Go back to the main view.
			*currentState = MAIN;
			return true;
		case 'j': // FALLTHROUGH
		case 'J': // Jump to an address in memory.
			jump_addr = popup_window("Enter a hex address to jump to: ");
			generate_context(window, &(prog->simulator), 0, jump_addr);
			touchwin(window);
			break;
		case KEY_UP: // FALLTHROUGH
		case 'w': // FALLTHROUGH
		case 'W': // Up button pressed, so move the view up.
			move_context(window, &(prog->simulator), UP);
			break;
		case KEY_DOWN: // FALLTHROUGH
		case 's': // FALLTHROUGH
		case 'S': // Down button pressed, so move the view down.
			move_context(window, &(prog->simulator), DOWN);
			break;
		case 'e': // FALLTHROUGH
		case 'E': // User wants to edit the currently selected value.
			*currentState = EDIT;
			return true;
		default:
			break;
		}
	}
}

static void run_machine(struct program *prog)
{
	bool simulating		= true;
	enum STATE currentState = MAIN;

	output_height = 2 * (LINES - 6) / 3 - 2;
	memory_output = (uint16_t *) malloc(sizeof(uint16_t) * output_height);

	while (simulating) {
		switch (currentState) {
		case MAIN:
			simulating = mainview(status, &currentState);
			break;
		case SIM:
			simulating = simview(output, status, prog, &currentState);
			break;
		case MEM:
			if (mem_populated == -1) {
				generate_context(context, &(prog->simulator), 0,
						 prog->simulator.PC);
			} else {
				generate_context(context, &(prog->simulator), selected,
						 mem_populated);
			}
			simulating = memview(context, prog, &currentState);
			break;
		case EDIT:
			currentState = MAIN;
			break;
		default:
			break;
		}
	}

	free(memory_output);
}

void start_machine(struct program *prog)
{
	initscr();

	raw();
	curs_set(0);
	noecho();
	cbreak();
	start_color();

	MSGHEIGHT = 5;
	MSGWIDTH  = COLS / 2;

	if (prog->infile == NULL) {
		prog->infile = (char *) malloc(sizeof(char) * MSGWIDTH);
		prompt((const char *) NULL, "Enter the .obj file: ", prog->infile);
	}

	status	= newwin(6, COLS, 1, 0);
	output	= newwin((LINES - 6) / 3, COLS, 7, 0);
	context = newwin(2 * (LINES - 6) / 3, COLS, (LINES - 6) / 3 + 7, 0);

	box(status, 0, 0);
	box(context, 0, 0);

	keypad(context, 1);

	scrollok(output, 1);

	prog->simulator = init_state;
	if (init_machine(prog)) return;

	run_machine(prog);

	endwin();
}
