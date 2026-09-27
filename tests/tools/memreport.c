// Benchmark helper, NOT part of the launcher: installed as C:jst on the
// benchmark disks, so both launcher versions "start a game" through it.
// Prints free memory and a 50 Hz timestamp over the AGK host channel.
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include "../../src/dbg.h"

int main(void) {
	struct DateStamp ds;
	DateStamp(&ds);
	dbgKv("chip", (LONG)AvailMem(MEMF_CHIP));
	dbgKv("fast", (LONG)AvailMem(MEMF_FAST));
	dbgKv("largest", (LONG)AvailMem(MEMF_LARGEST));
	dbgKv("t", ds.ds_Minute * 3000 + ds.ds_Tick);
	dbgKs("mem", "report");
	dbgEnd();
	return 0;
}
