// The launcher session: config, inventory, the list on screen, launching.
#ifndef JL_APP_H
#define JL_APP_H

#include <exec/types.h>

#define JL_VERSION "2.1"

// Standalone (jl-menu run by hand): runs until the user quits, launching
// games itself (the menu stays loaded). Returns a DOS return code.
LONG appRun(void);

#include "shared.h"
// As the stub's module: one menu session; fills sh (action, what to launch).
LONG appRunModule(tJlShared *sh);

#endif
