#include "hwinput.h"
#include "input.h"
#include <hardware/cia.h>
#include <hardware/custom.h>
#include <proto/exec.h>

#define HWREGS ((volatile struct Custom *)0xDFF000)
#define CIAA_PRA (*(volatile UBYTE *)0xBFE001)
#define MOUSE_TRIGGER 6   // v0.x: the CDTV remote moves the mouse 6-8 counts a frame

typedef struct { UBYTE code; UWORD bit; } tKeyMap;

// Raw key codes, as in v0.x
static const tKeyMap s_pKeys[] = {
	{0x4C, JL_IN_UP}, {0x4D, JL_IN_DOWN}, {0x4F, JL_IN_LEFT}, {0x4E, JL_IN_RIGHT},
	{0x44, JL_IN_FIRE}, {0x43, JL_IN_FIRE},        // Return, keypad Enter
	{0x33, JL_IN_CANCEL},                          // C
	{0x21, JL_IN_SCAN}, {0x0F, JL_IN_SCAN},        // S, keypad/remote 0
	{0x23, JL_IN_FOLDER}, {0x1D, JL_IN_FOLDER},    // F, keypad/remote 1
	{0x45, JL_IN_QUIT},                            // Esc
};

static UWORD s_uwKeys;
static UWORD s_uwMouse;

static UWORD keyBit(UBYTE code) {
	for(UBYTE i = 0; i < sizeof(s_pKeys) / sizeof(s_pKeys[0]); ++i) {
		if(s_pKeys[i].code == code) return s_pKeys[i].bit;
	}
	return 0;
}

static void readKeys(struct Window *win) {
	struct IntuiMessage *msg;
	while((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) {
		if(msg->Class == RAWKEY) {
			UWORD bit = keyBit((UBYTE)(msg->Code & 0x7F));
			s_uwKeys = (msg->Code & IECODE_UP_PREFIX) ? (UWORD)(s_uwKeys & ~bit) : (UWORD)(s_uwKeys | bit);
		}
		ReplyMsg((struct Message *)msg);
	}
}

static UWORD readJoystick(void) {
	UWORD d = HWREGS->joy1dat;
	UWORD held = 0;
	if(d & 0x0002) held |= JL_IN_RIGHT;
	if(d & 0x0200) held |= JL_IN_LEFT;
	if(((d >> 1) ^ d) & 0x0001) held |= JL_IN_DOWN;
	if(((d >> 1) ^ d) & 0x0100) held |= JL_IN_UP;
	return held;
}

static UWORD readButtons(void) {
	UWORD held = 0;
	UWORD pot = HWREGS->potinp;
	if(!(CIAA_PRA & CIAF_GAMEPORT1)) held |= JL_IN_FIRE;  // port 2 fire
	if(!(CIAA_PRA & CIAF_GAMEPORT0)) held |= JL_IN_FIRE;  // left mouse / remote A
	if(!(pot & 0x4000)) held |= JL_IN_CANCEL;             // port 2 button 2
	if(!(pot & 0x0400)) held |= JL_IN_CANCEL;             // right mouse / remote B
	return held;
}

static UWORD readMouse(void) {
	UWORD now = HWREGS->joy0dat;
	BYTE dx = (BYTE)((now & 0xFF) - (s_uwMouse & 0xFF));
	BYTE dy = (BYTE)((now >> 8) - (s_uwMouse >> 8));
	s_uwMouse = now;
	UWORD held = 0;
	if(dy <= -MOUSE_TRIGGER) held |= JL_IN_UP;
	if(dy >= MOUSE_TRIGGER) held |= JL_IN_DOWN;
	if(dx <= -MOUSE_TRIGGER) held |= JL_IN_LEFT;
	if(dx >= MOUSE_TRIGGER) held |= JL_IN_RIGHT;
	return held;
}

void hwInputReset(struct Window *win) {
	s_uwKeys = 0;
	s_uwMouse = HWREGS->joy0dat;
	if(win) readKeys(win);
	s_uwKeys = 0;
}

UWORD hwInputRead(struct Window *win) {
	readKeys(win);
	return (UWORD)(s_uwKeys | readJoystick() | readButtons() | readMouse());
}
