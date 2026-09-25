/*
 * Declarations owned by `sound/fn_800E46E8.cpp` (docs/plan.md 6.5 rule 2).  A consumer includes this
 * header instead of declaring the symbol itself.  Keep it minimal.
 * Declarations owned by `sound/fn_800E46E8.cpp` (docs/plan.md 6.5 rule 2): the sound-manager entry
 * points the rest of the `sound` band calls.  A consumer includes this header instead of declaring the
 * symbol itself.  Keep it minimal - the signatures are the ones `sound/fn_800E46E8.cpp` defines.
 */
#ifndef MHTRI_SOUND_FN_800E46E8_H
#define MHTRI_SOUND_FN_800E46E8_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The depth-compare selector `set_zmode` maps a `_GXCompare` value through. */
s32 fn_800E46E8(u32 kind);
/* Volume / stream-state entry points. */
void fn_800E4908(u8 is_bgm, f32 volume);
void fn_800E4D60(u32 idx);

/* The relocation-table installer and the slot lookups. */
void fn_800E4B4C(u32 idx, void* desc, u32 a, u32 b);
void* fn_800E4F4C(u32 idx, u32 entry_idx);
void* fn_800E7FC4(u32 idx, u32 key);
void* fn_800E80DC(u32 a, u32 b, u32 c);

/* Player reset / level-table entry points. */
void fn_800E8294(void);
void fn_800E84F0(s32 volume);
void fn_800E85E8(u32 idx);
void fn_800E8634(u32 idx, u32 entry_idx);
/* The stream stop `bgm_stop_all()` in `sound/fn_800F2A94.cpp` tail-calls. */
void fn_800E4A7C(void);

#ifdef __cplusplus
}
#endif

/* `sound/fn_800E46E8.cpp` defines these two at C++ scope, so they mangle
 * (`set_stream_main_vol_flag__FUcUc`, `PlayStream__FUlUl`) and the target object references them that
 * way.  Declaring them inside the extern "C" block above gave every consumer the plain C name
 * (relocaudit).  They are declared here, once, with the linkage the owner emits. */
#ifdef __cplusplus
void set_stream_main_vol_flag(u8 slot, u8 is_bgm);
void PlayStream(u32 a, u32 b);
#endif

#endif /* MHTRI_SOUND_FN_800E46E8_H */
