#include "app.h"
#include "config.h"
#include "dbg.h"
#include "hwinput.h"
#include "input.h"
#include "inventory.h"
#include "launch.h"
#include "scan.h"
#include "sfx.h"
#include "store.h"
#include "sys.h"
#include "ui.h"
#include "view.h"
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>

#define CONFIG_FILE "S:jl-config.cfg"
#define BLINK_ON 40    // v0.x: frames the cursor is shown
#define BLINK_OFF 10   // ... and hidden
#define FOLDER_COOLDOWN (JL_REPEAT_PAGE * 2)

static const char INFO_HELP[] = "F or 1 to switch to folder view, S or 0 to scan folders";
static const char INFO_CONFIRM_SCAN[] = "Enter or Button1 to confirm / C or  Button 2  to cancel";
static const char INFO_EMPTY[] = "No game found: S or 0 to scan folders";
static const char INFO_FAILED[] = "Could not start: ";

typedef enum { RUN_QUIT, RUN_LAUNCH } tRun;

typedef struct {
	tJlConfig cfg;
	tJlStore store;
	tJlView view;
	BOOL hasView;
	tJlInput in;
	UBYTE blinkLeft;
	BOOL isCursorShown;
	BOOL hasLaunchFailed;
	char launchPath[JL_CFG_STR * 2];
	char launchSlave[JL_CFG_STR];
	char info[120];
} tApp;

// ------------------------------------------------------------------ data

static void loadConfig(tJlConfig *cfg) {
	jlConfigDefaults(cfg);
	ULONG len;
	char *text = jlReadFile(CONFIG_FILE, &len);
	if(text) {
		jlConfigParse(cfg, text, len);
		jlFree(text);
	}
	if(!cfg->scanDirCount) { // v0.x default
		jlStrCat(cfg->scanDirs[0], JL_CFG_STR, "RAM:");
		cfg->scanDirCount = 1;
	}
}

static void loadInventory(tApp *a) {
	storeLoad(&a->store, a->cfg.inventoryFile);
}

// Saves the inventory in name order if it wasn't, so the next start skips the
// sort. Done when the user waits anyway (after a scan, on launch or quit),
// never before the menu shows.
static void keepSorted(tApp *a) {
	if(!a->store.inv.wasSorted) {
		dbgKv("resorted", storeSaveSorted(&a->store, a->cfg.inventoryFile));
		dbgEnd();
		a->store.inv.wasSorted = 1;
	}
}

static void bindView(tApp *a) {
	if(a->hasView) {
		jlViewRebind(&a->view, &a->store.inv);
	}
	else {
		jlViewInit(&a->view, &a->store.inv, a->cfg.folderModeByDefault);
		a->hasView = TRUE;
	}
}

// ---------------------------------------------------------------- report

static void reportView(const tApp *a, const char *event) {
	char label[40];
	UBYTE len = 0;
	label[0] = '\0';
	if(jlViewCount(&a->view)) {
		const char *l = jlViewLabel(&a->view, a->view.nav.index, &len);
		if(len > sizeof(label) - 1) len = sizeof(label) - 1;
		CopyMem((APTR)l, label, len);
		label[len] = '\0';
		for(UBYTE i = 0; i < len; ++i) {
			if(label[i] == ' ') label[i] = '_'; // keep k=v parsable
		}
	}
	dbgKs("ev", event);
	dbgKv("mode", a->view.mode);
	dbgKv("sel", a->view.nav.index);
	dbgKv("count", jlViewCount(&a->view));
	dbgKv("page", jlNavPageStart(a->view.nav.index));
	dbgKs("label", label);
	dbgEnd();
}

// ---------------------------------------------------------------- screen

static void showHelp(const tApp *a) {
	uiInfo(a->store.inv.count ? INFO_HELP : INFO_EMPTY, UI_PEN_WHITE);
}

static void redraw(tApp *a) {
	uiDrawPage(&a->view);
	a->blinkLeft = BLINK_ON;
	a->isCursorShown = TRUE;
}

static void blink(tApp *a) {
	if(--a->blinkLeft) {
		return;
	}
	a->isCursorShown = !a->isCursorShown;
	a->blinkLeft = a->isCursorShown ? BLINK_ON : BLINK_OFF;
	uiShowCursor(&a->view, a->isCursorShown);
}

static void nextFrame(void) {
	WaitTOF();
	dbgTick();
	sfxProcess();
}

// ---------------------------------------------------------------- actions

