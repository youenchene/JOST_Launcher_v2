// C:jl, what the user runs: JOST Launcher v2 for Kickstart 1.3 (A500 /
// CDTV). A small resident loop: load the menu (C:jl-menu), unload it, run the
// chosen game through jst, load the menu again. While a game runs only this
// stub stays in memory (v0.x kept its whole program and started a new copy of
// itself after each game). See AGENTS.md.
#include "../shared.h"
#include "../launch.h"
#include "../dbg.h"
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define MENU_NAME "C:jl-menu"
#define MENU_STACK 8192

static const char s_szVersion[] __attribute__((used)) = "$VER: jl 2.1 (2026)";

// Runs one menu session. The menu's code and stack are gone when this returns.
static BOOL runMenu(tJlShared *sh) {
	static const char hex[] = "0123456789abcdef";
	char args[13] = "jl=";
	ULONG v = (ULONG)sh;
	for(int i = 0; i < 8; ++i) args[3 + i] = hex[(v >> (28 - 4 * i)) & 15];
	args[11] = '\n';
	args[12] = '\0';
	BPTR seg = LoadSeg((STRPTR)MENU_NAME);
	UBYTE *stack = seg ? AllocMem(MENU_STACK, MEMF_ANY) : NULL;
	sh->action = 0;
	if(stack) {
		*(ULONG *)(stack + MENU_STACK - 4) = MENU_STACK;
		jlCallSeg((APTR)(((ULONG)seg << 2) + 4), args, 12, stack + MENU_STACK - 4);
		FreeMem(stack, MENU_STACK);
	}
	if(seg) UnLoadSeg(seg);
	return sh->action == 1;
}

int main(void) {
	tJlShared *sh = AllocMem(sizeof(*sh), MEMF_ANY | MEMF_CLEAR);
	if(!sh) {
		return RETURN_FAIL;
	}
	sh->magic = JL_SHARED_MAGIC;
	while(runMenu(sh)) {
		BOOL isOk = launchSlave(sh->jstCommand, sh->launchPath, sh->launchSlave);
		sh->launchFailed = !isOk;
		dbgKv("returned", isOk);
		dbgEnd();
	}
	FreeMem(sh, sizeof(*sh));
	return RETURN_OK;
}
