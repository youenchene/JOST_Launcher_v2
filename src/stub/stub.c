// SPIKE C: the resident part. Loads C:jl as a menu module, unloads it before
// the game, launches jst (LoadSeg mode), loads the menu again afterwards.
#include "../shared.h"
#include "../launch.h"
#include "../dbg.h"
#include "../sys.h"
#include <dos/dos.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>

#define MENU_STACK 8192

LONG jlCallSeg(APTR entry, const char *args, LONG len, APTR stackTop);

static BOOL runMenu(tJlShared *sh, UBYTE *stack) {
	BPTR seg = LoadSeg((STRPTR)"C:jl");
	if(!seg) {
		return FALSE;
	}
	char args[16] = "jl=";
	static const char hex[] = "0123456789abcdef";
	ULONG v = (ULONG)sh;
	for(int i = 0; i < 8; ++i) args[3 + i] = hex[(v >> (28 - 4 * i)) & 15];
	args[11] = '\n';
	args[12] = '\0';
	sh->action = 0;
	*(ULONG *)(stack + MENU_STACK - 4) = MENU_STACK;
	jlCallSeg((APTR)(((ULONG)seg << 2) + 4), args, 12, stack + MENU_STACK - 4);
	UnLoadSeg(seg);
	return sh->action == 1;
}

int main(void) {
	tJlShared *sh = AllocMem(sizeof(*sh), MEMF_ANY | MEMF_CLEAR);
	UBYTE *stack = AllocMem(MENU_STACK, MEMF_ANY);
	if(sh && stack) {
		sh->magic = JL_SHARED_MAGIC;
		while(runMenu(sh, stack)) {
			BOOL isOk = launchSlave(2, sh->jstCommand, sh->launchPath, sh->launchSlave);
			sh->launchFailed = !isOk;
			dbgKv("returned", isOk);
			dbgEnd();
		}
		launchCleanup();
	}
	if(stack) FreeMem(stack, MENU_STACK);
	if(sh) FreeMem(sh, sizeof(*sh));
	return 0;
}
