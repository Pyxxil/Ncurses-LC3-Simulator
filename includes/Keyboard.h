#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <curses.h>

/*
 * For now, these will be constants. At a later stage this will (hopefully)
 * change to allow the user to change the key's.
 */


// General Keyboard controls
const int QUIT         = 'q';
const int GOBACK       = 'b';

const int KEYUP        = KEY_UP;
const int KEYDOWN      = KEY_DOWN;

// Keyboard controls for the Simulator View
const int STEP_NEXT    = 'n';
const int PAUSE        = 'p';
const int START        = 's';
const int RUN          = 'r';
const int RESTART      = 'R';
const int CONTINUE     = 'c';
const int CONTINUE_RUN = 'C';

// Keyboard controls for the Main View
const int LOGDUMP      = 'd';
const int MEMVIEW      = 'm';
const int FILESEL      = 'f';
const int SIMVIEW      = 's';

// Keyboard controls for the Memory View
const int JUMP         = 'j';
const int EDITFILE     = 'e';
const int SETPC        = 's';


#endif // KEYBOARD_H