static tJlDir toDir(UWORD ev) {
	if(ev & JL_IN_UP) return JL_DIR_UP;
	if(ev & JL_IN_DOWN) return JL_DIR_DOWN;
	if(ev & JL_IN_LEFT) return JL_DIR_LEFT;
	return JL_DIR_RIGHT;
}

static void move(tApp *a, UWORD ev) {
	UWORD old = a->view.nav.index;
	tJlDir dir = toDir(ev);
	tJlNavResult r = jlNavMove(&a->view.nav, dir);
	BOOL isSide = dir == JL_DIR_LEFT || dir == JL_DIR_RIGHT;
	if(r == JL_NAV_BUMP) {
		sfxPlay(SFX_BUMP);
		jlInputCooldown(&a->in, JL_REPEAT_MOVE);
		return;
	}
	if(r == JL_NAV_PAGE) {
		redraw(a);
		jlInputCooldown(&a->in, JL_REPEAT_PAGE);
	}
	else {
		uiMoveCursor(&a->view, old);
		a->blinkLeft = BLINK_ON;
		a->isCursorShown = TRUE;
		jlInputCooldown(&a->in, JL_REPEAT_MOVE);
	}
	sfxPlay(isSide ? SFX_PAGE : SFX_MOVE);
	reportView(a, "move");
}

static void toggleFolders(tApp *a) {
	jlViewToggleFolders(&a->view);
	redraw(a);
	showHelp(a);
	jlInputCooldown(&a->in, FOLDER_COOLDOWN);
	reportView(a, "folders");
}

static BOOL confirmScan(tApp *a) {
	uiInfo(INFO_CONFIRM_SCAN, UI_PEN_ORANGE);
	dbgPrint("AGK scan confirm\n");
	for(;;) {
		nextFrame();
		UWORD ev = jlInputStep(&a->in, hwInputRead(uiWindow()));
		if(ev & JL_IN_FIRE) return TRUE;
		if(ev & (JL_IN_CANCEL | JL_IN_QUIT)) return FALSE;
	}
}

static void onScanProgress(UWORD count, void *userData) {
	tApp *a = userData;
	char num[8];
	UBYTE n = 0;
	do { num[n++] = (char)('0' + count % 10); count /= 10; } while(count && n < 7);
	UWORD len = jlStrLen(a->info);
	char *p = a->info;
	while(len && p[len - 1] != '(') --len; // keep "Scanning dir : X ("
	p[len] = '\0';
	while(n && len < sizeof(a->info) - 2) p[len++] = num[--n];
	p[len++] = ')';
	p[len] = '\0';
	uiInfo(a->info, UI_PEN_ORANGE);
	dbgTick(); // the scan is synchronous: keep the test harness's frame clock alive
}

static void scan(tApp *a) {
	if(!confirmScan(a)) {
		showHelp(a);
		return;
	}
	a->info[0] = '\0';
	jlStrCat(a->info, sizeof(a->info), "Scanning dir : ");
	jlStrCat(a->info, sizeof(a->info), a->cfg.scanDirs[0]);
	jlStrCat(a->info, sizeof(a->info), " (");
	onScanProgress(0, a);
	LONG n = scanWriteInventory(&a->cfg, onScanProgress, a);
	loadInventory(a);
	keepSorted(a);
	a->hasView = FALSE; // new list: start at the top
	bindView(a);
	redraw(a);
	showHelp(a);
	dbgKv("scanned", n);
	dbgEnd();
	reportView(a, "scan");
	hwInputReset(uiWindow());
	jlInputInit(&a->in, hwInputRead(uiWindow()));
}

static void rememberLaunch(tApp *a, const tJlEntry *e) {
	a->launchPath[0] = a->launchSlave[0] = '\0';
	jlStrCat(a->launchPath, sizeof(a->launchPath), e->path);
	jlStrCat(a->launchSlave, sizeof(a->launchSlave), e->slave);
	a->info[0] = '\0';
	jlStrCat(a->info, sizeof(a->info), "Launching : ");
	jlStrCat(a->info, sizeof(a->info), e->name);
	uiInfo(a->info, UI_PEN_ORANGE);
	sfxPlay(SFX_LAUNCH);
	dbgKs("launch", e->slave);
	dbgEnd();
	while(sfxIsBusy()) nextFrame(); // v0.x waited 3 s; the sound is enough
}

