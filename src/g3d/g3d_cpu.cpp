/*
 * nw4r g3d: g3d_cpu.cpp - the CPU-side display-list copy/fill helpers, `.text` 0x8009A748-0x8009AA78
 * (2 functions, 0x330 B).
 *
 * Naming - which evidence class decided it.  Class 1 decides: the range's own `.data` pool holds the
 * bare source-file name `g3d_cpu.cpp` (lbl_80591860 at 0x80591860), which is the file argument of every
 * `nw4r::db::Panic` assert reached from both bodies.  `langcheck.py --unit g3d/g3d_cpu.cpp` agrees the
 * `.cpp` suffix and the `Panic__Q24nw4r2dbFPCciPCce` relocation are C++ (the front-end mangles the
 * variadic `e` marker), so the unit is `src/g3d/g3d_cpu.cpp` in the existing g3d lib (Wii/1.3,
 * cflags_g3d).  Class 2 fails: `dumpmap.py lookup 0x8009A748` / `0x8009A910` answers the `zz_009a748_`
 * / `zz_009a910_` placeholders, not a name.  Class 3 agrees: every caller of the range is nw4r g3d
 * (fn_80075DCC, g3d_state.cpp, g3d_resfile.cpp and the g3d_resmat band), so the module is `g3d`.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x8009A748`, which answers a `zz_` placeholder, and with
 * config/RMHE08/symbols.txt, whose every `.text` entry in 0x8009A748..0x8009AA78 is a bare
 * `fn_XXXXXXXX`).  The map's stems stand and are used as the identifiers.
 *
 * What the unit is.  Two 32-byte-block memory primitives used to move a display-list buffer around on
 * the CPU: fn_8009A748 copies `size` bytes as 32-byte blocks (each block four 8-byte doubles) and
 * fn_8009A910 fills `size` bytes with the pooled 0.0f constant, one 32-byte block at a time.  Both
 * assert their destination is a valid 4-byte-aligned pointer and that `size` is a multiple of 32.
 *
 * Section claim: `.text` 0x8009A748-0x8009AA78, `extab` 0x800099E0-0x800099F0 (two 8-byte unwind-only
 * records, `@etb_800099E0`/`@etb_800099E8`), `extabindex` 0x80022AAC-0x80022AC4 (two 0xC-byte entries,
 * `@eti_80022AAC`/`@eti_80022AB8`).  The boundaries are the function before (fn_8009A720, which cites a
 * different TU) and after (gx/fn_8009AA78.c at 0x8009AA78); the extab and extabindex ranges abut
 * gx/fn_8009AA78.c's 0x800099F0 / 0x80022AC4 with no gap.
 *
 * Measurement path: registered here for the first time, so MAIN has no split object for the range; the
 * source compiles with the g3d lib's real command line and each symbol is scored with objdiff
 * `report generate` against the retired per-symbol objects `auto_fn_8009A748_text.o` /
 * `auto_fn_8009A910_text.o` under build/RMHE08/obj/.
 *
 * Reconstruction status.  `fn_8009A748` measures 100.00 % (official report metric) and its object is a
 * 456-byte `.text` identical to the target's.  `fn_8009A910` measures 76.00 %: the two bodies are the
 * same 90 instructions and the object's `.text` (0x330), `extab` (0x10) and `extabindex` (0x18) sizes
 * all equal the target's, but the 36 stores that make up both loops differ in their *opcode* alone -
 * the target keeps the f32 fill in the **paired-single** form (`psq_st f0,off,0,qr0`, an 8-byte pair
 * store of a broadcast scalar) where every MWCC in `build/compilers` emits a scalar 8-byte `stfd`
 * (odd halves) or two `stfs` (odd opcode and instruction count).  This is the compiler-build residual
 * the sibling units already document: retail's `.comment` byte is 0x0e versus the 0x0f every available
 * compiler emits, so the retail build's paired-single codegen for scalar floats is unreachable here
 * (see `src/g3d/g3d_resanmchr.cpp` fn_8008B650/B704 and `.pi/notes/800898b0-fn-800898b0-acfb.md`).
 * Evidence that it cannot be a source shape is in this unit's note (`.pi/notes/<branch>.md`): all 33
 * compilers under `build/compilers` were scanned with both the f32 and the f64 fill shapes (none emits
 * `psq_st` for a body store), plus `-O1..-O4,p`, `-opt full/speed`, `-func_align 4/8`, `-vector on`,
 * `-fp spfu/dpfp/efpu`, `-fp_contract off` and `#pragma peephole off/on`.  The landed body is the
 * closest measured variant: the 8-byte block slot of the f64 copy this unit's other half uses, filled
 * from the same pooled 0.0f the target loads with `lfs` (so the load and every offset, base register,
 * counter and branch in the two loops match; only the store mnemonic is stfd instead of psq_st).
 *
 * The file's own panic strings (`g3d_cpu.cpp`, the three assert messages) and the pooled 0.0f constant
 * are unsplit `.data`/`.sdata2` the target references as map symbols; nothing here defines them.
 */

