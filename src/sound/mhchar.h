/* The `sound` band's `MHchar` header: `MHchar`, the actor's joint/model block, is defined once in
 * `pl.h` (docs/plan.md 6.5 rule 1), with every member the SE and model methods of
 * `src/sound/mhchar.cpp` and `src/sound/fn_800DD1F0.h` implement; this header keeps the include path
 * the sound band's consumers use. */
#ifndef MHTRI_SOUND_MHCHAR_H
#define MHTRI_SOUND_MHCHAR_H

#include "pl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800E31E8 - the model's node count (a tail call into `ResMdl::GetResNodeNumEntries`).  GUESS name. */
int mhchar_node_count(MHchar* self);
/* 0x800E0560 - resets the model object `self` points at (the shell pool runs it over a fresh
 * `shell_chara_heap_tbl` slot). */
void mhchar_reset(MHchar* self);

/* 0x800E26B4 */
void mhchar_mat_blend_set(MHchar* self, u32 index, f32 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_MHCHAR_H */
