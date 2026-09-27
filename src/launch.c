#include "launch.h"
#include "dbg.h"
#include "sys.h"
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define SCRIPT_MAX 600
#define JST_STACK 16384

// ------------------------------------------------------------ helpers

static UWORD addQuoted(char *dst, UWORD cap, const char *prefix, const char *arg) {
	if(prefix) {
		jlStrCat(dst, cap, prefix);
		jlStrCat(dst, cap, " ");
	}
	jlStrCat(dst, cap, "\"");
	jlStrCat(dst, cap, arg);
	return jlStrCat(dst, cap, "\"\n");
}

static BPTR consoleOut(BPTR *nil) {
	BPTR out = Output();
	*nil = 0;
	if(!out) { // started from Workbench: no console
		out = *nil = Open((STRPTR)"NIL:", MODE_NEWFILE);
	}
	return out;
}

// ------------------------------------------------------------ 0: script

static BOOL launchScript(const char *jst, const char *path, const char *slave) {
	char text[SCRIPT_MAX];
	text[0] = '\0';
	addQuoted(text, SCRIPT_MAX, "cd", path);
	UWORD len = addQuoted(text, SCRIPT_MAX, jst, slave);
	BPTR fh = Open((STRPTR)LAUNCH_SCRIPT, MODE_NEWFILE);
	if(!fh) {
		return FALSE;
	}
	BOOL isOk = Write(fh, text, len) == len;
	Close(fh);
	BPTR nil, out = consoleOut(&nil);
	isOk = isOk && Execute((STRPTR)"execute " LAUNCH_SCRIPT, 0, out) != 0;
	if(nil) Close(nil);
	return isOk;
}

// ------------------------------------------------------------ 1: execute

static BOOL launchExecute(const char *jst, const char *slave) {
	char cmd[SCRIPT_MAX];
	cmd[0] = '\0';
	UWORD len = addQuoted(cmd, SCRIPT_MAX, jst, slave);
	cmd[len - 1] = '\0'; // Execute takes one line, no newline
	BPTR nil, out = consoleOut(&nil);
	BOOL isOk = Execute((STRPTR)cmd, 0, out) != 0;
	if(nil) Close(nil);
	return isOk;
}

// ------------------------------------------------------------ 2: loadseg

// Calls a loaded command like the 1.3 shell does: d0/a0 = argument line
// (ending in '\n'), on a fresh stack (the shell's default is 4K; ours is
// what's left of jl's). RunCommand() would do this, but it's V36+.
LONG jlCallSeg(APTR entry, const char *args, LONG len, APTR stackTop);
__asm__(
	"	.text\n"
	"	.even\n"
	"	.globl	_jlCallSeg\n"
	"_jlCallSeg:\n"
	"	movem.l	d2-d7/a2-a6,-(sp)\n"
	"	move.l	48(sp),a1\n"      // entry  (4 return + 44 saved)
	"	move.l	52(sp),a0\n"      // args
	"	move.l	56(sp),d0\n"      // len
	"	move.l	60(sp),a2\n"      // stack top
	"	move.l	sp,-(a2)\n"       // our sp, on the new stack
	"	move.l	a2,sp\n"
	"	jsr	(a1)\n"
	"	move.l	(sp),sp\n"
	"	movem.l	(sp)+,d2-d7/a2-a6\n"
	"	rts\n"
);

static BPTR s_jstSeg;

static BPTR loadJst(const char *jst) {
	if(s_jstSeg) {
		return s_jstSeg;
	}
	char name[120];
	name[0] = '\0';
	BOOL hasPath = FALSE;
	for(const char *c = jst; *c; ++c) hasPath |= (*c == ':' || *c == '/');
	if(!hasPath) jlStrCat(name, sizeof(name), "C:");
	jlStrCat(name, sizeof(name), jst);
	s_jstSeg = LoadSeg((STRPTR)name);
	return s_jstSeg;
}

static BOOL launchSeg(const char *jst, const char *slave) {
	BPTR seg = loadJst(jst);
	UBYTE *stack = seg ? AllocMem(JST_STACK, MEMF_ANY) : NULL;
	if(!stack) {
		return FALSE;
	}
	char args[SCRIPT_MAX];
	args[0] = '\0';
	UWORD len = addQuoted(args, SCRIPT_MAX, NULL, slave);
	struct Process *me = (struct Process *)FindTask(NULL);
	struct CommandLineInterface *cli = BADDR(me->pr_CLI);
	BSTR oldName = 0;
	static ULONG s_bname[16];         // BSTR: longword aligned
	UBYTE *bname = (UBYTE *)s_bname;
	if(cli) { // some commands read their name from the CLI
		UBYTE n = 0;
		while(jst[n] && n < 62) { bname[n + 1] = (UBYTE)jst[n]; ++n; }
		bname[0] = n;
		oldName = cli->cli_CommandName;
		cli->cli_CommandName = MKBADDR(bname); // needs longword alignment
	}
	// BCPL commands (jst on 1.3: RdArgs) read the argument line from the
	// current input stream's buffer, where the shell leaves it. Fake that:
	// a NIL: handle whose buffer holds the line, as pr_CIS during the call.
	static ULONG s_argBuf[SCRIPT_MAX / 4 + 1];   // BPTR target: longword aligned
	CopyMem(args, s_argBuf, len);
	BPTR nilIn = Open((STRPTR)"NIL:", MODE_OLDFILE);
	struct FileHandle *fh = BADDR(nilIn);
	BPTR oldCis = me->pr_CIS;
	if(fh) {
		fh->fh_Buf = MKBADDR(s_argBuf);
		fh->fh_Pos = 0;
		fh->fh_End = len;
		me->pr_CIS = nilIn;
	}
	APTR entry = (APTR)(((ULONG)seg << 2) + 4);
	*(ULONG *)(stack + JST_STACK - 4) = JST_STACK; // 1.3 shell layout: size at the top
	jlCallSeg(entry, args, len, stack + JST_STACK - 4);
	me->pr_CIS = oldCis;
	if(fh) {
		fh->fh_Buf = 0; // ours: Close() must not free it
		fh->fh_Pos = fh->fh_End = 0;
		Close(nilIn);
	}
	if(cli) cli->cli_CommandName = oldName;
	FreeMem(stack, JST_STACK);
	return TRUE;
}

void launchCleanup(void) {
	if(s_jstSeg) {
		UnLoadSeg(s_jstSeg);
		s_jstSeg = 0;
	}
}

// ------------------------------------------------------------ entry

BOOL launchSlave(UBYTE mode, const char *jst, const char *path, const char *slave) {
	if(mode == 0) {
		return launchScript(jst, path, slave);
	}
	BPTR dir = Lock((STRPTR)path, ACCESS_READ);
	if(!dir) {
		return FALSE;
	}
	BPTR old = CurrentDir(dir);
	BOOL isOk = mode == 1 ? launchExecute(jst, slave) : launchSeg(jst, slave);
	CurrentDir(old);
	UnLock(dir);
	return isOk;
}
