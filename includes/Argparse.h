#ifndef ARGPARSE_H
#define ARGPARSE_H

#include "Error.h"

void errhandle(struct program const *);
int argparse(int, char **, struct program *);

#endif // ARGPARSE_H
