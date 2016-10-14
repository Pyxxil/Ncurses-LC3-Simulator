#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stdio.h>

#include "Structs.h"

extern struct symbolTable tableHead;

void populateSymbolsFromFile(struct program *prog);
struct symbol *findSymbolByAddress(uint16_t address);
bool parse(struct program *prog);
bool symbolsEmpty();
void freeTable(struct symbolTable *table);

#endif // PARSER_H
