#include "inventory.h"

typedef int (*tCmp)(const tJlInventory *inv, uint16_t a, uint16_t b);

static char lowerAscii(char c) {
	return (c >= 'A' && c <= 'Z') ? (char)(c + ('a' - 'A')) : c;
}

int jlStrCmpNoCase(const char *a, const char *b) {
	const char *pa = a, *pb = b;
	while(*pa && lowerAscii(*pa) == lowerAscii(*pb)) {
		++pa;
		++pb;
	}
	int d = (unsigned char)lowerAscii(*pa) - (unsigned char)lowerAscii(*pb);
	if(d) {
		return d;
	}
	while(*a && *a == *b) {
		++a;
		++b;
	}
	return (unsigned char)*a - (unsigned char)*b;
}

static int cmpLenNoCase(const char *a, uint8_t la, const char *b, uint8_t lb) {
	uint8_t n = la < lb ? la : lb;
	for(uint8_t i = 0; i < n; ++i) {
		int d = (unsigned char)lowerAscii(a[i]) - (unsigned char)lowerAscii(b[i]);
		if(d) {
			return d;
		}
	}
	return (int)la - (int)lb;
}

static int cmpByName(const tJlInventory *inv, uint16_t a, uint16_t b) {
	const tJlEntry *ea = &inv->entries[a], *eb = &inv->entries[b];
	if(ea->key != eb->key) {
		return ea->key < eb->key ? -1 : 1;
	}
	return jlStrCmpNoCase(ea->name, eb->name);
}

// Same order as jlStrCmpNoCase on the first 4 chars (NUL pads short names).
static uint32_t nameKey(const char *s) {
	uint32_t k = 0;
	for(uint8_t i = 0; i < 4; ++i) {
		k = (k << 8) | (unsigned char)lowerAscii(*s);
		if(*s) ++s;
	}
	return k;
}

static int cmpByFolder(const tJlInventory *inv, uint16_t a, uint16_t b) {
	const tJlEntry *ea = &inv->entries[a], *eb = &inv->entries[b];
	return cmpLenNoCase(ea->folder, ea->folderLen, eb->folder, eb->folderLen);
}

static int isSorted(const tJlInventory *inv, const uint16_t *v, uint16_t n, tCmp cmp) {
	for(uint16_t i = 1; i < n; ++i) {
		if(cmp(inv, v[i - 1], v[i]) > 0) {
			return 0;
		}
	}
	return 1;
}

// Stable bottom-up merge sort of index array v (n items) using tmp (n items).
// Returns 1 if v was already sorted.
static int mergeSort(const tJlInventory *inv, uint16_t *v, uint16_t *tmp, uint16_t n, tCmp cmp) {
	if(isSorted(inv, v, n, cmp)) {
		return 1; // inventories are saved sorted: loading stays O(n)
	}
	uint16_t *src = v, *dst = tmp;
	for(uint32_t width = 1; width < n; width <<= 1) {
		for(uint32_t lo = 0; lo < n; lo += width << 1) {
			uint32_t mid = lo + width < n ? lo + width : n;
			uint32_t hi = lo + (width << 1) < n ? lo + (width << 1) : n;
			uint32_t i = lo, j = mid, k = lo;
			while(i < mid && j < hi) {
				dst[k++] = cmp(inv, src[j], src[i]) < 0 ? src[j++] : src[i++];
			}
			while(i < mid) dst[k++] = src[i++];
			while(j < hi) dst[k++] = src[j++];
		}
		uint16_t *t = src; src = dst; dst = t;
	}
	if(src != v) {
		for(uint16_t i = 0; i < n; ++i) v[i] = src[i];
	}
	return 0;
}

static char *trimEnd(char *start, char *end) {
	while(end > start && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) {
		--end;
	}
	*end = '\0';
	return start;
}

// Returns the start of the next line.
static char *scanToEol(char *c, char *end) {
	while(c < end && *c != '\n') ++c;
	return c < end ? c + 1 : end;
}

// Scans to the next ';' or end of line. Returns the stop position.
static char *scanField(char *c, char *end) {
	while(c < end && *c != ';' && *c != '\n') ++c;
	return c;
}

