/*
 * The `g3d`-module symbols whose owning units are not reconstructed yet (docs/plan.md 6.5 rule 2).
 *
 * A symbol in a not-yet-reconstructed region is declared once, in `include/unsplit/<module>.h`, grouped
 * by the module whose registered units bracket its address.  These addresses all fall between registered
 * `g3d` units, so they live here; the consumers (`g3d/g3d_calcvtx.cpp`, `g3d/g3d_calcworld.cpp`,
 * `g3d/g3d_camera.cpp`, `g3d/g3d_resanm.c`, `gx/fn_8009AA78.c`) include this header instead of
 * re-declaring them.  Keep it minimal: only the declarations a consumer needs.
 *
 * The functions are C-linkage (their map names are plain `fn_XXXXXXXX`), so the declarations are
 * `extern "C"` for the C++ consumers and plain for the C ones.
 */
#ifndef MHTRI_UNSPLIT_G3D_H
#define MHTRI_UNSPLIT_G3D_H

#include "types.h"

/* The `g3d_calcworld`/`g3d_camera` resource types the returned pointers name; only ever used through a
 * pointer here, so the incomplete type is enough. */
struct G3DWorkObj;
struct RenderModeObj;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800696E4..0x80069754 - the `g3d_resvtx_ac.h` accessor group (callers: g3d_calcvtx.cpp). */
void* fn_800696E4(const void* p);
const char* fn_80069748(void);
s32 fn_80069754(const void* p);

/* 0x800731EC..0x80073354 (callers: g3d_calcvtx.cpp). */
void fn_800731EC(void* p, u32 v);
void* fn_800732F0(const void* p);
const char* fn_80073354(void);

/* 0x8006FDCC..0x8007100C - the `g3d_calcworld` node/resource helpers (callers: g3d_calcworld.cpp). */
u32 fn_8006FDCC(const void* p);
struct G3DWorkObj* fn_8006FF50(void);
s32* fn_80070054(void* pOut, const void* pKey);
void* fn_8007012C(void);
void fn_8007100C(void* pDst, const void* pSrc);

/* 0x80075DCC/0x80075DD8/0x80088584 - the trig and render-mode helpers (callers: g3d_camera.cpp). */
void fn_80075DCC(f32* pOutSin, f32* pOutCos, f32 angle);
void fn_80075DD8(void* p);
struct RenderModeObj* fn_80088584(void);

/* 0x80082F18 - the frame-round helper (caller: g3d_resanm.c). */
u16 fn_80082F18(f32 value);

/* 0x800868A0/0x80077420 - the pipe-command writers (caller: gx/fn_8009AA78.c). */
void fn_800868A0(u32 value);
void fn_80077420(u16 command, u8 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_G3D_H */
