/* Leaf header (docs/plan.md 6.5 rule 2): `mhchar_draw_callback_set`, `mhchar_res_node_get`, which `sound/mhchar` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_SOUND_MHCHAR_DRAW_CALLBACK_SET_H
#define MHTRI_SOUND_MHCHAR_DRAW_CALLBACK_SET_H

#include "types.h"
#include "pl.h"
#include "g3d/g3d_resnode.h"

#ifdef __cplusplus
extern "C" {

/* 0x800E090C - stores the model's draw callback (MHchar +0x3C). */
void mhchar_draw_callback_set(MHchar* chr, void (*callback)(void));

/* 0x800E0B9C - reads resource node `index` of the model into `node`. */
void mhchar_res_node_get(MHchar* chr, nw4r::g3d::ResNode* node, u32 index);

}
#endif

#endif /* MHTRI_SOUND_MHCHAR_DRAW_CALLBACK_SET_H */
