// Test-harness channel (Amiga Game Kit "host" protocol), OS-friendly version.
// Strings go to AGK's patched vAmiga via three writes to the NOOP register
// ($DFF1FE): free on real hardware, where those writes do nothing. We don't
// link the kit's runtime because it depends on ACE; this launcher doesn't.
#ifndef JL_DBG_H
#define JL_DBG_H

#include <exec/types.h>

void dbgPrint(const char *text);   // "AGK ..." lines end with '\n'
void dbgTick(void);                // start of a launcher frame (harness tick sync)
void dbgReady(void);               // "AGK ready" at the next tick
// "AGK <key>=<value> ..." helpers: dbgKv() appends, dbgEnd() sends the line.
void dbgKv(const char *key, LONG value);
void dbgKs(const char *key, const char *value);
void dbgEnd(void);

#endif