// The path field, in the same pass: folder = the directory containing the
// slave's directory, as in v0.x: "Games:Games/_PAC/Loom" -> "_PAC",
// "Games:Loom" -> "Games:Loom" (no '/': the whole path).
static char *scanPath(tJlEntry *e, char *c, char *end) {
	char *start = c, *dirStart = c, *prevSlash = 0, *lastSlash = 0;
	for(; c < end; ++c) {
		char ch = *c;
		if(ch == '/') {
			prevSlash = lastSlash;
			lastSlash = c;
		}
		else if(ch == ':') {
			dirStart = c + 1;
			prevSlash = lastSlash = 0;
		}
		else if(ch == ';' || ch == '\n') {
			break;
		}
	}
	if(lastSlash) {
		e->folder = prevSlash ? prevSlash + 1 : dirStart;
		uint32_t len = (uint32_t)(lastSlash - e->folder);
		e->folderLen = (uint8_t)(len > 255 ? 255 : len);
	}
	else {
		uint32_t len = (uint32_t)(c - start);
		e->folder = start;
		e->folderLen = (uint8_t)(len > 255 ? 255 : len);
	}
	return c;
}

// One line "name;path;slave" in one pass, split in place. Returns the start
// of the next line; *ok = 0 for a malformed or empty line.
static char *parseLine(tJlEntry *e, char *line, char *end, int *ok) {
	char *c = scanField(line, end);
	*ok = 0;
	if(c >= end || *c != ';') {
		return scanToEol(c, end);
	}
	char *pathStart = c + 1;
	e->name = trimEnd(line, c);
	c = scanPath(e, pathStart, end);
	if(c >= end || *c != ';') {
		return scanToEol(c, end);
	}
	e->path = trimEnd(pathStart, c);
	char *slave = c + 1;
	c = slave;
	while(c < end && *c != '\n') ++c;
	char *next = c < end ? c + 1 : end;
	e->slave = trimEnd(slave, c);
	if(!e->name[0] || !e->slave[0]) {
		return next;
	}
	e->key = nameKey(e->name);
	*ok = 1;
	return next;
}

uint16_t jlInvCountLines(const char *buf, uint32_t len) {
	uint32_t n = 1;
	for(uint32_t i = 0; i < len; ++i) {
		n += buf[i] == '\n';
	}
	return n > 0xFFFF ? 0xFFFF : (uint16_t)n;
}

static uint32_t align4(uint32_t v) {
	return (v + 3) & ~3UL;
}

uint32_t jlInvWorkSize(uint16_t maxEntries) {
	uint32_t n = maxEntries;
	return align4(n * sizeof(tJlEntry)) + 4 * align4(n * 2 + 2);
}

void jlInvEnsureFolders(tJlInventory *inv) {
	if(inv->hasFolders) {
		return;
	}
	inv->hasFolders = 1;
	uint16_t n = inv->count;
	for(uint16_t i = 0; i < n; ++i) inv->byFolder[i] = inv->byName[i];
	mergeSort(inv, inv->byFolder, inv->tmp, n, cmpByFolder); // stable: names stay sorted per folder
	inv->folderCount = 0;
	for(uint16_t i = 0; i < n; ++i) {
		if(i == 0 || cmpByFolder(inv, inv->byFolder[i - 1], inv->byFolder[i]) != 0) {
			inv->folderStart[inv->folderCount++] = i;
		}
	}
	inv->folderStart[inv->folderCount] = n;
}

uint16_t jlInvBuild(tJlInventory *inv, char *buf, uint32_t len, void *work, uint16_t maxEntries) {
	uint8_t *w = work;
	uint32_t listBytes = align4((uint32_t)maxEntries * 2 + 2);
	inv->entries = (tJlEntry *)w;
	w += align4((uint32_t)maxEntries * sizeof(tJlEntry));
	inv->byName = (uint16_t *)w; w += listBytes;
	inv->byFolder = (uint16_t *)w; w += listBytes;
	inv->tmp = (uint16_t *)w; w += listBytes;
	inv->folderStart = (uint16_t *)w;
	inv->folderStart[0] = 0;
	inv->folderCount = 0;
	inv->hasFolders = 0;

	uint16_t n = 0;
	char *line = buf, *end = buf + len;
	while(line < end && n < maxEntries) {
		int ok;
		line = parseLine(&inv->entries[n], line, end, &ok);
		if(ok) {
			inv->byName[n] = n;
			++n;
		}
	}
	inv->count = n;
	inv->wasSorted = (uint8_t)mergeSort(inv, inv->byName, inv->tmp, n, cmpByName);
	return n;
}
