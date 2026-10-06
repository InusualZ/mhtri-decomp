/* g3d/fn_800680CC.h - the cross-unit declarations of `g3d/fn_800680CC.cpp` (C linkage).  The `g3d_resvtx_ac.h`
 *   group is in `g3d/g3d_calcvtx.cpp`'s measured spellings; the owner does not include this header, and its own
 *   forward prototypes are ABI-identical. */
#ifndef MHTRI_G3D_FN_800680CC_H
#define MHTRI_G3D_FN_800680CC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8006946C..0x80069768 - the `g3d_resvtx_ac.h` accessor group (callers: g3d_calcvtx.cpp). */
#ifdef __cplusplus
bool fn_8006946C(const void* p, s32 id); /* C++ only: `bool` is not a C type */
#endif

/* 0x80068634 - the checked resource resolver: panics when the handle's reference is invalid, then
 * returns the body word at +0x0 (the resolved data pointer).  Owner: g3d/fn_800680CC.cpp. */
u32 fn_80068634(void* p);
/* 0x800689B0 - the debug-checked resource accessor (caller: g3d_resanmcamera.cpp).  Returns the
 * resolved data pointer for a `ResCommon`-style handle. */
u32 fn_800689B0(void* p);
/* 0x800697A4 - the name-record reader the ScnMdl type query (g3d/g3d_scnmdl.cpp's fn_8007EA10)
 * resolves against (rule 2: declared in its owner's header, not in the consumer). */
u32 fn_800697A4(void);

/* 0x8006E6B4/0x800695EC/0x8006993C - the `ResFile` revision-check getters `g3d/g3d_resfile.cpp` calls. */
u32 fn_8006E6B4(void* p);   /* 0x8006E6B4 - `*(u32*)p != 0` */
u32 fn_800695EC(void* p);   /* 0x800695EC - the checked resource resolver */
u32 fn_8006993C(void* p);   /* 0x8006993C - the checked resource resolver */
/* 0x8006E2AC - the `ResTexSrt` non-const slot-array resolver: panics on an invalid handle word, else returns the
 * base of the 0x34-byte slot array (`ResTexSrt::SetEffectMtx` in `g3d/g3d_resmat.cpp`). */

/* 0x8006CDBC - the checked resource resolver the `g3d/g3d_resfile.cpp` getters and the
 * `g3d/g3d_resanmtexsrt.cpp` field readers call. */
u32 fn_8006CDBC(void* p);
/* 0x8006D9FC - the indexed handle reader the `g3d_resanmtexsrt.cpp` field readers call (rule 2). */
u32 fn_8006D9FC(void* self, u32 idx);
/* 0x8006D9A4 - the keyed field reader (a sibling of the `g3d_resanmtexsrt.cpp` reader). */
u32 fn_8006D9A4(void* self, void* key);

#ifdef __cplusplus
}
#endif


#endif /* MHTRI_G3D_FN_800680CC_H */
