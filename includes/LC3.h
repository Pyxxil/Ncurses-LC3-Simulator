#ifndef LC3_H
#define LC3_H

#include <curses.h>
#include <stdint.h>
#include <stdbool.h>

struct LC3 {
	uint16_t PC, memory[0xffff],
		 registers[8], IR;
	unsigned char CC;
	bool halted, isPaused;
	void (*populate)(struct LC3 *);
};

extern void execute_next(struct LC3 *, WINDOW *);
extern void  print_state(struct LC3 *, WINDOW *);

#endif	// LC3_H