#include "types.h"
#include "g3d/g3d_cpu.h"

/* The target was built with the peephole pass **off**: it keeps the unfused `clrlwi`+`cmpwi` pair for
 * every alignment test where the lib's `-O3` folds them into one record form (`clrlwi.`).  Same lever
 * as the neighbouring gx/fn_8009AA78.c (its own header records it); the pragma scopes it to this unit. */
#pragma peephole off

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker.  Declared in the namespace (C++ linkage), never as the mangled spelling
 * (rule 9); the owner (nw4r::db) is unsplit, so the declaration lives with its consumers. */
namespace nw4r {
namespace db {

void Panic(const char* pFile, int line, const char* pFmt, ...);

} // namespace db
} // namespace nw4r

/* The panic file/format strings the target references as map symbols (unsplit `.data`). */
extern char lbl_80591860[]; /* "g3d_cpu.cpp"                                        .data 0x80591860 */
extern char lbl_8059186C[]; /* "NW4R:Failed assertion pDst && !((u32)pDst & 0x3)"  .data 0x8059186C */
extern char lbl_805918A0[]; /* "NW4R:Failed assertion pSrc && !((u32)pSrc & 0x3)"  .data 0x805918A0 */
extern char lbl_805918D4[]; /* "NW4R:Failed assertion size % 32 == 0"              .data 0x805918D4 */

/* The pooled fill value the fill body loads with `lfs lbl_80795F50@sda21`. */
extern f32 lbl_80795F50; /* 0.0f  .sdata2 0x80795F50 */

/* `NW4R_ASSERT(expr)`: the file is the TU's own name, the message is the literalised expression and the
 * line is baked into the `Panic(file, line, msg)` call - the target passes the original file's line
 * numbers (28/29/30 and 94/95), not this reconstruction's, so they are explicit. */
#define G3D_CPU_ASSERT(expr, line, msg) \
    ((expr) ? (void)0 : nw4r::db::Panic(lbl_80591860, line, msg))

/* Copies `size` bytes from `pSrc` to `pDst` as 32-byte blocks (four 8-byte doubles each - the shape
 * MWCC lowers the block move to: `lfd`/`stfd` pairs, four blocks per unrolled iteration). */
extern "C" void fn_8009A748(void* pDst, const void* pSrc, u32 size) {
    G3D_CPU_ASSERT(pDst && !((u32)pDst & 0x3), 28, lbl_8059186C);
    G3D_CPU_ASSERT(pSrc && !((u32)pSrc & 0x3), 29, lbl_805918A0);
    G3D_CPU_ASSERT(size % 32 == 0, 30, lbl_805918D4);

    f64* dst = (f64*)pDst;
    const f64* src = (const f64*)pSrc;
    size /= 32;
    while (size != 0) {
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        src += 4;
        dst += 4;
        size--;
    }
}

/* Fills `size` bytes at `pDst` with the 0.0f value, one 32-byte block (four 8-byte slots) at a time.
 * The target stores the scalar as four broadcast paired-single pairs (`psq_st`); this build's compiler
 * emits the scalar 8-byte `stfd` form instead - the opcode-only residual the header records. */
extern "C" void fn_8009A910(void* pDst, u32 size) {
    G3D_CPU_ASSERT(pDst && !((u32)pDst & 0x3), 94, lbl_8059186C);
    G3D_CPU_ASSERT(size % 32 == 0, 95, lbl_805918D4);

    f32 value = lbl_80795F50;
    f64* dst = (f64*)pDst;
    size /= 32;
    while (size != 0) {
        dst[0] = value;
        dst[1] = value;
        dst[2] = value;
        dst[3] = value;
        dst += 4;
        size--;
    }
}
