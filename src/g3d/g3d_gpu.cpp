/* g3d_gpu.cpp - the 2 functions at `.text` 0x8009B140..0x8009B374 (564 B) of the discovery proposal
 * `8009B140_fn_8009B140`.
 *
 * Naming - which evidence class decided it.  Class 1 decides: the range's own `.data` pool holds the
 * bare source-file name `g3d_gpu.cpp` (lbl_80591900 at 0x80591900), the `pFile` argument of the
 * `nw4r::db::Panic` assert fn_8009B140 reaches; the `.cpp` suffix and the
 * `Panic__Q24nw4r2dbFPCciPCce` relocation both say C++ (langcheck's evidence kinds `source-cpp` and
 * `mangled-undefined`).  Module `g3d`: `g3d_gpu.cpp` is the `g3d_`-<name> scheme the sibling
 * `src/g3d/g3d_cpu.cpp` uses (0x8009A748..0x8009AA78 - same evidence class, same lib), and the two
 * bodies drive the GX write-gather-pipe writers the g3d/gx bands own.  So the unit is
 * `src/g3d/g3d_gpu.cpp` in the existing g3d lib (Wii/1.3, cflags_g3d).  Class 2 fails:
 * `python tools/symbols/dumpmap.py lookup 0x8009B140` and `0x8009B2CC` answer the `zz_009b140_` /
 * `zz_009b2cc_` placeholders, not a name.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x8009B140`/`0x8009B2CC`, which answer `zz_` placeholders,
 * and with config/RMHE08/symbols.txt, whose every `.text` entry in 0x8009B140..0x8009B374 is a bare
 * `fn_XXXXXXXX`).  The map's stems stand and are used as the identifiers.
 *
 * Section claim.  `.text` 0x8009B140..0x8009B374, `extab` 0x80009A28..0x80009A38 (the two 8-byte
 * unwind-only records `@etb_80009A28`/`@etb_80009A30`) and `extabindex` 0x80022B18..0x80022B30 (the
 * two 0xC-byte entries `@eti_80022B18`/`@eti_80022B24`); both abut the neighbouring registration
 * (`gx/fn_8009ACE4.c` claims extab ..0x80009A28 and extabindex ..0x80022B18), so the ranges are
 * gapless.  The `.data`/`.sdata2` the range references (lbl_80591900, lbl_8059190C, lbl_80795F6C) are
 * *not* claimed: they stay in the unclaimed pool, which is how `src/g3d/g3d_cpu.cpp` treats its own.
 *
 * Seam - recorded, not settled.  The discovery note ("one source file (g3d_gpu.cpp): a candidate seam
 * inside it was not taken") means the range is what the byte cap left of a longer piece: the range's
 * own name starts at fn_8009B140 (its first referrer, so the *left* edge is sound), while the right
 * edge at fn_8009B374 is the cap, not a proven TU end - the next function may belong to the same
 * file.  Nothing in this range settles where g3d_gpu.cpp ends.
 *
 * What the bodies do.  Both are texture-matrix helpers of the g3d GPU path, reached from
 * `fn_80087CCC` (the texgen table builder: it packs an 8-entry table of 6-bit texgen settings and
 * calls fn_8009B140 with it, and fn_8009B2CC once per entry that wants a matrix).
 *   * `fn_8009B140(Array8*)` validates the record pointer against the seven mapped Wii memory ranges
 *     (the same `NW4R:Pointer Error` guard the ef and g3d units use), then packs the record's eight
 *     6-bit settings into two command words: +0x10..+0x1C at bits 0/6/12/18 and +0x00..+0x0C at bits
 *     6/12/18/24 of the second word.  It emits the fixed XF header `fn_8009AC44(0x1018, 2)` and then
 *     the two words through the pipe writer `fn_8009AB1C`.
 *   * `fn_8009B2CC(const Mat33*, u32 id)` expands a stored 3x3 rotation into a 3x4 texture matrix
 *     (the fourth column is the pooled 0.0f) and loads it as texgen matrix `id`
 *     (`GXLoadTexMtxImm(..., id, GX_MTX3x4)`), between the `MTX34_ctor`/`fn_80050508` matrix
 *     begin/end pair the rest of the g3d/ef units use.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit g3d/g3d_gpu.cpp`.
 *
 * Reconstruction status.  Both symbols measure 100.00 % (official report metric,
 * `build/RMHE08/report.json`: 564/564 code bytes, 2/2 functions, 40/40 of the unit's
 * extab+extabindex) and the object's `.text`, `extab` and `extabindex` are byte-identical to the two
 * retired split objects `auto_fn_8009B140_text.o` / `auto_fn_8009B2CC_text.o`, with the same
 * relocation records (12 `.text` records, same offsets/types/symbols; `relocaudit` clean after the
 * `GXLoadTexMtxImm` declaration was given the C linkage the map's plain name requires).  The only
 * object-level difference is the extabindex entries' local extab object names (ours `@66`/`@76`,
 * retail `@etb_80009A28`/`@etb_80009A30`), which do not reach the linked DOL.  Measurement path: the
 * unit is registered for the first time, so MAIN has no split object for the range yet; the source is
 * compiled with the g3d lib's real command line and each symbol scored with
 * `python tools/units/recompile.py g3d/g3d_gpu.cpp --measure <symbol>` against the retired per-symbol
 * objects under build/RMHE08/obj/.
 *
 * Naming inside the two bodies.  The record type takes the name the range's own assert message gives
 * it (`Array8`), which is why the fields carry their packing slot rather than a guessed meaning; the
 * assert macro is the project's proven materialised-BOOL expansion (`g3d/g3d_basic.cpp`,
 * `ef/ef_point.cpp`), and `IsValidPointer`'s seven ranges are spelled out there rather than taken from
 * `include/ef.h`, whose copy is the effect lane's (a second copy of an inline is what the check is
 * for).  `Mat33` is local: only this unit reaches the stored 3x3.
 */

