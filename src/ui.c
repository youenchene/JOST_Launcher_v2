#include "ui.h"
#include "sys.h"
#include <exec/memory.h>
#include <graphics/gfxmacros.h>
#include <graphics/copper.h>
#include <graphics/text.h>
#include <hardware/custom.h>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#define CUSTOMREGS (*(struct Custom *)0xDFF000)
#define SCREEN_W 640
#define SCREEN_H 256
#define CHAR_W 8
#define LIST_TOP 4          // v0.x: Locate x,0.5
#define INFO_Y 245          // v0.x: Locate x,30.7
#define COL_CHARS 40
#define LABEL_MAX 37        // a column is 40 chars; labels start at char 3
#define COPPER_INS 340

static struct TextAttr s_topaz8 = {(STRPTR)"topaz.font", 8, FS_NORMAL, FPF_ROMFONT};
static struct Screen *s_pScreen;
static struct Window *s_pWindow;
static struct RastPort *s_pRp;
static UWORD *s_pBlankPointer;   // chip RAM (sprite DMA); __chip hunks lose their flag here
#define BLANK_POINTER_SIZE (6 * sizeof(UWORD))
static UBYTE s_ubBaseline;

// v0.x palette: black, white, light blue, orange
static const UWORD s_pPalette[4] = {0x000, 0xFFF, 0x8CF, 0xFA5};

static void move(struct UCopList *ucl, UBYTE color, UWORD rgb) {
	CMOVE(ucl, CUSTOMREGS.color[color], rgb);
}

static void waitLine(struct UCopList *ucl, UWORD line) {
	CWAIT(ucl, line, 0);
}

// The v0.x "copper juice": orange top/bottom rules, a light blue gradient on
// every text row, and an orange/grey gradient on the info line.
static struct UCopList *buildCopper(void) {
	struct UCopList *ucl = AllocMem(sizeof(*ucl), MEMF_PUBLIC | MEMF_CLEAR);
	if(!ucl) {
		return NULL;
	}
	CINIT(ucl, COPPER_INS);
	move(ucl, 3, 0xFA5);
	waitLine(ucl, 1);
	move(ucl, 3, 0xE73);
	for(UWORD row = 0; row < 29; ++row) {
		for(UWORD y = 0; y < 5; ++y) {
			waitLine(ucl, 4 + y * 2 + row * 8);
			move(ucl, 2, (UWORD)(((8 + y) << 8) | ((12 + y / 2) << 4) | 15));
		}
	}
	waitLine(ucl, 240);
	move(ucl, 3, 0xE73);
	waitLine(ucl, 241);
	move(ucl, 3, 0xFA5);
	for(UWORD y = 0; y < 5; ++y) {
		waitLine(ucl, 245 + y * 2);
		move(ucl, 3, (UWORD)(0xF00 | ((10 + y / 2) << 4) | (5 + y)));
		move(ucl, 1, (UWORD)((15 - y) * 0x111));
	}
	CEND(ucl);
	return ucl;
}

static void drawFrame(const char *version) {
	SetRast(s_pRp, UI_PEN_BG);
	SetAPen(s_pRp, UI_PEN_ORANGE);
	RectFill(s_pRp, 10, 0, 630, 1);
	RectFill(s_pRp, 10, 240, 630, 241);
	SetDrMd(s_pRp, JAM2);
	SetBPen(s_pRp, UI_PEN_BG);
	Move(s_pRp, 1 * CHAR_W, INFO_Y + s_ubBaseline);
	Text(s_pRp, (STRPTR)version, 3);
}

static BOOL openWindow(void) {
	struct NewWindow nw = {0};
	nw.Width = SCREEN_W;
	nw.Height = SCREEN_H;
	nw.DetailPen = nw.BlockPen = (UBYTE)-1;
	nw.IDCMPFlags = RAWKEY;
	nw.Flags = BACKDROP | BORDERLESS | ACTIVATE | RMBTRAP | NOCAREREFRESH;
	nw.Screen = s_pScreen;
	nw.Type = CUSTOMSCREEN;
	s_pWindow = OpenWindow(&nw);
	if(!s_pWindow) {
		return FALSE;
	}
	s_pBlankPointer = AllocMem(BLANK_POINTER_SIZE, MEMF_CHIP | MEMF_CLEAR);
	if(s_pBlankPointer) {
		SetPointer(s_pWindow, s_pBlankPointer, 1, 16, 0, 0);
	}
	s_pRp = s_pWindow->RPort;
	ShowTitle(s_pScreen, FALSE); // else the title bar layer hides lines 0-10
	return TRUE;
}

