#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include "OptParse.h"

static size_t internalArgOffset;
static size_t internalArgCount;
static size_t internalArgIndex;
static size_t internalOffset;

static bool matchedWithEquals = false;

static char const *equals = NULL;

static unsigned char reset = 1;

options returnedOption = {
        .longOption = NULL,
        .shortOption = '\0',
        .option = NONE,
};

typedef enum OPT_TYPE
{
        NOT_OPT = 0,
        LONG,
        SHORT,
} OPT_TYPE;


/*
 * There are two types of options, those that are short (preceded by a single
 * '-'), or long (preceded by two '-').
 */
static OPT_TYPE optionType(char const *option)
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
 *   a) Not preceded by a '-', or
 *   b) Preceded by a '-', but the first character following it is a digit.
 */
static inline bool isValidArgument(char **argumentValues)
{
        return (internalArgCount - internalArgOffset) > internalArgIndex &&
               (isdigit(argumentValues[internalArgIndex + internalArgOffset][1]) ||
                       optionType(argumentValues[internalArgIndex + internalArgOffset]) == NOT_OPT);
}

static void grabArg(char **from, options *option)
{
        bool valid = false;
        if ((matchedWithEquals && equals + 1) || (valid = isValidArgument(from))) {
                returnedOption = (options) {
                        .longOption = valid ? from[internalArgIndex + internalArgOffset] : equals + 1,
                        .shortOption = option->shortOption,
                        .option = option->option,
                };
        } else {
                returnedOption.option = NONE;
        }
}

static bool isLongMatch(char **against, char const *option)
{
        char const *possibleMatch = against[internalArgIndex] + 2;
        size_t optionLength = strlen(option);
        size_t matchLength = strlen(possibleMatch);
        equals = strchr(possibleMatch, '=');

        matchedWithEquals = false;

        if (!strncmp(possibleMatch, option, optionLength)) {
                if (equals && (size_t) equals - (size_t) possibleMatch == optionLength) {
                        return (matchedWithEquals = true);
                }

                return optionLength == matchLength;
        }

        return false;
}

static bool isShortMatch(char **against, unsigned char option)
{
        char const *possibleMatch = against[internalArgIndex];
        equals = strchr(possibleMatch, '=');

        matchedWithEquals = false;

        if (possibleMatch[internalOffset] == option) {
                if (equals && (size_t) equals ==
                                      (size_t) possibleMatch + internalOffset + 1) {
                        return (matchedWithEquals = true);
                }

                return true;
        }

        return false;
}

int parseOptions(options *__options, int argumentCount, char **argumentValues)
{
        if (reset) {
                internalArgCount = (size_t) argumentCount;
                internalArgIndex = 1;
                internalOffset = 1;
                internalArgOffset = 1;
                reset = 0;
        }

        if (internalArgCount <= internalArgIndex + internalArgOffset - 1) {
                return 0;
        }

        OPT_TYPE opt_type = optionType(argumentValues[internalArgIndex]);
        returnedOption = (options) {
                .longOption = NULL,
                .shortOption = '\0',
                .option = NONE,
        };

        for (options *_option = __options; NULL != _option->longOption &&
                                           _option->shortOption; _option++) {
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
                        internalOffset = 1;
                        internalArgOffset = 1;
                        if (isLongMatch(argumentValues, _option->longOption)) {
                                switch (_option->option) {
                                case REQUIRED: // FALLTHROUGH
                                case OPTIONAL:
                                        grabArg(argumentValues, _option);
                                        if (returnedOption.option != NONE) {
                                                internalArgIndex++;
                                        }
                                        break;
                                case NONE: // FALLTHROUGH
                                default:
                                        returnedOption.longOption =
                                                argumentValues[internalArgIndex];
                                        break;
                                }
                                internalArgIndex++;
                                return _option->shortOption;
                        }
                        break;
                        /*
                         * Quite a few programs allow the user to specify multiple short
                         * options on a single flag, e.g. '-tvs', where each of the
                         * characters after the '-' are separate options.
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
                        if ('\0' == argumentValues[internalArgIndex][internalOffset]) {
                                internalArgIndex += internalArgOffset;
                                internalOffset = 1;
                                internalArgOffset = 1;
                                return parseOptions(__options, argumentCount, argumentValues);
                        } else if (isShortMatch(argumentValues, _option->shortOption)) {
                                switch (_option->option) {
                                case REQUIRED: // FALLTHROUGH
                                case OPTIONAL:
                                        grabArg(argumentValues, _option);
                                        if (matchedWithEquals) {
                                                internalArgIndex += internalArgOffset;
                                                internalOffset = 0;
                                                internalArgOffset = 1;
                                        } else if (returnedOption.option != NONE) {
                                                internalArgOffset++;
                                        }
                                        break;
                                case NONE:
                                default:
                                        break;
                                }

                                internalOffset++;
                                return returnedOption.shortOption = _option->shortOption;
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
                        returnedOption.longOption = argumentValues[internalArgIndex];
                        internalOffset = 1;
                        internalArgOffset = 1;
                        internalArgIndex++;
                        return NO_OPT;
                }
        }

        /*
         * We couldn't match any of the given options with the current flag, so
         * let's return a semi-helpful reason for it.
         */
        switch (opt_type) {
        case SHORT:
                returnedOption.shortOption =
                        (unsigned char) argumentValues[internalArgIndex][internalOffset++];
                return INVALID_SHORT_OPT;
        case LONG:
                returnedOption.longOption = argumentValues[internalArgIndex++];
                return INVALID_LONG_OPT;
        case NOT_OPT:
        default:
                returnedOption.longOption = argumentValues[internalArgIndex++];
                return NO_OPT;
        }
}

