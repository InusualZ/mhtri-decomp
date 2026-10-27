/* sound/snd_stream_bgm_flag_get.h - leaf: `sound/snd_bank_loader.cpp`'s 0x800F298C: 1 when stream `id`'s entry for `channel` carries a zero +0x20
 * halfword (the value `set_stream_main_vol_flag` takes as its is-BGM flag). */
#ifndef MHTRI_SOUND_SND_STREAM_BGM_FLAG_GET_H
#define MHTRI_SOUND_SND_STREAM_BGM_FLAG_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

u8 snd_stream_bgm_flag_get(s32 channel, u32 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SND_STREAM_BGM_FLAG_GET_H */
