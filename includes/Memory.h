#ifndef MEMORY_H
#define MEMORY_H

#include <curses.h>

#include "Structs.h"
#include "Enums.h"

extern uint16_t selected;
extern uint16_t output_height;
extern uint16_t *memory_output;
extern uint16_t selected_address;

void update(WINDOW *, struct program *prog);
int populate_memory(struct program *prog);
void print_memory(WINDOW *, struct program *, uint16_t *, const char);

void generate_context(WINDOW *, struct program *prog, int, uint16_t);
void move_context(WINDOW *, struct program *prog, enum DIRECTION);

#endif // MEMORY_H
