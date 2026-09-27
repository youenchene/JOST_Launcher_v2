// What the list shows: all slaves by name, the folders, or one folder's
// slaves (with ".." first). Portable C (host unit-tested).
#ifndef JL_VIEW_H
#define JL_VIEW_H

#include <stdint.h>
#include "inventory.h"
#include "nav.h"

typedef enum { JL_VIEW_LIST, JL_VIEW_FOLDERS, JL_VIEW_FOLDER } tJlViewMode;

typedef struct {
	tJlInventory *inv;       // not const: folder views build its folder index
	tJlViewMode mode;
	uint16_t folder;     // open folder (JL_VIEW_FOLDER)
	tJlNav nav;
} tJlView;

typedef enum {
	JL_ACT_NONE,     // nothing to do (empty view)
	JL_ACT_REDRAW,   // the view changed: redraw the whole list
	JL_ACT_LAUNCH,   // *entry is the slave to start
} tJlAction;

void jlViewInit(tJlView *v, tJlInventory *inv, int folderMode);

// Re-attach to a reloaded inventory, keeping mode and selection when they
// still fit (the inventory file is the same after a game: same indices).
void jlViewRebind(tJlView *v, tJlInventory *inv);

uint16_t jlViewCount(const tJlView *v);
// Label of item i (not NUL-terminated for folders): returns chars, *len = length.
const char *jlViewLabel(const tJlView *v, uint16_t i, uint8_t *len);

// Fire on the selected item: launch it, open the folder, or go back up.
tJlAction jlViewActivate(tJlView *v, const tJlEntry **entry);
// F / remote 1: list <-> folders.
void jlViewToggleFolders(tJlView *v);

#endif
