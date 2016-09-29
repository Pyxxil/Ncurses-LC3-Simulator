#ifndef MEMORY_H
#define MEMORY_H

#include <curses.h>

#include "Structs.h"
#include "Enums.h"

extern uint16_t selected;
extern uint16_t output_height;
extern uint16_t *memory_output;
extern uint16_t selected_address;

void update(WINDOW *, struct LC3 *);
int populate_memory(struct program *);
void print_memory(WINDOW *, struct program *, uint16_t *, const char);

void generate_context(WINDOW *, struct LC3 *, int, uint16_t);
void move_context(WINDOW *, struct LC3 *, enum DIRECTION);

#endif // MEMORY_H
