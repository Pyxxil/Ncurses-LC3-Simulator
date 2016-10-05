## Features
* [x] Run .obj files
* [ ] Handle files as input or generated
  * Mostly there, just need an option to switch the file while running
* [ ] Assemble Files
  * [ ] Output a .obj file, .sym file, .bin file, and .hex file
  * [ ] Proper Error Handling
* [ ]

## Debugging
* [ ] Add a function to dump the simulator contents
  * [ ] Output similar to:
  ```
  =====================
  REG       HEX     DEC
  R0  --  0x0000      0
  R1  --  0x0000      0
  R2  --  0x0000      0
  R3  --  0x0000      0
  R4  --  0x0000      0
  R5  --  0x0000      0
  R6  --  0x0000      0
  R7  --  0x0000      0
  PC  --  0x0000      0
  IR  --  0x0000      0
  CC  --  Z
  =====================
  ```
* [ ] Make a log file to help with the above
  * [ ] Output time, current instruction, and any files written

## UI
* [ ] Make a popup for any sim dumps
  * [ ] Displays dumped contents file name
  * [ ] Displays any current needed information (e.g. The current highlighted memory address)
  * [ ] Allows the user to dump the contents of the output screen
* [ ] Make the UI more scalable
  * [ ] For differing terminal sizes
* [x] Output the memory context
  * [x] Make it highlightable
  * [x] Allow the user to move around
* [x] Popup window for inputting file / changing file
* [x] Popup window to figure where the user wants to jump to in memory.

## Additional Features
* [ ] Allow users to change what keys do what.
* [ ] Proper Error Handling (in general)

## Documentation
* [ ] Create a file that contains all startup key configurations
* [ ] Work on comments.
* [ ] Provide some examples (Currently: 3)
