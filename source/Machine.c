#include <string.h> // strlen is helpful.
#include <stdlib.h> // uint16_t.

#include "Keyboard.h"
#include "Machine.h"
#include "Logging.h"
#include "Memory.h"
#include "LC3.h"

static WINDOW *status, *output, *context;
static int MESSAGE_WIDTH, MESSAGE_HEIGHT;
int memPopulated = -1;

static struct LC3 const init_state = {
        .CC        =    'Z',
        .isHalted  =  false,
        .isPaused  =   true,
};

static void prompt(char const *err, char const *message, char *file)
{
        int messageWidth = MESSAGE_WIDTH + (int) strlen(message);

        WINDOW *popup = newwin(MESSAGE_HEIGHT, messageWidth,
                (LINES - MESSAGE_HEIGHT) / 2, (COLS - messageWidth) / 2);

        box(popup, 0, 0);
        echo();

        if (NULL != err) {
                mvwaddstr(popup, MESSAGE_HEIGHT / 2 - 1,
                        (MESSAGE_WIDTH - (int) strlen(err)) / 2, err);
        }

        mvwaddstr(popup, MESSAGE_HEIGHT / 2, 1, message);
        wgetstr(popup, file);
        noecho();
}

static int init_machine(struct program *program)
{
        int ret;
        program->simulator = init_state;

        while (1 == (ret = populateMemory(program))) {
                prompt("Invalid file name.", "Enter the .obj file: ",
                        program->objectfile);
        }

        return ret;
}

/*
 * Print the current state at the top of the screen.
 */

static void set_state(enum STATE *currentState)
{
        mvprintw(0, 0, "Currently Viewing: %- 15s",
                (MEM == *currentState) ? "Memory View" :
                (MAIN == *currentState) ? "Main View" :
                (SIM == *currentState) ? "Simulator" :
                /* Default */     "Unknown");
        refresh();

        touchwin(status);
        touchwin(output);
        touchwin(context);

        wnoutrefresh(status);
        wnoutrefresh(output);
        wnoutrefresh(context);

        doupdate();
}

static int popup_window(char const *message, int original_value, bool ignore)
{
        int message_width = MESSAGE_WIDTH + (int) strlen(message);
        int ret = 0;
        char string[7] = {0};
        char *end = NULL;

        WINDOW *popup = newwin(MESSAGE_HEIGHT, message_width,
                (LINES - MESSAGE_HEIGHT) / 2, (COLS - message_width) / 2);

        box(popup, 0, 0);
        echo();

        mvwaddstr(popup, 2, 1, message);

        if (ignore) {
                wgetch(popup);
        } else {
                wgetnstr(popup, string, 6);
                string[6] = '\0';

                ret = (int) strtol(string, &end, 16);
                if (*end || !strlen(string)) {
                        ret = original_value;
                }
        }

        noecho();
        delwin(popup);

        return ret;
}

static bool simulator_view(WINDOW *out, WINDOW *state, struct program *program,
                           enum STATE *current_state)
{
        int input;
        static int timeout = 0;

        set_state(current_state);

        while (1) {
                wtimeout(state, timeout);
                input = wgetch(status);

                if (program->simulator.memory[program->simulator.PC].isBreakpoint) {
                        popup_window("Breakpoint hit!", 0, true);
                        program->simulator.isPaused = true;
                        program->simulator.memory[program->simulator.PC]
                                .isBreakpoint = false;
                }

                if (QUIT == input) {
                        return false;
                } else if (GOBACK == input) {
                        *current_state = MAIN;
                        program->simulator.isPaused = true;
                        return true;
                } else if (PAUSE == input) {
                        program->simulator.isPaused = !program->simulator.isPaused;
                } else if (START == input || RUN == input) {
                        program->simulator.isPaused = program->simulator.isHalted;
                } else if (RESTART == input) {
                        memPopulated = -1;
                        init_machine(program);
                        wclear(out);
                        wrefresh(out);
                } else if (STEP_NEXT == input) {
                        executeNext(&(program->simulator), output);
                        program->simulator.isPaused = true;
                        printState(&(program->simulator), state);
                } else if (CONTINUE == input) {
                        memPopulated = -1;
                        init_machine(program);
                } else if (CONTINUE_RUN == input) {
                        memPopulated = -1;
                        init_machine(program);
                        program->simulator.isPaused = false;
                }

                if (!program->simulator.isPaused && !program->simulator.isHalted) {
                        executeNext(&(program->simulator), out);
                        timeout = 0;
                } else {
                        set_state(current_state);
                        printState(&(program->simulator), state);
                        timeout = -1;
                        generateContext(context, program, 0, program->simulator.PC);
                }
        }
}

