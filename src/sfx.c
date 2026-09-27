#include "sfx.h"
#include "sys.h"
#include <devices/audio.h>
#include <exec/memory.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <proto/exec.h>

#define HWREGS ((volatile struct Custom *)0xDFF000)
#define ALL_CHANNELS 0x000F
#define PAL_CLOCKS_PER_FRAME 70938UL  // 3546895 Hz / 50

typedef enum { STATE_IDLE, STATE_START, STATE_PLAYING } tState;

static struct MsgPort *s_pPort;
static struct IOAudio *s_pIo;
static BOOL s_isOpen;
static tState s_eState;
static UWORD s_uwFramesLeft;
static UWORD *s_pSilence;           // chip: 1 word of silence + the samples
static const UBYTE *s_pChip[SFX_COUNT];
static ULONG s_ulChipSize;

static BOOL copyToChip(void) {
	s_ulChipSize = 4;
	for(UBYTE i = 0; i < SFX_COUNT; ++i) s_ulChipSize += (ULONG)g_pSfx[i].words * 2;
	UBYTE *p = AllocMem(s_ulChipSize, MEMF_CHIP | MEMF_CLEAR);
	if(!p) {
		return FALSE;
	}
	s_pSilence = (UWORD *)p;
	p += 4;
	for(UBYTE i = 0; i < SFX_COUNT; ++i) {
		CopyMem((APTR)g_pSfx[i].data, p, (ULONG)g_pSfx[i].words * 2);
		s_pChip[i] = p;
		p += (ULONG)g_pSfx[i].words * 2;
	}
	return TRUE;
}
static UBYTE s_ubChannels[] = {ALL_CHANNELS}; // v0.x played on all four channels

BOOL sfxOpen(void) {
	if(!copyToChip()) {
		return FALSE;
	}
	s_pPort = jlCreatePort();
	s_pIo = AllocMem(sizeof(*s_pIo), MEMF_PUBLIC | MEMF_CLEAR);
	if(!s_pPort || !s_pIo) {
		sfxClose();
		return FALSE;
	}
	s_pIo->ioa_Request.io_Message.mn_ReplyPort = s_pPort;
	s_pIo->ioa_Request.io_Message.mn_Node.ln_Pri = 127;
	s_pIo->ioa_Data = s_ubChannels;
	s_pIo->ioa_Length = sizeof(s_ubChannels);
	s_isOpen = OpenDevice((STRPTR)AUDIONAME, 0, (struct IORequest *)s_pIo, 0) == 0;
	return s_isOpen;
}

void sfxPlay(tSfxId id) {
	if(!s_isOpen) {
		return;
	}
	const tSfxSample *s = &g_pSfx[id];
	HWREGS->dmacon = ALL_CHANNELS; // stop; restarted by sfxProcess next frame
	for(UBYTE ch = 0; ch < 4; ++ch) {
		HWREGS->aud[ch].ac_ptr = (UWORD *)s_pChip[id];
		HWREGS->aud[ch].ac_len = s->words;
		HWREGS->aud[ch].ac_per = s->period;
		HWREGS->aud[ch].ac_vol = 64;
	}
	s_eState = STATE_START;
	s_uwFramesLeft = (UWORD)(((ULONG)s->words * s->period * 2) / PAL_CLOCKS_PER_FRAME) + 2;
}

void sfxProcess(void) {
	if(s_eState == STATE_START) {
		HWREGS->dmacon = DMAF_SETCLR | ALL_CHANNELS;
		s_eState = STATE_PLAYING;
		return;
	}
	if(s_eState == STATE_PLAYING) {
		// Paula has latched the sample; queue silence to loop once it ends.
		for(UBYTE ch = 0; ch < 4; ++ch) {
			HWREGS->aud[ch].ac_ptr = s_pSilence;
			HWREGS->aud[ch].ac_len = 1;
		}
		s_eState = STATE_IDLE;
	}
	if(s_uwFramesLeft) {
		--s_uwFramesLeft;
	}
}

BOOL sfxIsBusy(void) {
	return s_eState != STATE_IDLE || s_uwFramesLeft != 0;
}

void sfxClose(void) {
	if(s_isOpen) {
		HWREGS->dmacon = ALL_CHANNELS;
		for(UBYTE ch = 0; ch < 4; ++ch) HWREGS->aud[ch].ac_vol = 0;
		CloseDevice((struct IORequest *)s_pIo);
		s_isOpen = FALSE;
	}
	if(s_pIo) {
		FreeMem(s_pIo, sizeof(*s_pIo));
		s_pIo = NULL;
	}
	jlDeletePort(s_pPort);
	s_pPort = NULL;
	if(s_pSilence) {
		FreeMem(s_pSilence, s_ulChipSize);
		s_pSilence = NULL;
	}
}
