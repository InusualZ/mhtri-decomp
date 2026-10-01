/* gx/fn_8009ACE4.c - the 2 functions at .text 0x8009ACE4..0x8009B140.
 *
 * Phase 4: the reconciled candidate gives this unit a further `.sdata2` 0x10 (the pool the two bodies read) that the source neither
 * defines nor claims, so the row is demoted from `Matching` to `NonMatching` until the data is emitted (flipcheck refuses a differing section).
 *
 * Registration (brief section 2).  Module, evidence class 3 (what the code does plus the naming scheme
 * of its neighbours): the range is the immediate continuation of `gx/fn_8009AA78.c`, reads the same
 * `.sdata2` prompt constants and emits through the same write-gather-pipe writers (`fn_8009AC98`/
 * `AC44`/`AB1C` owned by that unit, `fn_800868A0` from `include/unsplit/g3d.h`), so it is registered in
 * the same `gx` lib block and directory.  Name, evidence class 4: the map has only the `fn_XXXXXXXX`
 * stem for both symbols and the runtime dump resolves only `zz_`, so the stem is kept and no name is
 * invented.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x8009ACE4` and 0x8009B0D8: both are bare `.text` entries in
 * config/RMHE08/symbols.txt, and the dump resolves only the `zz_` placeholder).
 *
 * `fn_8009ACE4` normalizes the record's six floats (offsets 0/4/8 and 16/20/24; element 12 is not read)
 * to one shared power-of-two exponent, then writes three BP opcode-0x61 words through `fn_800868A0`,
 * each packing two 11-bit magnitudes and the exponent bits under the command byte `base + 6/7/8`.
 * `fn_8009B0D8` emits the fixed CP array-base words (regs 0x30/0x40) and the XF 0x10 load, then the two
 * payload words.
 *
 * Match: both symbols measure 100.00 % (official report metric) and the object's `.text` is
 * byte-identical to the two retired split objects (auto_fn_8009ACE4_text.o + auto_fn_8009B0D8_text.o,
 * 1116 B) with the same relocation records; `extab` (16 B) and `extabindex` (24 B) match too.  The only
 * object-level difference is the local extab object names (ours `@108`/`@114`, retail `@etb_80009A18`/
 * `@etb_80009A20`), which do not reach the linked DOL.
 *
 * Codegen levers:
 *   * `#pragma peephole off` - the same setting `gx/fn_8009AA78.c` needs.  With the peephole on, the
 *     `clrlwi`/`slwi` pairs fuse and `fn_8009ACE4` drops to 82.46 %.
 *   * The normalize loop is a `do/while` whose `exp >= 46` bound is an inner `break`.  Written as
 *     `while (exp < 46) { ...; if (all < 1.0f) break; }`, MWCC makes it a counted `mtctr`/`bdnz` loop and
 *     unrolls it twice (87.02 %, 0x4A8 B), which the target does not do.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit gx/fn_8009ACE4.c`.
 */

#include "types.h"
#include "fn_8004CAD8.h"    /* fn_8005220C - the `fabs` helper this range's owner declares (rule 2) */
#include "gx/fn_8009AA78.h" /* fn_8009AB1C/AC44/AC98, owned by gx/fn_8009AA78.c (rule 2) */
#include "unsplit/g3d.h"    /* fn_800868A0, the 0x61 BP word writer (rule 2, unsplit) */

/* The `.sdata2` floats this function reads.  The band has no registered owner (only `main.cpp` owns a
 * `.sdata2` range, 0x80795AA0-0x80795AD8), so rule 2 leaves them in the unsplit gap and they are
 * declared here. */
extern const f32 lbl_80795F5C; /* 1.0f */
extern const f32 lbl_80795F60; /* 0.5f */
extern const f32 lbl_80795F64; /* 2.0f */
extern const f32 lbl_80795F68; /* 1024.0f */

#pragma peephole off

