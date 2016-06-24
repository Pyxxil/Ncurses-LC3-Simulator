#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdbool.h>
#include <stdint.h>

struct LC3 {
	uint16_t PC, memory[0xffff],
		registers[8], IR;
	unsigned char CC;
	bool halted, isPaused;
	void (*populate)(struct LC3 *);
};

extern void populate_memory(struct LC3 *, char const *);

#endif // LC3_H

