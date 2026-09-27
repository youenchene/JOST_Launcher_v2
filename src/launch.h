// Starts a slave through jst and returns when it quits. The launcher has
// already closed its screen and freed its lists, so the game gets all the
// memory but our code.
//
// SPIKE: three ways (config launch_mode), measured by tools/bench.py:
//   0 script:  write RAM:jl-launch ("cd" + "jst"), Execute("execute ...")
//   1 execute: CurrentDir() ourselves, Execute("jst \"slave\"")
//   2 loadseg: CurrentDir(), LoadSeg(jst) once, call it on its own stack
#ifndef JL_LAUNCH_H
#define JL_LAUNCH_H

#include <exec/types.h>

#define LAUNCH_SCRIPT "RAM:jl-launch"

BOOL launchSlave(UBYTE mode, const char *jstCommand, const char *path, const char *slave);
void launchCleanup(void);   // frees a cached jst (mode 2)

#endif
