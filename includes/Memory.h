#ifndef MEMORY_H
#define MEMORY_H

#include <curses.h>

#include "Enums.h"
#include "Structs.h"

extern int selected;

extern uint16_t output_height;
extern uint16_t *memory_output;
extern uint16_t selected_address;

extern void populate_memory(struct program *);
extern void print_memory(WINDOW *, struct program *, uint16_t *, const char);

extern void generate_context(WINDOW *, struct LC3 *, int, uint16_t);
extern void move_context(WINDOW *, struct LC3 *, enum DIRECTION);

#endif // MEMORY_H
