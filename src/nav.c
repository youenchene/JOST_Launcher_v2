#include "nav.h"

void jlNavInit(tJlNav *nav, uint16_t count, uint16_t index) {
	nav->count = count;
	nav->index = (count && index < count) ? index : 0;
}

static uint16_t target(const tJlNav *nav, tJlDir dir, int *ok) {
	uint16_t i = nav->index, last = nav->count ? nav->count - 1 : 0;
	uint16_t colStart = (uint16_t)(i - i % JL_ROWS);
	*ok = 1;
	switch(dir) {
		case JL_DIR_UP:
			if(i > 0) return i - 1;
			break;
		case JL_DIR_DOWN:
			if(i < last) return i + 1;
			break;
		case JL_DIR_LEFT:
			if(i >= JL_ROWS) return i - JL_ROWS;
			break;
		case JL_DIR_RIGHT:
			if((uint32_t)i + JL_ROWS <= last) return i + JL_ROWS;
			if((uint32_t)colStart + JL_ROWS <= last) return last; // short last column
			break;
	}
	*ok = 0;
	return i;
}

tJlNavResult jlNavMove(tJlNav *nav, tJlDir dir) {
	if(!nav->count) {
		return JL_NAV_BUMP;
	}
	int ok;
	uint16_t next = target(nav, dir, &ok);
	if(!ok) {
		return JL_NAV_BUMP;
	}
	uint16_t oldPage = jlNavPageStart(nav->index);
	nav->index = next;
	return jlNavPageStart(next) == oldPage ? JL_NAV_MOVED : JL_NAV_PAGE;
}
