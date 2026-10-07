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

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_MHCHAR_H */
