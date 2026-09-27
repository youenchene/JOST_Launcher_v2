#include "input.h"

void jlInputInit(tJlInput *in, uint16_t held) {
	in->prev = held;
	in->cooldown = 0;
}

static uint16_t firstDir(uint16_t held) {
	static const uint16_t order[] = {JL_IN_UP, JL_IN_DOWN, JL_IN_LEFT, JL_IN_RIGHT};
	for(uint8_t i = 0; i < 4; ++i) {
		if(held & order[i]) return order[i];
	}
	return 0;
}

uint16_t jlInputStep(tJlInput *in, uint16_t held) {
	uint16_t events = (uint16_t)(held & ~in->prev & ~JL_IN_DIRS);
	uint16_t dirs = held & JL_IN_DIRS;
	if(!dirs) {
		in->cooldown = 0; // a fresh press acts at once
	}
	else if(dirs & ~in->prev) {
		events |= firstDir(dirs & ~in->prev); // a new direction acts at once
	}
	else if(in->cooldown) {
		--in->cooldown;
	}
	else {
		events |= firstDir(dirs);
	}
	in->prev = held;
	return events;
}

void jlInputCooldown(tJlInput *in, uint8_t frames) {
	in->cooldown = frames;
}
