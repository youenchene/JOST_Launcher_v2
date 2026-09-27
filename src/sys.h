// Small AmigaOS 1.3 helpers: tracked allocations (AllocVec is V36+) and a
// message port (CreateMsgPort is V36+).
#ifndef JL_SYS_H
#define JL_SYS_H

#include <exec/types.h>
#include <exec/ports.h>

void *jlAlloc(ULONG size, ULONG flags);   // remembers its size; NULL on failure
void jlFree(void *p);                      // NULL is fine

struct MsgPort *jlCreatePort(void);
void jlDeletePort(struct MsgPort *port);

// Reads a whole file into a jlAlloc'd buffer with one spare byte (NUL).
// Returns NULL if the file can't be read; *len gets its size.
char *jlReadFile(const char *path, ULONG *len);

UWORD jlStrLen(const char *s);
// Appends src to dst (capacity cap incl. NUL). Returns the new length.
UWORD jlStrCat(char *dst, UWORD cap, const char *src);

#endif
