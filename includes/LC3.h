#ifndef LC3_H
#define LC3_H

#include <curses.h>

#include "Simulator.h"

extern void print_state(struct LC3 *, WINDOW *);
extern void    simulate(struct LC3 *, WINDOW *);

#endif // LC3_H

