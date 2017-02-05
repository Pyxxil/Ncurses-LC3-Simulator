#ifndef LC3_H
#define LC3_H

#include <curses.h>

#include "Structs.h"

extern void executeNext(struct LC3 *, WINDOW *);
extern void printState(struct LC3 *, WINDOW *);

#endif // LC3_H
