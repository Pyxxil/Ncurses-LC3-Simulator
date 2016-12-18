#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "OptParse.h"

static size_t internal_arg_offset;
static size_t internal_arg_count;
static size_t internal_arg_index;
static size_t internal_offset;

static bool matched_with_equals = false;

static char const *equals = NULL;

static unsigned char reset = 1;

options returned_option = {
	.long_option = NULL,
	.short_option = '\0',
	.option = NONE,
};

typedef enum OPT_TYPE {
	NOT_OPT = 0,
	LONG,
	SHORT,
} OPT_TYPE;


/*
 * There are two types of options, those that are short (preceeded by a single
 * '-'), or long (preceeded by two '-').
 */
static OPT_TYPE option_type(char const *option)
{
	if ('-' == *option) {
		if ('-' == option[1]) {
			return LONG;
		}

		return SHORT;
	}

	return NOT_OPT;
}


/*
 * A valid argument, at least for now, is one which is either:
 *   a) Not preceeded by a '-', or
 *   b) Preceeded by a '-', but the first character following it is a digit.
 */
static inline bool is_valid_argument(char **argument_values)
{
	return (internal_arg_count - internal_arg_offset) > internal_arg_index &&
		(isdigit(argument_values[internal_arg_index + internal_arg_offset][1]) ||
		 option_type(argument_values[internal_arg_index + internal_arg_offset]) == NOT_OPT);
}

static void grab_arg(char **from, options *option)
{
	bool valid = false;
	if ((matched_with_equals && equals + 1) || (valid = is_valid_argument(from))) {
		returned_option = (options) {
			.long_option = valid ? from[internal_arg_index + internal_arg_offset] :
				equals + 1,
			.short_option = option->short_option,
			.option = option->option,
		};
	} else {
		returned_option.option = NONE;
	}
}

static bool is_long_match(char **against, char const *option)
{
	char const *possible_match = against[internal_arg_index] + 2;
	size_t option_length = strlen(option);
	size_t match_length = strlen(possible_match);
	equals = strchr(possible_match, '=');

	matched_with_equals = false;

	if (!strncmp(possible_match, option, option_length)) {
		if (equals && (size_t) equals - (size_t) possible_match == option_length) {
			return matched_with_equals = true;
		}

		return option_length == match_length;
	}

	return false;
}

static bool is_short_match(char **against, unsigned char option)
{
	char const *possible_match = against[internal_arg_index];
	equals = strchr(possible_match, '=');

	matched_with_equals = false;

	if (possible_match[internal_offset] == option) {
		if (equals && (size_t) equals == (size_t) possible_match +
				internal_offset + 1) {
			return matched_with_equals = true;
		}

		return true;
	}

	return false;
}

int parse_options(options *__options, int argument_count, char **argument_values)
{
	if (reset) {
		internal_arg_count = (size_t) argument_count;
		internal_arg_index = 1;
		internal_offset = 1;
		internal_arg_offset = 1;
		reset = 0;
	}

	if (internal_arg_count <= internal_arg_index + internal_arg_offset - 1) {
		return 0;
	}

	OPT_TYPE opt_type = option_type(argument_values[internal_arg_index]);
	returned_option = (options) {
		.long_option = NULL,
		.short_option = '\0',
		.option = NONE,
	};

	for (options *_option = __options; NULL != _option->long_option &&
			_option->short_option; _option++) {
		switch (opt_type) {
		/*
		 * A long option is of the type '--long-opt', where any character
		 * is possible, as long as there are two leading '-' characters.
		 *
		 * Options that take arguments can be provided by the user in
		 * either of two ways:
		 * 	--long-opt=arg
		 * or
		 * 	--long-opt arg
		 * Both will supply the same information back to the program.
		 */
		case LONG:
			internal_offset = 1;
			internal_arg_offset = 1;
			if (is_long_match(argument_values, _option->long_option)) {
				switch (_option->option) {
				case REQUIRED: // FALLTHROUGH
				case OPTIONAL:
					grab_arg(argument_values, _option);
					if (returned_option.option != NONE) {
						//internal_arg_index++;
					}
					break;
				case NONE: // FALLTHROUGH
				default:
					returned_option.long_option =
						argument_values[internal_arg_index];
					break;
				}
				internal_arg_index++;
				return _option->short_option;
			}
			break;
		/*
		 * Quite a few programs allow the user to specify multiple short
		 * options on a single flag, e.g. '-tvs', where each of the
		 * characters after the '-' are seperate options.
		 *
		 * I'm not sure if I'll keep the following behaviour around, but
		 * currently the following are all parsed as the same string:
		 * 	-xyz file1 file2 file3
		 * 	-xyz=file3 file1 file2
		 * 	-x file1 -y file2 -z file3
		 * Will, on matching the flags x, y, and z, will provide a
		 * matched object containing file1 for the x option, file2 for
		 * the y option, and file3 for the z option.
		 *
		 * Negative numbers, if given after a flag that takes an
		 * optional or required argument, are not seen as options
		 * themselves. However, if they are given as sole options, e.g.
		 * 	./Program -1
		 * then the -1 will be parsed as an option, and will likely
		 * return an INVALID_SHORT_OPT.
		 */
		case SHORT:
			if ('\0' == argument_values[internal_arg_index][internal_offset]) {
				internal_arg_index += internal_arg_offset;
				internal_offset = 1;
				internal_arg_offset = 1;
				return parse_options(__options, argument_count, argument_values);
			} else if (is_short_match(argument_values, _option->short_option)) {
				switch (_option->option) {
				case REQUIRED: // FALLTHROUGH
				case OPTIONAL:
					grab_arg(argument_values, _option);
					if (matched_with_equals) {
						internal_arg_index += internal_arg_offset;
						internal_offset = 0;
						internal_arg_offset = 1;
					} else if (returned_option.option != NONE) {
						internal_arg_offset++;
					}
					break;
				case NONE:
				default:
					break;
				}

				internal_offset++;
				return returned_option.short_option = _option->short_option;
			}
			break;
		/*
		 * When a command line argument is supplied, generally without at
		 * least one preceding '-' character, we will default to saying
		 * it's not an option. This sort of allows the user to provide
		 * things to fill an array, e.g. in the case of gcc:
		 *	gcc file1.c file2.c
		 * this compiles, and links, the two provided files together.
		 */
		case NOT_OPT:  // FALLTHROUGH
		default:
			returned_option.long_option =
				argument_values[internal_arg_index];
			internal_offset = 1;
			internal_arg_offset = 1;
			internal_arg_index++;
			return NO_OPT;
		}
	}

	/*
	 * We couldn't match any of the given options with the current flag, so
	 * let's return a semi-helpful reason for it.
	 */
	switch (opt_type) {
	case SHORT:
		returned_option.short_option =
			(unsigned char) argument_values[internal_arg_index]
				[internal_offset++];
		return INVALID_SHORT_OPT;
	case LONG:
		returned_option.long_option = argument_values[internal_arg_index++];
		return INVALID_LONG_OPT;
	case NOT_OPT:
	default:
		returned_option.long_option = argument_values[internal_arg_index++];
		return NO_OPT;
	}
}

void reset_parser(void)
{
	reset = 1;
}

