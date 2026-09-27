#include "store.h"
#include "dbg.h"
#include "sys.h"
#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define OUT_BUF 4096

static UWORD s_pEmptyStart[1];

static LONG ticksNow(void) {
	struct DateStamp ds;
	DateStamp(&ds);
	return ds.ds_Minute * 3000 + ds.ds_Tick; // 50 per second
}

void storeFree(tJlStore *s) {
	jlFree(s->buf);
	jlFree(s->work);
	s->buf = s->work = NULL;
	s->inv.count = s->inv.folderCount = 0;
	s->inv.folderStart = s_pEmptyStart; // a valid empty inventory
	s->inv.hasFolders = 1;
	s->inv.wasSorted = 1;
}

void storeLoad(tJlStore *s, const char *path) {
	storeFree(s);
	ULONG len;
	LONG t0 = ticksNow();
	char *buf = jlReadFile(path, &len);
	LONG t1 = ticksNow();
	if(!buf) {
		return;
	}
	UWORD max = jlInvCountLines(buf, len);
	LONG tc = ticksNow();
	void *work = jlAlloc(jlInvWorkSize(max), MEMF_ANY);
	if(!work) {
		jlFree(buf);
		return;
	}
	s->buf = buf;
	s->work = work;
	jlInvBuild(&s->inv, buf, len, work, max);
	dbgKv("load_bytes", (LONG)len);
	dbgKv("read_ticks", t1 - t0);
	dbgKv("count_ticks", tc - t1);
	dbgKv("build_ticks", ticksNow() - tc); // parse + sort
	dbgKv("sorted", s->inv.wasSorted);
	dbgEnd();
}

typedef struct {
	BPTR fh;
	char *buf;
	UWORD used;
	BOOL isOk;
} tOut;

static void put(tOut *o, const char *text) {
	while(*text) {
		if(o->used == OUT_BUF) {
			o->isOk &= Write(o->fh, o->buf, OUT_BUF) == OUT_BUF;
			o->used = 0;
			dbgTick(); // slow on floppy: keep the test harness's frame clock alive
		}
		o->buf[o->used++] = *text++;
	}
}

BOOL storeSaveSorted(const tJlStore *s, const char *path) {
	if(s->inv.wasSorted || !s->inv.count) {
		return TRUE;
	}
	tOut o = {0, jlAlloc(OUT_BUF, MEMF_ANY), 0, TRUE};
	o.fh = o.buf ? Open((STRPTR)path, MODE_NEWFILE) : 0;
	if(!o.fh) {
		jlFree(o.buf);
		return FALSE; // e.g. a read-only disk: fine, we just sort again next time
	}
	for(UWORD i = 0; i < s->inv.count; ++i) {
		const tJlEntry *e = &s->inv.entries[s->inv.byName[i]];
		put(&o, e->name); put(&o, ";");
		put(&o, e->path); put(&o, ";");
		put(&o, e->slave); put(&o, "\n");
	}
	if(o.used) o.isOk &= Write(o.fh, o.buf, o.used) == o.used;
	Close(o.fh);
	jlFree(o.buf);
	return o.isOk;
}
