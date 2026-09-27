// Test-disk helper, NOT part of the launcher: under vAmiga, Kickstart 1.3
// reports VBlankFrequency=60 and graphics runs NTSC (200 rows) on a PAL
// machine, which clips a 256-line screen. Real PAL 1.3 machines report 50 Hz
// and PAL, so we set exactly those values before jl starts.
#include <exec/execbase.h>
#include <graphics/gfxbase.h>
#include <proto/exec.h>
#include <proto/graphics.h>

int main(void) {
	UWORD vpos = *(volatile UWORD *)0xDFF004;
	UWORD max = 0;
	for(ULONG i = 0; i < 40000; ++i) { // find the frame height: > 262 lines = PAL
		UWORD v = (UWORD)(((*(volatile UWORD *)0xDFF004 & 1) << 8) | (*(volatile UWORD *)0xDFF006 >> 8));
		if(v > max) max = v;
	}
	(void)vpos;
	if(max > 262 && !(GfxBase->DisplayFlags & PAL)) {
		Forbid();
		SysBase->VBlankFrequency = 50;
		GfxBase->DisplayFlags = (UWORD)((GfxBase->DisplayFlags & ~NTSC) | PAL);
		GfxBase->NormalDisplayRows = 256;
		GfxBase->MaxDisplayRow = 311;
		Permit();
	}
	return 0;
}
