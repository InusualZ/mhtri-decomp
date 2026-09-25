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
/* 0x800689B0 - the debug-checked resource accessor (caller: g3d_resanmcamera.cpp).  Returns the
 * resolved data pointer for a `ResCommon`-style handle. */
u32 fn_800689B0(void* p);
/* 0x800697A4 - the name-record reader the ScnMdl type query (g3d/g3d_scnmdl.cpp's fn_8007EA10)
 * resolves against (rule 2: declared in its owner's header, not in the consumer). */
u32 fn_800697A4(void);

/* 0x8006E6B4/0x800695EC/0x8006993C - the `ResFile` revision-check getters, added when
 * `g3d/g3d_resfile.cpp` registered (rule 2). */
u32 fn_8006E6B4(void* p);   /* 0x8006E6B4 - `*(u32*)p != 0` */
u32 fn_800695EC(void* p);   /* 0x800695EC - the checked resource resolver */
u32 fn_8006CDBC(void* p);   /* 0x8006CDBC - the checked resource resolver */
u32 fn_8006993C(void* p);   /* 0x8006993C - the checked resource resolver */
/* 0x8006E2AC - the `ResTexSrt` non-const slot-array resolver (`ResTexSrt::SetEffectMtx` panics through
 * it when the handle word is invalid, then returns the base of the 0x34-byte slot array).  Declared
 * here for `g3d/g3d_resmat.cpp` (rule 2); disjoint from the `g3d_resfile.cpp` getters above, so both
 * sides' declarations survive. */
u8* fn_8006E2AC(void* pSelf);


#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_FN_800680CC_H */
