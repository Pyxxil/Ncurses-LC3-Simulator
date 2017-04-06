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

// TODO:
//      - For Main.c:
//              - Allow the user to toggle warnings with a flag (maybe --no-warnings)
//      - For Machine.c:
//              - If the symbol file can't be found, then show the user a warning,
//                but carry on execution.
//                      - Tell them the file we were searching for, maybe prompt for
//                        correct file?
//      - Make a singlylinkedlist struct, that has 3 elements (something like this):
//              struct singlyLinkedList {
//                      void *data;
//                      void (*insert)(struct singlyLinkedList *, void *);
//                      struct singlyLinkedList *next;
//              }
//              struct singlyLinkedList list = {
//                      .data = NULL,
//                      .insert = &insert,
//                      .next = NULL,
//              };
//              void insert(struct singlyLinkedList *this, void *data)
//              {
//                      if (NULL != this->next) {
//                              this->next->insert(this->next);
//                              return;
//                      }
//                      struct singlyLinkedList *next = malloc(sizeof(singlyLinkedList));
//                      // etc.
//              }
//      - Separate the assembling portion into multiple parts:
//              Assemble(file) : bool -> parse(line) : linkedList *
//              parse would return a tokenized version of the line, e.g.
//                      parse("LABEL .fill 0x0 ; Comment") -> listHead(label("LABEL"), type(LABEL))->
//                                                              next(directive(FILL), type(DIRECTIVE))->
//                                                              next(immediate(0x0), type(IMMEDIATE))->
//                                                              next(type(COMMENT))
//              This allows modularisation as well as being able to more easily spread out
//              the logic into 2 projects (an assembler and a simulator)
//              This would agree with the idea to rewrite this in C++ (for strings, getline).
//	- Move the OS code to be builtin (?)
//		- What if someone wants to explicitly alter it -- without using instructions.
//

#define STR(x) #x
#define OSPATH(path) STR(path)
#define OS_SYM_FILE OSPATH(OS_PATH) "/LC3_OS.sym"

#define ERROR(str, ...)   fprintf(stderr, "ERROR: "   str ".\n", __VA_ARGS__)
#define WARNING(str, ...) fprintf(stderr, "WARNING: " str ".\n", __VA_ARGS__)
#define NOTE(str, ...)    fprintf(stderr, "NOTE: "    str ".\n", __VA_ARGS__)

// This will have to do for now... It allows the use of '//' as a comment.
static char lastSkippedChar = 0;

#define MAX_LABEL_LENGTH 80

/*
 * Copy the contents of one string to another, allocating enough memory to the
 * string we want to copy to.
 */

static void strmcpy(char **to, char const *from)
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
	while (EOF != (c = fgetc(file)) && isspace(c)) {
		if ('\n' == c) {
                        break;
                }
	}

	ungetc(c, file);
	lastSkippedChar = (char) c;
}

/*
 * Go to the next line of the file.
 */

static void nextLine(FILE *file)
{
	int c = 0;
	while ('\n' != (c = fgetc(file)) && EOF != c)
		/* NO OP */;

	ungetc(c, file);
}

static struct symbolTable *tableTail;
struct symbolTable tableHead = {
	.sym = NULL,
	.next = NULL,
};

struct list
{
	uint16_t instruction;
	struct list *next;
};

struct list *listTail;
struct list listHead = {
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

	if (NULL != listTail) {
		listTail = NULL;
	}
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

