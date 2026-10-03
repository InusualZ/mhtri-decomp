/*
 * The `g3d/g3d_state.cpp` unit's cross-unit declarations (docs/plan.md 6.5 rule 2).
 *
 * `g3d_state.cpp` (`.text` 0x8008452C-0x800898B0) owns the nw4r g3d texture/state helpers that the
 * `g3d/g3d_resmat.cpp` handle accessors resolve their resources through, and the resource-range store
 * helper `g3d/g3d_resfile.cpp`'s accessor cluster calls.  A consumer includes this header instead of
 * declaring them itself.
 *
 * All of them keep C linkage (their map names are plain `fn_XXXXXXXX` stems).
 */
#ifndef MHTRI_G3D_G3D_STATE_H
#define MHTRI_G3D_G3D_STATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80086390 - the `ResTexSrt` handle's non-const resource resolver (returns the slot array). */
u8* fn_80086390(void* pSelf);

/* 0x80087870 - the const twin of fn_80086390 (used by `ResTexSrt::GetEffectMtx`). */
u8* fn_80087870(void* pSelf);

/* 0x80089690 - the resource-range store (caller: g3d/g3d_resfile.cpp's accessor cluster).  Added
 * when `g3d/g3d_resfile.cpp` registered; disjoint from the resolvers below, so both sides' intent
 * survives this add/add conflict. */
void fn_80089690(void* pBase, u32 size);

/* 0x80089624..0x80089844 - the `ResTlut`-style slot-array resolvers the g3d_resmat thunks tail-call
 * with a zero index. */
u8* fn_80089624(void* pSelf, u32 index);
u8* fn_80089694(void* pSelf, u32 index);
u8* fn_80089700(void* pSelf, u32 index);
u8* fn_8008976C(void* pSelf, u32 index);
u8* fn_800897D8(void* pSelf, u32 index);
u8* fn_80089844(void* pSelf, u32 index);

#ifdef __cplusplus
}
#endif

/* Declarations moved here from `include/unsplit/g3d.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

void mtx34_inverse(Mtx34* out, const Mtx34* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_STATE_H */
