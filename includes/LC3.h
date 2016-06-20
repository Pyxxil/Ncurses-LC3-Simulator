#ifndef LC3_H
#define LC3_H

#include <stdint.h>
#include <curses.h>

typedef struct {
	unsigned short PC,
		       memory[0xffff],
		       registers[8],
		       IR;
	unsigned char CC;
	bool halted, isPaused;
} LC3;

extern void print_state(LC3 *, WINDOW *);
extern void    simulate(LC3 *, WINDOW *);

#endif // LC3_H

