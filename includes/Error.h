#ifndef ERROR_H
#define ERROR_H

#include "Structs.h"

/*
 * Flags:    0x00000000FFFF
 * Warnings: 0x0000FFFF0000
 * Errors:   0xFFFF00000000
 */

// Flags
#define ASSEMBLE      0x000000000001
#define ASSEMBLE_ONLY 0x000000000002

__attribute__((noreturn)) void read_error(void);

void tidyUp(struct program *);

#endif // ERROR_H

