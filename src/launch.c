#include "launch.h"
#include "sys.h"
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define ARGS_MAX 400
#define JST_STACK 4096   // the 1.3 shell's default stack

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

// "\"slave\"\n" (the line a command gets); returns its length.
static UWORD argLine(char *dst, const char *slave) {
	dst[0] = '\0';
	jlStrCat(dst, ARGS_MAX, "\"");
	jlStrCat(dst, ARGS_MAX, slave);
	return jlStrCat(dst, ARGS_MAX, "\"\n");
}

static BPTR loadCommand(const char *cmd) {
	char name[120];
	BOOL hasPath = FALSE;
	for(const char *c = cmd; *c; ++c) hasPath |= (*c == ':' || *c == '/');
	name[0] = '\0';
	if(!hasPath) jlStrCat(name, sizeof(name), "C:");
	jlStrCat(name, sizeof(name), cmd);
	return LoadSeg((STRPTR)name);
}

// Sets the process up like the 1.3 shell does for a command, then calls it.
// BCPL-style commands (jst on 1.3: RdArgs) read the argument line from the
// current input stream's buffer, where the shell leaves it: we fake that with
// a NIL: handle whose buffer holds the line, as pr_CIS during the call.
static BOOL callCommand(BPTR seg, const char *cmd, const char *args, UWORD len) {
	static ULONG s_argBuf[ARGS_MAX / 4 + 1];   // BPTR targets: longword aligned
	static ULONG s_name[16];
	UBYTE *stack = AllocMem(JST_STACK, MEMF_ANY);
	BPTR nilIn = stack ? Open((STRPTR)"NIL:", MODE_OLDFILE) : 0;
	if(!nilIn) {
		if(stack) FreeMem(stack, JST_STACK);
		return FALSE;
	}
	struct Process *me = (struct Process *)FindTask(NULL);
	struct CommandLineInterface *cli = BADDR(me->pr_CLI);
	struct FileHandle *fh = BADDR(nilIn);
	CopyMem((APTR)args, s_argBuf, len);
	fh->fh_Buf = MKBADDR(s_argBuf);
	fh->fh_Pos = 0;
	fh->fh_End = len;
	BPTR oldCis = me->pr_CIS;
	me->pr_CIS = nilIn;
	BSTR oldName = 0;
	if(cli) { // some commands read their name from the CLI
		UBYTE *b = (UBYTE *)s_name, n = 0;
		while(cmd[n] && n < 62) { b[n + 1] = (UBYTE)cmd[n]; ++n; }
		b[0] = n;
		oldName = cli->cli_CommandName;
		cli->cli_CommandName = MKBADDR(s_name);
	}
	*(ULONG *)(stack + JST_STACK - 4) = JST_STACK;
	jlCallSeg((APTR)(((ULONG)seg << 2) + 4), args, len, stack + JST_STACK - 4);
	if(cli) cli->cli_CommandName = oldName;
	me->pr_CIS = oldCis;
	fh->fh_Buf = 0; // ours: Close() must not free it
	fh->fh_Pos = fh->fh_End = 0;
	Close(nilIn);
	FreeMem(stack, JST_STACK);
	return TRUE;
}

static BOOL executeCommand(const char *cmd, const char *args) {
	char line[ARGS_MAX + 120];
	line[0] = '\0';
	jlStrCat(line, sizeof(line), cmd);
	jlStrCat(line, sizeof(line), " ");
	UWORD n = jlStrCat(line, sizeof(line), args);
	if(n && line[n - 1] == '\n') line[n - 1] = '\0'; // Execute takes one line
	BPTR out = Output(), nil = 0;
	if(!out) { // started from Workbench: no console
		out = nil = Open((STRPTR)"NIL:", MODE_NEWFILE);
	}
	BOOL isOk = Execute((STRPTR)line, 0, out) != 0;
	if(nil) Close(nil);
	return isOk;
}

BOOL launchSlave(const char *cmd, const char *path, const char *slave) {
	BPTR dir = Lock((STRPTR)path, ACCESS_READ);
	if(!dir) {
		return FALSE;
	}
	char args[ARGS_MAX];
	UWORD len = argLine(args, slave);
	BPTR old = CurrentDir(dir);
	BPTR seg = loadCommand(cmd);
	BOOL isOk;
	if(seg) {
		isOk = callCommand(seg, cmd, args, len);
		UnLoadSeg(seg);
	}
	else {
		isOk = executeCommand(cmd, args); // not in C:, or a shell alias/script
	}
	CurrentDir(old);
	UnLock(dir);
	return isOk;
}
