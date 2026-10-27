/* sound/snd_bgm_fade_level_get.h - leaf: `sound/fn_800E46E8.cpp`'s 0x800E492C, which returns the float the stream layer keeps at .sbss 0x807949B8
 * (the BGM controller treats 0.0 as "no BGM level"). */
#ifndef MHTRI_SOUND_SND_BGM_FADE_LEVEL_GET_H
#define MHTRI_SOUND_SND_BGM_FADE_LEVEL_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

f32 snd_bgm_fade_level_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_BGM_FADE_LEVEL_GET_H */
