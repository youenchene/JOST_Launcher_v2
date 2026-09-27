// Sound effects: the four v0.x samples, copied to chip RAM while the menu is
// open (Paula only reads chip RAM) and freed before a game starts.
// Channels are allocated through audio.device (so we never fight another
// program for Paula); playing is then a few register writes.
#ifndef JL_SFX_H
#define JL_SFX_H

#include <exec/types.h>

typedef enum { SFX_PAGE, SFX_MOVE, SFX_BUMP, SFX_LAUNCH, SFX_COUNT } tSfxId;

typedef struct {
	const UBYTE *data;  // anywhere: sfxOpen() copies it to chip RAM
	UWORD words;        // length in words
	UWORD period;       // Paula period (PAL)
} tSfxSample;

extern const tSfxSample g_pSfx[SFX_COUNT];

BOOL sfxOpen(void);        // no sound (but no failure) if channels are busy
void sfxPlay(tSfxId id);
void sfxProcess(void);     // once per frame: finishes starting / stopping sounds
BOOL sfxIsBusy(void);
void sfxClose(void);

#endif
