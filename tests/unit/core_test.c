// Host tests for the portable core. Run with: agk unit
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "input.h"
#include "view.h"
#include "inventory.h"
#include "nav.h"
#include "scanfmt.h"

static int s_failures;

#define CHECK(cond) do { \
	if(!(cond)) { printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); ++s_failures; } \
} while(0)

typedef struct {
	tJlInventory inv;
	char *buf;
	void *work;
} tLoaded;

static tLoaded load(const char *text) {
	tLoaded l;
	uint32_t len = (uint32_t)strlen(text);
	l.buf = malloc(len + 1);
	memcpy(l.buf, text, len + 1);
	uint16_t max = jlInvCountLines(l.buf, len);
	l.work = malloc(jlInvWorkSize(max));
	jlInvBuild(&l.inv, l.buf, len, l.work, max);
	jlInvEnsureFolders(&l.inv);
	return l;
}

static void unload(tLoaded *l) {
	free(l->buf);
	free(l->work);
}

static const char *nameAt(const tJlInventory *inv, uint16_t i) {
	return inv->entries[inv->byName[i]].name;
}

static int folderIs(const tJlInventory *inv, uint16_t f, const char *want) {
	const tJlEntry *e = jlInvFolderFirst(inv, f);
	return e->folderLen == strlen(want) && !memcmp(e->folder, want, e->folderLen);
}

static void testParsesAndTrimsV0Padding(void) {
	tLoaded l = load("Loom;Games:G/_PAC/Loom;Loom.slave        \r\n");
	CHECK(l.inv.count == 1);
	CHECK(!strcmp(l.inv.entries[0].name, "Loom"));
	CHECK(!strcmp(l.inv.entries[0].path, "Games:G/_PAC/Loom"));
	CHECK(!strcmp(l.inv.entries[0].slave, "Loom.slave"));
	unload(&l);
}

static void testSkipsMalformedLines(void) {
	tLoaded l = load("\nbad line\nA;p\n;p;s\nOk;Games:x/y;Ok.slave");
	CHECK(l.inv.count == 1);
	CHECK(!strcmp(nameAt(&l.inv, 0), "Ok"));
	unload(&l);
}

static void testSortsCaseInsensitively(void) {
	tLoaded l = load("zak;a:b/c;z.slave\nAigle;a:b/c;a.slave\nbeneath;a:b/c;b.slave\n"
		"ADIJunior;a:b/c;d.slave\n");
	CHECK(l.inv.count == 4);
	CHECK(!strcmp(nameAt(&l.inv, 0), "ADIJunior"));
	CHECK(!strcmp(nameAt(&l.inv, 1), "Aigle"));
	CHECK(!strcmp(nameAt(&l.inv, 2), "beneath"));
	CHECK(!strcmp(nameAt(&l.inv, 3), "zak"));
	unload(&l);
}

// Regression: v0.x lost or misordered games (insertion into a fixed-size
// Blitz list, 255 max). Every entry must survive, in order, at any size.
static void testLargeInventoryStaysCompleteAndSorted(void) {
	enum { N = 3000 };
	char *text = malloc(N * 48);
	char *p = text;
	for(int i = 0; i < N; ++i) {
		int k = (i * 7919) % N; // scrambled order
		p += sprintf(p, "Game%05d;Games:Games/F%02d/Game%05d;G.slave\n", k, k % 37, k);
	}
	tLoaded l = load(text);
	CHECK(l.inv.count == N);
	int sorted = 1;
	for(uint16_t i = 1; i < l.inv.count; ++i) {
		sorted &= jlStrCmpNoCase(nameAt(&l.inv, i - 1), nameAt(&l.inv, i)) < 0;
	}
	CHECK(sorted);
	CHECK(l.inv.folderCount == 37);
	uint32_t total = 0;
	for(uint16_t f = 0; f < l.inv.folderCount; ++f) total += jlInvFolderSize(&l.inv, f);
	CHECK(total == N);
	unload(&l);
	free(text);
}

static void testFolderIsParentOfGameDir(void) {
	tLoaded l = load(
		"Loom;Games:Games/_PAC/Loom;Loom.slave\n"
		"Mickey;Games:Games/_PAC/Maupiti/Mickey;M.slave\n"
		"Defender;Games:Games/_Cinemaware/Defender;D.slave\n"
		"Top;Games:Top;T.slave\n"
		"Alone;Games:Games/_pac/Alone;A.slave\n");
	CHECK(l.inv.folderCount == 4);
	CHECK(folderIs(&l.inv, 0, "_Cinemaware"));
	CHECK(folderIs(&l.inv, 1, "_pac") || folderIs(&l.inv, 1, "_PAC")); // one folder, any case
	CHECK(folderIs(&l.inv, 2, "Games:Top"));
	CHECK(folderIs(&l.inv, 3, "Maupiti"));
	CHECK(jlInvFolderSize(&l.inv, 1) == 2);
	CHECK(!strcmp(jlInvFolderEntry(&l.inv, 1, 0)->name, "Alone"));
	CHECK(!strcmp(jlInvFolderEntry(&l.inv, 1, 1)->name, "Loom"));
	unload(&l);
}

