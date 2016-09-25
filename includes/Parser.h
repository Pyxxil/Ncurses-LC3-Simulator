#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#include "Token.h"

struct Parser {
	FILE *file;
	size_t length;
};

void getNext(struct Parser *);
void initParser(struct Parser *);

#endif // PARSER_H
