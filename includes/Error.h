#ifndef ERROR_H
#define ERROR_H

#include "Structs.h"

extern char *input_files,
       *incorrect_opts,
       *unimplemented_opts,
       *deprecated_opts,
       *no_args_provided;

#define HELP_FLAG 		0x001
#define NOT_IMPLEMENTED_OPT 	0x002
#define NO_ARG_PROVIDED		0x004
#define INCORRECT_OPT 		0x008
#define WARN_DEPRECATED 	0x010

#define MUL_NOT_IMPLEMENTED 	0x100
#define MUL_INCORRECT_OPT 	0x200
#define MUL_INPUT_FILES 	0x400
#define MUL_MUL_INPUT_FILES	MUL_INPUT_FILES
#define MUL_NO_ARG_PROVIDED 	0x800


extern void read_error();

extern void tidyup(struct program *);

#endif // ERROR_H