static void testKeyOrderMatchesFullCompare(void) {
	// Names that differ after 4 chars, share a prefix, differ in case or length
	tLoaded l = load("abcdz;p/q/r;s.slave\nABCDa;p/q/r;s.slave\nab;p/q/r;s.slave\n"
		"abc;p/q/r;s.slave\nAb;p/q/r;s.slave\n_x;p/q/r;s.slave\n9;p/q/r;s.slave\n");
	for(uint16_t i = 1; i < l.inv.count; ++i) {
		CHECK(jlStrCmpNoCase(nameAt(&l.inv, i - 1), nameAt(&l.inv, i)) < 0);
	}
	CHECK(!l.inv.wasSorted);
	unload(&l);
	tLoaded s = load("a;p/q/r;s.slave\nB;p/q/r;s.slave\nc;p/q/r;s.slave\n");
	CHECK(s.inv.wasSorted);
	unload(&s);
}

static void testEmptyInventory(void) {
	tLoaded l = load("");
	CHECK(l.inv.count == 0);
	CHECK(l.inv.folderCount == 0);
	unload(&l);
}

static void testConfig(void) {
	tJlConfig c;
	jlConfigDefaults(&c);
	CHECK(!strcmp(c.inventoryFile, "S:jl-inventory.data"));
	const char *t = "scan_dir_1=Games:Games/\r\ninventory_file=Games:inv.data\n"
		"folder_mode_by_default=TRUE\nscan_dir_2=Demos:\nbogus=1\njst_command=whdload\n";
	jlConfigParse(&c, t, (uint32_t)strlen(t));
	CHECK(c.scanDirCount == 2);
	CHECK(!strcmp(c.scanDirs[0], "Games:Games"));
	CHECK(!strcmp(c.scanDirs[1], "Demos:"));
	CHECK(!strcmp(c.inventoryFile, "Games:inv.data"));
	CHECK(!strcmp(c.jstCommand, "whdload"));
	CHECK(c.folderModeByDefault == 1);
}

static void testNavigation(void) {
	tJlNav n;
	jlNavInit(&n, 70, 0);
	CHECK(jlNavMove(&n, JL_DIR_UP) == JL_NAV_BUMP);
	CHECK(jlNavMove(&n, JL_DIR_LEFT) == JL_NAV_BUMP);
	CHECK(jlNavMove(&n, JL_DIR_DOWN) == JL_NAV_MOVED && n.index == 1);
	CHECK(jlNavMove(&n, JL_DIR_RIGHT) == JL_NAV_MOVED && n.index == 30);
	CHECK(jlNavCol(30) == 1 && jlNavRow(30) == 1);
	CHECK(jlNavMove(&n, JL_DIR_RIGHT) == JL_NAV_PAGE && n.index == 59);
	CHECK(jlNavCol(59) == 0 && jlNavRow(59) == 1);
	CHECK(jlNavMove(&n, JL_DIR_RIGHT) == JL_NAV_BUMP); // 88 > 69 and no 3rd column
	CHECK(jlNavMove(&n, JL_DIR_LEFT) == JL_NAV_PAGE && n.index == 30);
	jlNavInit(&n, 70, 57);
	CHECK(jlNavMove(&n, JL_DIR_DOWN) == JL_NAV_PAGE && n.index == 58);
	CHECK(jlNavMove(&n, JL_DIR_UP) == JL_NAV_PAGE && n.index == 57);
	jlNavInit(&n, 40, 5);
	CHECK(jlNavMove(&n, JL_DIR_RIGHT) == JL_NAV_MOVED && n.index == 34);
	jlNavInit(&n, 31, 5);  // short last column: jump to the last item
	CHECK(jlNavMove(&n, JL_DIR_RIGHT) == JL_NAV_MOVED && n.index == 30);
	jlNavInit(&n, 0, 0);
	CHECK(jlNavMove(&n, JL_DIR_DOWN) == JL_NAV_BUMP);
	jlNavInit(&n, 10, 99);
	CHECK(n.index == 0);
}

