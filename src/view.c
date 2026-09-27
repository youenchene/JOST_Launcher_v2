#include "view.h"

static uint8_t strLen8(const char *s) {
	uint8_t n = 0;
	while(s[n] && n < 255) ++n;
	return n;
}

static uint16_t countFor(const tJlView *v) {
	switch(v->mode) {
		case JL_VIEW_LIST: return v->inv->count;
		case JL_VIEW_FOLDERS: return v->inv->folderCount;
		default: return (uint16_t)(jlInvFolderSize(v->inv, v->folder) + 1);
	}
}

static void enter(tJlView *v, tJlViewMode mode, uint16_t folder, uint16_t index) {
	if(mode != JL_VIEW_LIST) {
		jlInvEnsureFolders(v->inv);
	}
	v->mode = mode;
	v->folder = folder;
	jlNavInit(&v->nav, countFor(v), index);
}

void jlViewInit(tJlView *v, tJlInventory *inv, int folderMode) {
	v->inv = inv;
	enter(v, folderMode ? JL_VIEW_FOLDERS : JL_VIEW_LIST, 0, 0);
}

void jlViewRebind(tJlView *v, tJlInventory *inv) {
	v->inv = inv;
	if(v->mode != JL_VIEW_LIST) {
		jlInvEnsureFolders(inv);
	}
	if(v->mode == JL_VIEW_FOLDER && v->folder >= inv->folderCount) {
		enter(v, JL_VIEW_FOLDERS, 0, 0);
		return;
	}
	enter(v, v->mode, v->folder, v->nav.index);
}

uint16_t jlViewCount(const tJlView *v) {
	return v->nav.count;
}

const char *jlViewLabel(const tJlView *v, uint16_t i, uint8_t *len) {
	const tJlEntry *e;
	switch(v->mode) {
		case JL_VIEW_LIST:
			e = &v->inv->entries[v->inv->byName[i]];
			break;
		case JL_VIEW_FOLDERS:
			e = jlInvFolderFirst(v->inv, i);
			*len = e->folderLen;
			return e->folder;
		default:
			if(i == 0) {
				*len = 2;
				return "..";
			}
			e = jlInvFolderEntry(v->inv, v->folder, (uint16_t)(i - 1));
			break;
	}
	*len = strLen8(e->name);
	return e->name;
}

tJlAction jlViewActivate(tJlView *v, const tJlEntry **entry) {
	uint16_t i = v->nav.index;
	if(!v->nav.count) {
		return JL_ACT_NONE;
	}
	switch(v->mode) {
		case JL_VIEW_LIST:
			*entry = &v->inv->entries[v->inv->byName[i]];
			return JL_ACT_LAUNCH;
		case JL_VIEW_FOLDERS:
			enter(v, JL_VIEW_FOLDER, i, 0);
			return JL_ACT_REDRAW;
		default:
			if(i == 0) {
				enter(v, JL_VIEW_FOLDERS, 0, v->folder); // back on the folder we came from
				return JL_ACT_REDRAW;
			}
			*entry = jlInvFolderEntry(v->inv, v->folder, (uint16_t)(i - 1));
			return JL_ACT_LAUNCH;
	}
}

void jlViewToggleFolders(tJlView *v) {
	enter(v, v->mode == JL_VIEW_LIST ? JL_VIEW_FOLDERS : JL_VIEW_LIST, 0, 0);
}