static BOOL activate(tApp *a) {
	const tJlEntry *e = NULL;
	tJlAction act = jlViewActivate(&a->view, &e);
	if(act == JL_ACT_LAUNCH) {
		rememberLaunch(a, e);
		return TRUE;
	}
	if(act == JL_ACT_REDRAW) {
		redraw(a);
		if(a->view.mode == JL_VIEW_FOLDER) {
			const tJlEntry *first = jlInvFolderFirst(&a->store.inv, a->view.folder);
			const char *name = first->folder;
			UBYTE len = first->folderLen;
			a->info[0] = '\0';
			jlStrCat(a->info, sizeof(a->info), "Current Folder : ");
			UWORD at = jlStrLen(a->info);
			if(len > sizeof(a->info) - at - 1) len = (UBYTE)(sizeof(a->info) - at - 1);
			CopyMem((APTR)name, a->info + at, len);
			a->info[at + len] = '\0';
			uiInfo(a->info, UI_PEN_ORANGE);
		}
		else {
			showHelp(a);
		}
		jlInputCooldown(&a->in, FOLDER_COOLDOWN);
		reportView(a, "open");
	}
	return FALSE;
}

// ------------------------------------------------------------------ loop

static tRun runUi(tApp *a) {
	struct Window *win = uiWindow();
	hwInputReset(win);
	jlInputInit(&a->in, hwInputRead(win));
	redraw(a);
	if(a->hasLaunchFailed) {
		a->info[0] = '\0';
		jlStrCat(a->info, sizeof(a->info), INFO_FAILED);
		jlStrCat(a->info, sizeof(a->info), a->launchSlave);
		uiInfo(a->info, UI_PEN_ORANGE);
		a->hasLaunchFailed = FALSE;
	}
	else {
		showHelp(a);
	}
	reportView(a, "start");
	dbgReady();
	for(;;) {
		nextFrame();
		UWORD ev = jlInputStep(&a->in, hwInputRead(win));
		if(ev & JL_IN_QUIT) return RUN_QUIT;
		if(ev & JL_IN_SCAN) scan(a);
		else if(ev & JL_IN_FOLDER) toggleFolders(a);
		else if(ev & (JL_IN_FIRE | JL_IN_CANCEL)) { // v0.x: A or B launches
			if(activate(a)) return RUN_LAUNCH;
		}
		else if(ev & JL_IN_DIRS) move(a, ev);
		blink(a);
	}
}

static tRun session(tApp *a) {
	loadInventory(a);
	bindView(a);
	if(!uiOpen(JL_VERSION)) {
		storeFree(&a->store);
		return RUN_QUIT;
	}
	sfxOpen();
	tRun r = runUi(a);
	sfxClose();
	uiClose();
	keepSorted(a);
	storeFree(&a->store); // the game gets this memory back
	return r;
}

LONG appRun(void) {
	tApp *a = jlAlloc(sizeof(*a), MEMF_ANY | MEMF_CLEAR);
	if(!a) {
		return RETURN_FAIL;
	}
	loadConfig(&a->cfg);
	storeFree(&a->store); // start from a valid empty inventory
	while(session(a) == RUN_LAUNCH) {
		BOOL isOk = launchSlave(a->cfg.jstCommand, a->launchPath, a->launchSlave);
		a->hasLaunchFailed = !isOk;
		dbgKv("returned", isOk);
		dbgEnd();
	}
	jlFree(a);
	return RETURN_OK;
}

// ------------------------------------------------------- module (C:jl-menu)

LONG appRunModule(tJlShared *sh) {
	tApp *a = jlAlloc(sizeof(*a), MEMF_ANY | MEMF_CLEAR);
	if(!a) {
		return RETURN_FAIL;
	}
	loadConfig(&a->cfg);
	storeFree(&a->store);
	if(sh->hasView) { // back from a game: same view, same selection
		a->hasView = TRUE;
		a->view.mode = (tJlViewMode)sh->viewMode;
		a->view.folder = sh->folder;
		a->view.nav.index = sh->index;
	}
	a->hasLaunchFailed = sh->launchFailed;
	jlStrCat(a->launchSlave, sizeof(a->launchSlave), sh->launchSlave);
	tRun r = session(a);
	sh->action = r == RUN_LAUNCH;
	sh->hasView = TRUE;
	sh->viewMode = (UBYTE)a->view.mode;
	sh->folder = a->view.folder;
	sh->index = a->view.nav.index;
	sh->launchPath[0] = sh->launchSlave[0] = sh->jstCommand[0] = '\0';
	jlStrCat(sh->launchPath, JL_SHARED_PATH, a->launchPath);
	jlStrCat(sh->launchSlave, JL_SHARED_NAME, a->launchSlave);
	jlStrCat(sh->jstCommand, JL_SHARED_NAME, a->cfg.jstCommand);
	jlFree(a);
	return RETURN_OK;
}