static void testScanFormat(void) {
	char b[128];
	CHECK(jlIsSlaveFile("Loom.Slave") && jlIsSlaveFile("x.SLAVE") && !jlIsSlaveFile(".slave"));
	CHECK(!jlIsSlaveFile("Loom.slave.info") && jlIsSkippedDir("Data") && !jlIsSkippedDir("data2"));
	uint16_t n = jlFormatLine(b, sizeof(b), "ADI", "G:E/ADI", "WB31_32.Slave", 1);
	b[n] = 0;
	CHECK(!strcmp(b, "ADI (WB31_32.Slave);G:E/ADI;WB31_32.Slave\n"));
	n = jlFormatLine(b, sizeof(b), "", "G:", "Top.slave", 0);
	b[n] = 0;
	CHECK(!strcmp(b, "Top;G:;Top.slave\n"));
	// Long slave names are kept whole (v0.x cut them at 30 chars)
	n = jlFormatLine(b, sizeof(b), "Indy", "G:Indy", "IndianaJonesAtlantisAdvFr.slave", 0);
	b[n] = 0;
	CHECK(!strcmp(b, "Indy;G:Indy;IndianaJonesAtlantisAdvFr.slave\n"));
	CHECK(jlFormatLine(b, 10, "Indy", "G:Indy", "Indy.slave", 0) == 0);
}

static void testInputRepeatAndEdges(void) {
	tJlInput in;
	jlInputInit(&in, JL_IN_FIRE);                       // fire held at start (quit a game)
	CHECK(jlInputStep(&in, JL_IN_FIRE) == 0);          // ... does nothing until released
	CHECK(jlInputStep(&in, 0) == 0);
	CHECK(jlInputStep(&in, JL_IN_FIRE) == JL_IN_FIRE);
	CHECK(jlInputStep(&in, JL_IN_FIRE) == 0);
	CHECK(jlInputStep(&in, JL_IN_DOWN) == JL_IN_DOWN);
	jlInputCooldown(&in, 2);
	CHECK(jlInputStep(&in, JL_IN_DOWN) == 0);
	CHECK(jlInputStep(&in, JL_IN_DOWN) == 0);
	CHECK(jlInputStep(&in, JL_IN_DOWN) == JL_IN_DOWN); // repeat
	jlInputCooldown(&in, 4);
	CHECK(jlInputStep(&in, JL_IN_UP) == JL_IN_UP);     // new direction: no wait
	jlInputCooldown(&in, 4);
	CHECK(jlInputStep(&in, 0) == 0);
	CHECK(jlInputStep(&in, JL_IN_UP) == JL_IN_UP);     // released and pressed again
}

static void testViewFoldersAndBack(void) {
	tLoaded l = load("B;G:x/F2/B;b.slave\nA;G:x/F1/A;a.slave\nC;G:x/F2/C;c.slave\n");
	tJlView v;
	const tJlEntry *e = 0;
	uint8_t len;
	jlViewInit(&v, &l.inv, 0);
	CHECK(jlViewCount(&v) == 3 && !strcmp(jlViewLabel(&v, 0, &len), "A"));
	jlViewToggleFolders(&v);
	CHECK(v.mode == JL_VIEW_FOLDERS && jlViewCount(&v) == 2);
	v.nav.index = 1;
	CHECK(jlViewActivate(&v, &e) == JL_ACT_REDRAW && v.mode == JL_VIEW_FOLDER);
	CHECK(jlViewCount(&v) == 3); // "..", B, C
	CHECK(!memcmp(jlViewLabel(&v, 0, &len), "..", 2) && len == 2);
	v.nav.index = 2;
	CHECK(jlViewActivate(&v, &e) == JL_ACT_LAUNCH && !strcmp(e->name, "C"));
	jlViewRebind(&v, &l.inv); // back from the game: same place
	CHECK(v.mode == JL_VIEW_FOLDER && v.nav.index == 2);
	v.nav.index = 0;
	CHECK(jlViewActivate(&v, &e) == JL_ACT_REDRAW && v.mode == JL_VIEW_FOLDERS && v.nav.index == 1);
	unload(&l);
}

int main(void) {
	testInputRepeatAndEdges();
	testViewFoldersAndBack();
	testParsesAndTrimsV0Padding();
	testSkipsMalformedLines();
	testSortsCaseInsensitively();
	testLargeInventoryStaysCompleteAndSorted();
	testFolderIsParentOfGameDir();
	testEmptyInventory();
	testKeyOrderMatchesFullCompare();
	testConfig();
	testNavigation();
	testScanFormat();
	return s_failures != 0;
}
