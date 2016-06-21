CC = gcc

OUTFILE = Simulator
SRCDIR  = source
SRC    := $(wildcard $(SRCDIR)/*.c)
OBJS   := $(patsubst %.c, %.o, $(SRC))
INCDIR 	= -Iincludes
LIBS 	= -lncurses

# Flags to use when compiling
CFLAGS 	= $(INCDIR) $(LIBS) -std=c11
# Flags to use when compiling the debug version
DFLAGS 	= -Wall -Werror -Wpedantic -Wextra -DDEBUG


all: build

# Not really needed, but it's nice to use.
debug: CC = gcc-6
debug: CFLAGS += $(DFLAGS)
debug: build

# To tag the files
tags:
	ctags $(SRC)

# For building
build: $(OBJS)
	$(CC) $(CFLAGS) -o $(OUTFILE) $(OBJS)

%.o: %.c $(INCDIR)/%.h
	$(CC) $(CFLAGS) -c $< -o $@

# Remove all unneccessary files
clean:
	@echo "Cleaning up"
	rm $(OUTFILE) $(OBJS)

