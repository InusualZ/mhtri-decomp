/* Leaf header (docs/plan.md 6.5 rule 2): symbols `sound/fn_800D7F54.cpp` defines, for a consumer that does not include
 * the owner's header.  C linkage (the map rows are plain names). */
#ifndef MHTRI_SOUND_SE_GUIDE_VOICE_PLAY_H
#define MHTRI_SOUND_SE_GUIDE_VOICE_PLAY_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800DBD1C - plays bank 21's sound `row`/`column` (rows 0..2, columns 0..5). */
void se_guide_voice_play(u32 row, u32 column);
/* 0x800DCFE4 - plays system sound 12. */
void sysSE_window_open_play(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_GUIDE_VOICE_PLAY_H */
