#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

#include "Parser.h"
#include "Token.h"

static const unsigned MAX_LABEL_LENGTH = 80;

/*
 * Convert every character in a string to uppercase, and compare it against the
 * provided string.
 */

static int uppercmp(char *from, char const *const to)
{
	char copy[100];
	strcpy(copy, from);

	for (size_t i = 0; copy[i]; i++) {
		copy[i] = toupper(copy[i]);
	}

	return strcmp(copy, to);
}

/*
 * Compare n number of characters in a string, each of which is converted to its
 * upper case equivalent.
 */

static int uppercmpn(char *from, char const *const to, size_t n)
{
	char copy[n + 1];
	strncpy(copy, from, n);
	copy[n] = '\0';

	return uppercmp(copy, to);
}

/*
 * Retrieve the hex form of the conditions supplied by the BR instruction.
 */

static uint16_t nzp(char const *const _nzp)
{
	uint16_t __nzp = 0;

	for (size_t i = 0; _nzp[i]; i++) {
		if (toupper(_nzp[i]) == 'N') {
			__nzp |= 0x0800;
		} else if (toupper(_nzp[i]) == 'Z') {
			__nzp |= 0x0400;
		} else if (toupper(_nzp[i]) == 'P') {
			__nzp |= 0x0200;
		} else {
			return 0x1;
		}
	}

	return __nzp;
}

/*
 * Skip all whitespace characters in a file. If a new line is reached, return
 * early so that the caller can handle it.
 */

static void skipWhitespace(FILE *file)
{
	int c = 0;
	while ((c = fgetc(file)) != EOF && isspace(c)) {
		if (c == '\n')
			break;
	}
	ungetc(c, file);
}

/*
 * Go to the next line of the file.
 */

static void nextLine(FILE *file)
{
	int c = 0;
	while ((c = fgetc(file)) != '\n' && c != EOF);
	ungetc(c, file);
}

/*
 * Write the hexadecimal instruction to the specified object file in binary.
 */

static void objWrite(uint16_t instruction, FILE *objFile)
{
	unsigned char bytes[2];
	bytes[0] = (instruction & 0xff00) >> 8;
	bytes[1] = instruction & 0xff;
	fwrite(bytes, 2, 1, objFile);
}

/*
 * Check whether the next token is a comment.
 */

static bool iscomment(FILE *file)
{
	int c = fgetc(file);

	if (c == ';') {
		return true;
	} else if (c == '/') {
		if ((c = fgetc(file)) == '/') {
			return true;
		}
		ungetc(c, file);
		return false;
	}

	return false;
}

/*
 * Check whether we have reached the end of the line.
 */

