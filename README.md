# Ncurses-LC3-Simulator

-- Work in Progress --

An ncurses simulator for the LC-3.

It is able to assemble, and run, programs. Assembly is still a work in progress,
but it works for the most part.

Usage:

```shell
./build.sh
```

Then, to assemble your LC-3 file:
```shell
$ ./LC3Simulator --assemble file [--assemble-only]
```
Without `--assemble-only` the simulator will run right after assembling
(assuming there were no errors during assembly).

To run your program in the simulator:
```shell
$ ./LC3Simulator --objectfile file
```

## Keymappings

**Note**: Each key is case sensitve.

The following can also be found in [includes/Keyboard.h](includes/Keyboard.h).

#### Main Menu
|Function           | Key   |
|:------------------|:-----:|
|Simulator Mode     |   s   |
|Memory Mode        |   m   |
|Log dump           |   d   |
|File Select        |   f   |
|Quit               |   q   |

#### Simulator
|Function           | Key   |
|:------------------|:-----:|
|Start              |   s   |
|Go back            |   b   |
|Step next          |   n   |
|Toggle Pause       |   p   |
|Restart            |   R   |
|Continue           |   c   |
|Reset & Continue   |   C   |
|Quit               |   q   |

### Memory
|Function           | Key   |
|:------------------|:-----:|
|Edit Line          |   e   |
|Go back            |   b   |
|Jump to line       |   j   |
|Toggle Breakpoint  |   B   |
|Move up one line   |   UP  |
|Move down one line |  DOWN |
|Quit               |   q   |
|Set PC to address  |   S   |

## LC-3 Assembly

Each instruction in the LC-3 is 16 bits long. The bits are split differently
depending on what instruction it is, however the upper 4 bits are reserved for
the opcode.

There are 8 registers in the LC-3, labeled R0 to R7. R7 is generally used to
hold the return address (and is assumed as such for instructions such as RET).

The following are the possible instruction for the LC-3:

+ AND
+ ADD
+ BR
+ JMP
+ JSR
+ JSRR
+ LD
+ LDI
+ LDR
+ LEA
+ NOT
+ RET (This is a shortcut for JMP R7)
+ RTI
+ ST
+ STI
+ STR
+ TRAP

A TRAP is a special instruction in the LC-3 -- it calls a built-in subroutine
for the Operating System running inside the LC-3. A valid TRAP operand is
one of the following:

+ 0x25 (HALT)
+ 0x24 (PUTSP)
+ 0x23 (IN)
+ 0x22 (PUTS)
+ 0x21 (OUT, PUTC)
+ 0x20 (GETC)

Some example LC-3 Assembly Files can be found in the [Examples](Examples)
directory.
