#ifdef __linux__
#define _XOPEN_SOURCE 500
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Error.h"
#include "Parser.h"
#include "Machine.h"
#include "OptParse.h"

static struct program *program = NULL;

__attribute__((noreturn)) static void close(int sig)
{
        (void) sig;

        if (NULL != program) {
                tidyUp(program);
        }

        freeTable(&tableHead);

        exit(EXIT_FAILURE);
}

__attribute__((noreturn)) static void usage(char const *const name)
{
        printf("Usage: %s [options]                                                \n\n"
                        "Options:                                                    \n"
                        "  -a [--assemble] file   Assemble the given file.           \n"
                        "  -v [--verbose] <level> Set the verbosity of the assembler.\n"
                        "  -o [--assemble-only]   Only assemble the given program.   \n",
                name
        );

        exit(EXIT_SUCCESS);
}

int main(int argc, char **argv)
{
        struct program prog = {
                .name         = NULL,
                .logfile      = NULL,
                .assemblyfile = NULL,
                .symbolfile   = NULL,
                .hexoutfile   = NULL,
                .binoutfile   = NULL,
                .objectfile   = NULL,
                .verbosity    = 0,
                .warn         = true,
        };

        program = &prog;

        signal(SIGINT, close);

        int opts = 0;

        options _options[] = {
                {
                        .longOption = "assemble",
                        .shortOption = 'a',
                        .option = REQUIRED,
                },
                {
                        .longOption = "assemble-only",
                        .shortOption = 'o',
                        .option = NONE,
                },
                {
                        .longOption = "verbose",
                        .shortOption = 'v',
                        .option = OPTIONAL,
                },
                {
                        .longOption = "objectfile",
                        .shortOption = 'f',
                        .option = REQUIRED,
                },
                {
                        .longOption = "no-warn",
                        .shortOption = 'n',
                        .option = NONE,
                },
                {
                        .longOption = "help",
                        .shortOption = 'h',
                        .option = NONE,
                },
                {
                        NULL, '\0', NONE,
                },
        };

        int option = 0;

        while ((option = parseOptions(_options, argc, argv)) != 0) {
                switch (option) {
                case 'a':
                        if (returnedOption.option == NONE) {
                                fprintf(stderr, "Option --assemble requires a file.\n");
                                exit(EXIT_FAILURE);
                        }

                        program->assemblyfile = strdup(returnedOption.longOption);
                        if (NULL == program->assemblyfile) {
                                perror(argv[0]);
                                exit(EXIT_FAILURE);
                        }

                        opts |= ASSEMBLE;
                        break;
                case 'n':
                        program->warn = false;
                        break;
                case 'f':
                        if (returnedOption.option == NONE) {
                                fprintf(stderr, "Option --objectfile requires a file.\n");
                                exit(EXIT_FAILURE);
                        }

                        program->objectfile = strdup(returnedOption.longOption);
                        if (NULL == program->objectfile) {
                                perror(argv[0]);
                                exit(EXIT_FAILURE);
                        }
                        break;
                case 'o':
                        opts |= ASSEMBLE_ONLY;
                        break;
                case 'v':
                        if (returnedOption.option == OPTIONAL) {
                                char *end = NULL;
                                program->verbosity = (int) strtol(
                                        returnedOption.longOption, &end, 10);
                                if (*end) {
                                        printf("Invalid verbosity level: %s\n",
                                                returnedOption.longOption);
                                        exit(0);
                                }
                        } else {
                                program->verbosity++;
                        }
                        break;
                case 'h':
                        usage(argv[0]);
                default:
                        if (returnedOption.longOption != NULL) {
                                fprintf(stderr, "Invalid opt: %s\n",
                                        returnedOption.longOption);
                        } else {
                                fprintf(stderr, "Invalid opt: -%c\n",
                                        returnedOption.shortOption);
                        }
                        exit(EXIT_FAILURE);
                }
        }

        if (opts & ASSEMBLE && !parse(program)) {
                // NO_OPT
        } else if (!(opts & ASSEMBLE_ONLY)) {
                startMachine(&prog);
        }

        tidyUp(&prog);
        freeTable(&tableHead);

        return 0;
}