	if (NULL != tableTail) {
		tableTail = NULL;
	}
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

static void __addSymbol(char const *const name, uint16_t address, int line)
{
	struct symbol *symbol = malloc(sizeof(struct symbol));

	if (NULL == symbol) {
		perror("LC3-Simulator");
		exit(EXIT_FAILURE);
	}

        struct symbolTable *table;
	table = malloc(sizeof(struct symbolTable));

	if (NULL == table) {
		perror("LC3-Simulator");
		exit(EXIT_FAILURE);
	}

        strmcpy(&symbol->name, name);
        symbol->address = address;
        symbol->fromOS = false;
        symbol->line = line;

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

static int addSymbol(char const *const name, uint16_t address, int line)
{
	if (NULL != findSymbol(name)) {
                return 1;
        }

        __addSymbol(name, address, line);
	return 0;
}

static void populateSymbols(char *fileName)
{
	FILE *file = fopen(fileName, "r");
	if (NULL == file) {
		perror("LC3Simulator");
		exit(EXIT_FAILURE);
	}

	uint16_t address;
	char label[MAX_LABEL_LENGTH];
	char beginning[3] = { 0 };
	int c = 0;

	for (size_t i = 4; i > 0; i--) {
		nextLine(file);
		c = fgetc(file);
	}

	ungetc(c, file);

	while (EOF != fscanf(file, "%s %s %hx", beginning, label, &address)) {
                __addSymbol(label, address, 0);
		memset(label, 0, MAX_LABEL_LENGTH);
	}

	fclose(file);
}

void populateOSSymbols(void)
{
	populateSymbols((char *) OS_SYM_FILE);
	// For now this will serve as a way of being able to tell whether
	// something is a part of the Operating System, or from the User's
	// program.
	for (struct symbolTable *table = tableHead.next; NULL != table;
			table = table->next) {
		table->sym->fromOS = true;
	}
}

/*
 * If started up without assembling before hand, the program won't be aware
 * of the symbols in the file that will be needed in the output of the program
 * in memory.
 */

void populateSymbolsFromFile(struct program *program)
{
	if (NULL == program->symbolfile) {
		char *ext = strrchr(program->objectfile, '.');
		size_t length = strlen(program->objectfile);

		if (NULL != ext) {
			program->symbolfile = calloc(length + 1, sizeof(char));
			if (NULL == program->symbolfile) {
				perror("LC3-Simulator");
				exit(EXIT_FAILURE);
			}

			strncpy(program->symbolfile, program->objectfile,
				(size_t) (ext - program->objectfile));
		} else {
			program->symbolfile = calloc(length + 5, sizeof(char));
			if (NULL == program->symbolfile) {
				perror("LC3-Simulator");
				exit(EXIT_FAILURE);
			}

			strcpy(program->symbolfile, program->objectfile);
		}

		strcat(program->symbolfile, ".sym");
	}

	populateSymbols(program->symbolfile);
}

/*
 * Copy one string to another character by character, converting them to
 * upper case as we go.
 */

static void copy_upper(char *const from, char *to)
{
        for (char *i = from; *i; ++i) {
                *to = (char) toupper(*i);
                to++;
        }
}

/*
 * Convert every character in a string to uppercase, and compare it against the
 * provided string.
 */

static int uppercmp(char *from, char const *const to)
{
	char copy[100] = { 0 };
	copy_upper(from, copy);
	return strcmp(copy, to);
}

/*
 * Retrieve the hex form of the conditions supplied by the BR instruction.
 */

static uint16_t nzp(char const *const _nzp)
{
	uint16_t __nzp = 0;

	for (size_t i = 0; _nzp[i]; i++) {
		if ('N' == toupper(_nzp[i])) {
			__nzp |= 0x0800;
		} else if ('Z' == toupper(_nzp[i])) {
			__nzp |= 0x0400;
		} else if ('P' == toupper(_nzp[i])) {
			__nzp |= 0x0200;
		} else {
			return 0x1;
		}
	}

	return (uint16_t) (__nzp ? __nzp : 0x0e00);
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
		binary[i] = (char) (*instruction & bit ? '1' : '0');
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

	if (';' == c) {
		return true;
	} else if ('/' == c) {
		c = fgetc(file);
		if ('/' == c) {
			return true;
		}
		ungetc(c, file);
	}

	ungetc(c, file);
	return false;
}

/*
 * Check whether we have reached the end of the line.
 */

static bool endOfLine(FILE *file)
{
	int c;
	skipWhitespace(file);

	c = fgetc(file);
	if ('\n' != c && EOF != c) {
		ungetc(c, file);
		if (!iscomment(file)) {
			return false;
		}
	}

	ungetc(c, file);
	return true;
}

static const size_t hashed_letters[26] = {
        100363, 99989, 97711, 97151, 92311, 80147,
        82279,  72997, 66457, 65719, 70957, 50262,
        48407,  51151, 41047, 39371, 35401, 37039,
        28697,  27791, 20201, 21523, 6449,  4813,
        16333,  13337,
};

static size_t hash(const char *const string, size_t length) {
        if (!length) {
                return 0;
        }

        size_t _hash = 37;
        size_t first_char_on_directive = length > 1 ? (size_t) *(string + 1) : 0;

        for (size_t index = 0; index < length; ++index) {
                if (*string == '.') {
                        if (*(string + index) == '.') {
                                _hash = (_hash * hashed_letters[first_char_on_directive - 0x41u]) ^
                                        (first_char_on_directive * hashed_letters[first_char_on_directive - 0x41u]);
                        } else {
                                _hash = (_hash * hashed_letters[((size_t) *(string + index)) - 0x41u]) ^
                                        (first_char_on_directive * hashed_letters[
                                                ((size_t) *(string + index)) - 0x41u]);
                        }
                } else {
                        _hash = (_hash * hashed_letters[((size_t) *(string + index)) - 0x41u]) ^
                                (((size_t) *string) * hashed_letters[(size_t) *(string + index) - 0x41u]);
                }
        }

        return _hash;
}

/*
 * Read the given file to find what the next token is.
 *
 * If none match, we return an OP_UNK. In general, only the first pass of the
 * file will care about OP_UNK/OP_BRUNK -- the first pass will use the token
 * found as a label.
 */

static enum Token nextToken(FILE *file, char *buffer)
{
	if (EOF == fscanf(file, "%99s", buffer)) {
                // TODO: This should probably return something like OP_NONE to signal the end.
		*buffer = '\0';
                return OP_NONE;
	}

        char upper_copy[100] = { 0 };

        enum Token token;

        copy_upper(buffer, upper_copy);
        size_t hashed = hash(upper_copy, strlen(upper_copy));

        switch (hashed) {
        case 0xc847e7858f3bda:  // hash("ADD")
                token = OP_ADD;
                break;
        case 0x6972ca4e0fe6aa:  // hash("AND")
                token = OP_AND;
                break;
        case 0x3155da34ef9599:  // hash("JMP")
                token = OP_JMP;
                break;
        case 0x1b83c7f078f08f:  // hash("JSR")
                token = OP_JSR;
                break;
        case 0x8cef8cf169d53357:  // hash("JSRR")
                token = OP_JSRR;
                break;
        case 0x389286e2ae:  // hash("LD")
                token = OP_LD;
                break;
        case 0x1ff918099c2706:  // hash("LDR")
                token = OP_LDR;
                break;
        case 0x395e0e09be9292:  // hash("LDI")
                token = OP_LDI;
                break;
        case 0x5251df343da02e:  // hash("LEA")
                token = OP_LEA;
                break;
        case 0x880559a3c58a1:  // hash("NOT")
                token = OP_NOT;
                break;
        case 0x230e9051f39cad:  // hash("RET")
                token = OP_RET;
                break;
        case 0x193d3c0bf91b3f:  // hash("RTI")
                token = OP_RTI;
                break;
        case 0x163a87a587:  // hash("ST")
                token = OP_ST;
                break;
        case 0xc901e4ff8fff4:  // hash("STR")
                token = OP_STR;
                break;
        case 0x168a8037dda834:  // hash("STI")
                token = OP_STI;
                break;
        case 0xf8bf61331cc727e5:  // hash("TRAP")
                token = OP_TRAP;
                break;
        case 0x41ca364764d222c1:  // hash("GETC")
                token = OP_GETC;
                break;
        case 0x7226fce93d74058f:  // hash("PUTC")
                token = OP_PUTC;
                break;
        case 0x502dd326219b2:  // hash("OUT")
                token = OP_OUT;
                break;
        case 0x45f5f141ac74c8d6:  // hash("HALT")
                token = OP_HALT;
                break;
        case 0x5709aa5303:  // hash("IN")
                token = OP_IN;
                break;
        case 0x2fd2a79caa1c2dd9:  // hash("PUTS")
                token = OP_PUTS;
                break;
        case 0xd2378ae5bb8f0363:  // hash("PUTSP")
                token = OP_PUTSP;
                break;
        case 0xae00dbad81af1338:  // hash(".ORIG")
                token = DIR_ORIG;
                break;
        case 0xef479fa15c8dd7c1:  // hash(".STRINGZ")
                token = DIR_STRINGZ;
                break;
        case 0x66e0b0b6cd5a4e20:  // hash(".FILL")
                token = DIR_FILL;
                break;
        case 0xd143ba5fa8981851:  // hash(".END")
                token = DIR_END;
                break;
        case 0xb97d9f7e2d9fd702:  // hash(".BLKW")
                token = DIR_BLKW;
                break;
        case 0x346c5d7733:  // hash("BR")
        case 0x6c916d80285dac45:  // hash("BRNZP")
                token = OP_BR;
                break;
        case 0x68bcc37dfdb0eef5:  // hash("BRZP")
        case 0x68bcc38217e14bbd:  // hash("BRPZ")
                token = OP_BRZP;
                break;
        case 0x94abd7909f4a83d7:  // hash("BRNP")
        case 0x94abd7c59fc129d7:  // hash("BRPN")
                token = OP_BRNP;
                break;
        case 0x53a77816176567d9:  // hash("BRNZ")
        case 0x53a77822b71faf99:  // hash("BRZN")
                token = OP_BRNZ;
                break;
        case 0x28eaa0470f8463:  // hash("BRN")
                token = OP_BRN;
                break;
        case 0xaab21915b9189:  // hash("BRZ")
                token = OP_BRZ;
                break;
        case 0x1f7e55ca7ca627:  // hash("BRP")
                token = OP_BRP;
                break;
        default:
                token = OP_LABEL;
                break;
        }

        return token;
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
		if (';' == label[i] || ':' == label[i] ||
				('/' == label[i] && '/' == label[i + 1])) {
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
	int c;
	int immediate;
	int base;
	char line[MAX_LABEL_LENGTH];
	char *end = NULL;

	skipWhitespace(file);

	c = fgetc(file);
	memset(line, 0, MAX_LABEL_LENGTH);

	if (allowedComma && ',' == c) {
		skipWhitespace(file);
		c = fgetc(file);
	}

	if ('-' == c) {
		base = 10;
		line[0] = '-';
	} else if ('#' == c) {
		base = 10;
		c = fgetc(file);
		if ('-' == c) {
			line[0] = '-';
		} else {
			line[0] = '0';
			ungetc(c, file);
		}
	} else if ('X' == toupper(c)) {
		base = 16;
		line[0] = '0';
	} else if (isdigit(c)) {
                if ('0' == c) {
                        c = fgetc(file);
                        if ('X' == toupper(c)) {
                                base = 16;
                                line[0] = '0';
                        } else if ('B' == toupper(c)) {
                                base = 2;
                                line[0] = '0';
                        } else {
                                ungetc(c, file);
                                base = 10;
                        }
                } else {
                        base = 10;
                        line[0] = (char) c;
                }
        } else if ('B' == toupper(c)) {
                base = 2;
                line[0] = '0';
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

	immediate = (int) strtol(line, &end, base);

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
	int c;
	skipWhitespace(file);

	c = fgetc(file);
	if (allowedComma && ',' == c) {
		skipWhitespace(file);
		c = fgetc(file);
	}

	if ('R' != toupper(c)) {
		ungetc(c, file);
		return 65535;
	}

	c = fgetc(file);
	if (c < '0' || c > '7') {
		ungetc(c, file);
		return 65535;
	}

	return (uint16_t) (c - 0x30);
}

bool parse(struct program *prog)
{
	uint16_t instruction = 0, pc = 0, operandOne, operandTwo, realPc = 0;
	int c, currentLine = 1, errors = 0, operandThree, pass = 1, errorCount = 0;
	char line[100] = {0}, label[MAX_LABEL_LENGTH], *end;
	bool origSeen = false, endSeen = false;

	enum Token tok;
	struct symbol *sym;

	FILE *asmFile;

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

	// This isn't the best place for this as it populates the symbol table
	// with information we don't need to show.
	populateOSSymbols();
	OSInstalled = true;

	asmFile = fopen(prog->assemblyfile, "r");
	if (NULL == asmFile) {
		perror("LC3-Simulator");
		exit(EXIT_FAILURE);
	}

	printf("STARTING FIRST PASS...\n");

	while (1) {
		memset(line, 0, sizeof(line));
		memset(label, 0, MAX_LABEL_LENGTH);
		instruction = 0;
		operandOne = 0;

		skipWhitespace(asmFile);
		c = fgetc(asmFile);

		if (EOF == c) {
			if (2 == pass) {
				printf("%d error%s found in second pass.\n",
					errors, 1 != errors ? "'s" : "");
				break;
			}
			printf("%d error%s found in first pass.\n",
				errors, 1 == errors ? "" : "'s");
			printf("STARTING SECOND PASS...\n");
			pass = 2;
			pc = realPc;
			rewind(asmFile);
			currentLine = 1;
			errorCount = errors;
			errors = 0;
			endSeen = false;
			continue;
		} else if ('\n' == c) {
			currentLine++;
			continue;
		} else  if (';' == c) {
			nextLine(asmFile);
			continue;
		} else if ('/' == c) {
			c = fgetc(asmFile);
			if ('/' == lastSkippedChar || '/' == c) {
				nextLine(asmFile);
				continue;
			}
			ungetc(c, asmFile);
		}

		ungetc(c, asmFile);
		tok = nextToken(asmFile, line);
		skipWhitespace(asmFile);

		if (origSeen) {
			pc++;
		}

		switch (tok) {
		case DIR_ORIG:
			if (1 != pass) {
				instruction = realPc;
				nextLine(asmFile);
				break;
			}

			if (origSeen) {
                                ERROR("Line %3d: Extra .ORIG directive", currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			} else if (endSeen) {
				ERROR("Line %3d: .END seen before .ORIG", currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

			operandThree = nextImmediate(asmFile, false);
			if (operandThree > 0xffff || operandThree < 0) {
				ERROR("Line %3d: Invalid operand for .ORIG", currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			} else if (operandThree < 0x3000) {
                                WARNING("Line %3d: .ORIG memory address is in OS memory",
                                        currentLine);
                        }

			pc = realPc = (uint16_t) operandThree;
			origSeen = true;
			break;
		case DIR_STRINGZ:
			c = fgetc(asmFile);

			if ('"' != c) {
				ERROR("Line %3d: No string supplied to .STRINGZ",
                                      currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

			memset(line, 0, 100);
			for (size_t i = 0; '"' != (c = fgetc(asmFile)) &&
					   i < 100; i++) {
				if ('\\' == c) {
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
					instruction = (uint16_t) (c & 0xff);
				}

				pc++;
				if (1 != pass) {
					insert(instruction);
				}
			}

			instruction = 0;
			line[strlen(line)] = '\0';
			break;
		case DIR_BLKW:
			operandThree = nextImmediate(asmFile, false);

			if (INT_MAX == operandThree) {
				ERROR("Line %3d: Invalid operand for .BLKW",
                                      currentLine);
			} else if (operandThree < 1) {
				ERROR("Line %3d: .BLKW requires an argument > 0",
                                      currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			} else if (operandThree > 150) {
				ERROR("Line %3d: .BLKW requires an argument < 150",
                                      currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

			pc += operandThree - 1;
			if (1 != pass) {
				instruction = 0;
				while (operandThree-- > 1) {
					insert(instruction);
				}
			}
			break;
		case DIR_FILL:
			if (1 == pass) {
				nextLine(asmFile);
				break;
			}

			operandThree = nextImmediate(asmFile, false);

			if (INT_MAX == operandThree) {
				if (EOF == fscanf(asmFile, "%79s", label)) {
					ERROR("Line %3d: Unexpected end of file", currentLine);
					ungetc(EOF, asmFile);
					errors++;
					continue;
				}

				extractLabel(label, asmFile);
				sym = findSymbol(label);

				if (NULL == sym) {
					ERROR("Line %3d: Invalid literal for base %d",
                                              currentLine, operandThree);
					nextLine(asmFile);
					errors++;
					continue;
				}
				instruction = sym->address;
			} else {
				instruction = (uint16_t) operandThree;
			}
			break;
		case DIR_END:
			if (1 != pass) {
				nextLine(asmFile);
				endSeen = true;
				break;
			}

			if (endSeen) {
				ERROR("Line %3d: Extra .END directive", currentLine);
				errors++;
			}

			endSeen = true;
			break;
		case OP_BR:
		case OP_BRN:            // FALLTHROUGH
		case OP_BRZ:            // FALLTHROUGH
		case OP_BRP:            // FALLTHROUGH
		case OP_BRNZ:           // FALLTHROUGH
		case OP_BRNP:           // FALLTHROUGH
		case OP_BRZP:           // FALLTHROUGH
			if (1 == pass) {
				nextLine(asmFile);
				break;
			}

			instruction = nzp(line + 2);
			if (instruction & 7) {
				ERROR("Line %3d: Invalid BR instruction",
                                      currentLine);
				errors++;
				continue;
			}

			if (EOF == fscanf(asmFile, "%79s", label)) {
				ERROR("Line %3d: Unexpected end of file",
                                      currentLine);
				ungetc(EOF, asmFile);
				errors++;
				continue;
			}

			extractLabel(label, asmFile);
			sym = findSymbol(label);
			if (NULL == sym) {
				ERROR("Line %3d: Invalid label '%s'",
                                      currentLine, label);
				nextLine(asmFile);
				errors++;
				continue;
			}

			operandThree = sym->address - pc + 1;
			if (operandThree < -256 || operandThree > 255) {
				ERROR("Line %3d: Label is too far away",
                                      currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

                        // Check if the last statement has the same condition code as this one,
                        // or if the last one was a BR(nzp), in which case it's covered by that
                        // one.
                        if ((instruction & 0xFE00) == (listTail->instruction & 0xFE00) ||
                                        (listTail->instruction & 0xFE00) == 0xFE00) {
                                WARNING("Line %3d: Statement possibly has no effect, "
                                                "as last line has same BR condition",
                                        currentLine);
                        }

			instruction |= (operandThree & 0x1ff);
			if (prog->verbosity) {
				if (prog->verbosity > 2) {
					printf("Line %3d:  ", currentLine);
				}
				printf("%-5s  R%d  %-30s  (%4d address%s away)",
					line, operandOne, sym->name, operandThree, operandThree > 1 ? "es" : "");
				puts("");
			}
			break;
		case OP_AND:
			instruction = 0x4000;
		case OP_ADD:            // FALLTHORUGH
			instruction += 0x1000;

			if (1 == pass) {
				nextLine(asmFile);
				break;
			}

			operandOne = nextRegister(asmFile, false);
			if (65535 == operandOne) {
				ERROR("Line %3d: Invalid operand provided to %s",
                                      currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			}

			operandTwo = nextRegister(asmFile, true);
			if (65535 == operandTwo) {
				ERROR("Line %3d: Invalid operand provided to %s",
                                      currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			}

			operandThree = nextRegister(asmFile, true);
			if (65535 == operandThree) {
				operandThree = nextImmediate(asmFile, true);
				if (operandThree > 15 || operandThree < -16) {
					ERROR("Line %3d: Invalid operand provided to %s",
						currentLine, line);
					nextLine(asmFile);
					errors++;
					continue;
				}
				operandThree &= 0x3f;
				instruction |= 0x20;
			}

			instruction = instruction | (uint16_t) (operandOne << 9 |
								operandTwo << 6 | operandThree);
			if (prog->verbosity) {
				if (prog->verbosity > 2) {
					printf("Line %3d:  ", currentLine);
				}
				printf("%-5s  R%d  R%d  ", line, operandOne, operandTwo);
				if (instruction & 0x20) {
					printf("#%d", ((int16_t) ((operandThree & 0x3f) << 11)) >> 11);
				} else {
					printf("R%d", operandThree & 7);
				}
				puts("");
			}
			break;
		case OP_NOT:
			instruction += 0x903f;

			if (1 == pass) {
				nextLine(asmFile);
				break;
			}

			operandOne = nextRegister(asmFile, false);
			if (65535 == operandOne) {
				ERROR("Line %3d: Invalid operand provided to NOT",
                                      currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

			operandTwo = nextRegister(asmFile, true);
			if (65535 == operandTwo) {
				ERROR("Line %3d: Invalid operand provided to NOT",
                                      currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

			instruction |= operandOne << 9 | operandTwo << 6;
			if (prog->verbosity) {
				if (prog->verbosity > 2) {
					printf("Line %3d:  ", currentLine);
				}
				printf("%-5s  R%d  R%d", line, operandOne, operandTwo);
				puts("");
			}
			break;
		case OP_JMP:
			instruction = 0x8000;
		case OP_JSRR:           // FALLTHORUGH
			instruction += 0x4000;

			if (1 == pass) {
				nextLine(asmFile);
				break;
			}

			operandOne = nextRegister(asmFile, false);
			if (65535 == operandOne) {
				ERROR("Line %3d: Invalid operand provided to %s",
                                      currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			}

			instruction |= operandOne << 6;
			if (prog->verbosity) {
				if (prog->verbosity > 2) {
					printf("Line %3d:  ", currentLine);
				}
				printf("%-5s  R%d", line, operandOne);
				puts("");
			}
			break;
		case OP_JSR:
			instruction = 0x4800;

			if (1 == pass) {
				nextLine(asmFile);
				break;
			}

			if (EOF == fscanf(asmFile, "%79s", label)) {
				ERROR("Line %3d: Unexpected end of file",
                                      currentLine);
				ungetc(EOF, asmFile);
				errors++;
				continue;
			}

			extractLabel(label, asmFile);
			sym = findSymbol(label);
			if (NULL == sym) {
				ERROR("Line %3d: Invalid label '%s'",
                                      currentLine, label);
				nextLine(asmFile);
				errors++;
				continue;
			}

			operandThree = sym->address - pc + 1;
			if (operandThree < -1024 || operandThree > 1023) {
				ERROR("Line %3d: Label is too far away",
                                      currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

			instruction |= (operandThree & 0x7ff);
			if (prog->verbosity) {
				if (prog->verbosity > 2) {
					printf("Line %3d:  ", currentLine);
				}
				printf("%-5s  R%d  %-30s  (%4d address%s away)",
                                        line, operandOne, sym->name, operandThree,
					operandThree > 1 ? "es" : "");
				puts("");
			}
			break;
		case OP_LEA:
			instruction = 0x3000;
		case OP_STI:            // FALLTHROUGH
			instruction += 0x1000;
		case OP_LDI:            // FALLTHORUGH
			instruction += 0x7000;
		case OP_ST:             // FALLTHORUGH
			instruction += 0x1000;
		case OP_LD:             // FALLTHORUGH
			instruction += 0x2000;

			if (1 == pass) {
				nextLine(asmFile);
				break;
			}

			operandOne = nextRegister(asmFile, false);
			if (65535 == operandOne) {
				ERROR("Line %3d: Invalid operand for %s",
                                      currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			}

			skipWhitespace(asmFile);
			c = fgetc(asmFile);

			if (',' == c) {
				skipWhitespace(asmFile);
			} else {
				ungetc(c, asmFile);
			}

			if (EOF == fscanf(asmFile, "%79s", label)) {
				ERROR("Line %3d: Unexpected end of file",
                                      currentLine);
				ungetc(EOF, asmFile);
				errors++;
				continue;
			}

			extractLabel(label, asmFile);
			sym = findSymbol(label);
			if (NULL == sym) {
				ERROR("Line %3d: Invalid label '%s'", currentLine, label);
				nextLine(asmFile);
				errors++;
				continue;
			}

			operandThree = sym->address - pc + 1;
			if (operandThree < -256 || operandThree > 255) {
				ERROR("Line %3d: Label is too far away ( %s  %d  %d   %s )",
					currentLine, line, operandOne, operandThree, sym->name);
				nextLine(asmFile);
				errors++;
				continue;
			}

			instruction |= operandOne << 9 | (operandThree & 0x1ff);
			if (prog->verbosity) {
				if (prog->verbosity > 2) {
					printf("Line %3d:  ", currentLine);
				}
				printf("%-5s  R%d  %-30s  (%4d address%s away)",
					line, operandOne, sym->name, operandThree,
					operandThree > 1 ? "es" : "");
				puts("");
			}
			break;
		case OP_STR:
			instruction = 0x1000;
		case OP_LDR:            // FALLTHROUGH
			instruction += 0x6000;

			if (1 == pass) {
				nextLine(asmFile);
				break;
			}

			operandOne = nextRegister(asmFile, false);
			if (65535 == operandOne) {
				ERROR("Line %3d: Invalid operand provided to %s",
                                      currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			}

			operandTwo = nextRegister(asmFile, true);
			if (65535 == operandTwo) {
				ERROR("Line %3d: Invalid operand provided to %s",
                                      currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			}

			operandThree = nextImmediate(asmFile, true);
			if (INT_MAX == operandThree) {
				ERROR("Line %3d: Invalid operand provided to %s",
                                      currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			} else if (operandThree < -32 || operandThree > 31) {
				ERROR("Line %3d: Third operand for %s needs to be >= -32 and <= 31",
                                      currentLine, line);
				nextLine(asmFile);
				errors++;
				continue;
			}

			instruction |= operandOne << 9 | operandTwo << 6 | (operandThree & 0x3f);
			if (prog->verbosity) {
				if (prog->verbosity > 2) {
					printf("Line %3d:  ", currentLine);
				}
				printf("%-5s  R%d  R%d  #%d", line, operandOne,
					operandTwo, operandThree);
				puts("");
			}
			break;
		case OP_RET:
			if (1 == pass) {
				nextLine(asmFile);
				break;
			}

			instruction = 0xc1c0;
			if (prog->verbosity) {
				if (prog->verbosity > 2) {
					printf("Line %3d:  ", currentLine);
				}
				printf("%-4s", line);
				puts("");
			}
			break;
		case OP_RTI:
			if (1 == pass) {
				nextLine(asmFile);
				break;
			}

			if (prog->verbosity) {
				if (prog->verbosity > 2) {
					printf("Line %3d:  ", currentLine);
				}
				printf("%-5s", line);
				puts("");
			}
			break;
		case OP_TRAP:
			if (1 == pass) {
				nextLine(asmFile);
				break;
			}

			operandThree = nextImmediate(asmFile, false);
			if (INT_MAX == operandThree) {
				ERROR("Line %3d: Invalid operand for TRAP",
                                      currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			} else if (operandThree < 0x20 || operandThree > 0x25) {
				ERROR("Line %3d: Invalid TRAP Routine",
                                      currentLine);
				nextLine(asmFile);
				errors++;
				continue;
			}

			instruction = (uint16_t) (0xf000 + (uint16_t) operandThree);
			if (prog->verbosity) {
				if (prog->verbosity > 2) {
					printf("Line %3d:  ", currentLine);
				}
				printf("%-5s 0x%x", line, operandThree);
				puts("");
			}
			break;
		case OP_HALT:
			instruction += (uint16_t) 0x0001;
		case OP_PUTSP:          // FALLTHROUGH
			instruction += (uint16_t) 0x0001;
		case OP_IN:             // FALLTHROUGH
			instruction += (uint16_t) 0x0001;
		case OP_PUTS:           // FALLTHROUGH
			instruction += (uint16_t) 0x0001;
		case OP_PUTC:           // FALLTHROUGH
		case OP_OUT:            // FALLTHROUGH
			instruction += (uint16_t) 0x0001;
		case OP_GETC:           // FALLTHROUGH
			instruction += (uint16_t) 0xF020;

			if (1 == pass) {
				nextLine(asmFile);
			} else if (prog->verbosity) {
				if (prog->verbosity > 2) {
					printf("Line %3d:  ", currentLine);
				}
				printf("%-62s", line);
				puts("");
			}
			break;
		case OP_LABEL:
		default:
			pc--;
			if (1 == pass) {
				end = strchr(line, ':');
				if (NULL != end) {
					char *colon = end + strlen(end) - 1;
					for (; colon != end; colon--) {
						*colon = '\0';
					}
					*colon = '\0';
				}

                                struct symbol *foundSymbol = findSymbolByAddress(pc);
                                if (NULL != foundSymbol) {
                                        // As far as I'm aware this should only happen if a label
                                        // has no instruction following it, e.g.:
                                        //      .ORIG 0x3000
                                        //      LABEL_ONE
                                        //      LABEL_TWO  ; Warning thrown for this line
                                        //      LABEL_THREE ; Warning also thrown
                                        //      .END
                                        // Of course, if more than 2 labels share an address this will
                                        // compare the last with the first, e.g. in the above example,
                                        // the following 2 warnings will be thrown:
                                        //       Line   2: 'LABEL_TWO' shares a memory address (0x3000) with 'LABEL_ONE'
                                        //       Line   3: 'LABEL_THREE' shares a memory address (0x3000) with 'LABEL_ONE
                                        // TODO: Possibly fix this so each symbol has a line number, and they can be
                                        // TODO: printed out to help the user find the troublesome code.
                                        WARNING("Line %3d: '%s' shares a memory address (%#04x) with '%s'",
                                                currentLine, line, pc, foundSymbol->name);
                                        NOTE("Previous label was declared on line %d", foundSymbol->line);
                                }

				if (addSymbol(line, pc, currentLine)) {
					ERROR("Line %3d: Multiple definitions of label '%s'",
                                              currentLine, line);
					nextLine(asmFile);
					errors++;
				}

				if (prog->verbosity) {
					if (prog->verbosity > 2) {
						printf("Line %3d: ", currentLine);
					}
					printf("Found label '%s'", line);
					if (prog->verbosity > 1) {
						printf(" with address 0x%4x", pc);
					}
					puts("");
				}
			}
			break;
		}

		if (1 != pass) {
			if (OP_LABEL != tok) {
				if (!endOfLine(asmFile)) {
					ERROR("Line %3d: Too many operands provided for %s",
                                              currentLine, line);
					nextLine(asmFile);
					errors++;
				} else if (!errors && origSeen && !endSeen) {
					insert(instruction);
				}
			}
		} else {
                        if (endSeen && tok != OP_NONE && uppercmp(line, ".END")) {
                                printf("OP_NONE = %d  op = %d  DIR_END = %d\n", OP_NONE, tok, DIR_END);
                                WARNING("Line %3d: Found %s after .END directive. It will be ignored",
                                        currentLine, line);
                                nextLine(asmFile);
                        }
                }
	}

	errorCount += errors;
	fclose(asmFile);

	if (!errorCount) {
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

		for (struct symbolTable *table = tableHead.next; NULL != table;
		     table = table->next) {
			if (!table->sym->fromOS) {
				symWrite(table->sym, symFile);
			}
		}

		for (struct list *list = listHead.next; NULL != list;
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

	return !errorCount;
}

