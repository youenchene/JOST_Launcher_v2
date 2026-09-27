// Inventory: the list of launchable slaves, parsed from the inventory file.
// Portable C (host unit-tested): no Amiga headers, no allocation of its own.
//
// File format, one slave per line (same as JOST Launcher v0.x):
//   name;path;slave
// e.g. "MonkeyIsland2Fr;Games:Games/_PointAndClick/MonkeyIsland2Fr;MonkeyIsland2Fr.slave"
// Trailing blanks (v0.x padded the slave field to 30 chars) and CRs are trimmed.
#ifndef JL_INVENTORY_H
#define JL_INVENTORY_H

#include <stdint.h>

typedef struct {
	const char *name;    // what the list shows
	const char *path;    // directory to CD into
	const char *slave;   // file given to jst
	const char *folder;  // folder-view group: parent dir of the slave's dir (not NUL-terminated)
	uint32_t key;        // first 4 chars of name, lowercased: most compares end here
	uint8_t folderLen;
	uint8_t pad[3];
} tJlEntry;

typedef struct {
	tJlEntry *entries;      // file order
	uint16_t count;
	uint16_t *byName;       // entry indices, sorted by name (list view)
	uint16_t *byFolder;     // entry indices grouped by folder, name order in each group
	uint16_t folderCount;
	uint16_t *folderStart;  // folderCount + 1 offsets into byFolder
	uint16_t *tmp;          // sort scratch
	uint8_t hasFolders;     // byFolder is built (jlInvEnsureFolders)
	uint8_t wasSorted;      // the file was already in name order
} tJlInventory;

// Upper bound of entries in a buffer (number of lines).
uint16_t jlInvCountLines(const char *buf, uint32_t len);

// Bytes jlInvBuild() needs in its work area for maxEntries entries.
uint32_t jlInvWorkSize(uint16_t maxEntries);

// Parses buf in place (delimiters become NULs) and sorts by name.
// work must be jlInvWorkSize(jlInvCountLines(buf, len)) bytes, 4-byte aligned.
// buf must have len + 1 writable bytes and stay alive as long as the inventory
// is used. Returns the entry count.
uint16_t jlInvBuild(tJlInventory *inv, char *buf, uint32_t len, void *work, uint16_t maxEntries);

// Builds the folder index on first use (only folder view needs it).
void jlInvEnsureFolders(tJlInventory *inv);

// Folder view helpers (after jlInvEnsureFolders).
static inline uint16_t jlInvFolderSize(const tJlInventory *inv, uint16_t folder) {
	return inv->folderStart[folder + 1] - inv->folderStart[folder];
}
static inline const tJlEntry *jlInvFolderEntry(const tJlInventory *inv, uint16_t folder, uint16_t i) {
	return &inv->entries[inv->byFolder[inv->folderStart[folder] + i]];
}
static inline const tJlEntry *jlInvFolderFirst(const tJlInventory *inv, uint16_t folder) {
	return jlInvFolderEntry(inv, folder, 0);
}

// Case-insensitive ASCII compare, ties broken case-sensitively (stable order).
int jlStrCmpNoCase(const char *a, const char *b);

#endif
