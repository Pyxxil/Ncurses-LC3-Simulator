#ifndef ERROR_H
#define ERROR_H

#include "Structs.h"

extern char *input_files,
       *incorrect_opts,
       *unimplemented_opts,
       *deprecated_opts,
       *no_args_provided;

#define HELP_FLAG 		0x0001
#define NO_ARG_PROVIDED 	0x0002
#define INCORRECT_OPT 		0x0004

#define WARN_DEPRECATED 	0x0010
#define WARN_UNIMPLEMENTED 	0x0020

#define MUL_INCORRECT_OPT 	0x0100
#define MUL_INPUT_FILES 	0x0200
#define MUL_NO_ARG_PROVIDED 	0x0400

#define MUL_WARN_UNIMPLEMENTED 	0x1000
#define MUL_WARN_DEPRECATED 	0x2000

#define MUL_MUL_INPUT_FILES	MUL_INPUT_FILES

extern void read_error(void);

extern void tidyup(struct program *);

#endif // ERROR_H
