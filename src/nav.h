// Cursor navigation over a two-column list, as in v0.x:
// 29 rows per column, 2 columns per page; up/down move one item (wrapping to
// the neighbour column), left/right move one column. Portable C (host tested).
#ifndef JL_NAV_H
#define JL_NAV_H

#include <stdint.h>

#define JL_ROWS 29
#define JL_COLS 2
#define JL_PAGE (JL_ROWS * JL_COLS)

typedef enum { JL_NAV_NONE, JL_NAV_MOVED, JL_NAV_PAGE, JL_NAV_BUMP } tJlNavResult;
typedef enum { JL_DIR_UP, JL_DIR_DOWN, JL_DIR_LEFT, JL_DIR_RIGHT } tJlDir;

typedef struct {
	uint16_t count;  // items in the list
	uint16_t index;  // selected item
} tJlNav;

static inline uint16_t jlNavPageStart(uint16_t index) {
	return (uint16_t)(index / JL_PAGE * JL_PAGE);
}

// Slot of an item on its page: column 0..1, row 0..28.
static inline uint8_t jlNavCol(uint16_t index) {
	return (uint8_t)((index - jlNavPageStart(index)) / JL_ROWS);
}
static inline uint8_t jlNavRow(uint16_t index) {
	return (uint8_t)((index - jlNavPageStart(index)) % JL_ROWS);
}

void jlNavInit(tJlNav *nav, uint16_t count, uint16_t index);
tJlNavResult jlNavMove(tJlNav *nav, tJlDir dir);

#endif