/* Normalizes the record's three column pairs to a shared power-of-two exponent and writes them as
 * three packed BP words: two 11-bit magnitudes plus two exponent bits per word. */
void fn_8009ACE4(int base, const f32* v) {
    s8 exp = 0;
    f32 f28 = v[0];
    f32 f27 = v[1];
    f32 f31 = v[2];
    f32 f26 = v[4];
    f32 f30 = v[5];
    f32 f29 = v[6];
    f32 f25 = fn_8005220C(f28);
    f32 f24 = fn_8005220C(f27);
    f32 f23 = fn_8005220C(f31);
    f32 f22 = fn_8005220C(f26);
    f32 f21 = fn_8005220C(f30);
    f32 f1 = fn_8005220C(f29);

    if (f25 >= lbl_80795F5C || f24 >= lbl_80795F5C || f23 >= lbl_80795F5C || f22 >= lbl_80795F5C
        || f21 >= lbl_80795F5C || f1 >= lbl_80795F5C) {
        do {
            if (exp >= 46) {
                break;
            }
            exp += 1;
            f28 *= lbl_80795F60;
            f27 *= lbl_80795F60;
            f31 *= lbl_80795F60;
            f26 *= lbl_80795F60;
            f30 *= lbl_80795F60;
            f29 *= lbl_80795F60;
            f25 *= lbl_80795F60;
            f24 *= lbl_80795F60;
            f23 *= lbl_80795F60;
            f22 *= lbl_80795F60;
            f21 *= lbl_80795F60;
            f1 *= lbl_80795F60;
        } while (f25 >= lbl_80795F5C || f24 >= lbl_80795F5C || f23 >= lbl_80795F5C
                 || f22 >= lbl_80795F5C || f21 >= lbl_80795F5C || f1 >= lbl_80795F5C);
    } else if (f25 < lbl_80795F60 && f24 < lbl_80795F60 && f23 < lbl_80795F60 && f22 < lbl_80795F60
               && f21 < lbl_80795F60 && f1 < lbl_80795F60) {
        do {
            exp -= 1;
            f28 *= lbl_80795F64;
            f27 *= lbl_80795F64;
            f31 *= lbl_80795F64;
            f26 *= lbl_80795F64;
            f30 *= lbl_80795F64;
            f29 *= lbl_80795F64;
            f25 *= lbl_80795F64;
            f24 *= lbl_80795F64;
            f23 *= lbl_80795F64;
            f22 *= lbl_80795F64;
            f21 *= lbl_80795F64;
            f1 *= lbl_80795F64;
        } while (f25 < lbl_80795F60 && f24 < lbl_80795F60 && f23 < lbl_80795F60 && f22 < lbl_80795F60
                 && f21 < lbl_80795F60 && f1 < lbl_80795F60 && exp > -17);
    }

    exp += 17;

    fn_800868A0(((base + 6) << 24) | ((exp & 3) << 22)
                | (((s32)(lbl_80795F68 * f28) & 0x7FF)
                   | (((s32)(lbl_80795F68 * f26) & 0x7FF) << 11)));
    fn_800868A0(((base + 7) << 24) | (((exp >> 2) & 3) << 22)
                | (((s32)(lbl_80795F68 * f27) & 0x7FF)
                   | (((s32)(lbl_80795F68 * f30) & 0x7FF) << 11)));
    fn_800868A0(((base + 8) << 24) | (((exp >> 4) & 3) << 22)
                | (((s32)(lbl_80795F68 * f31) & 0x7FF)
                   | (((s32)(lbl_80795F68 * f29) & 0x7FF) << 11)));
}

/* Emits the fixed CP array-base and XF setup this record type needs, then the two payload words. */
void fn_8009B0D8(void) {
    fn_8009AC98(0x30, 0x3CF3CF00);
    fn_8009AC98(0x40, 0x00F3CF3C);
    fn_8009AC44(0x1018, 2);
    fn_8009AB1C(0x3CF3CF00);
    fn_8009AB1C(0x00F3CF3C);
}
