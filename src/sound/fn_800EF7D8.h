/*
 * sound/fn_800EF7D8.h - the declarations `src/sound/fn_800EF7D8.cpp` owns (docs/plan.md 6.5 rule 2).  The
 * two loaders are C++ free functions - the map's `title_se_load__Fv` (0x800F18F0) and `title_bgm_load__Fv`
 * (0x800F2228) - loading the title's sound-effect bank and its BGM stream.  Added with
 * `quest/arenatask.cpp`, the first consumer.
 *
 * `snd_bank_layout` (0x800EF7D8, plain C name) lays the six bank tables out from the file cursor and
 * `scene_se_bank_load` (0x800EF9C0, plain C name) loads a scene's SE bank; `srt_ready_ck__Fl` (0x800F04DC)
 * answers whether one of the three BGM streams has its `srt` in.  The two plain-C names are GUESSES from
 * their bodies (the map carried `fn_800EF7D8` / `fn_800EF9C0`).
 */
#ifndef MHTRI_SOUND_FN_800EF7D8_H
#define MHTRI_SOUND_FN_800EF7D8_H

#include "types.h"

#ifdef __cplusplus
extern "C" void se_slot_req(u8 arg0); /* 0x800F16D4 - requests the sound-effect slot `arg0` */
extern "C" {
void snd_bank_layout(u8 mode);
void scene_se_bank_load(u8 a, u8 b);
}

void title_se_load(void);
void title_bgm_load(void);
s32 srt_ready_ck(s32 stream);
#endif

#endif /* MHTRI_SOUND_FN_800EF7D8_H */
