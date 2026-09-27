// The launcher session: config, inventory, the list on screen, launching.
#ifndef JL_APP_H
#define JL_APP_H

#include <exec/types.h>

#define JL_VERSION "2.0"

// Runs until the user quits. Returns a DOS return code.
LONG appRun(void);

#endif
