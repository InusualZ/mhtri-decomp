/*
 * nw4r g3d: g3d_resanmfog.cpp - the fog animation-channel evaluator, `.text`
 * 0x8008F6E8-0x8008F8E4 (2 functions, 0x1FC B).
 *
 * Registered once, at its final home (docs/plan.md 12), from the pooled proposal
 * `proposal/8008F6E8_fn_8008F6E8`.  Naming - evidence class 1, a `__FILE__` string:
 * `fn_8008F6E8`'s `nw4r::db::Panic` assert (line 43) passes `.data` 0x805903F0 =
 * "g3d_resanmfog.cpp" (`python tools/symbols/dumpmap.py lookup 0x805903F0`), and the next
 * function `fn_8008F8E4` - outside this range - passes `.data` 0x80590440 =
 * "g3d_resanmlight.cpp".  So the range is exactly the `g3d_resanmfog.cpp` TU and the right
 * edge 0x8008F8E4 is a proven TU seam.  The module is `g3d` (the registered
 * `g3d_resanm*.c` neighbours) and the extension `.cpp` is the `__FILE__` suffix.
 *
 * `fn_8008F8C8` is `fn_8008F6E8`'s own channel-offset resolver (it is referenced from no
 * other object's disassembly but this one), so it is the same TU's second function.
 *
 * Language: C++ - `langcheck.py` says the retail TU is C++ and the bodies are the nw4r g3d
 * resource pattern (the mangled `nw4r::db::Panic` assert, `IS_VALID_PTR` out of
 * `nw4r/g3d/res_common.h`).  The two map symbols carry plain `fn_XXXXXXXX` stems, so they are
 * defined `extern "C"` to keep the bare symbol (playbook 48).
 *
 * Sections: .text 0x8008F6E8-0x8008F8E4, extab 0x80009100-0x80009108 (one unwind-only 8-byte
 * entry for `fn_8008F6E8`, produced by the lib's `-Cpp_exceptions on`) and extabindex
 * 0x80021D5C-0x80021D68 (one 12-byte record).  Both are claimed in splits.txt.  No
 * `.ctors`/`.dtors` word points into the range.
 *
 * Seam: the left edge 0x8008F6E8 is the proposal cap (the preceding TU is unclaimed); the right
 * edge 0x8008F8E4 is `g3d_resanmlight.cpp`'s first body and a proven seam.
 *
 * Naming note: the symbol map has only `fn_XXXXXXXX` for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x8008F6E8` / `0x8008F8C8`: both are bare
 * `zz_XXXXXXXX_` placeholders in the runtime dump too), so there is no real name to recover and
 * stylelint's rule 7 refuses the landing without this line.
 *
 * This is the sibling of `g3d/g3d_resanmamblight.c`'s `fn_8008A000`: resolve the object, read
 * the per-channel "inline value" bits out of the flags word, evaluate the near/far float
 * channels and the color channel at the frame, and store the four result words.
 *
 * Measured (official report metric, `tools/units/recompile.py g3d/g3d_resanmfog.cpp --measure`):
 * fn_8008F6E8 100.00 %, fn_8008F8C8 98.57 %.
 *
 * Residual - fn_8008F8C8: the body is instruction-identical but the base word lands in r3 where
 * the target keeps it in r0 (`lwz r3, 0x0(r3); add r3, r3, r4` here vs `lwz r0, 0x0(r3); add r3,
 * r0, r4` in the target).  It is the C++ front-end's register choice for the one named temporary;
 * the same C-idiom source under the C front-end gives r0 (that is what the C sibling
 * `g3d_resanmamblight.c`'s `fn_8008A204` does), but this TU is a `.cpp` and must stay C++.  Every
 * conformant reshape tried (a plain-expression return, a result temporary, an inverted branch)
 * scores 43-71 %, so the best variant is kept and the register difference is recorded.
 *
 * Argument order (rule 9 discipline): the retail fog call sites schedule the float `frame` argument
 * before the integer flag word, and MWCC evaluates a call's arguments left to right - so the true
 * signatures are `(self, f32 frame, s32 flag)`, not the `(self, s32 flag, f32 frame)` the older
 * `g3d_resanmamblight.c` reconstruction assumed.  The two spellings are ABI-identical (the PPC EABI
 * assigns integer and float arguments to separate register files), so declaring the evidenced order
 * changes only codegen, never the callee; it takes fn_8008F6E8 from 95 % to 100 %.
 */

#include "types.h"
#include "nw4r/g3d/res_common.h"          /* IS_VALID_PTR (rule 1) */
#include "g3d/fn_800680CC.h"              /* fn_80068634, owner g3d/fn_800680CC.cpp (rule 2) */
#include "g3d/g3d_anmchr.h"               /* fn_8005B1E4, owner g3d/g3d_anmchr.cpp (rule 2) */
#include "g3d/g3d_resanmamblight.h"       /* fn_8008A188/fn_8008A1A8, owner g3d/g3d_resanmamblight.c (rule 2) */
#include "unsplit/g3d.h"                  /* fn_8008A644, no owner yet (rule 2) */

#pragma peephole off
#pragma fp_contract off

/* nw4r::db::Panic - the assert failure handler (variadic).  Called through its namespace owner,
 * never its mangled spelling (rule 9). */
namespace nw4r {
namespace db {

void Panic(const char *file, int line, const char *message, ...);

} /* namespace db */
} /* namespace nw4r */

/* The `.data` `__FILE__`/assert strings the retail body passes by address (unsplit `.data`). */
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
