#ifndef ENUMS_H
#define ENUMS_H

enum OPCODE {
	BR   = 0x0000,
	ADD  = 0x1000,
	LD   = 0x2000,
	ST   = 0x3000,
	JSR  = 0x4000,
	AND  = 0x5000,
	LDR  = 0x6000,
	STR  = 0x7000,
	RTI  = 0x8000,
	NOT  = 0x9000,
	LDI  = 0xa000,
	STI  = 0xb000,
	JMP  = 0xc000,
	RES  = 0xd000,
	LEA  = 0xe000,
	TRAP = 0xf000,
};

enum STATE {
	MAIN = 0x0,
	SIM  = 0x1,
	MEM  = 0x2,
	EDIT = 0x3,
};

enum DIRECTION {
	UP          = 0x0,
	DOWN        = 0x2,
};

#endif // ENUMS_H
