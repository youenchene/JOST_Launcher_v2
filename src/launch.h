// Starts a slave through jst and returns when it quits (Kickstart 1.3).
//
// jst is loaded with LoadSeg() and called directly, the way the 1.3 shell
// runs a command: no shell, no script file, no C:Execute. It's loaded for
// each launch (jst isn't reentrant: its variables would survive into the
// next game) and gets a 4K stack, like the shell's default. If LoadSeg
// fails (e.g. jst isn't in C:), Execute() through a shell is the fallback.
#ifndef JL_LAUNCH_H
#define JL_LAUNCH_H

#include <exec/types.h>

BOOL launchSlave(const char *jstCommand, const char *path, const char *slave);

// Calls a loaded command's first hunk on a new stack (stackTop: the stack's
// last longword, which holds its size, as the 1.3 shell sets it up).
// d0/a0 = argument line. Returns the command's d0.
LONG jlCallSeg(APTR entry, const char *args, LONG len, APTR stackTop);

#endif
