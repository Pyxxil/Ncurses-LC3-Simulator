#ifndef MEMORY_H
#define MEMORY_H

#include "LC3.h"
#include "Enums.h"

extern int selected;

extern uint16_t output_height;
extern uint16_t *memory_output;
extern uint16_t selected_address;

extern void populate_memory(struct LC3 *, const char *);
extern void print_memory(WINDOW *, struct LC3 *, uint16_t *, const char);

extern void create_context(WINDOW *, struct LC3 *, int, uint16_t);
extern void move_context(WINDOW *, struct LC3 *, enum DIRECTION);

#endif	// MEMORY_H
