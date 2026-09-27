// Input rules: which held buttons become which launcher events, and key
// repeat. Portable C (host unit-tested); src/hwinput.c fills the held mask
// from the keyboard, joysticks, mouse and the CDTV remote.
#ifndef JL_INPUT_H
#define JL_INPUT_H

#include <stdint.h>

enum {
	JL_IN_UP = 1 << 0,
	JL_IN_DOWN = 1 << 1,
	JL_IN_LEFT = 1 << 2,
	JL_IN_RIGHT = 1 << 3,
	JL_IN_FIRE = 1 << 4,    // Return, fire, A/B
	JL_IN_CANCEL = 1 << 5,  // C, 2nd button
	JL_IN_SCAN = 1 << 6,    // S, remote 0
	JL_IN_FOLDER = 1 << 7,  // F, remote 1
	JL_IN_QUIT = 1 << 8,    // Esc
};
#define JL_IN_DIRS (JL_IN_UP | JL_IN_DOWN | JL_IN_LEFT | JL_IN_RIGHT)

// Repeat delays in frames (v0.x values).
#define JL_REPEAT_MOVE 4
#define JL_REPEAT_PAGE 8

typedef struct {
	uint16_t prev;      // held mask of the previous frame
	uint8_t cooldown;   // frames until a held direction repeats
} tJlInput;

// held: what is held right now, so buttons held at start (e.g. the fire that
// quit a game) don't trigger anything until released.
void jlInputInit(tJlInput *in, uint16_t held);

// One frame. Buttons fire on press. A newly pressed direction fires at once;
// a held one repeats when the cooldown is over (up, down, left, right priority).
uint16_t jlInputStep(tJlInput *in, uint16_t held);

void jlInputCooldown(tJlInput *in, uint8_t frames);

#endif
