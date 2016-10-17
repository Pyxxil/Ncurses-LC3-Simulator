# Ncurses-LC3-Simulator

-- Work in Progress --

An ncurses simulator for the LC-3.

Currently it only supports using .obj files that are created when the actual LC-3 Simulator assembles your program.

Usage:

First run `make`, and then, in the same directory, run the following:

```shell
$ ./LC3Simulator [OPTION] <file>
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

