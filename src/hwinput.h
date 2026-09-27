// Reads what is held right now: keyboard (via the window's RAWKEY messages),
// joystick in port 2, mouse / CDTV remote in port 1 (movement and buttons).
#ifndef JL_HWINPUT_H
#define JL_HWINPUT_H

#include <exec/types.h>
#include <intuition/intuition.h>

void hwInputReset(struct Window *win);   // forget keys, re-read the mouse counters
UWORD hwInputRead(struct Window *win);   // JL_IN_* mask

#endif
