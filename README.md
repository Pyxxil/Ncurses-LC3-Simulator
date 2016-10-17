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

## Keymappings

**Note**: Each key is case sensitve.

The following can also be found in [includes/Keyboard.h](includes/Keyboard.h).

#### Main Menu
|Function           | Key        |
|:------------------|:----------:|
|Simulator Mode     |  s         |
|Memory Mode        |  m         |
|Log dump           |  d         |
|File Select        |  d         |
|Quit               |  q         |

#### Simulator
|Function           | Key        |
|:------------------|:----------:|
|Start              |  s         |
|Go back            |  b         |
|Step next          |  n         |
|Toggle Pause       |  p         |
|Restart            |  R         |
|Continue           |  c         |
|Reset & Continue   |  C         |
|Quit               |  q         |

### Memory
|Function           | Key        |
|:------------------|:----------:|
|Edit Line          |  e         |
|Go back            |  b         |
|Jump to line       |  j         |
|Toggle Breakpoint  |  B         |
|Move up one line   |  KEY\_UP   |
|Move down one line |  KEY\_DOWN |
|Quit               |  q         |