#include "types.h"
#include "nw4r/math.h"      /* nw4r::math::MTX34 */
#include "gx/fn_8009AA78.h" /* fn_8009AB1C / fn_8009AC44 - owner gx/fn_8009AA78.c (rule 2) */
#include "fn_8004CAD8.h"    /* MTX34_ctor / fn_80050508 - owner src/fn_8004CAD8.cpp (rule 2) */

/* `Panic(const char* pFile, int line, const char* pFmt, ...)`; the map's name is the C++ mangling
 * `Panic__Q24nw4r2dbFPCciPCce`, so it is called through its owner, never by the mangled spelling
 * (rule 9).  nw4r::db is unsplit, so the declaration lives with its consumers. */
namespace nw4r {
namespace db {

void Panic(const char* pFile, int line, const char* pFmt, ...);

} // namespace db
} // namespace nw4r

/* SDK GX entry point; the SDK band has no registered owner, so the declaration sits with its consumer.
 * The map name is the plain `GXLoadTexMtxImm`, i.e. C linkage - without `extern "C"` the C++
 * front-end mangles it (`GXLoadTexMtxImm__FPCvUlUl`) and the reference no longer pairs with the
 * target's relocation (relocaudit; the same spelling main.cpp/g3d_camera.cpp use for the GX SDK).  The
 * third argument is `GXTexMtxType` (`GX_MTX3x4 == 0`); only the value reaches the object. */
extern "C" void GXLoadTexMtxImm(const void* pMtx, u32 id, u32 type);

/* The file's own pooled strings and the pooled 0.0f, referenced but never defined here (rule 8.4:
 * the unit does not claim the `.data`/`.sdata2` pools). */
extern char lbl_80591900[]; /* "g3d_gpu.cpp"                                                       .data 0x80591900 */
extern char lbl_8059190C[]; /* "NW4R:Pointer Error\n\tArray8(=%p) is not valid pointer."          .data 0x8059190C */
extern const f32 lbl_80795F6C; /* 0.0f - the texture matrix's translation column                 .sdata2 0x80795F6C */

/* The nw4r resource pointer assert: `ptr` must fall in one of the seven mapped Wii memory ranges.
 * The six materialised BOOLs and the two-test first `if` (`top_` caches the 0xFF000000 test, which the
 * 0xC0000000 test then reuses) are the target's exact shape - the same expansion the sibling
 * `src/g3d/g3d_basic.cpp` and `src/ef/ef_point.cpp` needed.  `line` is the original file's line, which
 * the target bakes into the call (`li r4, 0xff`). */
