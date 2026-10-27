/* sound/snd_bgm_loading_get.h - leaf: `sound/snd_bank_loader.cpp`'s 0x800F2A84, which returns the bank work's +0x97AF byte
 * (`bgm_loading_2`); the BGM controller skips its area frame while it is 1. */
#ifndef MHTRI_SOUND_SND_BGM_LOADING_GET_H
#define MHTRI_SOUND_SND_BGM_LOADING_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

u8 snd_bgm_loading_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_BGM_LOADING_GET_H */
