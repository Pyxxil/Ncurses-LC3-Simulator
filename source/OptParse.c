#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "OptParse.h"

static size_t internal_arg_offset;
static size_t internal_arg_count;
static size_t internal_arg_index;
static size_t internal_offset;

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
		 * is possible.
		 *
		 * Some programs allow the user, if the option has an optional/required
		 * argument, to end it with an '=' value. This would mean the
		 * option looks like so:
		 * 	--long-opt=argument
		 *
		 * This is something that hopefully will be implemented at a
		 * later date, for now the argument must be supplied as a second
		 * command line string, such as:
		 * 	--long-opt argument
		 *
		 * See the comments above the line 'case SHORT' below for a few
		 * problems with the current implementation.
		 */
		case LONG:
			internal_offset = 1;
			internal_arg_offset = 1;
			if (!strcmp(argument_values[internal_arg_index] + 2,
					_option->long_option)) {
				switch (_option->option) {
				case REQUIRED:
					if (is_valid_argument(argument_values)) {
						returned_option.long_option =
							argument_values[++internal_arg_index];
						returned_option.option = REQUIRED;
					} else {
						//returned_option.long_option =
						//	argument_values[internal_arg_index];
						returned_option.option = NONE;
					}
					break;
				case OPTIONAL:
					if (is_valid_argument(argument_values)) {
						returned_option.long_option =
							argument_values[++internal_arg_index];
						returned_option.option = OPTIONAL;
					} else {
						//returned_option.long_option =
						//	argument_values[internal_arg_index];
						returned_option.option = NONE;
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
		 * currently the following is valid (assume f requires an arg):
		 * 	-fff file1 file2 file3
		 * would result in the parser returning 3 flags, each with a
		 * different file, that is to say it's semantically equivalent
		 * to this:
		 * 	-f file1 -f file2 -f file3
		 *
		 * One of the problems currently with how the parser deals with
		 * determing whether the current argument is a flag or not is to
		 * check whether it starts with '-'. This means that if you want
		 * to have the user pass in a negative number, you'd either have
		 * to work around using quotes (or some other delimiter) to
		 * gather these
		 *  -> Currently have a work around. The problem with this
		 *  -> is that it doesn't currently work with optionals (in
		 *  -> that it doesn't check whether, say, -1 is a valid
		 *  -> option before just assuming it's a valid argument for
		 *  -> the option).
		 *
		 *  -> It might be possible that, later, when the parser allows
		 *  -> using '-' at the end of an option to give the argument that
		 *  -> the work around above might not be needed, but for now it
		 *  -> will do.
		 */
		case SHORT:
			if ('\0' == argument_values[internal_arg_index][internal_offset]) {
				internal_arg_index += internal_arg_offset;
				internal_offset = 1;
				internal_arg_offset = 1;
				return parse_options(__options, argument_count, argument_values);
			} else if (argument_values[internal_arg_index][internal_offset]
					== _option->short_option) {
				switch (_option->option) {
				case REQUIRED:
					if (is_valid_argument(argument_values)) {
						returned_option.long_option =
							argument_values[internal_arg_index - 1];
						returned_option.option = REQUIRED;
						internal_arg_offset++;
					} else {
						returned_option.option = NONE;
					}
					break;
				case OPTIONAL:
					if (is_valid_argument(argument_values)) {
						returned_option.long_option =
							argument_values[internal_arg_index +
								internal_arg_offset];
						returned_option.option = OPTIONAL;
						internal_arg_offset++;
					} else {
						returned_option.option = NONE;
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
			(unsigned char) argument_values[internal_arg_index][internal_offset++];
		return INVALID_SHORT_OPT;
	case LONG:
		returned_option.long_option = argument_values[internal_arg_index++];
		return INVALID_LONG_OPT;
	case NOT_OPT:
		returned_option.long_option = argument_values[internal_arg_index++];
		return NO_OPT;
	}
}

void reset_parser(void)
{
	reset = 1;
}

