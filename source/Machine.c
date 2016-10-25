#include <string.h> // strlen is helpful.
#include <stdlib.h> // uint16_t.
#include <ctype.h>  // isxdigit

#include "Keyboard.h"
#include "Machine.h"
#include "Logging.h"
#include "Memory.h"
#include "LC3.h"

static WINDOW *status, *output, *context;
static int MSGWIDTH, MSGHEIGHT;
int mem_populated = -1;

static struct LC3 const init_state = {
	.memory	   = {{ 0 }},
	.registers =  { 0 },
	.IR	   =    0,
	.PC	   =    0,
	.CC	   =   'Z',
	.isHalted  =  false,
	.isPaused  =   true,
};

static void prompt(char const *err, char const *message, char *file)
{
	int msgwidth = MSGWIDTH + strlen(message);

	WINDOW *popup = newwin(MSGHEIGHT, msgwidth, (LINES - MSGHEIGHT) / 2,
			(COLS - msgwidth) / 2);

	box(popup, 0, 0);
	echo();

	if (err != NULL) {
		mvwaddstr(popup, MSGHEIGHT / 2 - 1,
			(MSGWIDTH - strlen(err)) / 2, err);
	}

	mvwaddstr(popup, MSGHEIGHT / 2, 1, message);
	wgetstr(popup, file);
	noecho();
}

static int init_machine(struct program *prog)
{
	int ret;
	prog->simulator = init_state;

	while ((ret = populate_memory(prog)) == 1) {
		prompt("Invalid file name.", "Enter the .obj file: ",
			prog->objectfile);
	}

	return ret;
}

/*
 * Print the current state at the top of the screen.
 */

static void sstate(enum STATE *currentState)
{
	mvprintw(0, 0, "Currently Viewing: %- 15s",
		(*currentState == MEM)  ? "Memory View" :
		(*currentState == MAIN) ? "Main View" :
		(*currentState == EDIT) ? "Editor" :
		(*currentState == SIM)  ? "Simulator" :
		"Unknown");
	refresh();

	touchwin(status);
	touchwin(output);
	touchwin(context);

	wnoutrefresh(status);
	wnoutrefresh(output);
	wnoutrefresh(context);

	doupdate();
}

static int popup_window(char const *message, int original_value)
{
	int msgwidth = MSGWIDTH + strlen(message);
	char string[7] = { 0 };

	WINDOW *popup = newwin(MSGHEIGHT, msgwidth, (LINES - MSGHEIGHT) / 2,
			(COLS - msgwidth) / 2);

	box(popup, 0, 0);
	echo();

	mvwaddstr(popup, 2, 1, message);
	wgetstr(popup, string);
	string[6] = '\0';

	char *end = NULL;
	int ret = (int) strtol(string, &end, 16);

	if (*end || strlen(string) == 0)
		ret = original_value;

	noecho();
	delwin(popup);

	return ret;
}

static bool simview(WINDOW *output, WINDOW *status, struct program *prog,
			enum STATE *currentState)
{
	int input;
	static int timeout = 0;

	sstate(currentState);

	while (1) {
		wtimeout(status, timeout);
		input = wgetch(status);

		if (prog->simulator.memory[prog->simulator.PC].isBreakpoint) {
			popup_window("Breakpoint hit!", 0);
			prog->simulator.isPaused = true;
			prog->simulator.memory[prog->simulator.PC]
				.isBreakpoint = false;
		}

		if (input == QUIT) {
			return false;
		} else if (input == GOBACK) {
			*currentState = MAIN;
			prog->simulator.isPaused = true;
			return true;
		} else if (input == PAUSE) {
			prog->simulator.isPaused = !(prog->simulator.isPaused);
		} else if (input == START || input == RUN) {
			if (!prog->simulator.isHalted)
				prog->simulator.isPaused = false;
		} else if (input == RESTART) {
			mem_populated = -1;
			init_machine(prog);
			wclear(output);
			wrefresh(output);
		} else if (input == STEP_NEXT) {
			execute_next(&(prog->simulator), output);
			prog->simulator.isPaused = true;
			print_state(&(prog->simulator), status);
		} else if (input == CONTINUE) {
			mem_populated = -1;
			init_machine(prog);
		} else if (input == CONTINUE_RUN) {
			mem_populated = -1;
			init_machine(prog);
			prog->simulator.isPaused = false;
		}

		if (!(prog->simulator.isPaused) && !(prog->simulator.isHalted)) {
			execute_next(&(prog->simulator), output);
			timeout = 0;
		} else {
			sstate(currentState);
			print_state(&(prog->simulator), status);
			timeout = -1;
			generate_context(context, prog, 0, prog->simulator.PC);
		}
	}
}

