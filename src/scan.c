#include "scan.h"
#include "scanfmt.h"
#include "sys.h"
#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define PATH_MAX 256
#define OUT_BUF 4096
#define LINE_MAX 400
#define MAX_DEPTH 12

typedef struct {
	BPTR fh;
	char *buf;
	UWORD used;
	BOOL isFailed;
	LONG count;
	char path[PATH_MAX];
	char line[LINE_MAX];
	tScanProgress progress;
	void *userData;
} tScan;

static void flush(tScan *s) {
	if(s->used && Write(s->fh, s->buf, s->used) != s->used) {
		s->isFailed = TRUE;
	}
	s->used = 0;
}

static void emit(tScan *s, const char *dirName, const char *file, UWORD nth) {
	UWORD n = jlFormatLine(s->line, LINE_MAX, dirName, s->path, file, nth);
	if(!n) return;
	if(s->used + n > OUT_BUF) flush(s);
	CopyMem(s->line, s->buf + s->used, n);
	s->used += n;
	++s->count;
}

// Appends "/name" (or "name" after a volume ':') to s->path; returns old length.
static UWORD pushPath(tScan *s, const char *name) {
	UWORD len = jlStrLen(s->path);
	if(len && s->path[len - 1] != ':' && s->path[len - 1] != '/') {
		jlStrCat(s->path, PATH_MAX, "/");
	}
	jlStrCat(s->path, PATH_MAX, name);
	return len;
}

static void scanDir(tScan *s, const char *dirName, UBYTE depth);

static void scanEntries(tScan *s, BPTR lock, struct FileInfoBlock *fib, const char *dirName, UBYTE depth) {
	UWORD nth = 0;
	while(ExNext(lock, fib)) {
		char *name = (char *)fib->fib_FileName;
		if(fib->fib_DirEntryType < 0) {
			if(jlIsSlaveFile(name)) emit(s, dirName, name, nth++);
		}
		else if(!jlIsSkippedDir(name) && depth < MAX_DEPTH) {
			UWORD old = pushPath(s, name);
			scanDir(s, name, (UBYTE)(depth + 1)); // name lives in fib: copied below
			s->path[old] = '\0';
		}
	}
}

static void scanDir(tScan *s, const char *dirName, UBYTE depth) {
	char name[108];
	UWORD i = 0;
	for(; dirName[i] && i < sizeof(name) - 1; ++i) name[i] = dirName[i];
	name[i] = '\0';
	struct FileInfoBlock *fib = AllocMem(sizeof(*fib), MEMF_PUBLIC); // longword aligned
	BPTR lock = Lock((STRPTR)s->path, ACCESS_READ);
	if(fib && lock && Examine(lock, fib) && fib->fib_DirEntryType > 0) {
		scanEntries(s, lock, fib, name, depth);
	}
	if(lock) UnLock(lock);
	if(fib) FreeMem(fib, sizeof(*fib));
	if(s->progress) s->progress((UWORD)s->count, s->userData);
}

LONG scanWriteInventory(const tJlConfig *cfg, tScanProgress progress, void *userData) {
	tScan *s = jlAlloc(sizeof(*s), MEMF_PUBLIC | MEMF_CLEAR);
	char *buf = jlAlloc(OUT_BUF, MEMF_PUBLIC);
	BPTR fh = (s && buf) ? Open((STRPTR)cfg->inventoryFile, MODE_NEWFILE) : 0;
	LONG result = -1;
	if(fh) {
		s->fh = fh;
		s->buf = buf;
		s->progress = progress;
		s->userData = userData;
		for(UBYTE d = 0; d < cfg->scanDirCount; ++d) {
			if(!cfg->scanDirs[d][0]) continue;
			s->path[0] = '\0';
			jlStrCat(s->path, PATH_MAX, cfg->scanDirs[d]);
			scanDir(s, "", 0);
		}
		flush(s);
		Close(fh);
		result = s->isFailed ? -1 : s->count;
	}
	jlFree(buf);
	jlFree(s);
	return result;
}
