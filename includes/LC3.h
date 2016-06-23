#ifndef LC3_H
#define LC3_H

#include <curses.h>

#include "Memory.h"

extern void execute_next(struct LC3 *, WINDOW *);
extern void  print_state(struct LC3 *, WINDOW *);

#endif // LC3_H

