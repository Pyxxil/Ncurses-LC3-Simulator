#ifndef ERROR_H
#define ERROR_H

#include "Structs.h"

extern char *input_files,
       *incorrect_opts,
       *unimplemented_opts,
       *deprecated_opts,
       *no_args_provided;

/*
 * Flags:    0x00000000FFFF
 * Warnings: 0x0000FFFF0000
 * Errors:   0xFFFF00000000
 */

// Flags
#define HELP 				0x000000000001
#define ASSEMBLE 			0x000000000002
#define ASSEMBLE_ONLY 			0x000000000004

// Warnings
#define WARN_DEPRECATED 		0x000000010000
#define WARN_UNIMPLEMENTED 		0x000000020000

#define MUL_WARN_UNIMPLEMENTED 		0x000000100000
#define MUL_WARN_DEPRECATED 		0x000000200000

// Errors
#define NO_ARG_PROVIDED 		0x000100000000
#define INCORRECT_OPT 			0x000200000000
#define INVALID_VERBOSE_LEVEL		0x000400000000

#define MUL_INCORRECT_OPT 		0x001000000000
#define MUL_INPUT_FILES 		0x002000000000
#define MUL_NO_ARG_PROVIDED 		0x004000000000
#define MUL_INVALID_VERBOSE_LEVEL 	0x008000000000

extern void read_error(void);

extern void tidyup(struct program *);

#endif // ERROR_H

