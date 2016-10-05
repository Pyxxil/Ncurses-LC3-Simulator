# Use gcc as the compiler
CC = gcc

# The program name
EXECUTABLE = LC3Simulator

# Source directory
SRCDIR = source
# Source files
SRC   := $(wildcard $(SRCDIR)/*.c)
# Object files (gathered by substitution)
OBJS  := $(SRC:.c=.o)
ASM   := $(SRC:.c=.s)

# Includes directory
INCDIR   = includes
# Flag to add the above directory to gcc's search path
INCLUDES = -I$(INCDIR)

# Curses library
CURSES   = ncurses
# Library dependencies
LIBS     = -l$(CURSES)

# Flags to use when compiling
CFLAGS = $(INCLUDES) $(LIBS) -std=c11 -g -O2
# Flags to add when compiling the debug version
DFLAGS = -Wall -Werror -Wpedantic -Wextra

RM = rm -f


# Default target if none is given.
.PHONY: default
default: build

# Add the debug flags
.PHONY: debug
debug: CFLAGS += $(DFLAGS)
# Build the system
debug: build

# Use ctags to generate tags for the c files
.PHONY: tags
tags: $(SRC)
	ctags --sort=yes --verbose $(wildcard $(INCDIR)/*.h) $^

# Build the executable file
.PHONY: build
build: $(OBJS)
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(OBJS)

.PHONY: asm
asm: $(ASM)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.s: %.c
	$(CC) $(CFLAGS) -c $< -S -o $@

# Remove all generated files
.PHONY: clean
clean:
	$(RM) $(EXECUTABLE) $(OBJS) $(ASM)

-include $(DEPFILES)

