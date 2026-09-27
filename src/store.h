// The inventory file on disk: load it (parse + sort), free it, and save it
// back in name order so the next start skips the sort.
#ifndef JL_STORE_H
#define JL_STORE_H

#include <exec/types.h>
#include "inventory.h"

typedef struct {
	tJlInventory inv;
	char *buf;    // the file, parsed in place
	void *work;
} tJlStore;

// Loads path into s. A missing/unreadable file gives an empty inventory.
void storeLoad(tJlStore *s, const char *path);
void storeFree(tJlStore *s);
// Rewrites path in name order when it wasn't (v0.x files, fresh scans).
BOOL storeSaveSorted(const tJlStore *s, const char *path);

#endif
