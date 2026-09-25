/*
 * The `g3d/fn_80075DCC.cpp` cluster's cross-unit declarations (docs/plan.md 6.5 rule 2).  A symbol a
 * registered unit owns is declared once, in that owner's header, and every consumer includes it; this
 * is that header for the nw4r g3d render/dispatch cluster registered from proposal `80075DCC`
 * (`.text` 0x80075DCC-0x8007C540).
 *
 * The seven plain-`fn_XXXXXXXX` symbols below used to sit in `include/unsplit/g3d.h`, the fallback
 * band for a g3d-module symbol with no registered owner.  Registering the cluster makes them owned, so
 * the declarations move here and the consumers (`g3d/g3d_basic.cpp`, `g3d/g3d_camera.cpp`,
 * `gx/fn_8009AA78.c`, `ef/ef_drawfreestrategy.cpp`, `ef/ef_drawstrategyimpl.cpp`,
 * `g3d/fn_80063888.cpp`) include this header instead.  The transfer itself is recorded in the outbox
 * as a `shared-file` request, because it edits files (`include/unsplit/g3d.h` and the consumers) that
 * the batch applies together.
 *
 * All of them carry the map's own `fn_XXXXXXXX` stem, so they keep C linkage.
 */
#ifndef MHTRI_G3D_FN_80075DCC_H
#define MHTRI_G3D_FN_80075DCC_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80075DCC - a one-line thunk: `fn_80500EF4(lbl_80795DFC * angle)`.  The callers pass a sin/cos
 * out-pair plus an angle; the shipped body reads only the angle. */
void fn_80075DCC(f32* pOutSin, f32* pOutCos, f32 angle);
/* 0x80075DD8 - sets up the camera projection for `p`. */
void fn_80075DD8(void* p);

/* 0x80077420 - the GX pipe-command writer (caller: gx/fn_8009AA78.c). */
void fn_80077420(u16 command, u8 value);

/* 0x80077DF0 - assembles an MTX34 from twelve floats (the first eight in FPRs, the last four on the
 * stack).  Callers: ef/ef_drawfreestrategy.cpp, ef/ef_drawstrategyimpl.cpp. */
void fn_80077DF0(Mtx34* dst, f32 m00, f32 m01, f32 m02, f32 m03,
                 f32 m10, f32 m11, f32 m12, f32 m13,
                 f32 m20, f32 m21, f32 m22, f32 m23);

/* 0x8007A5E4/0x8007A5A8/0x8007A724 - the 3-float setters (caller: g3d/fn_80063888.cpp).  The object
 * arrives in r3 and the three values in f1-f3. */
void fn_8007A5E4(void* self, f32 x, f32 y, f32 z);
void fn_8007A5A8(void* self, f32 x, f32 y, f32 z);
void fn_8007A724(void* self, f32 x, f32 y, f32 z);

/* 0x8007B5F4/0x8007BB8C - the ScnRoot state lookups `g3d/g3d_state.cpp` calls (rule 2, moved out of
 * include/unsplit/g3d.h when this unit registered, 2026-09-25).  `fn_8007BB8C` stores what it finds
 * through its out-parameter and returns that parameter; `fn_8007B5F4` registers `pKey` under the
 * state object. */
u32 fn_8007B5F4(void* pSelf, const u32* pKey);
void** fn_8007BB8C(void** pOut, const char* pName);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_FN_80075DCC_H */
