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
#include "nw4r/math.h"

/* The `g3d_calcworld`/`g3d_camera` resource types the returned pointers name; only ever used through a
 * pointer here, so the incomplete type is enough. */
struct G3DWorkObj;
struct RenderModeObj;

#ifdef __cplusplus
extern "C" {
#endif

/* The `enemy` lane's declarations for the same band (its units call these too).  Merged 2026-09-24: two
 * lanes formalized into this one header, and their signatures disagreed only on `fn_8006FDCC`
 * (`u32(const void*)` here vs `s32(void*)`); neither consumer's codegen depends on it (one assigns it to
 * a `u32 mtxID`, the other uses it as an array index), so the verified `const void*` form is kept and the
 * return-type question is recorded rather than silently settled. */

/* 0x80069664..0x800883C4 - the g3d node/resource helpers (callers: enemy/fn_80138074.c).
 * `fn_80069664`'s return type corrected to `s32` when g3d/g3d_resanmlight.cpp became a consumer and uses
 * the word it returns (the target reads `*(u32*)(*self + 4)`); the two existing callers use it as a
 * statement, so the type is source-compatible. */
s32 fn_80069664(void* self);
/* 0x80092330 - the `ResAnmScn` channel-record resolve chain (caller: g3d/g3d_resanmlight.cpp).
 * 0x80463E74 - the global frame thunk that `fn_8008FFFC` tail-calls.  Both are unsplit, so the band
 * header is their home (rule 2).  Added when g3d/g3d_resanmlight.cpp registered. */
s32 fn_80092330(void* self, void* key);
f32 fn_80463E74(void);
void fn_800710BC(Mtx34* out, const Mtx34* a, const Mtx34* b);
void fn_80080B10(void* arg0, u32 arg1);
void fn_800810DC(void* arg0, s32 arg1);
void fn_800883C4(Mtx34* out, const Mtx34* src);
void fn_800834F0(void* p); /* constructs one 8-byte sub-object (ef_particle's parameter record) */

/* 0x8006946C..0x80069768 - the `g3d_resvtx_ac.h` accessor group moved to its owner's header,
 * `include/g3d/fn_800680CC.h`, once `g3d/fn_800680CC.cpp` registered as the owner (rule 2). */

/* 0x80088E84..0x8008918C - the three `GetBaseVtx` shapes the 0x8007270C blend starts from (callers:
 * g3d_calcvtx.cpp).  Each fills the base-vertex pointer and the vertex stride for one resource. */
void fn_80088E84(const void* p, const void** ppBaseVtx, u8* pStride);
void fn_80088FFC(const void* p, const void** ppBaseVtx, u8* pStride);
void fn_8008918C(const void* p, const void** ppBaseVtx, u8* pStride);

/* 0x800731EC..0x80073354 (callers: g3d_calcvtx.cpp).  Moved into the unit's own forward-declaration
 * block once `g3d/g3d_calcvtx.cpp` was registered as their owner (rule 2, 2026-09-24). */

/* 0x8006FDCC..0x8007100C - the `g3d_calcworld` node/resource helpers (callers: g3d_calcworld.cpp). */
u32 fn_8006FDCC(const void* p);
struct G3DWorkObj* fn_8006FF50(void);
s32* fn_80070054(void* pOut, const void* pKey);
void* fn_8007012C(void);
void fn_8007100C(void* pDst, const void* pSrc);

/* 0x80088584 - the render-mode helper (callers: g3d_camera.cpp, ef/ef_drawfreestrategy.cpp).  The
 * 0x80075DCC/0x80075DD8/0x80077DF0 group moved to its owner header `include/g3d/fn_80075DCC.h` once
 * `g3d/fn_80075DCC.cpp` registered (rule 2, 2026-09-25). */
struct RenderModeObj* fn_80088584(void);

/* 0x80082F18 - the frame-round helper (caller: g3d_resanm.c). */
u16 fn_80082F18(f32 value);

/* 0x8007A724/0x8007A5E4/0x8007A5A8 - the 3-float setters moved to their owner header
 * `include/g3d/fn_80075DCC.h` once `g3d/fn_80075DCC.cpp` registered (rule 2, 2026-09-25). */
/* 0x8008A644 - the float analog of `fn_8008A188`: with `flag` set returns the inline float at
 * +0x0 of `self`, else evaluates the referenced float channel (caller: g3d_resanmfog.cpp).  The
 * `(self, f32 frame, s32 flag)` order is the one the retail fog call site schedules (MWCC evaluates
 * arguments left to right); it is ABI-identical to `(self, s32 flag, f32 frame)`.  The owner is the
 * unclaimed 0x8008A28C+ resanm run, so it stays in the unsplit band for now. */
f32 fn_8008A644(u32 *self, f32 frame, s32 flag);

/* 0x800868A0 - the pipe-command writer (caller: gx/fn_8009AA78.c).  fn_80077420 moved to its owner
 * header `include/g3d/fn_80075DCC.h` (rule 2, 2026-09-25). */
void fn_800868A0(u32 value);


/* Merged 2026-09-24: a second lane formalized into this shared header.  Declarations it
 * needed that the first did not; a symbol both named keeps the first (verified) signature. */
s32 fn_80082BCC(s32 model);
void fn_8007F0CC(s32 root, u32 id);

/* 0x8007A510 - the softreset/return-to-title request (caller: src/mh3_pad.cpp). */
void fn_8007A510(void);

/* 0x8079124C - the `.sdata` word the `g3d_state.cpp` cluster hands back the address of (caller:
 * g3d/g3d_state.cpp).  A data symbol with no registered owner, so the band is its home (rule 2).
 * The `fn_8007B5F4`/`fn_8007BB8C` lookups that were declared here moved to their owner's header
 * `include/g3d/fn_80075DCC.h` when `g3d/fn_80075DCC.cpp` registered (rule 2, 2026-09-25). */
extern u32 lbl_8079124C;

/* 0x80092584..0x80093690 - the ten resource-category count/item accessors of the `ResFile`
 * container (0x80092588/84, 0x80092B10/0C, 0x8009284C/48, 0x80092CC4/C0, 0x80092E78/74,
 * 0x8009302C/28, 0x800931E0/DC, 0x80093394/90, 0x80093548/44, 0x80093690/8C).  The band sits between
 * the registered `g3d/g3d_resanmfog.cpp` and `g3d/g3d_resfile.cpp` (both `g3d`), so it is a sound
 * g3d-module home (rule 2).  Each pair is `<category> count(self)` and `<category> item(self, i)`. */
u32 fn_80092588(void* p);
void* fn_80092584(void* p, u32 i);
u32 fn_80092B10(void* p);
void* fn_80092B0C(void* p, u32 i);
u32 fn_8009284C(void* p);
void* fn_80092848(void* p, u32 i);
u32 fn_80092CC4(void* p);
void* fn_80092CC0(void* p, u32 i);
u32 fn_80092E78(void* p);
void* fn_80092E74(void* p, u32 i);
u32 fn_8009302C(void* p);
void* fn_80093028(void* p, u32 i);
u32 fn_800931E0(void* p);
void* fn_800931DC(void* p, u32 i);
u32 fn_80093394(void* p);
void* fn_80093390(void* p, u32 i);
u32 fn_80093548(void* p);
void* fn_80093544(void* p, u32 i);
u32 fn_80093690(void* p);
void* fn_8009368C(void* p, u32 i);
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus /* C++-only: the `const u32&` cannot be spelled in C, but the map name is plain
                    * (C linkage) and the target object references `fn_8006F304`, not the mangled
                    * `__FPvRCUl`; `extern "C"` on this one declaration keeps the reference parameter
                    * (load-bearing for the caller's stack layout - see eft002.cpp's file header) while
                    * emitting the plain symbol. */
extern "C" void fn_8006F304(void* dst, const u32& src);
#endif

#endif /* MHTRI_UNSPLIT_G3D_H */
