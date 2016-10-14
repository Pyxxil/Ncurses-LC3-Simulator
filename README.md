# Ncurses-LC3-Simulator

-- Work in Progress --

An ncurses simulator for the LC-3.

It is able to assemble, and run, programs (currently this takes two calls).

Assembly is still a work in progress, but it works for the most part.

Usage:

First run `make`.

Then, to assemble your LC-3 file:
```shell
$ ./LC3Simulator --assemble /path/to/file.asm
```

To run your program in the simulator:
```shell
$ ./LC3Simulator --objectfile /path/to/file.obj
```

