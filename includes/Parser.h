#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <stdbool.h>

#include "Structs.h"

extern bool OSInstalled;

extern struct symbolTable tableHead;

void populateOSSymbols(void);
void populateSymbolsFromFile(struct program *prog);
struct symbol *findSymbolByAddress(uint16_t address);
bool parse(struct program *prog);
bool symbolsEmpty(void);
void freeTable(struct symbolTable *table);

#endif // PARSER_H
