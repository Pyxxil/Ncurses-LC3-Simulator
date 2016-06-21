# Use gcc as the compiler
CC = gcc

# The program name
OUTFILE = LC3Simulator

# Source directory
SRCDIR   = source
# Source files
SRC     := $(wildcard $(SRCDIR)/*.c)
# Object files
OBJS    := $(patsubst %.c, %.o, $(SRC))

# Includes directory
INCDIR 	 = includes
# Flag to add the above directory to gcc's search path
INCLUDES = -I$(INCDIR)
# Library dependencies
LIBS     = -lncurses

# Flags to use when compiling
CFLAGS 	 = $(INCLUDES) $(LIBS) -std=c11
# Flags to add when compiling the debug version
DFLAGS 	 = -Wall -Werror -Wpedantic -Wextra -DDEBUG

GCC6    := $(shell which gcc-6 2>/dev/null)

# When run without any command-line options, run the build
# If using make > 3.8, this should work
.DEFAULT_GOAL := build

# Just in case the user isn't using make > 3.8, we'll use this instead
.PHONY: default
default: build

ifdef GCC6
# Using Homebrew's version of gcc-6 for debug, because clang throws a
# warning when -lncurses is linked but not used, but only if it exists
# on the system
debug: CC = gcc-6
endif
# Add the debug flags
debug: CFLAGS += $(DFLAGS)
# Build the system
debug: build

# Use ctags to generate tags for the c files
tags:
	ctags $(SRC)

# Build the executable file
build: $(OBJS)
	$(CC) $(CFLAGS) -o $(OUTFILE) $(OBJS)

%.o: %.c $(INCDIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $@

# Remove all generated files
clean:
	@echo "Cleaning up"
	rm $(OUTFILE) $(OBJS)

