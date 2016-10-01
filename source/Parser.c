#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>

#include "Parser.h"

static int uppercmp(char *from, char const *const to)
{
	char copy[100];
	strcpy(copy, from);

	for (size_t i = 0; copy[i]; i++) {
		copy[i] = toupper(copy[i]);
	}

	return strcmp(copy, to);
}

static int uppercmpn(char *from, char const *const to, size_t n)
{
	char copy[n + 1];
	strncpy(copy, from, n);

	return uppercmp(copy, to);
}

static uint16_t nzp(char const *const _nzp)
{
	uint16_t __nzp = 0;

	for (size_t i = 0; _nzp[i]; i++) {
		if (_nzp[i] == 'n' || _nzp[i] == 'N') {
			if (__nzp & 0x0800) {
				__nzp |= 0x4;
			}
			__nzp |= 0x0800;
		} else if (_nzp[i] == 'z' || _nzp[i] == 'Z') {
			if (__nzp & 0x0400) {
				__nzp |= 0x2;
			}
			__nzp |= 0x0400;
		} else if (_nzp[i] == 'p' || _nzp[i] == 'P') {
			if (__nzp & 0x0200) {
				__nzp |= 0x1;
			}
			__nzp |= 0x0200;
		}
	}

	return __nzp;
}

/*
 * Copy the contents of one string to another, allocating enough memory to the
 * string we want to copy to.
 */

static inline void strmcpy(char **to, char const *from)
{
	size_t len = strlen(from) + 1;
	*to = (char *) malloc(sizeof(char) * len);
	strncpy(*to, from, len);
}