static bool main_view(WINDOW *state, enum STATE *current_state,
                      struct program *program)
{
        int input;

        while (1) {
                set_state(current_state);
                input = wgetch(state);
                if (QUIT == input) {
                        return false;
                } else if (LOGDUMP == input) {
                        if (logDump(program)) {
                                return false;
                        }
                } else if (SIMVIEW == input) {
                        *current_state = SIM;
                        return true;
                } else if (MEMVIEW == input) {
                        *current_state = MEM;
                        return true;
                } else if (FILESEL == input) {
                        prompt((char const *) NULL, "Enter the .obj file: ",
                                program->objectfile);
                        if (init_machine(program)) {
                                return false;
                        }
                }
        }
}

static bool memory_view(WINDOW *window, struct program *program,
                        enum STATE *current_state)
{
        int input, jump_address, new_value;

        while (1) {
                set_state(current_state);
                input = wgetch(window);
                if (QUIT == input) {
                        return false;
                } else if (GOBACK == input) {
                        *current_state = MAIN;
                        return true;
                } else if (JUMP == input) {
                        jump_address = popup_window(
                                "Enter a hex address to jump to: ",
                                selectedAddress, false);
                        if (jump_address == selectedAddress)
                                continue;
                        generateContext(window, program, 0, (uint16_t) jump_address);
                } else if (KEYUP == input) {
                        moveContext(window, program, UP);
                } else if (KEYDOWN == input) {
                        moveContext(window, program, DOWN);
                } else if (EDITFILE == input) {
                        new_value = popup_window(
                                "Enter the new instruction (in hex): ",
                                program->simulator.memory[selectedAddress].value,
                                false);
                        program->simulator.memory[selectedAddress].value =
                                (uint16_t) new_value;
                        update(window, program);
                } else if (SETPC == input) {
                        program->simulator.PC = selectedAddress;
                } else if (BREAKPOINTSET == input) {
                        program->simulator.memory[selectedAddress]
                                .isBreakpoint = !program->simulator.memory[
                                selectedAddress].isBreakpoint;
                }
        }
}

static void run_machine(struct program *program)
{
        bool simulating = true;
        enum STATE currentState = MAIN;

        outputHeight = (uint16_t) (2 * (LINES - 6) / 3 - 2);
        memoryOutput = malloc(sizeof(uint16_t) * outputHeight);
        if (NULL == memoryOutput) {
                perror("LC3-Simulator");
                exit(EXIT_FAILURE);
        }

        generateContext(context, program, 0, program->simulator.PC);

        while (simulating) {
                set_state(&currentState);

                switch (currentState) {
                case MAIN:
                        simulating = main_view(status, &currentState, program);
                        break;
                case SIM:
                        simulating = simulator_view(output, status, program,
                                &currentState);
                        break;
                case MEM:
                        if (-1 == memPopulated) {
                                generateContext(context, program, 0,
                                        program->simulator.PC);
                        } else {
                                generateContext(context, program, selected,
                                        (uint16_t) memPopulated);
                        }
                        simulating = memory_view(context, program, &currentState);
                        break;
                default:
                        break;
                }
        }

        free(memoryOutput);
        memoryOutput = NULL;
}

static void exit_handle(void)
{
        fflush(stdout);

        delwin(status);
        delwin(output);
        delwin(context);
        endwin();

        if (NULL != memoryOutput) {
                free(memoryOutput);
        }
}

void startMachine(struct program *program)
{
        initscr();
        atexit(exit_handle);

        raw();
        curs_set(0);
        noecho();
        cbreak();
        start_color();
        init_pair(1, COLOR_RED, COLOR_BLACK);

        MESSAGE_HEIGHT = 5;
        MESSAGE_WIDTH = COLS / 2;

        status = newwin(6, COLS, 1, 0);
        output = newwin((LINES - 6) / 3, COLS, 7, 0);
        context = newwin(2 * (LINES - 6) / 3, COLS, (LINES - 6) / 3 + 7, 0);

        box(status, 0, 0);
        box(context, 0, 0);

        keypad(context, 1);

        scrollok(output, 1);

        if (NULL == program->objectfile) {
                program->objectfile = malloc(sizeof(char) * (size_t) MESSAGE_WIDTH);
                prompt(NULL, "Enter the .obj file: ", program->objectfile);
        }

        program->simulator = init_state;

        if (!init_machine(program)) {
                run_machine(program);
        }

        delwin(status);
        delwin(output);
        delwin(context);
        endwin();
}

