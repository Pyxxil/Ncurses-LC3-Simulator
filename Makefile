# Use gcc as the compiler
CC = gcc

# The program name
EXECUTABLE = LC3Simulator
OSPATH = $(shell pwd)

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

DEFINES  = -DOS_PATH="$(OSPATH)"

# Flags to use when compiling
CFLAGS = $(INCLUDES) -std=c11 -g -O2 $(DEFINES)
# Flags to add when compiling the debug version
DFLAGS = -Wall -Werror -Wpedantic -Wextra

RM = rm -f

EXTRA = ""

# Default target if none is given.
.PHONY: default
default: build

# Add the debug flags
.PHONY: debug
debug: CFLAGS += $(DFLAGS)
debug: EXTRA = " (DEBUG)"
debug: mostlyclean
# Build the system
debug: build

# Use ctags to generate tags for the c files
.PHONY: tags
tags: $(SRC)
	ctags --sort=yes --verbose $(wildcard $(INCDIR)/*.h) $^

# Build the executable file
.PHONY: build
build: building
build: $(OBJS)
	@$(CC) $(CFLAGS) $(LIBS) -o $(EXECUTABLE) $(OBJS)

.PHONY: building
building:
	@echo " Creating executable -> $(EXECUTABLE)"

.PHONY: asm
asm: $(ASM)

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@
	@echo $(EXTRA) "CC $< -> $@"

%.s: %.c
	$(CC) $(CFLAGS) -c $< -S -o $@

.PHONY: mostlyclean
# Remove most of the files created during compilation.
mostlyclean:
	@$(foreach obj,$(OBJS),if [[ -s $(obj) ]]; then echo " RM $(obj)"; $(RM) $(obj); fi; )
	@$(foreach asm,$(ASM),if [[ -s $(asm) ]]; then echo " RM $(asm)"; $(RM) $(asm); fi; )

# Remove all generated files
.PHONY: clean
clean: mostlyclean
	@if [[ -s $(EXECUTABLE) ]]; then echo " RM $(EXECUTABLE)"; $(RM) $(EXECUTABLE); fi;