bool parse(char const *fileName)
{
	printf("Starting pass 2...\n");

	FILE *file = fopen(fileName, "r");

	char *fileBase = strstr(fileName, ".asm");
	char *symFileName, *hexFileName, *binFileName;

	FILE *symFile, *hexFile, *binFile;

	size_t length = strlen(fileName);

	if (!fileBase) {
		strmcpy(&symFileName, fileName);
		strmcpy(&hexFileName, fileName);
		strmcpy(&binFileName, fileName);

		symFileName = (char *) realloc(symFileName, length + 5);
		hexFileName = (char *) realloc(hexFileName, length + 5);
		binFileName = (char *) realloc(binFileName, length + 5);
	} else {
		size_t base = fileBase - fileName;
		symFileName = (char * ) malloc(length + 5);
		hexFileName = (char * ) malloc(length + 5);
		binFileName = (char * ) malloc(length + 5);

		strncpy(symFileName, fileName, base);
		strncpy(hexFileName, fileName, base);
		strncpy(binFileName, fileName, base);
	}

	strcat(symFileName, ".sym");
	strcat(hexFileName, ".hex");
	strcat(binFileName, ".bin");

	symFile = fopen(symFileName, "w+");
	hexFile = fopen(hexFileName, "w+");
	binFile = fopen(binFileName, "w+");

	int c, currentLine = 1, errors = 0;

	char line[100] = { 0 };
	char label[100] = { 0 };

	char operand1[100] = { 0 }, operand2[100] = { 0 }, operand3[100] = { 0 };
	char *end = NULL;

	bool origSeen = false, endSeen = false, doContinue = false;

	uint16_t instruction = 0;

	while ((c = fgetc(file)) != EOF) {
		if (isspace(c)) {
			while (isspace(c = fgetc(file))) {
				if (c == '\n') {
					currentLine++;
				}
			}
			ungetc(c, file);

		} else if (c == ';') {
			while ((c = fgetc(file)) != '\n' && c != -1);
			currentLine++;
			ungetc(c, file);
		} else if (c == '.') {
			ungetc(c, file);
			fscanf(file, "%99s", line);

			if (!uppercmp(line, ".ORIG")) {
				printf(".ORIG ");
				if (origSeen) {
					fprintf(stderr, "  Line %d: Extra .ORIG "
						"directive.\n", currentLine);
					errors ++;
				} else if (endSeen) {
					fprintf(stderr, "  Line %d: .END seen "
						"before .ORIG.\n", currentLine);
					errors ++;
				} else {
					while (isspace(c = fgetc(file))) {
						if (c == '\n') {
							fprintf(stderr,
								"Line %d: "
								"No address "
								"supplied to "
								".ORIG directive"
								".\n",
								currentLine);
							currentLine ++;
							doContinue = true;
							break;
						}
					}
					if (doContinue) {
						doContinue = false;
						continue;
					}

					fscanf(file, "%99s", operand1);
					instruction = (uint16_t) strtoul(
						operand1, &end, 16
					);

					if (*end) {
						fprintf(stderr, "Line %d: "
							"Invalid address '%s'",
							operand1);
					} else {

					}
				}
				origSeen = true;
			} else if (!uppercmp(line, ".STRINGZ")) {
				printf(".STRINGZ ");
			} else if (!uppercmp(line, ".BLKW")) {
				printf(".BLKW ");
			} else if (!uppercmp(line, ".FILL")) {
				printf(".FILL ");
			} else if (!uppercmp(line, ".END")) {
				printf(".END ");
				if (!origSeen) {
					fprintf(stderr, "  Line %d: .END seen "
						"before .ORIG.\n", currentLine);
					errors ++;
				} else if (endSeen) {
					fprintf(stderr, "  Line %d: Extra .END "
						"directive.\n", currentLine);
					errors ++;
				}
				endSeen = true;
			} else {
				fprintf(stderr, "\tLine %d: Unknown directive "
					"'%s'\n", currentLine, line);
				errors ++;
			}
		} else {
			ungetc(c, file);
			fscanf(file, "%s", line);

			if (!uppercmpn(line, "BR", 2)) {
				instruction = nzp(line + 2);
				printf("%s  %x", line, instruction);
				if (instruction & 7) {
					fprintf(stderr, "  Line %d: "
						"Invalid BR instruction.\n",
						currentLine);
					errors ++;
				} else if (errors == 0) {
					while (isspace(c = fgetc(file)) &&
							c != EOF);
					ungetc(c, file);

				}
			} else if (!uppercmp(line, "AND")) {
				printf("AND ");
			} else if (!uppercmp(line, "ADD")) {
				printf("ADD ");
			} else if (!uppercmp(line, "NOT")) {
				printf("NOT ");
			} else if (!uppercmp(line, "JMP")) {
				printf("JMP ");
			} else if (!uppercmp(line, "JSR")) {
				printf("JSR ");
			} else if (!uppercmp(line, "JSRR")) {
				printf("JSRR ");
			} else if (!uppercmp(line, "LEA")) {
				printf("LEA ");
			} else if (!uppercmp(line, "LD")) {
				printf("LD ");
			} else if (!uppercmp(line, "LDI")) {
				printf("LDI ");
			} else if (!uppercmp(line, "LDR")) {
				printf("LDR ");
			} else if (!uppercmp(line, "ST")) {
				printf("ST ");
			} else if (!uppercmp(line, "STI")) {
				printf("STI ");
			} else if (!uppercmp(line, "STR")) {
				printf("STR ");
			} else if (!uppercmp(line, "RET")) {
				printf("RET ");
			} else if (!uppercmp(line, "TRAP")) {
				printf("TRAP ");
			} else if (!uppercmp(line, "GETC")) {
				printf("GETC ");
			} else if (!uppercmp(line, "PUTS")) {
				printf("PUTS ");
			} else if (!uppercmp(line, "PUTC")) {
				printf("PUTC ");
			} else if (!uppercmp(line, "OUT")) {
				printf("OUT ");
			} else if (!uppercmp(line, "PUTS")) {
				printf("PUTSP ");
			} else if (!uppercmp(line, "IN")) {
				printf("IN ");
			} else {

			}
		}
		memset(line, 0, 100);
		memset(label, 0, 100);
	}

	printf("%d error%s\n", errors, errors != 1 ? "'s" : "");

	fclose(file);
	fclose(symFile);
	fclose(hexFile);
	fclose(binFile);

	return errors == 0;
}

