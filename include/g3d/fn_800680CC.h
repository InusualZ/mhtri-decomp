/*
 * The `g3d/fn_800680CC.cpp` cluster's cross-unit declarations (docs/plan.md 6.5 rule 2).  The right-hand
 * `g3d/g3d_calcvtx.cpp` unit (0x8007270C-0x800736F8) consumes this `g3d_resvtx_ac.h` accessor group and
 * used to get it from `include/unsplit/g3d.h`, because the range was unclaimed; registering
 * `g3d/fn_800680CC.cpp` (0x800680CC-0x8006EAC0) makes them owned, so the declarations move here.
 *
 * All keep C linkage (plain `fn_XXXXXXXX` map names).  The consumer's measured spellings are kept - the
 * `g3d/g3d_calcvtx.cpp` call sites were scored against them - and the owner's own forward prototypes in
 * `fn_800680CC.cpp` are ABI-identical (same argument registers), so the two never collide: the owner
 * does not include this header.
 */
#ifndef MHTRI_G3D_FN_800680CC_H
#define MHTRI_G3D_FN_800680CC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8006946C..0x80069768 - the `g3d_resvtx_ac.h` accessor group (callers: g3d_calcvtx.cpp). */
void* fn_800695D4(const void* p);
void* fn_800695DC(const void* p);
void* fn_800696E4(const void* p);
const char* fn_80069748(void);
s32 fn_80069754(const void* p);
void fn_80069768(void* pDst, const void* pSrc);
u32 fn_800696C0(const void* p);
bool fn_8006946C(const void* p, s32 id);

/* 0x80068634 - the checked resource resolver: panics when the handle's reference is invalid, then
 * returns the body word at +0x0 (the resolved data pointer).  Owner: g3d/fn_800680CC.cpp. */
u32 fn_80068634(void* p);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_FN_800680CC_H */
