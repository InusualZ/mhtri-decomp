/* g3d/g3d_state.h - the texture/state helpers of `g3d/g3d_state.cpp` that `g3d/g3d_resmat.cpp` resolves through,
 *   and the resource-range store `g3d/g3d_resfile.cpp` calls (C linkage). */
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

/* 0x80089690 - the resource-range store `g3d/g3d_resfile.cpp`'s accessors call. */
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

/* More of the unit's cross-unit declarations (rule 2). */
#ifdef __cplusplus
extern "C" {
#endif

void mtx34_inverse(Mtx34* out, const Mtx34* src);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_STATE_H */
