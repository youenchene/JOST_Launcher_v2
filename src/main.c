// JOST Launcher v2: a TinyLauncher-style menu for WHDLoad slaves started
// with jst, for Kickstart 1.3 (A500 / CDTV). See AGENTS.md.
#include "app.h"

static const char s_szVersion[] __attribute__((used)) = "$VER: jl " JL_VERSION " (2026)";

static ULONG parseHex(const char *s) {
	ULONG v = 0;
	for(; *s; ++s) {
		char c = *s;
		v = (v << 4) | (ULONG)(c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10);
	}
	return v;
}

int main(int argc, char **argv) {
	// SPIKE C: loaded by the stub as a module
	if(argc > 1 && argv[1][0] == 'j' && argv[1][1] == 'l' && argv[1][2] == '=') {
		tJlShared *sh = (tJlShared *)parseHex(argv[1] + 3);
		if(sh->magic == JL_SHARED_MAGIC) {
			return (int)appRunModule(sh);
		}
	}
	return (int)appRun();
}
