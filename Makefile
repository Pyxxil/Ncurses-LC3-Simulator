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

# Includes directory
INCDIR   = includes
# Flag to add the above directory to gcc's search path
INCLUDES = -I$(INCDIR)
# Library dependencies
LIBS     = -lncurses

# Flags to use when compiling
CFLAGS = $(INCLUDES) $(LIBS) -std=c11 -O2 -g
# Flags to add when compiling the debug version
DFLAGS = -Wall -Werror -Wpedantic -Wextra

RM = rm -f


# Default target if none is given.
default: build

# Add the debug flags
debug: CFLAGS += $(DFLAGS)
# Build the system
debug: build

# Use ctags to generate tags for the c files
tags: $(SRC)
	ctags --sort=yes --verbose $(wildcard $(INCDIR)/*.h) $^

# Build the executable file
build: $(OBJS)
	$(CC) $(CFLAGS) -o $(EXECUTABLE) $(OBJS)

%.o: %.c $(INCDIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $@

# Remove all generated files
clean:
	$(RM) $(EXECUTABLE) $(OBJS)
