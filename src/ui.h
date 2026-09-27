// The launcher screen: v0.x look (640x256 hires, 4 colours, topaz 8, the
// copper gradients) on a real Intuition screen, so the OS keeps working and
// the screen costs only its 40K of chip RAM.
#ifndef JL_UI_H
#define JL_UI_H

#include <exec/types.h>
#include "view.h"

#define UI_PEN_BG 0
#define UI_PEN_WHITE 1
#define UI_PEN_ITEM 2
#define UI_PEN_ORANGE 3

BOOL uiOpen(const char *version);
void uiClose(void);
struct Window *uiWindow(void);

void uiDrawPage(const tJlView *v);                    // whole list area
void uiMoveCursor(const tJlView *v, UWORD oldIndex);  // same page: 2 rows
void uiShowCursor(const tJlView *v, BOOL isVisible);  // blinking
void uiInfo(const char *text, UBYTE pen);             // bottom line

#endif
