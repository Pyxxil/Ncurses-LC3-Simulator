#ifndef ERROR_H
#define ERROR_H

#include "Structs.h"

#define HELP_FLAG	     0x1
#define MULTIPLE_INPUT_FILES 0x2
#define NOT_IMPLEMENTED_OPT  0x4
#define INCORRECT_OPTS	     0x8

extern void read_error();

extern void tidyup(struct program *);

#endif // ERROR_H
