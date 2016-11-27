#include <stdio.h>
#include <stdbool.h>

#include "Logging.h"

/*
 * Dump contents of a program to it's log file.
 *
 * Each dump should look something like this:
 * =====================
 * REG      HEX      DEC
 * ---------------------
 * RO  --  0x0000      0
 * R1  --  0x0000      0
 * R2  --  0x0000      0
 * R3  --  0x0000      0
 * R4  --  0x0000      0
 * R5  --  0x0000      0
 * R6  --  0x0000      0
 * R7  --  0x0000      0
 * PC  --  0x0000      0
 * IR  --  0x0000      0
 * CC  --  Z
 * =====================
 *
 * Returns:
 *     0 if nothing went wrong.
 *    >0 if there was an error.
 *
 */

unsigned int logDump(struct program const *program)
{
	static bool beenHere = false;
	FILE *logfile;

	if (!beenHere && NULL == program->logfile)
		return ErrNoFile;

	beenHere = true;

	logfile = fopen(program->logfile, "a");

	if (NULL == logfile)
		return ErrFileOpen;

	fprintf(logfile, "=====================\n");
	fprintf(logfile, "REG       HEX     DEC\n");
	fprintf(logfile, "---------------------\n");

	for (unsigned char reg = 0; reg < 8; reg++) {
		fprintf(logfile, "R%d  --  0x%04x  %5d\n", reg,
			program->simulator.registers[reg],
			program->simulator.registers[reg]);
	}

	fprintf(logfile, "PC  --  0x%04x  %5d\n", program->simulator.PC,
		program->simulator.PC);

	fprintf(logfile, "IR  --  0x%04x  %5d\n", program->simulator.IR,
		program->simulator.IR);

	fprintf(logfile, "CC  --  %c\n", program->simulator.CC);
	fprintf(logfile, "=====================\n");

	fclose(logfile);

	return 0;
}

