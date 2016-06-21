## Debug
* Add a function to dump the simulator contents
  * Output similar to:
  ```
  =====================
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
* Make a log file to help with the above
  * Output time, current instruction, and any files written

## UI
* Make a popup for any sim dumps we make
  * Dismissable with a simple keystroke
  * Will just be a popup window with a border that:
    * Displays dumped contents file name
    * Displays any current needed information (e.g. The current highlighted
      memory address)
    * Allow the user to dump the contents of the output screen (?)
* Make the UI more scalable
  * For differing terminal sizes
* Output the memory context
  * Make it highlightable
  * Allow the user to move around

