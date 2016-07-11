#ifndef PARSER_H
#define PARSER_H


struct Token {
	ORIG = 0x01,
	END  = 0x02,
	HEX  = 0x03,
	DEC  = 0x04,

	// Opcodes
	OPAND  = 0x05,
	OPADD  = 0x06,
	OPBR   = 0x07,
	OPJMP  = 0x08,
	OPJSR  = 0x09,
	OPJSRR = 0x0a,
	OPNOT  = 0x0b,
	OPLD   = 0x0c,
	OPLDR  = 0x0d,
	OPLEA  = 0x0e,
	OPST   = 0x0f,
	OPSTR  = 0x10,
	OPSTI  = 0x11,
	OPLDI  = 0x12,
	OPTRAP = 0x13,
	OPRTI  = 0x14,
	OPRES  = 0x15,

	BEGCOM = 0x16, // Beginning of a comment.

	// Directives
	STRING = 0x17,
	STRZ   = 0x18,
	FILL   = 0x19,
	BLKW   = 0x1a,

	// Psedo OP's.
	RET = 0x1b,

	// Trap codes
	GETC   = 0x1c,
	IN     = 0x1d,
	OUT    = 0x1e,
	PUTS   = 0x1f,
	PUTSP  = 0x20,
	HALT   = 0x21,
};

struct Parser {
	Token token;
};

#endif // PARSER_H
