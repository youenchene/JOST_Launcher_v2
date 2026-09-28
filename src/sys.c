#include "sys.h"
#include <exec/execbase.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>

void *jlAlloc(ULONG size, ULONG flags) {
	ULONG *p = AllocMem(size + 4, flags);
	if(!p) {
		return NULL;
	}
	*p = size + 4;
	return p + 1;
}

void jlFree(void *p) {
	if(p) {
		ULONG *base = (ULONG *)p - 1;
		FreeMem(base, *base);
	}
}

struct MsgPort *jlCreatePort(void) {
	BYTE sig = AllocSignal(-1);
	if(sig == -1) {
		return NULL;
	}
	struct MsgPort *port = AllocMem(sizeof(*port), MEMF_PUBLIC | MEMF_CLEAR);
	if(!port) {
		FreeSignal(sig);
		return NULL;
	}
	port->mp_Node.ln_Type = NT_MSGPORT;
	port->mp_Flags = PA_SIGNAL;
	port->mp_SigBit = sig;
	port->mp_SigTask = FindTask(NULL);
	port->mp_MsgList.lh_Head = (struct Node *)&port->mp_MsgList.lh_Tail;
	port->mp_MsgList.lh_TailPred = (struct Node *)&port->mp_MsgList.lh_Head;
	return port;
}

void jlDeletePort(struct MsgPort *port) {
	if(port) {
		FreeSignal(port->mp_SigBit);
		FreeMem(port, sizeof(*port));
	}
}

char *jlReadFile(const char *path, ULONG *len) {
	BPTR fh = Open((STRPTR)path, MODE_OLDFILE);
	if(!fh) {
		return NULL;
	}
	Seek(fh, 0, OFFSET_END);
	LONG size = Seek(fh, 0, OFFSET_BEGINNING);
	char *buf = size >= 0 ? jlAlloc((ULONG)size + 1, MEMF_ANY) : NULL;
	if(buf && Read(fh, buf, size) != size) {
		jlFree(buf);
		buf = NULL;
	}
	Close(fh);
	if(buf) {
		buf[size] = '\0';
		*len = (ULONG)size;
	}
	return buf;
}

UWORD jlStrLen(const char *s) {
	UWORD n = 0;
	while(s[n]) ++n;
	return n;
}

UWORD jlStrCat(char *dst, UWORD cap, const char *src) {
	UWORD n = jlStrLen(dst);
	while(*src && n + 1 < cap) dst[n++] = *src++;
	dst[n] = '\0';
	return n;
}

// Same measurement tests/tools/palfix.c uses on the emulator: sample the
// vertical beam position for a while and find its max. A PAL frame is 312-313
// lines, NTSC 262-263, so seeing more than 262 proves the hardware is PAL
// even if GfxBase disagrees.
void jlForcePal(void) {
	if(GfxBase->DisplayFlags & PAL) {
		return;
	}
	UWORD max = 0;
	for(ULONG i = 0; i < 40000; ++i) {
		UWORD v = (UWORD)(((*(volatile UWORD *)0xDFF004 & 1) << 8) | (*(volatile UWORD *)0xDFF006 >> 8));
		if(v > max) max = v;
	}
	if(max > 262) {
		Forbid();
		SysBase->VBlankFrequency = 50;
		GfxBase->DisplayFlags = (UWORD)((GfxBase->DisplayFlags & ~NTSC) | PAL);
		GfxBase->NormalDisplayRows = 256;
		GfxBase->MaxDisplayRow = 311;
		Permit();
	}
}
