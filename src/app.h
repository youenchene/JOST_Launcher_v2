// The launcher session: config, inventory, the list on screen, launching.
#ifndef JL_APP_H
#define JL_APP_H

#include <exec/types.h>

#define JL_VERSION "2.0"

// Runs until the user quits. Returns a DOS return code.
LONG appRun(void);

#include "shared.h"
// SPIKE C: one menu session as a module; fills sh (action, what to launch).
LONG appRunModule(tJlShared *sh);

#endif
