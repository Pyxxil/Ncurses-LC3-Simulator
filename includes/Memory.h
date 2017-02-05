#ifndef MEMORY_H
#define MEMORY_H

#include <curses.h>

#include "Structs.h"
#include "Enums.h"

extern int selected;
extern uint16_t outputHeight;
extern uint16_t *memoryOutput;
extern uint16_t selectedAddress;

void update(WINDOW *, struct program *);
int populateMemory(struct program *);
void printMemory(WINDOW *, struct program *, uint16_t *, const char);

void generateContext(WINDOW *, struct program *, int, uint16_t);
void moveContext(WINDOW *, struct program *, enum DIRECTION);

#endif // MEMORY_H
