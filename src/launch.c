#include "launch.h"
#include "sys.h"
#include <dos/dos.h>
#include <proto/dos.h>

#define SCRIPT_MAX 600

static UWORD addQuoted(char *dst, UWORD cap, const char *prefix, const char *arg) {
	jlStrCat(dst, cap, prefix);
	jlStrCat(dst, cap, " \"");
	jlStrCat(dst, cap, arg);
	return jlStrCat(dst, cap, "\"\n");
}

static BOOL writeScript(const char *jstCommand, const char *path, const char *slave) {
	char text[SCRIPT_MAX];
	text[0] = '\0';
	addQuoted(text, SCRIPT_MAX, "cd", path);
	UWORD len = addQuoted(text, SCRIPT_MAX, jstCommand, slave);
	BPTR fh = Open((STRPTR)LAUNCH_SCRIPT, MODE_NEWFILE);
	if(!fh) {
		return FALSE;
	}
	BOOL isOk = Write(fh, text, len) == len;
	Close(fh);
	return isOk;
}

BOOL launchSlave(const char *jstCommand, const char *path, const char *slave) {
	if(!writeScript(jstCommand, path, slave)) {
		return FALSE;
	}
	BPTR out = Output();
	BPTR nil = 0;
	if(!out) { // started from Workbench: no console
		out = nil = Open((STRPTR)"NIL:", MODE_NEWFILE);
	}
	BOOL isOk = Execute((STRPTR)"execute " LAUNCH_SCRIPT, 0, out) != 0;
	if(nil) Close(nil);
	return isOk;
}
