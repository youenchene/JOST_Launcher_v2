#include "dbg.h"
#include <hardware/intbits.h>
#include <hardware/custom.h>

#define HWREGS ((volatile struct Custom *)0xDFF000)
#define NOOP (*(volatile UWORD *)0xDFF1FE)
#define LINE_MAX 200

static BOOL s_isReadyPending;
static char s_line[LINE_MAX + 2];
static UWORD s_len;

void dbgPrint(const char *text) {
	ULONG addr = (ULONG)text;
	UWORD wasOn = HWREGS->intenar & INTF_INTEN;
	HWREGS->intena = INTF_INTEN; // the 3-word sequence must not be split
	NOOP = 0xA6E0;
	NOOP = (UWORD)(addr >> 16);
	NOOP = (UWORD)addr;
	if(wasOn) {
		HWREGS->intena = INTF_SETCLR | INTF_INTEN;
	}
}

void dbgTick(void) {
	if(s_isReadyPending) {
		s_isReadyPending = FALSE;
		dbgPrint("AGK ready\n");
	}
	NOOP = 0xA6E1;
}

void dbgReady(void) {
	s_isReadyPending = TRUE;
}

static void put(const char *s) {
	while(*s && s_len < LINE_MAX) s_line[s_len++] = *s++;
}

static void putNum(LONG v) {
	char buf[12];
	UBYTE n = 0;
	ULONG u = v < 0 ? (ULONG)-v : (ULONG)v;
	do {
		buf[n++] = (char)('0' + u % 10); // only on events, never per frame
		u /= 10;
	} while(u);
	if(v < 0) buf[n++] = '-';
	while(n && s_len < LINE_MAX) s_line[s_len++] = buf[--n];
}

static void startKey(const char *key) {
	put(s_len ? " " : "AGK ");
	put(key);
	put("=");
}

void dbgKv(const char *key, LONG value) {
	startKey(key);
	putNum(value);
}

void dbgKs(const char *key, const char *value) {
	startKey(key);
	put(value);
}

void dbgEnd(void) {
	s_line[s_len++] = '\n';
	s_line[s_len] = '\0';
	dbgPrint(s_line);
	s_len = 0;
}
