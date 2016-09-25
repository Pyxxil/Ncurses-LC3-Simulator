#ifndef LOGGING_H
#define LOGGING_H

#include "Structs.h"

enum logError {
	ErrNoFile   = 1,
	ErrFileOpen = 2,
};

unsigned int logDump(struct program const *);

#endif // LOGGING_H
