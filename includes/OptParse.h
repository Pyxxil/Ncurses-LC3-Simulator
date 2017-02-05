#ifndef OPTPARSE_H
#define OPTPARSE_H

typedef enum OPTION_TYPE {
	OPTIONAL,
	REQUIRED,
	NONE,
} OPTION_TYPE;

typedef enum ARGUMENT_TYPES {
	INVALID_ARGUMENT  = -3,
	INVALID_LONG_OPT  = -4,
	INVALID_SHORT_OPT = -5,
	NO_OPT            = -6,
	NO_SUPPLIED_ARG   = -7,
} ARGUMENT_TYPES;

typedef struct options {
	char const *longOption;
	unsigned char shortOption;
	OPTION_TYPE option;
} options;

extern options returnedOption;

int parseOptions(options *__options, int argumentCount, char **argumentValues);

#endif // OPTPARSE_H
