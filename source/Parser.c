#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

#include "Parser.h"
#include "Token.h"

#ifndef OS_PATH
#error "No Path has been supplied for the Operating System."
#endif

#define STR(x) #x
#define OSPATH(path) STR(path)
#define OS_SYM_FILE OSPATH(OS_PATH) "/LC3_OS.sym"

static const unsigned MAX_LABEL_LENGTH = 80;

/*
 * Copy the contents of one string to another, allocating enough memory to the
 * string we want to copy to.
 */

static inline void strmcpy(char **to, char const *from)
{
	size_t len = strlen(from) + 1;
	*to = malloc(sizeof(char) * len);
	if (NULL == *to) {
		perror("LC3-Simulator");
		exit(EXIT_FAILURE);
	}
	strncpy(*to, from, len);
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

static struct symbolTable *tableTail;
struct symbolTable tableHead = {
	.sym = NULL,
	.next = NULL,
};

struct list {
	uint16_t instruction;
	struct list *next;
};

struct list *listTail;
struct list listHead  = {
	.instruction = 0,
	.next = NULL,
};

static void insert(uint16_t instruction)
{
	struct list *list = malloc(sizeof(struct list));
	if (NULL == list) {
		perror("LC3-Simulator");
		exit(EXIT_FAILURE);
	}

	list->instruction = instruction;
	list->next = NULL;

	if (NULL == listHead.next) {
		listHead.next = list;
	} else {
		listTail->next = list;
	}

	listTail = list;
}

static void __freeList(struct list *list)
{
	if (NULL != list->next) {
		__freeList(list->next);
		free(list->next);
	}
}

static void freeList(struct list *list)
{
	__freeList(list);

	if (NULL != listTail)
		listTail = NULL;
}

static void __freeTable(struct symbolTable *table)
{
	if (NULL != table->next) {
		__freeTable(table->next);
		free(table->next);
	}

	if (NULL != table->sym) {
		free(table->sym->name);
		free(table->sym);
	}
}

void freeTable(struct symbolTable *table)
{
	__freeTable(table);

	if (NULL != tableTail)
		tableTail = NULL;
}

/*
 * Add a symbol by name. This isn't exactly optimal, but without hashing/maps
 * this is probably the easier way to do it. It also allows us to easily look up
 * an address in order.
 */
static struct symbol *findSymbol(char const *const name)
{
	struct symbolTable *symTable = tableHead.next;

	while (NULL != symTable) {
		if (!strcmp(symTable->sym->name, name)) {
			return symTable->sym;
		}
		symTable = symTable->next;
	}

	return NULL;
}

/*
 * Find out whether there exists a symbol at a specific address.
 *
 * As each symbol is added to the table in order, we can quit early if the
 * address of the current symbol is greater than the address we're looking for.
 */
struct symbol *findSymbolByAddress(uint16_t address)
{
	struct symbolTable *symTable = tableHead.next;

	while (NULL != symTable) {
		if (NULL == symTable->sym) {
			return NULL;
		}
		if (symTable->sym->address == address) {
			return symTable->sym;
		} else if (symTable->sym->address > address) {
			return NULL;
		}
		symTable = symTable->next;
	}

	return NULL;
}

static void __addSymbol(char const *const name, uint16_t address)
{
	struct symbol *symbol = malloc(sizeof(struct symbol));
	if (NULL == symbol) {
		perror("LC3-Simulator");
		exit(EXIT_FAILURE);
	}

	strmcpy(&symbol->name, name);
	symbol->address = address;
	symbol->fromOS = false;

	struct symbolTable *table = malloc(sizeof(struct symbolTable));
	if (NULL == table) {
		perror("LC3-Simulator");
		exit(EXIT_FAILURE);
	}

	table->sym = symbol;
	table->next = NULL;

	if (NULL == tableHead.next) {
		tableHead.next = table;
	} else {
		tableTail->next = table;
	}

	tableTail = table;
}

/*
 * Add a Symbol by name and address into the Symbol Table.
 */

static int addSymbol(char const *const name, uint16_t address)
{
	if (NULL != findSymbol(name))
		return 1;

	__addSymbol(name, address);
	return 0;
}

static void populateSymbols(char *fileName)
{
	FILE *file = fopen(fileName, "r");

	int c;
	for (size_t i = 4; i > 0; i--) {
		nextLine(file);
		c = fgetc(file);
	}

	uint16_t address;
	char label[MAX_LABEL_LENGTH];
	char beginning[3];
	ungetc(c, file);

	while (fscanf(file, "%s %s %hx", beginning, label, &address) != EOF) {
		__addSymbol(label, address);
		memset(label, 0, MAX_LABEL_LENGTH);
	}

	fclose(file);
}

void populateOSSymbols()
{
	populateSymbols(OS_SYM_FILE);
	// For now this will serve as a way of being able to tell whether
	// something is a part of the Operating System, or from the User's
	// program.
	for (struct symbolTable *table = tableHead.next; table != NULL;
			table = table->next) {
		table->sym->fromOS = true;
	}

}

/*
 * If started up without assembling before hand, the program won't be aware
 * of the symbols in the file that will be needed in the output of the program
 * in memory.
 */

void populateSymbolsFromFile(struct program *prog)
{
	if (NULL == prog->symbolfile) {
		char *ext = strrchr(prog->objectfile, '.');
		size_t length = strlen(prog->objectfile);
		if (NULL != ext) {
			prog->symbolfile = calloc(length + 1, sizeof(char));
			if (NULL == prog->symbolfile) {
				perror("LC3-Simulator");
				exit(EXIT_FAILURE);
			}

			strncpy(prog->symbolfile, prog->objectfile,
				(size_t) (ext - prog->objectfile));
		} else {
			prog->symbolfile = calloc(length + 5, sizeof(char));
			if (NULL == prog->symbolfile) {
				perror("LC3-Simulator");
				exit(EXIT_FAILURE);
			}

			strcpy(prog->symbolfile, prog->objectfile);
		}

		strcat(prog->symbolfile, ".sym");
	}

	populateSymbols(prog->symbolfile);
	printf("%s\n", prog->symbolfile);
}

/*
 * Convert every character in a string to uppercase, and compare it against the
 * provided string.
 */

static int uppercmp(char *from, char const *const to)
{
	char copy[100];
	strcpy(copy, from);

	for (size_t i = 0; copy[i]; i++) {
		copy[i] = (char) toupper(copy[i]);
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

static void objWrite(uint16_t *instruction, FILE *objFile)
{
	unsigned char bytes[2];
	bytes[0] = (unsigned char) ((*instruction & 0xff00) >> 8);
	bytes[1] = (unsigned char) (*instruction & 0xff);
	fwrite(bytes, 2, 1, objFile);
}

static void binWrite(uint16_t *instruction, FILE *binFile)
{
	char binary[] = "0000000000000000";
	for (int i = 15, bit = 1; i >= 0; i--, bit <<= 1) {
		binary[i] = *instruction & bit ? '1' : '0';
	}

	fprintf(binFile, "%s\n", binary);
}

static void hexWrite(uint16_t *instruction, FILE *hexFile)
{
	fprintf(hexFile, "%04X\n", *instruction);
}

static void symWrite(struct symbol *symbol, FILE *symFile)
{
	fprintf(symFile, "//\t%-18s %04X\n", symbol->name, symbol->address);
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

static bool endOfLine(FILE *file)
{
	skipWhitespace(file);
	int c = fgetc(file);

	if (c != '\n' && c != EOF) {
		ungetc(c, file);
		if (!iscomment(file)) {
			return false;
		}
	}

	ungetc(c, file);
	return true;
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
	if (fscanf(file, "%99s", buffer) == EOF) {
		*buffer = '\0';
	}

	if (!uppercmp(buffer, "ADD")) {
		return OP_ADD;
	} else if (!uppercmp(buffer, "AND"))      {
		return OP_AND;
	} else if (!uppercmp(buffer, "JMP"))      {
		return OP_JMP;
	} else if (!uppercmp(buffer, "JSR"))      {
		return OP_JSR;
	} else if (!uppercmp(buffer, "JSRR"))     {
		return OP_JSRR;
	} else if (!uppercmp(buffer, "LD"))       {
		return OP_LD;
	} else if (!uppercmp(buffer, "LDR"))      {
		return OP_LDR;
	} else if (!uppercmp(buffer, "LDI"))      {
		return OP_LDI;
	} else if (!uppercmp(buffer, "LEA"))      {
		return OP_LEA;
	} else if (!uppercmp(buffer, "NOT"))      {
		return OP_NOT;
	} else if (!uppercmp(buffer, "RET"))      {
		return OP_RET;
	} else if (!uppercmp(buffer, "RTI"))      {
		return OP_RTI;
	} else if (!uppercmp(buffer, "ST"))       {
		return OP_ST;
	} else if (!uppercmp(buffer, "STR"))      {
		return OP_STR;
	} else if (!uppercmp(buffer, "STI"))      {
		return OP_STI;
	} else if (!uppercmp(buffer, "TRAP"))     {
		return OP_TRAP;
	} else if (!uppercmp(buffer, "GETC"))     {
		return OP_GETC;
	} else if (!uppercmp(buffer, "PUTC"))     {
		return OP_PUTC;
	} else if (!uppercmp(buffer, "OUT"))      {
		return OP_OUT;
	} else if (!uppercmp(buffer, "HALT"))     {
		return OP_HALT;
	} else if (!uppercmp(buffer, "IN"))       {
		return OP_IN;
	} else if (!uppercmp(buffer, "PUTS"))     {
		return OP_PUTS;
	} else if (!uppercmp(buffer, "PUTSP"))    {
		return OP_PUTSP;
	} else if (!uppercmp(buffer, ".ORIG"))    {
		return DIR_ORIG;
	} else if (!uppercmp(buffer, ".STRINGZ")) {
		return DIR_STRINGZ;
	} else if (!uppercmp(buffer, ".FILL"))    {
		return DIR_FILL;
	} else if (!uppercmp(buffer, ".END"))     {
		return DIR_END;
	} else if (!uppercmp(buffer, ".BLKW"))    {
		return DIR_BLKW;
	} else if (!uppercmpn(buffer, "BR", 2))   {
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
 * Sometimes, we might come across an instruction that looks something like
 * the following (Note the trailing comment):
 * 	LD R2, LABEL; Hello
 *
 * The way that the file is read to grab that label would mean that we get the
 * extraneous parts (the '; Hello'), but we don't want that part, we just want
 * 'LABEL' as the label.
 * So, we want to remove any extraneous parts from the label, and return them to
 * the file.
 */

static void extractLabel(char *label, FILE *file)
{
	size_t length = strlen(label);
	size_t i = 0;
	for (; i < length; i++) {
		if (label[i] == ';' || label[i] == ':' ||
				(label[i] == '/' && label[i + 1] == '/') ) {
			break;
		}
	}
	for (; --length >= i;) {
		ungetc(label[length], file);
		label[length] = '\0';
	}
}

/*
 * Find the next immediate value in the file. If found, return it.
 *
 * An immediate value takes the form:
 * 	allowedComma = True:
 * 	--> [ \t]*,?[ \t]*((0?[xX][\da-fA-F]+)|(#?-?\d+))
 * 	e.g. '  , #2'  or ' ,  x2'  or ',-2'
 * 	allowedComma = False:
 * 	--> [ \t]+((0?[xX][\da-fA-F]+)|(#?-?\d+))
 * 	e.g. ' #2'  or ' x2'  or '    -2'
 *
 * If the value isn't found, or is in the wrong format, return INT_MAX as an
 * error.
 *
 */

static int nextImmediate(FILE *file, bool allowedComma)
{
	skipWhitespace(file);

	int c = fgetc(file);
	int immediate;
	int repr;

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
			line[0] = (char) c;
		}
	} else {
		ungetc(c, file);
		return INT_MAX;
	}

	c = fgetc(file);
	for (size_t i = 1; i < MAX_LABEL_LENGTH && isxdigit(c); i++) {
		line[i] = (char) c;
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

	return (uint16_t) (c - 0x30);
}

bool parse(struct program *prog)
{
	if (NULL == prog->assemblyfile) {
		fprintf(stderr, "No assembly file provided.\n");
		return false;
	} else {
		size_t length = strlen(prog->assemblyfile) + 1;
		char *ext = strrchr(prog->assemblyfile, '.');

		if (NULL == prog->symbolfile) {
			if (NULL != ext) {
				prog->symbolfile = calloc(length, sizeof(char));
				if (NULL == prog->symbolfile) {
					perror("LC3-Simulator");
					exit(EXIT_FAILURE);
				}

				strncpy(prog->symbolfile, prog->assemblyfile,
					(size_t) (ext - prog->assemblyfile));
			} else {
				prog->symbolfile = calloc(length + 5,
							sizeof(char));
				if (NULL == prog->symbolfile) {
					perror("LC3-Simulator");
					exit(EXIT_FAILURE);
				}

				strcpy(prog->symbolfile, prog->assemblyfile);
			}

			strcat(prog->symbolfile, ".sym");
		}

		if (NULL == prog->hexoutfile) {
			if (NULL != ext) {
				prog->hexoutfile = calloc(length, sizeof(char));
				if (NULL == prog->hexoutfile) {
					perror("LC3-Simulator");
					exit(EXIT_FAILURE);
				}

				strncpy(prog->hexoutfile, prog->assemblyfile,
					(size_t) (ext - prog->assemblyfile));
			} else {
				prog->hexoutfile = calloc(length + 5,
							sizeof(char));
				if (NULL == prog->hexoutfile) {
					perror("LC3-Simulator");
					exit(EXIT_FAILURE);
				}

				strcpy(prog->hexoutfile, prog->assemblyfile);
			}

			strcat(prog->hexoutfile, ".hex");
		}

		if (NULL == prog->binoutfile) {
			if (NULL != ext) {
				prog->binoutfile = calloc(length, sizeof(char));
				if (NULL == prog->binoutfile) {
					perror("LC3-Simulator");
					exit(EXIT_FAILURE);
				}

				strncpy(prog->binoutfile, prog->assemblyfile,
					(size_t) (ext - prog->assemblyfile));
			} else {
				prog->binoutfile = calloc(length + 5,
							sizeof(char));
				if (NULL == prog->binoutfile) {
					perror("LC3-Simulator");
					exit(EXIT_FAILURE);
				}

				strcpy(prog->binoutfile, prog->assemblyfile);
			}

			strcat(prog->binoutfile, ".bin");
		}

		if (NULL == prog->objectfile) {
			if (NULL != ext) {
				prog->objectfile = calloc(length, sizeof(char));
				if (NULL == prog->objectfile) {
					perror("LC3-Simulator");
					exit(EXIT_FAILURE);
				}

				strncpy(prog->objectfile, prog->assemblyfile,
					(size_t) (ext - prog->assemblyfile));
			} else {
				prog->objectfile = calloc(length + 5,
							sizeof(char));
				if (NULL == prog->objectfile) {
					perror("LC3-Simulator");
					exit(EXIT_FAILURE);
				}

				strcpy(prog->objectfile, prog->assemblyfile);
			}

			strcat(prog->objectfile, ".obj");
		}
	}

	uint16_t instruction = 0, pc = 0, oper1, oper2, actualPC;
	int c, currentLine = 1, errors = 0, oper3, pass = 1;
	char line[100] = { 0 }, label[MAX_LABEL_LENGTH], *end;
	bool origSeen = false, endSeen = false;

	enum Token tok;
	struct symbol *sym;

	// This isn't the best place for this as it populates the symbol table
	// with information we don't need to show.
	populateOSSymbols();
	bool OSInstalled = true;
	(void) OSInstalled;

	FILE *asmFile = fopen(prog->assemblyfile, "r");
	if (NULL == asmFile) {
		perror("LC3-Simulator");
		exit(EXIT_FAILURE);
	}

	printf("STARTING FIRST PASS...\n");

	while (1) {
		memset(line,  0, sizeof(line));
		memset(label, 0, MAX_LABEL_LENGTH);
		instruction = 0;
		oper1 = 0;
		oper2 = 0;
		oper3 = 0;

		skipWhitespace(asmFile);
		c = fgetc(asmFile);

		if (c == EOF) {
			if (pass == 2) {
				printf("%d error%s found in second pass.\n",
					errors, errors != 1 ? "'s" : "");
				break;
			}
			printf("%d error%s found in first pass.\n",
				errors, errors == 1 ? "" : "'s");
			printf("STARTING SECOND PASS...\n");
			pass = 2;
			pc = actualPC;
			rewind(asmFile);
			currentLine = 1;
			errors = 0;
			endSeen = false;
			continue;
		} else if (c == '\n') {
			currentLine++;
			continue;
		} else if (c == ';' || c == '/') {
			if (c == '/' && (c = fgetc(asmFile)) != '/') {
				ungetc(c, asmFile);
			} else {
				nextLine(asmFile);
				continue;
			}
		}

		ungetc(c, asmFile);
		tok = nextToken(asmFile, line);
		skipWhitespace(asmFile);

		if (origSeen)  pc++;

		switch (tok) {
		case DIR_ORIG:
			if (pass != 1) {
				instruction = actualPC;
				nextLine(asmFile);
				break;
			}

			if (origSeen) {
				fprintf(stderr, "  Line %d: Extra .ORIG "
					"directive.\n", currentLine);
				nextLine(asmFile);
				errors ++;
				continue;
			} else if (endSeen) {
				fprintf(stderr, "  Line %d: .END seen "
					"before .ORIG.\n", currentLine);
				nextLine(asmFile);
				errors ++;
				continue;
			}

			oper3 = nextImmediate(asmFile, false);
			if (oper3 > 0xffff || oper3 < 0) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"for .ORIG.\n", currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

			pc = actualPC = (uint16_t) oper3;
		//	printf("PC  %x\n", pc);
			origSeen = true;
			break;
		case DIR_STRINGZ:
			c = fgetc(asmFile);

			if (c != '"') {
				fprintf(stderr, "  Line %d: No string "
					"supplied to .STRINGZ.\n",
					currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

			memset(line, 0, 100);
			for (size_t i = 0; (c = fgetc(asmFile)) != '"' &&
					i < 100; i++) {
				if (c == '\\') {
					switch (c = fgetc(asmFile)) {
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
						ungetc(c, asmFile);
						break;
					}
				} else {
					line[i] = (char) c;
					instruction = c & 0xff;
				}

				pc++;
				if (pass != 1) {
					insert(instruction);
				}
			}

			instruction = 0;
			line[strlen(line)] = '\0';
			break;
		case DIR_BLKW:
			oper3 = nextImmediate(asmFile, false);

			if (oper3 == INT_MAX) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"for .BLKW.\n", currentLine);
			} else if (oper3 < 1) {
				fprintf(stderr, "  Line %d: .BLKW "
					"requires an argument > 0.\n",
					currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			} else if (oper3 > 150) {
				fprintf(stderr, "  Line %d: .BLKW "
					"requires an argument < 150.\n",
					currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

			pc += oper3 - 1;
			if (pass != 1) {
				oper1 = (uint16_t) oper3;
				instruction = 0;
				while (oper3-- > 1) {
					insert(instruction);
				}
			}
			break;
		case DIR_FILL:
			if (pass == 1) {
				nextLine(asmFile);
				break;
			}

			oper3 = nextImmediate(asmFile, false);

			if (oper3 == INT_MAX) {
				if (fscanf(asmFile, "%79s", label) == EOF) {
					fprintf(stderr, "Line %d: Unexpected "
							"end of file.\n",
							currentLine);
					ungetc(EOF, asmFile);
					errors++;
					continue;
				}

				extractLabel(label, asmFile);
				sym = findSymbol(label);

				if (NULL == sym) {
					fprintf(stderr, "  Line %d: Invalid "
						"literal for base %d.\n",
						currentLine, oper3);
					nextLine(asmFile);
					errors++;
					continue;
				}
				instruction = sym->address;
			} else {
				instruction = (uint16_t) oper3;
			}
			break;
		case DIR_END:
			if (pass != 1) {
				nextLine(asmFile);
				endSeen = true;
				break;
			}

			if (endSeen) {
				fprintf(stderr, "  Line %d: Extra .END "
					"directive.\n", currentLine);
				errors ++;
			}

			endSeen = true;
			break;
		case OP_BR: case OP_BRN: case OP_BRZ: case OP_BRP:
		case OP_BRNZ: case OP_BRNP: case OP_BRZP:
			if (pass == 1) {
				nextLine(asmFile);
				break;
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

			if (fscanf(asmFile, "%79s", label) == EOF) {
				fprintf(stderr, "Line %d: Unexpected "
						"end of file.\n",
						currentLine);
				ungetc(EOF, asmFile);
				errors++;
				continue;
			}

			extractLabel(label, asmFile);
			sym = findSymbol(label);
			if (NULL == sym) {
				fprintf(stderr, "  Line %d: Invalid "
					"label '%s'.\n", currentLine,
					label);
				nextLine(asmFile);
				errors++;
				continue;
			}

			oper3 = sym->address - pc + 1;
			if (oper3 < -256 || oper3 > 255) {
				fprintf(stderr, "  Line %d: Label is "
					"too far away.\n", currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

			instruction |= (oper3 & 0x1ff);
			if (prog->verbosity) {
				printf("%-5s  R%d  %-30s  (%4d addresses away)",
					line, oper1, sym->name, oper3);
				if (prog->verbosity > 2) {
					printf("  (line %3d)", currentLine);
				}
				puts("");
			}
			break;
		case OP_AND:	// FALLTHORUGH
			instruction = 0x4000;
		case OP_ADD:
			instruction += 0x1000;

			if (pass == 1) {
				nextLine(asmFile);
				break;
			}

			oper1 = nextRegister(asmFile, false);
			if (oper1 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to %s.\n", currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			}

			oper2 = nextRegister(asmFile, true);
			if (oper2 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to %s.\n", currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			}

			oper3 = nextRegister(asmFile, true);
			if (oper3 == 65535) {
				oper3 = nextImmediate(asmFile, true);
				if (oper3 > 15 || oper3 < -16) {
					fprintf(stderr, "  Line %d: Invalid "
						"operand provided to %s.\n",
						currentLine, line);
					nextLine(asmFile);
					errors++;
					continue;
				}
				oper3 &= 0x3f;
				instruction |= 0x20;
			}

			instruction = instruction | oper1 << 9 | oper2 << 6 | oper3;
			if (prog->verbosity) {
				printf("%-5s  R%d  R%d ", line, oper1, oper2);
				if (prog->verbosity > 2) {
					printf("  (line %3d)", currentLine);
				}
				puts("");
			}
			break;
		case OP_NOT:
			instruction += 0x903f;

			if (pass == 1) {
				nextLine(asmFile);
				break;
			}

			oper1 = nextRegister(asmFile, false);
			if (oper1 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to NOT.\n", currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

			oper2 = nextRegister(asmFile, true);
			if (oper2 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to NOT.\n", currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

			instruction |= oper1 << 9 | oper2 << 6;
			if (prog->verbosity) {
				printf("%-5s  R%d  R%d",
					line, oper1, oper2);
				if (prog->verbosity > 2) {
					printf("  (line %3d)", currentLine);
				}
				puts("");
			}
			break;
		case OP_JMP:	// FALLTHORUGH
			instruction = 0x8000;
		case OP_JSRR:
			instruction += 0x4000;

			if (pass == 1) {
				nextLine(asmFile);
				break;
			}

			oper1 = nextRegister(asmFile, false);
			if (oper1 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to %s.\n", currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			}

			instruction |= oper1 << 6;
			if (prog->verbosity) {
				printf("%-5s  R%d", line, oper1);
				if (prog->verbosity > 2) {
					printf("  (line %3d)", currentLine);
				}
				puts("");
			}
			break;
		case OP_JSR:
			instruction = 0x4800;

			if (pass == 1) {
				nextLine(asmFile);
				break;
			}

			if (fscanf(asmFile, "%79s", label) == EOF) {
				fprintf(stderr, "Line %d: Unexpected "
						"end of file.\n",
						currentLine);
				ungetc(EOF, asmFile);
				errors++;
				continue;
			}

			extractLabel(label, asmFile);
			sym = findSymbol(label);
			if (NULL == sym) {
				fprintf(stderr, "  Line %d: Invalid "
					"label '%s'.\n", currentLine,
					label);
				nextLine(asmFile);
				errors++;
				continue;
			}

			oper3 = sym->address - pc + 1;
			if (oper3 < -1024 || oper3 > 1023) {
				fprintf(stderr, "  Line %d: Label is "
					"too far away.\n", currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

			instruction |= (oper3 & 0x7ff);
			if (prog->verbosity) {
				printf("%-5s  R%d  %-30s  (%4d addresses away)",
					line, oper1, sym->name, oper3);
				if (prog->verbosity > 2) {
					printf("  (line %3d)", currentLine);
				}
				puts("");
			}
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

			if (pass == 1) {
				nextLine(asmFile);
				break;
			}

			oper1 = nextRegister(asmFile, false);
			if (oper1 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"for %s.\n", currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			}

			skipWhitespace(asmFile);
			c = fgetc(asmFile);

			if (c == ',') {
				skipWhitespace(asmFile);
			} else {
				ungetc(c, asmFile);
			}

			if (fscanf(asmFile, "%79s", label) == EOF) {
				fprintf(stderr, "Line %d: Unexpected "
						"end of file.\n",
						currentLine);
				ungetc(EOF, asmFile);
				errors++;
				continue;
			}

			extractLabel(label, asmFile);
			sym = findSymbol(label);
			if (NULL == sym) {
				fprintf(stderr, "  Line %d: Invalid "
					"label '%s'.\n", currentLine,
					label);
				nextLine(asmFile);
				errors++;
				continue;
			}

			oper3 = sym->address - pc + 1;
			if (oper3 < -256 || oper3 > 255) {
				fprintf(stderr, "  Line %d: Label is "
					"too far away ( %s  %d  %d   %s ).\n",
					currentLine, line, oper1, oper3,
					sym->name);
				nextLine(asmFile);
				errors++;
				continue;
			}

			instruction |= oper1 << 9 | (oper3 & 0x1ff);
			if (prog->verbosity) {
				printf("%-5s  R%d  %-30s  (%4d addresses away)",
					line, oper1, sym->name, oper3);
				if (prog->verbosity > 2) {
					printf("  (line %3d)", currentLine);
				}
				puts("");
			}
			break;
		case OP_STR:	// FALLTHROUGH
			instruction  = 0x1000;
		case OP_LDR:
			instruction += 0x6000;

			if (pass == 1) {
				nextLine(asmFile);
				break;
			}

			oper1 = nextRegister(asmFile, false);
			if (oper1 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to %s.\n", currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			}

			oper2 = nextRegister(asmFile, true);
			if (oper2 == 65535) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to %s.\n", currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			}

			oper3 = nextImmediate(asmFile, true);
			if (oper3 == INT_MAX) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"provided to %s.\n", currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			} else if (oper3 < -32 || oper3 > 31) {
				fprintf(stderr, "  Line %d: 3rd operand for %s "
					"needs to be >= -32 and <= 31.\n",
					currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			}

			instruction |= oper1 << 9 | oper2 << 6 | (oper3 & 0x3f);
			if (prog->verbosity) {
				printf("%-5s  R%d  R%d #%d",
					line, oper1, oper2, oper3);
				if (prog->verbosity > 2) {
					printf("  (line %3d)", currentLine);
				}
				puts("");
			}
			break;
		case OP_RET:
			if (pass == 1) {
				nextLine(asmFile);
				break;
			}

			instruction = 0xc1c0;
			if (prog->verbosity) {
				printf("%-4s", line);
				if (prog->verbosity > 2) {
					printf("  (line %3d)", currentLine);
				}
				puts("");
			}
			break;
		case OP_RTI:
			if (pass == 1) {
				nextLine(asmFile);
				break;
			}

			if (prog->verbosity) {
				printf("%-5s", line);
				if (prog->verbosity > 2) {
					printf(" (line %3d)", currentLine);
				}
				puts("");
			}
			break;
		case OP_TRAP:
			if (pass == 1) {
				nextLine(asmFile);
				break;
			}

			oper3 = nextImmediate(asmFile, false);
			if (oper3 == INT_MAX) {
				fprintf(stderr, "  Line %d: Invalid operand "
					"for TRAP.\n", currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			} else if (oper3 < 0x20 || oper3 > 0x25) {
				fprintf(stderr, "  Line %d: Invalid "
					"TRAP Routine.\n", currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

			instruction = 0xf000 + oper3;
			if (prog->verbosity) {
				printf("%-5s 0x%x", line, oper3);
				if (prog->verbosity > 2) {
					printf("  (line %3d)", currentLine);
				}
				puts("");
			}
			break;
		case OP_HALT: case OP_PUTS: case OP_PUTC: case OP_GETC:
		case OP_OUT: case OP_PUTSP: case OP_IN:
			instruction = 0xf025;

			if (pass == 1) {
				nextLine(asmFile);
				break;
			}

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

			if (prog->verbosity) {
				printf("%-5s", line);
				if (prog->verbosity > 2) {
					printf(" (line %3d)", currentLine);
				}
				puts("");
			}
			break;
		case OP_BRUNK: case OP_UNK:
			pc--;
			if (pass == 1) {
				end = strchr(line, ':');
				if (NULL != end) {
					char *colon = end + strlen(end) - 1;
					for (; colon != end; colon--) {
						*colon = '\0';
					}
					*colon = '\0';
				}

				if (addSymbol(line, pc)) {
					fprintf(stderr, "  Line %d: "
						"Multiple definitions of label "
						"'%s'\n", currentLine, line);
					nextLine(asmFile);
					errors++;
				}

				if (prog->verbosity) {
					printf("Found label '%s'", line);
					if (prog->verbosity > 2) {
						printf(" on line %d", currentLine);
					}
					if (prog->verbosity > 1) {
						printf(" with address 0x%4x", pc);
					}
					puts("");
				}
			}
			break;
		}

		if (pass != 1) {
			if (tok != OP_BRUNK && tok != OP_UNK) {
				if (!endOfLine(asmFile)) {
					fprintf(stderr, "  Line %d: Too many "
						"operands provided for %s.\n",
						currentLine, line);
					nextLine(asmFile);
					errors++;
				} else if (errors == 0 && origSeen && !endSeen) {
					insert(instruction);
				}
			}
		}
	}

	fclose(asmFile);

	if (errors == 0) {
		FILE *symFile = fopen(prog->symbolfile, "w+");
		FILE *hexFile = fopen(prog->hexoutfile, "w+");
		FILE *binFile = fopen(prog->binoutfile, "w+");
		FILE *objFile = fopen(prog->objectfile, "wb+");
		if (NULL == symFile || NULL == hexFile || NULL == binFile ||
				NULL == objFile) {
			perror("LC3-Simulator");
			exit(EXIT_FAILURE);
		}

		fprintf(symFile, "// Symbol table\n");
		fprintf(symFile, "// Scope level 0:\n");
		fprintf(symFile, "//\tSymbol Name        Page Address\n");
		fprintf(symFile, "//\t-----------------  ------------\n");

		for (struct symbolTable *table= tableHead.next; table != NULL;
				table = table->next) {
			if (!table->sym->fromOS) {
				symWrite(table->sym, symFile);
			}
		}

		for (struct list *list = listHead.next; list != NULL;
				list = list->next) {
			hexWrite(&(list->instruction), hexFile);
			binWrite(&(list->instruction), binFile);
			objWrite(&(list->instruction), objFile);
		}

		fclose(symFile);
		fclose(hexFile);
		fclose(binFile);
		fclose(objFile);
	}

	freeList(&listHead);

	return errors == 0;
}

