#ifndef ENUMS_H
#define ENUMS_H

enum OPCODE {
	BR	= 0x0,
	ADD	= 0x1,
	LD	= 0x2,
	ST	= 0x3,
	JSR	= 0x4,
	AND	= 0x5,
	LDR	= 0x6,
	STR	= 0x7,
	RTI	= 0x8,
	NOT	= 0x9,
	LDI	= 0xa,
	STI	= 0xb,
	JMP	= 0xc,
	RES	= 0xd,
	LEA	= 0xe,
	TRAP	= 0xf,
};

enum TRAP {
	GETC	= 0x20,
	OUT	= 0x21,
	PUTS	= 0x22,
	IN	= 0x23,
	PUTSP	= 0x24,
	HALT	= 0x25,
};

enum STATE {
	MAIN	= 0x0,
	SIM	= 0x1,
	MEM	= 0x2,
	EDIT	= 0x4,
};

enum POSITION {
	TOP	= 0x0,
	CEN	= 0x1,
	BOT	= 0x2,
	REL	= 0x3,
};

#endif // ENUMS_H

