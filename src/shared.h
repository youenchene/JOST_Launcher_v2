// SPIKE C: what the resident stub (src/stub/stub.c) and the menu module (jl,
// loaded with "jlshared=<address>") share across a game: the selection and
// what to launch. The menu is unloaded while the game runs.
#ifndef JL_SHARED_H
#define JL_SHARED_H

#include <exec/types.h>

#define JL_SHARED_MAGIC 0x4A4C5632  // "JLV2"
#define JL_SHARED_PATH 216
#define JL_SHARED_NAME 108

typedef struct {
	ULONG magic;
	UBYTE hasView, viewMode, launchFailed, launchMode;
	UWORD folder, index;
	UBYTE action;                    // 0 quit, 1 launch
	char launchPath[JL_SHARED_PATH];
	char launchSlave[JL_SHARED_NAME];
	char jstCommand[JL_SHARED_NAME];
} tJlShared;

#endif