static bool mainview(WINDOW *status, enum STATE *currentState,
			struct program *prog)
{
	int input;

	while (1) {
		sstate(currentState);
		input = wgetch(status);
		if (input == QUIT) {
			return false;
		} else if (input == LOGDUMP) {
			if (logDump(prog))
				return false;
		} else if (input == SIMVIEW) {
			*currentState = SIM;
			return true;
		} else if (input == MEMVIEW) {
			*currentState = MEM;
			return true;
		} else if (input == FILESEL) {
			prompt((char const *) NULL, "Enter the .obj file: ",
				prog->objectfile);
			if (init_machine(prog))
				return false;
		}
	}
}

static bool memview(WINDOW *window, struct program *prog,
		enum STATE *currentState)
{
	int input, jump_addr, new_value;

	while (1) {
		sstate(currentState);
		input = wgetch(window);
		if (input == QUIT) {
			return false;
		} else if (input == GOBACK) {
			*currentState = MAIN;
			return true;
		} else if (input == JUMP) {
			jump_addr = popup_window(
				"Enter a hex address to jump to: ",
				selected_address);
			if (jump_addr == selected_address)
				continue;
			generate_context(window, prog, 0, jump_addr);
		} else if (input == KEYUP) {
			move_context(window, prog, UP);
		} else if (input == KEYDOWN) {
			move_context(window, prog, DOWN);
		} else if (input == EDITFILE) {
			new_value = popup_window(
				"Enter the new instruction (in hex): ",
				prog->simulator.memory[selected_address].value);
			prog->simulator.memory[selected_address].value =
				new_value;
			update(window, prog);
		} else if (input == SETPC) {
			prog->simulator.PC = selected_address;
		} else if (input == BREAKPOINTSET) {
			prog->simulator.memory[selected_address]
				.isBreakpoint = !prog->simulator.memory[
					selected_address].isBreakpoint;
		}
	}
}

static void run_machine(struct program *prog)
{
	bool simulating = true;
	enum STATE currentState = MAIN;

	output_height = 2 * (LINES - 6) / 3 - 2;
	memory_output = malloc(sizeof(uint16_t) * output_height);
	if (NULL == memory_output) {
		perror("LC3-Simulator");
		exit(EXIT_FAILURE);
	}

	generate_context(context, prog, 0, prog->simulator.PC);

	while (simulating) {
		sstate(&currentState);

		switch (currentState) {
		case MAIN:
			simulating = mainview(status, &currentState, prog);
			break;
		case SIM:
			simulating = simview(output, status, prog,
					&currentState);
			break;
		case MEM:
			if (mem_populated == -1) {
				generate_context(context, prog, 0,
						prog->simulator.PC);
			} else {
				generate_context(context, prog, selected,
						mem_populated);
			}
			simulating = memview(context, prog, &currentState);
			break;
		case EDIT:
			break;
		default:
			break;
		}
	}

	free(memory_output);
}

void exit_handle(void)
{
	fflush(stdout);
	delwin(status);
	delwin(output);
	delwin(context);
	endwin();
}

void start_machine(struct program *prog)
{
	initscr();
	atexit(exit_handle);

	raw();
	curs_set(0);
	noecho();
	cbreak();
	start_color();
	init_pair(1, COLOR_RED, COLOR_BLACK);

	MSGHEIGHT = 5;
	MSGWIDTH  = COLS / 2;

	status	= newwin(6, COLS, 1, 0);
	output	= newwin((LINES - 6) / 3, COLS, 7, 0);
	context = newwin(2 * (LINES - 6) / 3, COLS, (LINES - 6) / 3 + 7, 0);

	box(status, 0, 0);
	box(context, 0, 0);

	keypad(context, 1);

	scrollok(output, 1);

	if (NULL == prog->objectfile) {
		prog->objectfile = malloc(sizeof(char) * MSGWIDTH);
		prompt(NULL, "Enter the .obj file: ", prog->objectfile);
	}

	prog->simulator = init_state;
	if (!init_machine(prog))
		run_machine(prog);

	delwin(status);
	delwin(output);
	delwin(context);
	endwin();
}
