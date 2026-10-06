/*
 * g3d/g3d_resanmfog.cpp - nw4r g3d fog animation-channel evaluator fn_8008F6E8 (resolve the object, read the
 *   per-channel inline-value bits, evaluate the near/far float channels and the colour channel at the frame, store
 *   the four result words) and its channel-offset resolver fn_8008F8C8.
 * RANGE. .text 0x8008F6E8-0x8008F8E4 (2 functions); extab, extabindex, .data 0x805903F0-0x80590440 (opens on
 *   "g3d_resanmfog.cpp").  The left edge is the discovery cap; the right edge is `g3d/g3d_resanmlight.cpp`'s first
 *   body, a proven seam.
 * NAMES. Map stems, defined `extern "C"` (the dump answers `zz_` placeholders).
 * RESIDUALS. fn_8008F8C8: retail keeps the base word in r0 (`lwz r0,0(r3); add r3,r0,r4`), ours in r3, the C++
 *   front-end's choice for the one named temporary; a plain-expression return, a result temporary and an inverted
 *   branch all score lower.
 *   flipcheck: `.data` is claimed and not emitted.
 * SHAPES. The channel evaluators take `(self, f32 frame, s32 flag)`: retail schedules the float argument first, and
 *   the ABI-identical `(self, s32 flag, f32 frame)` changes fn_8008F6E8's codegen.
 */

#include "types.h"
#include "nw4r/g3d/res_common.h"          /* IS_VALID_PTR (rule 1) */
#include "g3d/fn_800680CC.h"              /* fn_80068634, owner g3d/fn_800680CC.cpp (rule 2) */
#include "g3d/g3d_anmchr.h"               /* fn_8005B1E4, owner g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/g3d_resanmamblight.h"       /* fn_8008A188/fn_8008A1A8, owner g3d/g3d_resanmamblight.c (rule 2) */
#include "unsplit/g3d.h"                  /* fn_8008A644, owner g3d/g3d_resanmcamera.cpp */

#pragma peephole off
#pragma fp_contract off

/* nw4r::db::Panic - the assert failure handler (variadic).  Called through its namespace owner,
 * never its mangled spelling (rule 9). */
namespace nw4r {
namespace db {

void Panic(const char *file, int line, const char *message, ...);

} /* namespace db */
} /* namespace nw4r */

/* The `.data` `__FILE__`/assert strings the retail body passes by address (this unit's `.data`, claimed, not
 * emitted). */
extern u8 lbl_805903F0[]; /* "g3d_resanmfog.cpp" */
extern u8 lbl_80590404[]; /* "NW4R:Pointer Error\npResult(=%p) is not valid pointer." */

/* The resolved animation resource the fog evaluator reads.  Only the fields this unit touches
 * are named; the record continues past +0x24. */
struct ResAnmFogData {
    /* +0x00 */ u32 unused_0x00;
    /* +0x04 */ u32 frameTableOffset; /* resolved against the object base for the key/frame table */
    /* +0x08 */ u8 pad_0x08[0xC];
    /* +0x14 */ u32 flags;            /* per-channel "inline value" bits (0x20000000/0x40000000/0x80000000) */
    /* +0x18 */ u32 typeWord;         /* copied verbatim into the result word */
    /* +0x1C */ u32 nearValue;        /* near-plane float channel, or an offset to one */
    /* +0x20 */ u32 farValue;         /* far-plane float channel, or an offset to one */
    /* +0x24 */ u32 colorValue;       /* color channel, or an offset to one */
}; /* size: 0x28 - lower bound (the record continues past the last field read here) */

/* The result the fog evaluator fills in. */
struct ResAnmFogResult {
    /* +0x00 */ u32 typeWord;
    /* +0x04 */ f32 near;
    /* +0x08 */ f32 far;
    /* +0x0C */ u32 color;
}; /* size: 0x10 */

/* Forward declarations for the unit's own functions. */
extern "C" s32 fn_8008F8C8(u32 *self, s32 offset);

/* Evaluates the fog animation at `frame` into `pResult`. */
extern "C" void fn_8008F6E8(void *arg0, ResAnmFogResult *pResult, f32 frame)
{
    ResAnmFogData *res;
    u32 flags;
    f32 clamped;

    u32 valid = IS_VALID_PTR(pResult);

    if (!valid) {
        nw4r::db::Panic((const char *)lbl_805903F0, 43, (const char *)lbl_80590404, pResult);
    }
    res = (ResAnmFogData *)fn_80068634(arg0);
    flags = res->flags;
    clamped = fn_8008A1A8((u16 *)(fn_8008F8C8((u32 *)arg0, res->frameTableOffset) + 0x34), frame);
    pResult->typeWord = res->typeWord;
    pResult->near = fn_8008A644(&res->nearValue, frame, (flags & 0x20000000) != 0);
    pResult->far = fn_8008A644(&res->farValue, frame, (flags & 0x40000000) != 0);
    fn_8005B1E4(&pResult->color, fn_8008A188(&res->colorValue, clamped, (flags & 0x80000000) != 0));
}

/* Resolves a sub-resource offset against the object base; a zero offset means "no sub-resource". */
extern "C" s32 fn_8008F8C8(u32 *self, s32 offset)
{
    s32 base = *self;

    if (offset != 0) {
        base += offset;
        return base;
    }
    return 0;
}
