// Scans the configured directories for *.slave files and writes the
// inventory file. Plain dos.library (1.3): Lock/Examine/ExNext.
#ifndef JL_SCAN_H
#define JL_SCAN_H

#include <exec/types.h>
#include "config.h"

// progress(count, userData) is called after each directory; may be NULL.
typedef void (*tScanProgress)(UWORD count, void *userData);

// Returns the number of slaves written, or -1 if the file can't be written.
LONG scanWriteInventory(const tJlConfig *cfg, tScanProgress progress, void *userData);

#endif
