#include <string.h>

#include "LC3.h"
#include "Machine.h"
#include "Enums.h"

char file_name[256];

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

static void run_machine(struct LC3 *simulator)
{
	WINDOW *status, *output, *context;

	state currentState = MAIN;

	initscr();

	raw();
	curs_set(0);
	noecho();
	cbreak();

	status  = newwin(6, COLS, 0, 0);
	output  = newwin((LINES - 6) / 3, COLS, 6, 0);
	context = newwin(2 * (LINES - 6) / 3 + 1, COLS, (LINES - 6) / 3 + 6, 0);

	box(status,  0, 0);
	box(context, 0, 0);

	wrefresh(status);
	wrefresh(output);
	wrefresh(context);

	scrollok(output, 1);

	bool simulating = true;
	int input;

	while (simulating) {
		switch (currentState) {
		case MAIN:
			switch (input = wgetch(status)) {
			case 'q':
			case 'Q':
				// Quit the program.
				simulating = false;
				break;
			case 's':
			case 'S':
				// Start simulating the machine.
				currentState = SIMULATING;
				simulator->isPaused = false;
				print_state(simulator, status);
				break;
			default:
				break;
			}
			break;
		case SIMULATING:
			wtimeout(status, 0);
			while ((currentState == SIMULATING) && simulating) {
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
					currentState = MAIN;
					simulator->isPaused = true;
					break;
				case 'p':
				case 'P':
					// Toggle the paused state.
					simulator->isPaused =
						!simulator->isPaused;
					break;
				case 'r':
				case 'R':
					// Reset the simulator.
					init_machine(simulator);
					print_state(simulator, status);
					wclear(output);
					wtimeout(status, -1);
					break;
				default:
					break;
				}
				if (!simulator->isPaused && !simulator->halted) {
					simulate(simulator, output);
					wtimeout(status,  0);
				} else if (simulator->isPaused) {
					print_state(simulator, status);
					wtimeout(status, -1);
				} else if (simulator->halted) {
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
			break;
		case MEM:
			switch (input = wgetch(context)) {
			case 'q':
			case 'Q':
				simulating = false;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}

	// Delete all of the windows, and return to normal mode.
	endwin();
}

void start_machine(char *file)
{
	memcpy(file_name, file, 256);
	struct LC3 simulator = init_state;
	init_machine(&simulator);
	run_machine(&simulator);
}