BOOL uiOpen(const char *version) {
	jlForcePal(); // some real CDTV/A500 units misreport NTSC; see sys.h
	struct NewScreen ns = {0};
	ns.Width = SCREEN_W;
	ns.Height = SCREEN_H;
	ns.Depth = 2;
	ns.DetailPen = 0;
	ns.BlockPen = 1;
	ns.ViewModes = HIRES;
	ns.Type = CUSTOMSCREEN | SCREENQUIET;
	ns.Font = &s_topaz8;
	s_pScreen = OpenScreen(&ns);
	if(!s_pScreen || !openWindow()) {
		uiClose();
		return FALSE;
	}
	LoadRGB4(&s_pScreen->ViewPort, (UWORD *)s_pPalette, 4);
	s_ubBaseline = (UBYTE)s_pRp->TxBaseline;
	drawFrame(version);
	struct UCopList *ucl = buildCopper();
	if(ucl) {
		Forbid();
		s_pScreen->ViewPort.UCopIns = ucl; // CloseScreen frees it
		Permit();
		RethinkDisplay();
	}
	return TRUE;
}

void uiClose(void) {
	if(s_pWindow) {
		ClearPointer(s_pWindow);
		CloseWindow(s_pWindow);
		s_pWindow = NULL;
	}
	if(s_pBlankPointer) {
		FreeMem(s_pBlankPointer, BLANK_POINTER_SIZE);
		s_pBlankPointer = NULL;
	}
	if(s_pScreen) {
		CloseScreen(s_pScreen);
		s_pScreen = NULL;
	}
}

struct Window *uiWindow(void) {
	return s_pWindow;
}

static void drawAt(UWORD x, UWORD y, const char *text, UWORD len, UBYTE pen) {
	SetAPen(s_pRp, pen);
	Move(s_pRp, x, y + s_ubBaseline);
	Text(s_pRp, (STRPTR)text, len);
}

// One list slot: ">" marker and label, in the pens given.
static void drawItem(const tJlView *v, UWORD index, UBYTE markColor, UBYTE textColor) {
	UWORD col = (UWORD)(jlNavCol(index) * COL_CHARS);
	UWORD y = (UWORD)(LIST_TOP + jlNavRow(index) * 8);
	UBYTE len;
	const char *label = jlViewLabel(v, index, &len);
	drawAt((UWORD)((col + 1) * CHAR_W), y, ">", 1, markColor);
	drawAt((UWORD)((col + 3) * CHAR_W), y, label, len > LABEL_MAX ? LABEL_MAX : len, textColor);
}

void uiDrawPage(const tJlView *v) {
	SetAPen(s_pRp, UI_PEN_BG);
	RectFill(s_pRp, 0, LIST_TOP, SCREEN_W - 1, LIST_TOP + JL_ROWS * 8 - 1);
	UWORD count = jlViewCount(v);
	if(!count) {
		return;
	}
	UWORD first = jlNavPageStart(v->nav.index);
	UWORD last = first + JL_PAGE < count ? first + JL_PAGE : count;
	for(UWORD i = first; i < last; ++i) {
		drawItem(v, i, UI_PEN_BG, UI_PEN_ITEM);
	}
	drawItem(v, v->nav.index, UI_PEN_WHITE, UI_PEN_WHITE);
}

void uiMoveCursor(const tJlView *v, UWORD oldIndex) {
	drawItem(v, oldIndex, UI_PEN_BG, UI_PEN_ITEM);
	drawItem(v, v->nav.index, UI_PEN_WHITE, UI_PEN_WHITE);
}

void uiShowCursor(const tJlView *v, BOOL isVisible) {
	if(!jlViewCount(v)) {
		return;
	}
	UBYTE pen = isVisible ? UI_PEN_WHITE : UI_PEN_BG;
	drawItem(v, v->nav.index, pen, pen);
}

void uiInfo(const char *text, UBYTE pen) {
	SetAPen(s_pRp, UI_PEN_BG);
	RectFill(s_pRp, 6 * CHAR_W, INFO_Y, SCREEN_W - 1, INFO_Y + 7);
	UWORD len = 0;
	while(text[len] && len < 74) ++len;
	drawAt(6 * CHAR_W, INFO_Y, text, len, pen);
}