static bool endOfLine(char const *const instruction,
		FILE *file, int const *const line)
{
	skipWhitespace(file);
	int c = fgetc(file);

	if (c != '\n' && c != EOF) {
		ungetc(c, file);
		if (!iscomment(file)) {
			fprintf(stderr, "  Line %d: Incorrect number of arguments "
				"supplied to %s.\n", *line, instruction);
			nextLine(file);
			return false;
		}
	}

	ungetc(c, file);
	return true;
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

struct symbol {
	char *name;
	uint16_t address;
};

struct symbolTable {
	struct symbol *sym;
	struct symbolTable *next;
};

static struct symbolTable table = {
	.sym = NULL,
	.next = NULL,
};

static void free_table(struct symbolTable *table)
{
	if (NULL != table->sym) {
		free(table->sym->name);
		free(table->sym);
	}
	if (NULL != table->next) {
		free_table(table->next);
		free(table->next);
	}
}

static struct symbol* findSymbol(char const *const name)
{
	struct symbolTable *symTable = &table;

	if (NULL == symTable->sym) {
		return NULL;
	}

	while (NULL != symTable->next) {
		if (!strcmp(symTable->sym->name, name)) {
			return symTable->sym;
		}
		symTable = symTable->next;
	}
	if (!strcmp(symTable->sym->name, name)) {
		return symTable->sym;
	} else {
		return NULL;
	}
}

/*
 * Add a Symbol by name and addres into the Symbol Table.
 */

static int addSymbol(char const *const name, uint16_t address)
{
	struct symbol *sym;
	struct symbolTable *symTable = &table;

	if (NULL == symTable->sym) {
		// This should probably change...
		// It's just there to check if the table has no values.
		sym = malloc(sizeof(struct symbol));
		strmcpy(&sym->name, name);
		sym->address = address;
		table.sym = sym;
		return 0;
	}

	sym = findSymbol(name);

	if (NULL == sym) {
		while (1) {
			if (NULL == symTable->next) {
				sym = malloc(sizeof(struct symbol));
				strmcpy(&sym->name, name);
				sym->address = address;

				struct symbolTable *_table =
					malloc(sizeof(struct symbolTable));
				_table->sym = sym;
				_table->next = NULL;
				symTable->next = _table;
				return 0;
			}

			symTable = symTable->next;
		}
	}
	return 1;
}

/*
 * Read the given file to find what the next token is.
 *
 * If none match, we return an OP_UNK. In general, only the first pass of the
 * file will care about OP_UNK/OP_BRUNK -- the first pass will use the token
 * found as a label.
 */

enum Token nextToken(FILE *file, char *buffer)
{
	fscanf(file, "%99s", buffer);

	if (!uppercmp(buffer, "ADD")) {
		return OP_ADD;
	} else if (!uppercmp(buffer, "AND"))  {
		return OP_AND;
	} else if (!uppercmp(buffer, "JMP"))  {
		return OP_JMP;
	} else if (!uppercmp(buffer, "JSR"))  {
		return OP_JSR;
	} else if (!uppercmp(buffer, "JSRR")) {
		return OP_JSRR;
	} else if (!uppercmp(buffer, "LD"))   {
		return OP_LD;
	} else if (!uppercmp(buffer, "LDR"))  {
		return OP_LDR;
	} else if (!uppercmp(buffer, "LDI"))  {
		return OP_LDI;
	} else if (!uppercmp(buffer, "LEA"))  {
		return OP_LEA;
	} else if (!uppercmp(buffer, "NOT"))  {
		return OP_NOT;
	} else if (!uppercmp(buffer, "RET"))  {
		return OP_RET;
	} else if (!uppercmp(buffer, "RTI"))  {
		return OP_RTI;
	} else if (!uppercmp(buffer, "ST"))   {
		return OP_ST;
	} else if (!uppercmp(buffer, "STR"))  {
		return OP_STR;
	} else if (!uppercmp(buffer, "STI"))  {
		return OP_STI;
	} else if (!uppercmp(buffer, "TRAP")) {
		return OP_TRAP;
	} else if (!uppercmp(buffer, "GETC")) {
		return OP_GETC;
	} else if (!uppercmp(buffer, "PUTC")) {
		return OP_PUTC;
	} else if (!uppercmp(buffer, "OUT")) {
		return OP_OUT;
	} else if (!uppercmp(buffer, "HALT")) {
		return OP_HALT;
	} else if (!uppercmp(buffer, "IN"))   {
		return OP_IN;
	} else if (!uppercmp(buffer, "PUTS")) {
		return OP_PUTS;
	} else if (!uppercmp(buffer, "PUTSP")) {
		return OP_PUTSP;
	} else if (!uppercmp(buffer, ".ORIG")) {
		return DIR_ORIG;
	} else if (!uppercmp(buffer, ".STRINGZ")) {
		return DIR_STRINGZ;
	} else if (!uppercmp(buffer, ".FILL")) {
		return DIR_FILL;
	} else if (!uppercmp(buffer, ".END")) {
		return DIR_END;
	} else if (!uppercmp(buffer, ".BLKW")) {
		return DIR_BLKW;
	} else if (!uppercmpn(buffer, "BR", 2)) {
		switch(nzp(buffer + 2)) {
		case 0x0001:
			return OP_BRUNK;
		case 0x0200:
			return OP_BRP;
		case 0x0400:
			return OP_BRZ;
		case 0x0600:
			return OP_BRZP;
		case 0x0800:
			return OP_BRN;
		case 0x0A00:
			return OP_BRNP;
		case 0x0C00:
			return OP_BRNZ;
		default:
			return OP_BR;
		}
	} else {
		return OP_UNK;
	}
}

/*
 * Find the next immediate value in the file. If found, return it.
 *
 * An immediate value takes the form:
 * 	allowedComma = True:
 * 	--> ([ \t]*[,])?[ \t]+(([0]?[xX]{1}[\da-fA-F]+)|([#]{1}[-]?[\d]+)|([-]?[\d]+))
 * 	e.g. '  , #2'
 * 	allowedComma = False:
 * 	--> [ \t]+(([0]?[xX]{1}[\da-fA-F]+)|([#]{1}[-]?[\d]+)|([-]?[\d]+))
 * 	e.g. ' #2'
 *
 * If the value is too large, or it isn't found, or is in the wrong format,
 * return INT_MAX as an error.
 *
 */

static int nextImmediate(FILE *file, bool allowedComma)
{
	skipWhitespace(file);

	int c = fgetc(file);
	int immediate;
	unsigned repr;

	char line[MAX_LABEL_LENGTH];
	memset(line, 0, MAX_LABEL_LENGTH);

	if (allowedComma && c == ',') {
		skipWhitespace(file);
		c = fgetc(file);
	}

	if (c == '-') {
		repr = 10;
		line[0] = '-';
	} else if (c == '#') {
		repr = 10;
		c = fgetc(file);
		if (c == '-') {
			line[0] = '-';
		} else {
			line[0] = '0';
			ungetc(c, file);
		}
	} else if (toupper(c) == 'X') {
		repr = 16;
		line[0] = '0';
	} else if (isdigit(c)) {
		if (c == '0') {
			c = fgetc(file);
			if (toupper(c) == 'X') {
				repr = 16;
				line[0] = '0';
			} else {
				ungetc(c, file);
				repr = 10;
			}
		} else {
			repr = 10;
			line[0] = c;
		}
	} else {
		ungetc(c, file);
		return INT_MAX;
	}

	c = fgetc(file);
	for (size_t i = 1; i < MAX_LABEL_LENGTH && isxdigit(c); i++) {
		line[i] = c;
		c = fgetc(file);
	}
	ungetc(c, file);

	char *end = NULL;
	immediate = (int) strtol(line, &end, repr);

	if (*end) {
		return INT_MAX;
	} else {
		return immediate;
	}
}

/*
 * Read the file until we hit the next register in the form of:
 * 	allowedComma = True (as in, it's not the first operand)
 * 	--> ([ \t]*[,][ \t]+|[ \t]+)[rR][0-7]
 * 	e.g. ' , R1'
 * 	allowedComma = False (the first operand)
 * 	--> [ \t]+[rR][0-7]
 * 	e.g. '  R1'
 *
 * If we never reach a register, then return 65535 as an error.
 * Otherwise, return which register it was (to decimal).
 */

static uint16_t nextRegister(FILE *file, bool allowedComma)
{
	skipWhitespace(file);
	int c = fgetc(file);

	if (allowedComma && c == ',') {
		skipWhitespace(file);
		c = fgetc(file);
	}

	if (toupper(c) != 'R') {
		ungetc(c, file);
		return 65535;
	}

	c = fgetc(file);
	if (c < '0' || c > '7') {
		ungetc(c, file);
		ungetc(c, file);
		return 65535;
	}

	return c - 0x30;
}

/*
 * Process the given file, retrieving labels and turning them into symbols.
 *
 * Each symbol is written to the symbol file given, and added to the symbol
 * table held above.
 */

static void process(FILE *file, FILE *symFile)
{
	static char const *const symTable =
		"// Symbol Table\n"
		"// Scope Level 0:\n"
		"//\tSymbol Name       Page Address\n"
		"//\t----------------  ------------------\n";

	fprintf(symFile, symTable);

	bool origSeen = false, endSeen = false;
	int c, currentLine = 0, errors = 0, operand;
	uint16_t pc = 0;

	char line[100] = { 0 }, *end = NULL;
	enum Token tok;

	printf("STARTING FIRST PASS...\n");

	while (1) {
		skipWhitespace(file);

		c = fgetc(file);

		if (c == EOF) {
			break;
		} else if (c == '\n') {
			currentLine++;
			continue;
		} else if (c == ';') {
			nextLine(file);
			continue;
		} else if (c == ':') {
			continue;
		} else {
			ungetc(c, file);
			tok = nextToken(file, line);

			switch (tok) {
			case DIR_ORIG:
				if (origSeen) {
					fprintf(stderr, "  Line %d: Extra .ORIG "
						"directive.\n", currentLine);
					errors ++;
					continue;
				} else if (endSeen) {
					fprintf(stderr, "  Line %d: .END seen "
						"before .ORIG.\n", currentLine);
					errors ++;
					continue;
				}

				c = nextImmediate(file, false);
				if (c > 0xffff || c < 0) {
					fprintf(stderr, "  Line %d: Invalid operand for "
						".ORIG.\n", currentLine);
				}

				pc = c;
				origSeen = true;
				printf(".ORIG  %#x\n", pc);
				break;
			case DIR_BLKW:
				operand = nextImmediate(file, false);

				if (operand == INT_MAX) {
					fprintf(stderr, "  Line %d: Invalid operand for "
						".BLKW.\n", currentLine);
				} else if (operand < 1) {
					fprintf(stderr, "  Line %d: .BLKW "
						"requires an argument > 0.\n",
						currentLine);
					nextLine(file);
					errors++;
					continue;
				} else if (operand > 150) {
					fprintf(stderr, "  Line %d: .BLKW "
						"requires an argument < 150.\n",
						currentLine);
					nextLine(file);
					errors++;
					continue;
				}

				pc += operand;
				break;
			case DIR_STRINGZ:
				skipWhitespace(file);
				c = fgetc(file);

				if (c != '"') {
					ungetc(c, file);
					fprintf(stderr, "  Line %d: No string "
						"supplied to .STRINGZ.\n",
						currentLine);
					nextLine(file);
					errors++;
					continue;
				}

				memset(line, 0, 100);
				for (size_t i = 0; (c = fgetc(file)) != '"' && i < 100; i++) {
					if (c == '\\') {
						switch (c = fgetc(file)) {
						case 'n':
							line[i] = '\n';
							break;
						case 't':
							line[i] = '\t';
							break;
						case '"':
							line[i] = c;
							break;
						default:
							ungetc(c, file);
							break;
						}
					} else {
						line[i] = c;
					}
					pc++;
				}
				nextLine(file);
				pc++; // Implicit null terminator
				break;
			case DIR_FILL:
				if (origSeen)
					pc++;
				nextLine(file);
				break;
			case DIR_END:
				endSeen = true;
				nextLine(file);
				break;
			case OP_AND:  case OP_ADD:  case OP_NOT:  case OP_RET:
			case OP_JSR:  case OP_BR:   case OP_BRN:  case OP_BRNZ:
			case OP_BRZP: case OP_BRP:  case OP_BRNP: case OP_JSRR:
			case OP_TRAP: case OP_GETC: case OP_PUTC: case OP_OUT:
			case OP_PUTS: case OP_IN:   case OP_JMP:  case OP_RTI:
			case OP_ST:   case OP_STR:  case OP_STI:  case OP_LD:
			case OP_LDI:  case OP_LDR:  case OP_LEA:  case OP_HALT:
			case OP_BRZ:  case OP_PUTSP:
				if (origSeen)
					pc++;
				nextLine(file);
				break;
			case OP_UNK: case OP_BRUNK:
				end = strchr(line, ':');
				if (NULL != end) {
					char *colon = end + strlen(end) - 1;
					for (; colon != end; colon--) {
						*colon = '\0';
					}
					ungetc(*colon, file);
					*colon = '\0';
				}
				printf("FOUND LABEL ==> '%s'  AT %#x\n",
					line, pc);
				if (addSymbol(line, pc)) {
					fprintf(stderr, "  Line %d: "
						"Multiple definitions of label "
						"'%s'\n", currentLine, line);
					nextLine(file);
					errors++;
				} else {
					fprintf(symFile, "//\t%-16s %4x\n",
						line, pc);
				}
				break;
			}
		}
	}

	printf("%d error%s found in first pass.\n", errors,
			errors == 1 ? "" : "'s");
	rewind(file);
}

bool parse(char const *fileName)
{
	FILE *file = fopen(fileName, "r");

	FILE *symFile, *hexFile, *binFile, *objFile;

	size_t length = strlen(fileName);

	char *symFileName = malloc(length + 5);
	char *hexFileName = malloc(length + 5);
	char *binFileName = malloc(length + 5);
	char *objFileName = malloc(length + 5);

	char *fileBase = strstr(fileName, ".asm");
	if (NULL != fileBase) {
		size_t pos = fileBase - fileName;

		strncpy(symFileName, fileName, pos);
		strcpy(symFileName + pos, ".sym");

		strncpy(hexFileName, fileName, pos);
		strcpy(hexFileName + pos, ".hex");

		strncpy(binFileName, fileName, pos);
		strcpy(binFileName + pos, ".bin");

		strncpy(objFileName, fileName, pos);
		strcpy(objFileName + pos, ".obj");
	} else {
		strcpy(symFileName, fileName);
		strcat(symFileName, ".sym");

		strcpy(symFileName, fileName);
		strcat(hexFileName, ".hex");

		strcpy(symFileName, fileName);
		strcat(binFileName, ".bin");

		strcpy(symFileName, fileName);
		strcat(objFileName, ".obj");
	}

	symFile = fopen(symFileName, "w+");
	hexFile = fopen(hexFileName, "w+");
	binFile = fopen(binFileName, "w+");
	objFile = fopen(objFileName, "wb+");

	process(file, symFile);

	printf("STARTING SECOND PASS...\n");

	int c, currentLine = 1, errors = 0;

	char line[100] = { 0 };

	char label[MAX_LABEL_LENGTH];
	char *end = NULL;

	bool origSeen = false, endSeen = false;

	uint16_t instruction = 0, pc = 0, oper1, oper2;
	int oper3;

	enum Token tok;
	struct symbol *sym;

	while (1) {
		memset(line,  0, sizeof(line));
		memset(label, 0, MAX_LABEL_LENGTH);
		instruction = 0;
		oper1 = 0;
		oper2 = 0;
		oper3 = 0;

		skipWhitespace(file);
		c = fgetc(file);

		if (c == EOF) {
			break;
		} else if (c == '\n') {
			currentLine++;
			continue;
		} else if (c == ';' || c == '/') {
			if (c == '/' && (c = fgetc(file)) != '/') {
				ungetc(c, file);
			} else {
				nextLine(file);
				continue;
			}
		}

		ungetc(c, file);
		tok = nextToken(file, line);
		skipWhitespace(file);

		if (origSeen) {
			pc++;
		}

		switch (tok) {
		case DIR_ORIG:
			if (origSeen) {
				fprintf(stderr, "  Line %d: Extra .ORIG "
					"directive.\n", currentLine);
				errors ++;
				continue;
			} else if (endSeen) {
				fprintf(stderr, "  Line %d: .END seen "
					"before .ORIG.\n", currentLine);
				errors ++;
				continue;
			}

			oper3 = nextImmediate(file, false);
			if (oper3 > 0xffff || oper3 < 0) {
				fprintf(stderr, "  Line %d: Invalid operand for "
					".ORIG.\n", currentLine);
			}

			pc = instruction = oper3;
			origSeen = true;
			printf(".ORIG  %#x\n", instruction);
			break;
		case DIR_STRINGZ:
			c = fgetc(file);

			if (c != '"') {
				fprintf(stderr, "  Line %d: No string "
					"supplied to .STRINGZ.\n",
					currentLine);
				nextLine(file);
				errors++;
				continue;
			}

			memset(line, 0, 100);
			for (size_t i = 0; (c = fgetc(file)) != '"' && i < 100; i++) {
				if (c == '\\') {
					switch (c = fgetc(file)) {
					case 'n':
						line[i] = '\n';
						instruction = 0x000a;
						break;
					case 't':
						line[i] = '\t';
						instruction = 0x0009;
						break;
					case '"':
						line[i] = '"';
						instruction = 0x0022;
						break;
					case '\\':
						line[i] = '\\';
						instruction = 0x005c;
						break;
					default:
						ungetc(c, file);
						break;
					}
				} else {
					line[i] = c;
					instruction = c & 0xff;
				}
				pc++;
				objWrite(instruction, objFile);
			}
			instruction = 0;
			line[strlen(line)] = '\0';

			printf(".STRINGZ \"%s\"\n", line);
			break;
		case DIR_BLKW:
			oper3 = nextImmediate(file, false);

			if (oper3 == INT_MAX) {
				fprintf(stderr, "  Line %d: Invalid operand for "
					".BLKW.\n", currentLine);
			} else if (oper3 < 1) {
				fprintf(stderr, "  Line %d: .BLKW "
					"requires an argument > 0.\n",
					currentLine);
				nextLine(file);
				errors++;
				continue;
			} else if (oper3 > 150) {
				fprintf(stderr, "  Line %d: .BLKW "
					"requires an argument < 150.\n",
					currentLine);
				nextLine(file);
				errors++;
				continue;
			}

			pc += oper3 - 1;
			oper1 = oper3;
			while (oper3 > 1) {
				objWrite(0, objFile);
				oper3--;
			}
			oper3--;

			printf(".BLKW  #%d\n", oper1);
			break;
		case DIR_FILL:
			oper3 = nextImmediate(file, false);

			if (oper3 == INT_MAX) {
				fscanf(file, "%79s", label);
				sym = findSymbol(label);

				if (NULL == sym) {
					fprintf(stderr, "  Line %d: Invalid "
						"literal for base %d.\n",
						currentLine, oper3);
					nextLine(file);
					errors++;
					continue;
				}
				instruction = sym->address;
			} else {
				instruction = oper3;
			}

			printf(".FILL  #%d\n", instruction);
			break;
		case DIR_END:
			if (endSeen) {
				fprintf(stderr, "  Line %d: Extra .END "
					"directive.\n", currentLine);
				errors ++;
			}
			endSeen = true;
			printf(".END\n");
			break;
		case OP_BR: case OP_BRN: case OP_BRZ: case OP_BRP:
		case OP_BRNZ: case OP_BRNP: case OP_BRZP:
			if (!origSeen) {
				fprintf(stderr, "  Line %d: "
					"BR instruction before .ORIG"
					"directive.\n", currentLine);
				nextLine(file);
				errors++;
				continue;
			} else if (endSeen) {
				fprintf(stderr, "  Line %d: "
					"BR instruction after .END"
					"directive.\n", currentLine);
				nextLine(file);
				errors++;
				continue;
			}

			if (strlen(line) == 2) {
				instruction = 0x7 << 9;
			} else {
				instruction = nzp(line + 2);
			}

			if (instruction & 7) {
				fprintf(stderr, "  Line %d: "
					"Invalid BR instruction.\n",
					currentLine);
				errors ++;
				continue;
			}

			fscanf(file, "%79s", label);
			sym = findSymbol(label);
			if (NULL == sym) {
				fprintf(stderr, "  Line %d: Invalid "
					"label '%s'.\n", currentLine,
					label);
				nextLine(file);
				errors++;
				continue;
			}

			oper3 = sym->address - pc;
			if (oper3 < -256 || oper3 > 255) {
				fprintf(stderr, "  Line %d: Label is "
					"too far away.\n", currentLine);
				nextLine(file);
				errors++;
				continue;
			}

			instruction |= (oper3 & 0x1ff);
			printf("%s  %s (%d addresses away)\n",
				line, label, oper3 + 1);
			break;
		case OP_AND:	// FALLTHORUGH
			instruction = 0x4000;
		case OP_ADD:
			instruction += 0x1000;

			oper1 = nextRegister(file, false);
			if (oper1 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to %s.\n", currentLine, line);
				nextLine(file);
				errors++;
				continue;
			}

			oper2 = nextRegister(file, true);
			if (oper2 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to %s.\n", currentLine, line);
				nextLine(file);
				nextLine(file);
				errors++;
				continue;
			}

			oper3 = nextRegister(file, true);
			if (oper3 == 65535) {
				oper3 = nextImmediate(file, true);
				if (oper3 > 15 || oper3 < -16) {
					fprintf(stderr, "  Line %d: Invalid operand "
						"provided to %s.\n",
						currentLine, line);
					nextLine(file);
					errors++;
					continue;
				}
				oper3 &= 0x3f;
				instruction |= 0x20;
			}

			instruction = instruction | oper1 << 9 | oper2 << 6 | oper3;
			printf("%s  R%d  R%d  %d\n", line, oper1, oper2, oper3);
			break;
		case OP_NOT:
			instruction += 0x903f;

			oper1 = nextRegister(file, false);
			if (oper1 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to NOT.\n", currentLine);
				nextLine(file);
				errors++;
				continue;
			}

			oper2 = nextRegister(file, true);
			if (oper2 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to NOT.\n", currentLine);
				nextLine(file);
				errors++;
				continue;
			}

			instruction |= oper1 << 9 | oper2 << 6;
			printf("NOT  R%d  R%d\n", oper1, oper2);
			break;
		case OP_JMP:	// FALLTHORUGH
			instruction = 0x8000;
		case OP_JSRR:
			instruction += 0x4000;

			oper1 = nextRegister(file, false);
			if (oper1 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to %s.\n", currentLine, line);
				nextLine(file);
				errors++;
				continue;
			}

			instruction |= oper1 << 6;
			printf("%s  R%d\n", line, oper1);
			break;
		case OP_JSR:
			instruction = 0x4800;

			fscanf(file, "%79s", label);
			sym = findSymbol(label);
			if (NULL == sym) {
				fprintf(stderr, "  Line %d: Invalid "
					"label '%s'.\n", currentLine,
					label);
				nextLine(file);
				errors++;
				continue;
			}

			oper3 = sym->address - pc;
			if (oper3 < -1024 || oper3 > 1023) {
				fprintf(stderr, "  Line %d: Label is "
					"too far away.\n", currentLine);
				nextLine(file);
				errors++;
				continue;
			}

			instruction |= (oper3 & 0x7ff);
			printf("JSR  %s (%d addresses away)\n",
				label, oper3 + 1);
			break;
		case OP_LEA:	// FALLTHORUGH
			instruction  = 0x3000;
		case OP_STI:	// FALLTHROUGH
			instruction += 0x1000;
		case OP_LDI:	// FALLTHORUGH
			instruction += 0x7000;
		case OP_ST:	// FALLTHORUGH
			instruction += 0x1000;
		case OP_LD:
			instruction += 0x2000;
			oper1 = nextRegister(file, false);
			if (oper1 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand for "
					"%s.\n", currentLine, line);
				nextLine(file);
				errors++;
				continue;
			}

			skipWhitespace(file);
			c = fgetc(file);

			if (c == ',') {
				skipWhitespace(file);
			} else {
				ungetc(c, file);
			}

			fscanf(file, "%79s", label);

			sym = findSymbol(label);
			if (NULL == sym) {
				fprintf(stderr, "  Line %d: Invalid "
					"label '%s'.\n", currentLine,
					label);
				nextLine(file);
				errors++;
				continue;
			}

			oper3 = sym->address - pc;
			if (oper3 < -256 || oper3 > 255) {
				fprintf(stderr, "  Line %d: Label is "
					"too far away.\n", currentLine);
				nextLine(file);
				errors++;
				continue;
			}

			instruction |= oper1 << 9 | (oper3 & 0x1ff);
			printf("%s  R%d  %s  (%d addresses away)\n",
				line, oper1, sym->name, oper3 + 1);
			break;
		case OP_STR:
			instruction  = 0x1000;
		case OP_LDR:
			instruction += 0x6000;

			oper1 = nextRegister(file, false);
			if (oper1 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to %s.\n", currentLine, line);
				nextLine(file);
				errors++;
				continue;
			}

			oper2 = nextRegister(file, true);
			if (oper2 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to %s.\n", currentLine, line);
				nextLine(file);
				errors++;
				continue;
			}

			oper3 = nextImmediate(file, true);
			if (oper3 == INT_MAX) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to %s.\n", currentLine, line);
				nextLine(file);
				errors++;
				continue;
			} else if (oper3 < -32 || oper3 > 31) {
				fprintf(stderr, "  Line %d: 3rd operand for %s "
					"needs to be >= -32 and <= 31.\n",
					currentLine, line);
				nextLine(file);
				errors++;
				continue;
			}

			instruction |= oper1 << 9 | oper2 << 6 | (oper3 & 0x3f);
			printf("%s  R%d  R%d  #%d\n", line, oper1, oper2, oper3);
			break;
		case OP_RET:
			instruction = 0xc1c0;
			printf("RET\n");
			break;
		case OP_RTI:
			printf("RTI\n");
			break;
		case OP_TRAP:
			fscanf(file, "%79s", label);

			instruction = (uint16_t) strtoul(label, &end, 16);
			if (*end) {
				fprintf(stderr, "  Line %d: Invalid "
					"literal for base 16.\n",
					currentLine);
				nextLine(file);
				errors++;
				continue;
			}

			if (instruction < 0x20 || instruction > 0x25) {
				fprintf(stderr, "  Line %d: Invalid "
					"TRAP Routine.\n", currentLine);
				nextLine(file);
				errors++;
				continue;
			}

			instruction += 0xf000;
			printf("TRAP x%x\n", instruction & 0xff);
			break;
		case OP_HALT: case OP_PUTS: case OP_PUTC: case OP_GETC:
		case OP_OUT: case OP_PUTSP: case OP_IN:
			instruction = 0xf025;

			switch (tok) {
			case OP_GETC:			// 0xF020
				instruction--;
			case OP_PUTC: case OP_OUT:	// 0xF021
				instruction--;
			case OP_PUTS:			// 0xF022
				instruction--;
			case OP_IN:			// 0xF023
				instruction--;
			case OP_PUTSP:			// 0xF024
				instruction--;
			case OP_HALT:			// oxF025
			default:
				break;
			}

			printf("%s\n", line);
			break;
		case OP_BRUNK: case OP_UNK:
			pc--;
			break;
		}

		if (tok != OP_BRUNK && tok != OP_UNK) {
			if (!endOfLine(line, file, &currentLine)) {
				nextLine(file);
				errors++;
			} else if (errors == 0 && origSeen && !endSeen) {
				objWrite(instruction, objFile);
			}
		}

	}

	printf("%d error%s found in second pass.\n",
		errors, errors != 1 ? "'s" : "");

	free(symFileName);
	free(hexFileName);
	free(binFileName);
	free(objFileName);

	free_table(&table);

	fclose(file);
	fclose(symFile);
	fclose(hexFile);
	fclose(binFile);
	fclose(objFile);

	return errors == 0;
}