#define G3D_GPU_POINTER_ASSERT(ptr, line, msg)                                                 \
    {                                                                                          \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;      \
        u32 top_ = (u32)(ptr) & 0xFF000000u;                                                    \
        if (!(top_ == 0x80000000u) && !(((u32)(ptr) & 0xFF800000u) == 0x81000000u))             \
            ok6_ = FALSE;                                                                        \
        if (!ok6_ && !(((u32)(ptr) & 0xF8000000u) == 0x90000000u))                              \
            ok5_ = FALSE;                                                                        \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                    \
            ok4_ = FALSE;                                                                        \
        if (!ok4_ && !(((u32)(ptr) & 0xFF800000u) == 0xC1000000u))                              \
            ok3_ = FALSE;                                                                        \
        if (!ok3_ && !(((u32)(ptr) & 0xF8000000u) == 0xD0000000u))                              \
            ok2_ = FALSE;                                                                        \
        if (!ok2_ && !(((u32)(ptr) & 0xFFFFC000u) == 0xE0000000u))                              \
            ok1_ = FALSE;                                                                        \
        if (!ok1_)                                                                              \
            nw4r::db::Panic(lbl_80591900, line, msg, (ptr));                                    \
    }

/* The eight-setting record fn_8009B140 packs.  Its own assert message names it ("Array8(=%p) is not
 * valid pointer."); the caller `fn_80087CCC` fills the eight words from a per-texgen table (the values
 * it writes - 0, 30, 60 - all fit the six bits the packing leaves each field).  The name of each field
 * is its slot in the two emitted command words: `lo_*` goes to the word at bits 0/6/12/18, `hi_*` to
 * the one at bits 6/12/18/24.
 * size: 0x20 (all eight words are read) */
struct Array8 {
    /* +0x00 */ u32 hi_bits_6;   /* -> second command word bits 6-11  */
    /* +0x04 */ u32 hi_bits_12;  /* -> second command word bits 12-17 */
    /* +0x08 */ u32 hi_bits_18;  /* -> second command word bits 18-23 */
    /* +0x0C */ u32 hi_bits_24;  /* -> second command word bits 24-29 */
    /* +0x10 */ u32 lo_bits_0;   /* -> first command word bits 0-5    */
    /* +0x14 */ u32 lo_bits_6;   /* -> first command word bits 6-11   */
    /* +0x18 */ u32 lo_bits_12;  /* -> first command word bits 12-17  */
    /* +0x1C */ u32 lo_bits_18;  /* -> first command word bits 18-23  */
};

/* The stored 3x3 rotation fn_8009B2CC expands.  The caller `fn_80087CCC` passes fn_80087F08's result
 * (the resource's matrix block) and the body reads all nine floats as one contiguous 0x24 block, so it
 * is a 3x3 and not the 3x4 `MTX34` the expanded form uses.  Only this unit reaches the type, so it
 * stays local (rule 1: a type moves to a header the second time a unit needs it).
 * size: 0x24 */
struct Mat33 {
    /* +0x00 */ f32 m[3][3];
};

extern "C" {

/* Emits the texgen record's eight settings as the two packed command words of the XF `0x1018` state,
 * plus their fixed header.  Caller: fn_80087CCC (0x80087D78), which packs the record it just built. */
void fn_8009B140(Array8* self) {
    G3D_GPU_POINTER_ASSERT(self, 255, lbl_8059190C);

    u32 hi = (self->hi_bits_6 << 6) | (self->hi_bits_12 << 12) | (self->hi_bits_18 << 18)
           | (self->hi_bits_24 << 24);
    u32 lo = self->lo_bits_0 | (self->lo_bits_6 << 6) | (self->lo_bits_12 << 12)
           | (self->lo_bits_18 << 18);

    fn_8009AC44(0x1018, 2);
    fn_8009AB1C(hi);
    fn_8009AB1C(lo);
}

/* Expands the stored 3x3 rotation `pSrc` into a 3x4 texture matrix (the fourth column is the pooled
 * 0.0f) and loads it as texgen matrix `id`.  Caller: fn_80087CCC (0x80087D24), once per record setting
 * that wants a matrix. */
void fn_8009B2CC(const Mat33* pSrc, u32 id) {
    nw4r::math::MTX34 mtx;

    MTX34_ctor(&mtx);
    mtx.m[0][0] = pSrc->m[0][0];
    mtx.m[0][1] = pSrc->m[0][1];
    mtx.m[0][2] = pSrc->m[0][2];
    mtx.m[0][3] = lbl_80795F6C;
    mtx.m[1][0] = pSrc->m[1][0];
    mtx.m[1][1] = pSrc->m[1][1];
    mtx.m[1][2] = pSrc->m[1][2];
    mtx.m[1][3] = lbl_80795F6C;
    mtx.m[2][0] = pSrc->m[2][0];
    mtx.m[2][1] = pSrc->m[2][1];
    mtx.m[2][2] = pSrc->m[2][2];
    mtx.m[2][3] = lbl_80795F6C;

    GXLoadTexMtxImm(fn_80050508(&mtx), id, 0 /* GX_MTX3x4 */);
}

} /* extern "C" */
