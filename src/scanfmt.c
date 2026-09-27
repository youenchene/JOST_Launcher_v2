#include "scanfmt.h"

static char lowerAscii(char c) {
	return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}

static uint16_t strLen(const char *s) {
	uint16_t n = 0;
	while(s[n]) ++n;
	return n;
}

static int equalsNoCase(const char *a, const char *b) {
	while(*a && lowerAscii(*a) == lowerAscii(*b)) {
		++a;
		++b;
	}
	return !*a && !*b;
}

int jlIsSlaveFile(const char *name) {
	uint16_t n = strLen(name);
	return n > 6 && equalsNoCase(name + n - 6, ".slave");
}

int jlIsSkippedDir(const char *name) {
	return equalsNoCase(name, "data");
}

typedef struct {
	char *out;
	uint16_t len, cap;
} tOut;

static void put(tOut *o, const char *s, uint16_t n) {
	for(uint16_t i = 0; i < n && o->len < o->cap; ++i) o->out[o->len++] = s[i];
}

uint16_t jlFormatLine(char *out, uint16_t cap, const char *dirName, const char *dirPath,
	const char *file, uint16_t nthInDir) {
	tOut o = {out, 0, cap};
	uint16_t fileLen = strLen(file);
	if(!dirName[0]) {
		put(&o, file, (uint16_t)(fileLen - 6)); // "Foo.slave" -> "Foo"
	}
	else {
		put(&o, dirName, strLen(dirName));
		if(nthInDir > 0) {
			put(&o, " (", 2);
			put(&o, file, fileLen);
			put(&o, ")", 1);
		}
	}
	put(&o, ";", 1);
	put(&o, dirPath, strLen(dirPath));
	put(&o, ";", 1);
	put(&o, file, fileLen);
	put(&o, "\n", 1);
	return o.len < o.cap ? o.len : 0; // full buffer = maybe truncated
}
