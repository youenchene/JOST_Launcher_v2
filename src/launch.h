// Starts a slave through jst and returns when it quits. The launcher has
// already closed its screen and freed its lists, so the game gets all the
// memory but our code (v0.x stayed fully loaded and started a second copy
// of itself after every game: memory leaked and the list broke).
#ifndef JL_LAUNCH_H
#define JL_LAUNCH_H

#include <exec/types.h>

#define LAUNCH_SCRIPT "RAM:jl-launch"

// Writes LAUNCH_SCRIPT ("cd path" + "jst slave") and executes it.
BOOL launchSlave(const char *jstCommand, const char *path, const char *slave);

#endif
