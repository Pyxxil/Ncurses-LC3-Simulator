#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>
#include <stdbool.h>

struct memory_slot {
	uint16_t address;
	uint16_t value;
	char	*label;
	char	*instruction;
};

struct LC3 {
	unsigned char	   CC;
	uint16_t	   PC;
	uint16_t	   IR;
	uint16_t	   registers[8];
	bool		   isHalted;
	bool		   isPaused;
	struct memory_slot memory[0xffff];
};

struct program {
	char *name;
	char *logfile;
	char *objfile;
	struct LC3 simulator;
};

#endif // STRUCTS_H
