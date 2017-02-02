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
	char const *long_option;
	unsigned char short_option;
	OPTION_TYPE option;
} options;

extern options returned_option;

int parse_options(options *__options, int argument_count, char **argument_values);

#endif // OPTPARSE_H
