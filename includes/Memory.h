#ifndef MEMORY_H
#define MEMORY_H

#include "LC3.h"

extern void populate_memory(struct LC3 *, const char *);
extern void print_memory(WINDOW *, struct LC3 *, uint16_t *, const char);

#endif // MEMORY_H

