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

// Kickstart 1.3's PAL/NTSC autodetection can misreport NTSC even on real PAL
// hardware (seen on a real CDTV through an RGB2HDMI). Undetected, a 256-line
// screen doesn't fit an NTSC frame and gets cropped at the bottom - v0.x never
// hit this because its Blitz Basic Slice command drove the copper directly,
// bypassing OpenScreen's mode guess. Measures the real beam and only forces
// PAL if the hardware proves to be PAL despite the misreport, so a genuine
// NTSC machine is left alone. Call once before opening the screen; cheap to
// call again (returns immediately once GfxBase agrees).
void jlForcePal(void);

#endif
