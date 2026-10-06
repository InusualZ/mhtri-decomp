/*
 * stage/stg_w.cpp - unit, `.text` 0x802AD9C0..0x802B5640 (131 functions, 31872 bytes).
 *
 * Fold of 3 registered units: stg_w.cpp, fn_802B2978.c, fn_802B2AA0.cpp.  The
 * functions below are the ones those sources define, in address order; every other function of the range keeps its
 * original bytes.  75 of 131 functions have a body here.
 *
 * FLAGS.  `cflags_main` for every absorbed source (stage lib); `#pragma peephole off` is carried per piece (the stage
 * band is peephole-off).
 *
 * RESIDUAL (record views).  The absorbed sources were written as separate units and each carries its own header view
 * of the records they share; where two views disagree on a record layout or a prototype of one extern "C" symbol they
 * are kept apart in a namespace (extern "C" names stay unmangled, so the symbols are unchanged) instead of being
 * unified by guess.  Unifying them (one record header, one prototype per symbol) is the open work and removes the
 * namespace.  Here: `view_fn_802B2AA0` holds the head of the old stage/fn_802B2AA0.cpp with `stage/fn_802B2AA0.h` and
 * `stage/stg_w.h`, whose prototypes (stage_map_kind_get, stage_water_enabled_ck, fn_802B04A0, ...) disagree with this
 * file's own definitions.
 *
 * `fn_802B2978` was a `.c` unit; it is defined extern "C" here.
 *
 * Sections: the unit's block in config/RMHE08/splits.txt (.bss, .ctors, .data, .sbss, .sdata, .sdata2, .text, extab,
 * extabindex).
 * NAMES. GUESS (from each body and its callers): stage_entry_angle_get, stage_cell_get, stage_gate_a_ck
 *   GUESS: stage_gate_b_ck
 */
/* ---- header inherited from src/stage/stg_w.cpp (written against its pre-phase-4 range) ---- */
/*
 * stage/stg_w.cpp - the stage-work block and its accessors.
 *
 * `.text` 0x802AD9C0-0x802B2978 (98 functions, 20408 B).
 *
 * Module `stage`.  The lib and the sibling below are `stage/` (`stage/fn_802B2978.c`), and the
 * range's own entry points name the subsystem: `get_stg_w` hands back the stage-work block `stage_w`
 * at 0x806B87C0, and `get_stg_weapon_work` / `get_stg_eft_col` / `get_now_mapno` / `get_now_areano`
 * are its accessors.  Language C++: the map defines six real manglings (`get_stg_w__Fv`,
 * `get_now_areano__Fv`, ...) and the range calls out through mangled names (`Pl_master_ck__FP4_PLW`,
 * `body_set__FP7_BODY_WP10_BODY_DATAUcUlUc`).
 *
 * Name.  No `__FILE__` string covers this range (the `menu_item.cpp` string at 0x805CDFC8 is
 * referenced only by the preceding range, at 0x802A54C0/0x802A5900/0x802A64DC) and the shared
 * runtime dump answers only `zz_XXXXXXXX_` for the range's unnamed functions, so the file name is
 * class 3 of brief section 2: what the code does plus the neighbours' scheme - the block the range
 * is built around is the stage (`stg`) work record.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for 92 of this range's 98 symbols (checked
 * with `python tools/symbols/symedit.py range 0x802AD9C0 0x802B2978` and
 * `python tools/symbols/dumpmap.py lookup` over the inventory: every unnamed entry is a bare
 * `fn_XXXXXXXX` in config/RMHE08/symbols.txt and the runtime dump answers `zz_XXXXXXXX_`); the six
 * named symbols are get_stg_w / get_stg_weapon_work / get_stg_eft_col / get_now_mapno /
 * get_now_areano / get_worldworld_pos.
 *
 * The stage-work block.  `stage_w` (map symbol, .bss 0x806B87C0, 0x2FE0 B) is a flat arena: a
 * pointer table at +0x04, the current map/area bytes at +0xBC5/+0xBC6, one 476-byte area record per
 * area from +0xC20, and scalar tails past the record array.  `StageWork`/`StageArea` mirror the
 * offsets the range touches; the two table-indexing shapes (`areas[areano].rows[i]`, stride 0x1DC /
 * 0x30) are what the disassembly computes, so they are kept verbatim.
 *
 * Inventory and evidence: `python tools/units/ledger.py unit stage/stg_w.cpp`.
 */
/* ---- header inherited from src/stage/fn_802B2978.c (written against its pre-phase-4 range) ---- */
/*
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 * One function: `fn_802B2978`, a byte-wise blend of two packed 32-bit colours.
 * `.text` 0x802B2978-0x802B2AA0 (296 B, the only function in the range).
 *
 * Per channel (bits 24, 16, 8, 0) it computes `from_byte + (int)(t * (float)(s16)(to_byte - from_byte))`,
 * masks the result to a byte and repacks the four channels in their original order.  It calls nothing and
 * reads no memory, so besides the function itself the only relocation is the compiler's implicit
 * int->float magic constant.  `(s16)` is load-bearing: without the sign-extended difference the object is
 * 8 instructions off (90.81 %).
 *
 * Flags: the unit builds with `cflags_main` (`auto`, `Wii/1.3`, `-O3 -inline noauto`), whose peephole is
 * on, but retail's codegen is the peephole-off one - 0x43300000 is materialised twice instead of once,
 * `srwi`+`clrlwi` stays unfused instead of becoming `rlwinm`, and the return value is built from
 * `slwi`/`or` instead of `rlwimi`.  Measured on the real command line: `-opt nopeephole` -> 99.93 %,
 * peephole on -> 75.27 %.  The neighbouring `fn_802B2AA0` (another unit) shows the same duplicated `lis`,
 * so the whole original TU was built peephole-off and a per-region `cflags` group is the real fix; the
 * `#pragma peephole off` / `reset` pair below is the source-level stand-in until that exists.
 *
 * Residual: 73 of 74 rows match.  The one that does not is `lfd f2, <magic>@sda21` - retail loads
 * `lbl_8079A460`, ours loads the pool entry MWCC names `@NN`.  That constant is the implicit int->float
 * magic, which cannot be named from source (playbook 29), so it is the residual and the unit's `.sdata2`
 * range stays unclaimed.  Measured (data-claim lane): `lbl_8079A460` is loaded by `stage/fn_802B2AA0.cpp`
 * and `stage/stg_w.cpp` as well, so the entry belongs to the one original TU those three fragments come
 * from and a claim would leave the unit's flip unlinkable (`undefined: 'lbl_8079A460'`).
 *
 * Name: still generated.  The runtime dump has only `zz_02b2978_` for it and the neighbours carry no
 * naming scheme, so no evidenced name exists; the two-edit rename rides a rename batch when one does.
 *
 * Inventory and evidence: `python tools/units/ledger.py unit auto/802B2978_fn_802B2978.c`.
 */
/* ---- header inherited from src/stage/fn_802B2AA0.cpp (written against its pre-phase-4 range) ---- */
/*
 * stage/fn_802B2AA0.cpp - the stage band's per-area runtime state: the `stage_w` block flags/timers,
 * the two 0x4F8-byte per-area objects and the area colour/effect drivers that read them.
 *
 * `.text` 0x802B2AA0-0x802B5C58 (39 functions, 12728 B), the run right after `stage/fn_802B2978.c`.
 * Registered once, at its final home (docs/plan.md 12).
 *
 * Name.  Module `stage`: the left neighbour is `stage/fn_802B2978.c`, the lib this file sits in is
 * `stage`, and the range's own entry points are the stage-work block (`stage_w`, .bss 0x806B87C0,
 * materialised as `0x806C0000-0x7840` in the prologues) and the two per-area objects at
 * `lbl_806BB7E0`.  No `__FILE__` string covers the range (the `menu_item.cpp` literal at 0x805CDFC8 is
 * referenced only by the band *below* 0x802B2AA0) and the runtime dump answers only `zz_XXXXXXXX_`, so
 * the file keeps the map's own stem (class 4, brief section 2).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/symedit.py range 0x802B2AA0 0x802B5C58`: every entry is a bare
 * `fn_XXXXXXXX = .text:0x802BXXXX;` line, and `python tools/symbols/dumpmap.py lookup <addr>` answers
 * the `zz_XXXXXXXX_` placeholder for each).
 *
 * Language C++.  The map's undefined set carries real manglings (`get_camera_pos__Fv`,
 * `setVisibility__6MHcharFUlb`, `calcVecAngXY__FPQ34nw4r4math4VEC3PUlPUl`,
 * `shell_se_req__FP5_se_wPQ34nw4r4math4VEC3UcUl`, `Pl_master_ck__FP4_PLW`, `get_now_mapno__Fv`) and
 * the retail object carries extab/extabindex, which a no-exceptions C unit could not.
 *
 * The band's types, its own entry points and the callee declarations live in
 * `stage/fn_802B2AA0.h` (docs/plan.md 6.5 rules 1-5); this file is the bodies.
 *
 * Sections: .text 0x802B2AA0-0x802B5C58; extab 0x80013D34-0x80013E3C (33 8-byte records - the
 * extabindex table has one entry per function that contains a `bl`); extabindex
 * 0x80031A4C-0x80031BD8 (33 x 12 B).  The six functions with no record (fn_802B2F3C, fn_802B3250,
 * fn_802B3260, fn_802B45BC, fn_802B45D4, fn_802B46DC) are exactly the ones with no call at all.
 *
 * Data.  `.data` 0x805CF56C-0x805CFBE8 (1660 B) is claimed: the 23-entry switch table MWCC emits for
 * fn_802B3270's `switch (st->mapno)` (0x805CF728, 92 B, arms read out of `main.elf`) sits inside it, and the
 * rest is the band's hand-written tables, which our object does not emit yet (the target object carries
 * 1660 B of `.data`, ours 92 B, so the unit's data row reads 0 % until they are reconstructed).  Every
 * pooled constant is `extern`-declared and never defined (playbook 29).
 *
 * `.sbss` 0x80794B60-0x80794B68 is claimed for `shell_set_func_ptr` (the shell-set job table pointer, typed
 * in `stage/shell_set_func_ptr.h`; 111 functions in 38 units read it, nothing in the DOL stores it, an
 * RSO does).  Definer: LOW confidence (about one in three) - `.sbss` follows the text order of the defining
 * TUs, which brackets the word between Pl/fn_8028F66C|fn_80295EF4 (0x80794B58, a `u8`) and light/light.cpp
 * (0x80794B68), i.e. any of menu_item, menu_message, stage/shell, Pl/pl_yure, stage/stg_w, this unit or
 * camera/fn_802B5C58; none of them stores it, and stg_w and this unit are the only ones of those that
 * read it.  This unit is taken as the definer because it is the stage band's shell-effect driver and
 * already owned the declaration.  The claim is the map's 8-byte row: our object emits the pointer only, so
 * `.sbss` is 4 B against the target's 8 B (the word at 0x80794B64 is read by nothing).
 *
 * Flags.  The whole band is peephole-OFF, measured rather than inferred: with the lib's `-O3` peephole
 * on, MWCC restores a saved `f32` with `psq_l f31,N(r1)` where retail has the unfused `li r0,N` +
 * `psq_lx f31,r1,r0,0,0`; the probe is the same four-instruction function with `-opt nopeephole`
 * added.  `#pragma peephole off` (playbook 39) reproduces retail's prologue/epilogue pair, and the
 * sibling `stage/fn_802B2978.c` and `stage/stg_w.cpp` already carry the same pragma for the same
 * reason.  A lib-level flag is the durable home; until that lands the pragma pair carries the
 * deviation here.
 *
 * Shapes.  fn_802B3270's list writes are `field[i] = v; i++;` through the *struct field*, not
 * `buf[i++] = v` through the array: retail re-loads `local.show`/`local.hide` off the stack before
 * every store, which only the field spelling produces.  Its dispatches come in both lowerings - the
 * three on `fn_802FB8EC`'s result are `switch`es (one contiguous compare chain, cases merged into a
 * `cmplwi/ble` range test where two share a body) while the `LbCheckKujiraEvent`/`fn_802FB9F8` tests
 * are `if`/`else if` (chain interleaved with the bodies); a rewrite to the other lowering costs
 * ~7 points.  `fn_802FB9F8` returns `u32`, not `u8` (retail compares `r3` raw, with no `clrlwi`).
 *
 * Residuals (measured 2026-09-26, real command line):
 *   - fn_802B414C 36.50 % (320 B): the two effect-strip loops; ours is 332 B, so the loop shape (the
 *     `for(;;)` pair the target's `beq`-to-epilogue implies) still differs.
 *   - fn_802B4E58 43.65 % (1284 B): the colour-cycle blend; ours is 1324 B.  The final
 *     `fn_80057DE0(0, 6, colour, lbl_8079A4C4)` call and the `psq_lx` prologue/epilogue pair are right,
 *     so the residual is the inline per-channel blend shape.
 *   - fn_802B2F60 77.56 % (752 B, ours 812 B): source shape.
 *   - fn_802B4ABC 78.64 % (416 B, ours 412 B): the condition order still differs.  Retail tests
 *     `now - base`'s `& 0x40` first and puts both `field_0x2FDC = 0` tails at the *end* of the function
 *     (0x2188 / 0x2194), while ours emits one of them inline; and retail compares the map number with
 *     `cmpwi` where MWCC emits `cmplwi` for the same `mapno != 5 && mapno != 0x10 && mapno != 0xA`
 *     (both the `s32`/`int`/`s8` spelling and a `switch` give `cmplwi`).  Its *semantics* are now
 *     right: `entry` is `p->offsets` (a sub-table of 8-byte records), which the earlier reconstruction
 *     read as `entry = p` - the outer table and the sub-table share the record layout.
 *   - The claimed jump table's two relocations still pair by *value*, not by name: retail's split names
 *     the table `jumptable_805CF728` where MWCC emits an anonymous `@NNNN` in our object.  The bytes
 *     and the section size are equal (100.0), so this is cosmetic - but it is why the object is not
 *     byte-identical to the target yet.
 *   - Every int->float conversion gets MWCC's own anonymous `.sdata2` slot where the split names
 *     `lbl_8079A470` (2^52) / `lbl_8079A460` (2^52 + 2^31); that constant cannot be named from source
 *     (playbook 29) and it is the same residual `fn_802B2978` carries.
 *   - The unit's `.ctors` word (0x8056F37C, dtk appended the range in the re-split) is not emitted by
 *     our object; no static object with a constructor exists in the reconstruction yet.
 *
 * `fn_802B2F60` and `fn_802B4ABC` keep their pre-existing source shapes.  Three rows that sat below the
 * bar are 100 % on shape alone, and each one's shape is load-bearing:
 *   - fn_802B3270: new here; its call site (`fn_802B4C5C`, which must pass all three arguments) moved
 *     that row 95.69 -> 96.38.
 *   - fn_802B4824: the outer loop must be a `for` with `index` initialised outside it (`for (; index <
 *     4U; index++)`), not a `do`/`while` - only a for-counter gets the range analysis that drops the
 *     `clrlwi` off `index < 4U`; and the per-area body re-reads `st->area_char[index]` (retail loads
 *     the element address once per iteration and reloads the pointer after `frame_init`).  The
 *     declarations are ordered `scale, amount, index, joint, st` because the allocator colours them by
 *     declaration order (98.50 % in the natural order) - a deliberate deviation from the file's style.
 *   - fn_802B45D4: `(x & 1) != 0`, not `!(x & 1)` - the `!` spelling makes MWCC emit the `cntlzw`/`srwi`
 *     pair where retail has `neg`/`or`/`srwi`, and the peephole is off in this band already, so the
 *     asymmetry is source, not flag.
 *
 * Inventory and evidence: `python tools/units/ledger.py unit stage/fn_802B2AA0.cpp`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "g3d/g3d_resmat.h"             /* nw4r::g3d::ResMdl (rule 2) */
#include "quest/quest_item_slot.h"   /* quest_id_head_ck, quest_id_tail_ck (rule 2) */
#include "lobby/lb_quest_screen.h"   /* the quest-work time and clock accessors (rule 2) */

#include "ef.h"
#include "g3d/g3d_calcview.h"

/* The stage-work block (map symbol, 0x806B87C0).  Never defined here (playbook 29): the object
 * emits only the references and `get_stg_w` is its accessor. */
extern u8 stage_w[];

/* The unit's pooled data the map already names (playbook 29). */
extern const u8 lbl_805CED40[];  /* id remap table, 0x18 B */
extern const u8 lbl_805CF038[];  /* id pair table, 0x18 B */
extern const f32 lbl_8079A480;
extern const f32 lbl_8079A47C;
extern const f32 lbl_8079A484;
extern const f32 lbl_8079A498;

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* The 48-byte row chunked as three VEC3s from +0x0C (fn_802B02C4's view). */
typedef struct StageAreaVecs {
    /* +0x00 */ u32 pad_0x00[3];
    /* +0x0C */ VEC3 vecs[3];
} StageAreaVecs; /* size: 0x30 */

/* One 48-byte row of an area record: 12 words, also read as three 12-byte VEC3s at +0x0C. */
typedef union StageAreaRow {
    u32 words[12];
    StageAreaVecs v;
} StageAreaRow; /* size: 0x30 */

/* One 476-byte per-area record from stage_w+0xC20.  Only the head is touched by this range. */
typedef struct StageArea {
    /* +0x000 */ GXColor color;
    /* +0x004 */ StageAreaRow rows[9];
    /* +0x1B4 */ u8 pad_0x1B4[0x28];
} StageArea; /* size: 0x1DC */

/* stage_w: the offsets this range touches. */
typedef struct StageWork {
    /* +0x0000 */ u8 pad_0x0000[0x0004];
    /* +0x0004 */ void* entries[0x2F0];         /* lookup by a byte id */
    /* +0x0BC4 */ u8 pad_0x0BC4[0x0001];
    /* +0x0BC5 */ u8 mapno;
    /* +0x0BC6 */ u8 areano;
    /* +0x0BC7 */ u8 pad_0x0BC7[0x0BD0 - 0x0BC7];
    /* +0x0BD0 */ u8 field_0xBD0;
    /* +0x0BD1 */ u8 pad_0x0BD1[0x0BD4 - 0x0BD1];
    /* +0x0BD4 */ f32 field_0xBD4;
    /* +0x0BD8 */ u32 field_0xBD8[12];
    /* +0x0C08 */ VEC3 field_0xC08;
    /* +0x0C14 */ u8 flags;
    /* +0x0C15 */ u8 pad_0x0C15[0x0C20 - 0x0C15];
    /* +0x0C20 */ StageArea areas[15];
    /* +0x2804 */ u8 pad_0x2804[0x2818 - 0x2804];
    /* +0x2818 */ f32 field_0x2818;
    /* +0x281C */ u8 bitmask[0x2FD4 - 0x281C];
    /* +0x2FD4 */ void* field_0x2FD4;
    /* +0x2FD8 */ void* field_0x2FD8;
    /* +0x2FDC */ u8 pad_0x2FDC[0x0004];
} StageWork; /* size: 0x2FE0 */

#define SW ((StageWork*)stage_w)

/* A record `fn_802AE564` selects: one loadable effect/resource descriptor per id.  Only the fields
 * this range reads are named; the size is traced from the 0x24/0x28 bytes the accessors touch. */
typedef struct StageElem {
    /* +0x00 */ u8 pad_0x00[0x24];
    /* +0x24 */ u32 field_0x24;
    /* +0x28 */ u8 field_0x28;
    /* +0x29 */ u8 pad_0x29[0x0B];
    /* +0x34 */ f32 field_0x34;
    /* +0x38 */ u8 field_0x38;
    /* +0x39 */ u8 pad_0x39[0x03];
} StageElem; /* size: 0x3C */

/* The head `fn_802AE564` returns: `kind` at +0, a word at +4 that indexes an array of descriptors,
 * an inline word table at +0x0C, a byte-indexed word array at +0x18 and a 16-byte-stride base at
 * +0x14. */
typedef struct StageGroup {
    /* +0x00 */ u8 kind;
    /* +0x01 */ u8 pad_0x01[0x03];
    /* +0x04 */ StageElem** elems;
    /* +0x08 */ u32 field_0x08;   /* screen_projection_get reads it */
    /* +0x0C */ u32* field_0x0C;
    /* +0x10 */ u8 pad_0x10[0x04];
    /* +0x14 */ u32* field_0x14;    /* base of a j*16-byte table */
    /* +0x18 */ u32* field_0x18;
} StageGroup; /* size: 0x1C */

/* ------------------------------------------------------------------------------------------------ */
/* foreign declarations                                                                              */
/* ------------------------------------------------------------------------------------------------ */

extern "C" void* fn_802AE564(u8 id);
extern "C" u32 stage_map_kind_get(u32 kind);
extern "C" bool fn_802B0B04(u8 index, u8 bit);
extern "C" u32 stage_map_area_count_get(u8 id);
extern "C" void fn_802B0FF0(void* self, u8 index);
extern "C" void fn_802B11A0(void* self, u8 index);
extern "C" void fn_802AFA7C(u8 index);
extern "C" void fn_802B2278(u8 a, u8 b);
extern "C" void* fn_802B1558(void* work, long which);
extern "C" void* fn_802B057C(void* base, s32 off);
extern "C" void* fn_802B0540(void* self);

/* ------------------------------------------------------------------------------------------------ */
/* functions                                                                                         */
/* ------------------------------------------------------------------------------------------------ */

/* The record `fn_802B050C` returns the body of. */
typedef struct StageObject {
    /* +0x000 */ u8 head[0x114];
    /* +0x114 */ u8 body[];
} StageObject; /* size: 0x114 (flexible body tail) */

/* The header of the resource `fn_800700C0` resolves (`fn_802B0540` reads its size at +0x48). */
typedef struct ResolverView {
    /* +0x00 */ u8 pad_0x00[0x48];
    /* +0x48 */ s32 size;
} ResolverView; /* size: 0x4C */
#include "mh3_pad.h" /* the owner header (rule 2) */
#include "sound/mhchar.h"
#include "sound/fn_800D7F54.h"
#include "unsplit/ef.h"
#include "unsplit/g3d.h"
#include "unsplit/Pl.h"
#include "unsplit/unknown.h"
#include "Pl/Pl_master_ck.h"
#include "fn_8004CAD8.h"
#include "ef/fn_800CDB2C.h"

#pragma peephole off

/* Returns the stage-work block. */
void* get_stg_w()
{
    return stage_w;
}

/* Returns the current map number. */
u8 get_now_mapno()
{
    return SW->mapno;
}

/* Returns the current area number. */
u8 get_now_areano()
{
    return SW->areano;
}

/* Copies the 4-byte word at `src` over the one at `dst`. */
extern "C" void fn_802AE280(void* dst, const void* src)
{
    *(u32*)dst = *(const u32*)src;
}

/* Copies `src`'s 4-byte word over `dst` and returns `dst`. */
extern "C" void* fn_802AE250(void* dst, const void* src)
{
    fn_802AE280(dst, src);
    return dst;
}

/* Narrows both arguments to a byte and forwards to fn_802B2278. */
extern "C" void fn_802B230C(u32 a, u32 b)
{
    fn_802B2278((u8)a, (u8)b);
}

/* Returns the stage's secondary block at stage_w + 0x2EC0. */
extern "C" void* fn_802B0420()
{
    return stage_w + 0x2EC0;
}

/* Tail-calls fn_802B1558(stage_w, 1). */
extern "C" void fn_802B1FDC()
{
    fn_802B1558(stage_w, 1);
}

/* Copies six bytes. */
extern "C" void fn_802AE980(u8* dst, const u8* src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[3] = src[3];
    dst[4] = src[4];
    dst[5] = src[5];
}

/* Returns the id-remapped byte at stage_w.bitmask[index]. */
extern "C" u8 fn_802B0AEC(u8 index)
{
    const u8* p = &SW->bitmask[0];
    return p[(u8)index];
}

/* Tests bit 0 of the stage flag byte. */
extern "C" bool stage_water_enabled_ck()
{
    return SW->flags & 1;
}

/* Tests bit 1 of the stage flag byte. */
extern "C" bool fn_802AFF58()
{
    return SW->flags & 2;
}

/* Tests bit (4 + bit) of stage_w.bitmask[index]. */
extern "C" bool fn_802B0B04(u8 index, u8 bit)
{
    return SW->bitmask[(u8)index] & (0x10 << (u8)bit);
}

/* Remaps an id through lbl_805CED40; an unmapped id (0xFF) is returned unchanged.  The parameter is
 * wider than a byte: retail masks it (`clrlwi`) before the table index, so the original signature
 * was not the byte one `unsplit/unknown.h` guessed. */
extern "C" u32 stage_map_kind_get(u32 kind)
{
    u8 mapped = lbl_805CED40[(u8)kind];
    if (mapped == 0xFF)
        return kind;
    return mapped;
}

/* Tail-calls fn_802B0B04 with the byte pair lbl_805CF038[2*index], [2*index+1]. */
extern "C" u32 fn_802B0998(u8 index)
{
    const u8* p = &lbl_805CF038[(u8)index * 2];
    return fn_802B0B04(p[0], p[1]);
}

/* Whether the float at `self`+8 is below the pooled constant. */
extern "C" u32 fn_802B0688(void* self)
{
    return ((f32*)self)[2] < lbl_8079A480;
}

/* Returns `base`'s word pointer plus `off`; a zero offset yields NULL. */
extern "C" void* fn_802B057C(void* base, s32 off)
{
    u32 p = *(u32*)base;
    if (off == 0)
        return 0;
    return (u8*)(p + off);
}

/* Returns the record at `self` plus the resource size from fn_800700C0(). */
extern "C" void* fn_802B0540(void* self)
{
    return fn_802B057C(self, ((const ResolverView*)&reinterpret_cast<const nw4r::g3d::ResMdl*>(self)->ref())->size);
}

/* Returns the entry `index` points at, biased by its 0x114-byte header, or NULL. */
extern "C" void* fn_802B050C(u8 index)
{
    void** entries = &SW->entries[0];
    StageObject* p = (StageObject*)entries[(u8)index];
    if (p == 0)
        return 0;
    return fn_802B0540(p->body);
}

/* Returns the word at +8 of the current map/area group. */
extern "C" u32 screen_projection_get()
{
    return ((StageGroup*)fn_802AE564((u8)get_now_mapno()))->field_0x08;
}

/* Stores `f` into the current area's record when the map/area pair matches. */
extern "C" void fn_802B08DC(f32 f)
{
    if ((SW->mapno == 6 || SW->mapno == 17) && SW->areano == 1)
        SW->field_0x2818 = f;
}

/* Whether `self`'s map id remaps to a "loadable" category. */
extern "C" u32 fn_802B0B38(void* self)
{
    s32 mapped = (u8)stage_map_kind_get(((StageWork*)self)->mapno);
    return mapped == 4 || mapped == 10;
}

/* Remaps the stage's current map id and returns the j-th element of the group's 16-byte-stride
 * table. */
extern "C" void* fn_802B0B7C(void* work, u8 j)
{
    StageGroup* g = (StageGroup*)fn_802AE564(((StageWork*)work)->mapno);
    return &g->field_0x14[(u8)j * 4];
}

/* Whether the j-th element of the group's 16-byte-stride table is non-zero. */
extern "C" bool fn_802B0BC0(u8 id, u8 j)
{
    StageGroup* g = (StageGroup*)fn_802AE564((u8)id);
    return g->field_0x14[(u8)j * 4];
}

/* Returns the j-th word of the group's inline table, or NULL for the unmapped id. */
extern "C" u32* fn_802AFE5C(u8 id, u8 j)
{
    StageGroup* g;
    if ((u8)id == 0xFF)
        return 0;
    g = (StageGroup*)fn_802AE564((u8)id);
    return &g->field_0x0C[(u8)j];
}

/* Returns the record's +0x24 word, or 0 for the unmapped id. */
extern "C" u32 stage_entry_angle_get(u8 id, u8 j)
{
    StageGroup* g;
    if ((u8)id == 0xFF)
        return 0;
    g = (StageGroup*)fn_802AE564((u8)id);
    return g->elems[(u8)j]->field_0x24;
}

/* Returns the record's +0x28 byte, or 0xFF for the unmapped id. */
extern "C" u32 fn_802AFEAC(u8 id, u8 j)
{
    StageGroup* g;
    if ((u8)id == 0xFF)
        return 0xFF;
    g = (StageGroup*)fn_802AE564((u8)id);
    return g->elems[(u8)j]->field_0x28;
}

/* Selects one of the two record groups by whether the id was remapped. */
extern "C" void* fn_802AE564(u8 id)
{
    u8 mapped = stage_map_kind_get((u8)id);
    return mapped == (u8)id ? SW->field_0x2FD4 : SW->field_0x2FD8;
}

/* Returns the selected group's head byte, or 0 for the unmapped id. */
extern "C" u32 stage_map_area_count_get(u8 id)
{
    StageGroup* g;
    if ((u8)id == 0xFF)
        return 0;
    g = (StageGroup*)fn_802AE564((u8)id);
    return g->kind;
}

/* Returns the current area's word [i][j] lookup. */
extern "C" u32 stage_cell_get(u8 i, u8 j)
{
    return SW->areas[SW->areano].rows[(u8)i].words[(u8)j];
}

/* Same lookup, but the area is an argument. */
extern "C" u32 fn_802B0290(u8 i, u8 j, u8 area)
{
    StageArea* a = &SW->areas[(u8)area];
    StageAreaRow* r = &a->rows[(u8)i];
    return r->words[(u8)j];
}

/* Copies the current area's row [i] 12-byte element [k] into `out`. */
extern "C" void fn_802B02C4(void* out, u8 i, u8 k)
{
    assignVec3((Vec*)out, (Vec*)&SW->areas[SW->areano].rows[(u8)i].v.vecs[(u8)k]);
}

/* Classifies the current map number into a load category. */
extern "C" u32 fn_802B0608()
{
    s32 m = SW->mapno;
    switch (m) {
    case 2:
    case 5:
    case 16:
    case 10:
        return 1;
    case 13:
    case 4:
    case 15:
        return 2;
    }
    return 0;
}

/* Resets the stage's per-area cursor block and re-seats the world vector. */
extern "C" void fn_802B2918()
{
    s32 i;
    SW->field_0xBD0 = 0;
    SW->field_0xBD4 = lbl_8079A484;
    for (i = 0; i < 12; i++)
        SW->field_0xBD8[i] = 0xFFFFFF00;
    setVector3(&SW->field_0xC08, lbl_8079A498, lbl_8079A498, lbl_8079A498);
}

/* Calls fn_802AFA7C for the two side indices. */
extern "C" void fn_802AFA40()
{
    u32 i;
    for (i = 0; i < 2; i++)
        fn_802AFA7C((u8)i);
}

/* Calls fn_802B0FF0 for each index below the current map's count. */
extern "C" void fn_802B1138(void* self)
{
    u8 n = (u8)stage_map_area_count_get(((StageWork*)self)->mapno);
    u8 i;
    for (i = 0; (u8)i < n; i++)
        fn_802B0FF0(self, i);
}

/* Calls fn_802B11A0 for each index below the current map's count. */
extern "C" void fn_802B13A8(void* self)
{
    u8 n = (u8)stage_map_area_count_get(((StageWork*)self)->mapno);
    u8 i;
    for (i = 0; (u8)i < n; i++)
        fn_802B11A0(self, i);
}

/* Returns the current map's descriptor field +0x34 for `id`, or a default float. */
extern "C" f32 fn_802B0430(u8 id)
{
    StageWork* w = SW;
    StageElem* e;
    if ((u8)id == 0xFF)
        return lbl_8079A47C;
    e = ((StageGroup*)fn_802AE564(w->mapno))->elems[(u8)id];
    if (e == 0)
        return lbl_8079A47C;
    return e->field_0x34;
}

/* Returns the current map's +0x18 word array entry for `id`, or 0. */
extern "C" u32 fn_802B04A0(u8 id)
{
    StageWork* w = SW;
    u32* arr;
    if ((u8)id == 0xFF)
        return 0;
    arr = ((StageGroup*)fn_802AE564(w->mapno))->field_0x18;
    if (arr == 0)
        return 0;
    return arr[(u8)id];
}

/* Returns the current map's descriptor field +0x38 for `id`, or 0. */
extern "C" u32 pl_act_kind_get(u8 id)
{
    StageWork* w = SW;
    StageElem* e;
    if ((u8)id == 0xFF)
        return 0;
    e = ((StageGroup*)fn_802AE564(w->mapno))->elems[(u8)id];
    if (e == 0)
        return 0;
    return e->field_0x38;
}

/* Sets bit (4 + j) of bitmask[id] when the current map's j-th table word is set. */
extern "C" void fn_802B09B8(u8 id, u8 j)
{
    u32* p = (u32*)fn_802B0B7C(stage_w, id);
    if (p[(u8)j] != 0)
        SW->bitmask[(u8)id] |= (u8)(0x10 << (u8)j);
}

/* Whether at least two of the three map flags (ids 3, 11, 4) are set. */
extern "C" u32 stage_gate_a_ck()
{
    u8 count = 0;
    if (fn_802B0998(3) == 1)
        count = 1;
    if (fn_802B0998(11) == 1)
        count++;
    if (fn_802B0998(4) == 1)
        count++;
    if (count >= 2)
        return 1;
    return 0;
}

#pragma peephole reset


#pragma peephole off

/* Blends the two packed colours channel by channel and returns the packed result. */
extern "C" u32 fn_802B2978(u32 from, u32 to, f32 t)
{
    u8 a, b;
    u32 r0, r1, r2, r3;

    a = from >> 24; b = to >> 24; r0 = (a + (int)(t * (f32)(s16)(b - a))) & 0xFF;
    a = from >> 16; b = to >> 16; r1 = (a + (int)(t * (f32)(s16)(b - a))) & 0xFF;
    a = from >> 8;  b = to >> 8;  r2 = (a + (int)(t * (f32)(s16)(b - a))) & 0xFF;
    a = from;       b = to;       r3 = (a + (int)(t * (f32)(s16)(b - a))) & 0xFF;

    return (r0 << 24) | (r1 << 16) | (r2 << 8) | r3;
}

#pragma peephole reset


#pragma peephole off

/* The C++-linkage callees the view below calls: declared at global scope so the calls resolve to the map's
 * manglings (a declaration inside the view namespace would mangle the namespace in). */
nw4r::math::VEC3 get_camera_pos(void);
u32 LbCheckKujiraEvent(void);
void eft028_set_koware(u8 kind, nw4r::math::VEC3* pos, u8 area, long param);

namespace view_fn_802B2AA0 {

namespace nw4r = ::nw4r;  /* the view headers below reopen it */

/* data keeps its map name (a view namespace would mangle it): the header is read with C linkage */
extern "C" {
#include "stage/fn_802B2AA0.h"
}
#include "stage/stg_w.h"

extern "C" {
/* The shell-set job table pointer (`.sbss` 0x80794B60); an RSO stores it, nothing in the DOL does. */
ShellSetFuncs* shell_set_func_ptr;
}

/* Blends the area's three colour groups by the camera/area angle and arms the six results as one
 * sound/effect frame set. */
extern "C" void fn_802B2AA0(StageBlendRec* rec, void* obj, u8 store)
{
    nw4r::math::VEC3 poly;
    nw4r::math::VEC3 world;
    nw4r::math::VEC3 cam;
    u32 ang_x;
    u32 ang_y;
    s32 base;
    u32 colour[6];
    u16 delta;
    f32 t;

    VEC3_ctor(&poly);
    base = camera_angle_y_get();
    subVec3(&world, &::get_camera_pos(), obj);
    copyVec3(&poly, &world);
    calcVecAngXY(&poly, &ang_x, &ang_y);

    delta = (u16)(ang_y - base);
    if (delta < 0x4000U) {
        t = (f32)delta / lbl_8079A49C;
        colour[0] = fn_802B2978(rec->group[0].base, rec->group[0].target_a, t);
        colour[1] = fn_802B2978(rec->group[0].base, rec->group[0].target_b, t);
        colour[2] = fn_802B2978(rec->group[2].base, rec->group[2].target_a, t);
        colour[3] = fn_802B2978(rec->group[2].base, rec->group[2].target_b, t);
        colour[4] = fn_802B2978(rec->group[1].base, rec->group[1].target_a, t);
        colour[5] = fn_802B2978(rec->group[1].base, rec->group[1].target_b, t);
    } else if (delta < 0x8000U) {
        t = (f32)(s32)(delta - 0x4000) / lbl_8079A49C;
        colour[0] = fn_802B2978(rec->group[0].target_a, rec->group[0].axis, t);
        colour[1] = fn_802B2978(rec->group[0].target_b, rec->group[0].axis, t);
        colour[2] = fn_802B2978(rec->group[2].target_a, rec->group[2].axis, t);
        colour[3] = fn_802B2978(rec->group[2].target_b, rec->group[2].axis, t);
        colour[4] = fn_802B2978(rec->group[1].target_a, rec->group[1].axis, t);
        colour[5] = fn_802B2978(rec->group[1].target_b, rec->group[1].axis, t);
    } else if (delta < 0xC000U) {
        t = (f32)(s32)(delta - 0x8000) / lbl_8079A49C;
        colour[0] = fn_802B2978(rec->group[0].axis, rec->group[0].target_b, t);
        colour[1] = fn_802B2978(rec->group[0].axis, rec->group[0].target_a, t);
        colour[2] = fn_802B2978(rec->group[2].axis, rec->group[2].target_b, t);
        colour[3] = fn_802B2978(rec->group[2].axis, rec->group[2].target_a, t);
        colour[4] = fn_802B2978(rec->group[1].axis, rec->group[1].target_b, t);
        colour[5] = fn_802B2978(rec->group[1].axis, rec->group[1].target_a, t);
    } else {
        t = (f32)(s32)(delta - 0xC000) / lbl_8079A49C;
        colour[0] = fn_802B2978(rec->group[0].target_b, rec->group[0].base, t);
        colour[1] = fn_802B2978(rec->group[0].target_a, rec->group[0].base, t);
        colour[2] = fn_802B2978(rec->group[2].target_b, rec->group[2].base, t);
        colour[3] = fn_802B2978(rec->group[2].target_a, rec->group[2].base, t);
        colour[4] = fn_802B2978(rec->group[1].target_b, rec->group[1].base, t);
        colour[5] = fn_802B2978(rec->group[1].target_a, rec->group[1].base, t);
    }

    if (rec->enabled == 0) {
        colour[4] = 0;
        colour[5] = 0;
    }
    if (store == 0) {
        fn_80057DE0(0, 6, colour, rec->se_frame);
        return;
    }
    fn_80057EF4(0, 6, colour, rec->se_frame);
}

/* Copies the current area's staged colour record out of the stage block and drives it. */
extern "C" void fn_802B2E2C(void)
{
    StageRuntime* st = (StageRuntime*)stage_w;
    StageColourRec rec;
    s32 handle;

    handle = fn_80082C80(pRoot, 0);
    fn_802AE250(&rec, &handle);
    fn_8007A1B8(&rec, 0, &rec.flags, &rec.b, 0, 0, &rec.a);

    if (GameMode_ck() == 2) {
        if (getItemListSelection() == 0) {
            fn_802B2F3C(&rec, &st->area[st->areano].colour_a);
        } else {
            fn_802B2F3C(&rec, &st->area[st->areano].colour_b);
        }
        fn_802AE1DC(&rec);
        return;
    }
    if (stage_water_enabled_ck() == 0) {
        return;
    }
    if (fn_802BE088() == 1U) {
        fn_802B2F3C(&rec, &st->area[st->areano].colour_b);
    } else {
        fn_802B2F3C(&rec, &st->area[st->areano].colour_a);
    }
    fn_802AE1DC(&rec);
}

/* Copies the 0x10-byte colour record at `src` into `dst`. */
extern "C" void fn_802B2F3C(StageColourRec* dst, const StageColourRec* src)
{
    dst->colour = src->colour;
    dst->a = src->a;
    dst->b = src->b;
    dst->flags = src->flags;
}

/* Builds the current area's visibility set from the map/area tables and hides every joint that is not
 * in it. */
extern "C" void fn_802B2F60(StageRuntime* st, u8 kind)
{
    nw4r::math::VEC3 poly;
    nw4r::math::VEC3 cam;
    MHchar* chr;
    StageJointLists* list;
    u8* keep;
    u8* drop;
    u8 group;
    u8 changed;
    u8 found;
    u8 i;
    u8 j;

    list = NULL;
    group = 0;
    changed = 0;
    found = 0;
    chr = (MHchar*)st->area_char[kind];
    VEC3_ctor(&poly);

    if (kind == 1) {
        if (lbl_805CF644[st->mapno] != NULL) {
            list = lbl_805CF644[st->mapno] + st->areano;
        }
    } else if (kind == 0) {
        switch (st->mapno) {
        case 1:
        case 12:
            if (st->areano == 7) {
                list = lbl_80792310;
            } else if (st->areano == 0xC) {
                list = lbl_80792330;
            }
            break;
        case 3:
        case 14:
            list = lbl_805CF60C[st->areano];
            break;
        case 9:
            list = lbl_807923C8[st->areano];
            group = 1;
            break;
        }
    }

    if (list == NULL) {
        return;
    }
    if (group == 1U && kind == 0) {
        copyVec3(&poly, &::get_camera_pos());
        group = my_player_no();
        if (fn_802B0688(&poly) == 1U) {
            keep = list->show;
            drop = list->hide;
            found = 1;
        } else {
            keep = list->hide;
            drop = list->show;
            found = 2;
        }
        if (st->joint_state[kind] != st->joint_state[kind + 4]) {
            changed = 1;
        }
    } else {
        if (::get_camera_pos().z == lbl_8079A448) {
            keep = list->show;
            drop = list->hide;
            found = 1;
        } else {
            keep = list->hide;
            drop = list->show;
            found = 2;
        }
        if (found != st->joint_state[kind]) {
            changed = 1;
        }
    }

    fn_802B0B7C(st, st->areano);

    if (changed != 0) {
        for (;;) {
            if (*keep == 0xFF) {
                break;
            }
            for (j = 0; j < 8; j++) {
                if (st->field_0x2EAD[j] == 0xFF) {
                    chr->setVisibility(1, 1);
                    break;
                }
                if (*keep == st->field_0x2EAD[j]) {
                    break;
                }
            }
            keep++;
        }
        for (;;) {
            if (*drop == 0xFF) {
                break;
            }
            chr->setVisibility(1, 0);
            drop++;
        }
    }
    st->joint_state[group * 4 + kind] = found;
    (void)i;
}

/* Tail-calls the colour/visibility driver for the map's area animation set 0. */
extern "C" void fn_802B3250(void)
{
    fn_802B2F60((StageRuntime*)stage_w, 0);
}

/* Tail-calls the colour/visibility driver for the map's area animation set 1. */
extern "C" void fn_802B3260(void)
{
    fn_802B2F60((StageRuntime*)stage_w, 1);
}

/* Builds the current area's joint visibility set for the map's animation mode and applies it to the
 * area's character: one list of joint ids is shown, the other hidden. */
extern "C" void fn_802B3270(StageRuntime* st, MHchar* chr, u8 mode)
{
    StageJointLists local;
    u8 hide_buf[0x24];
    u8 show_buf[0x24];
    StageJointLists* lists = NULL;
    s32 flag = 0;
    u8 hide_i = 0;
    u8 show_i = 0;

    switch (st->mapno) {
    case 1:
    case 12: {
        u8 area = st->areano;

        lists = lbl_805CF6A0[area];
        switch (area) {
        case 0:
            if (mode == 1) {
                return;
            }
            if (quest_id_head_ck() == 1) {
                flag = 1;
            }
            break;
        case 1:
            if (quest_id_tail_ck() == 1) {
                flag = 1;
            }
            break;
        case 6:
            if (mode == 0) {
                return;
            }
            if (mode == 1) {
                flag = 1;
            }
            break;
        }
        break;
    }
    case 8:
    case 19:
        if (st->areano == 0) {
            lists = &lbl_80792410;
            flag = 0;
        }
        break;
    case 11:
    case 20:
        if (st->areano == 0) {
            lists = &lbl_80792410;
            flag = 1;
        }
        break;
    case 22: {
        u8 area = st->areano;

        if (area == 0) {
            const u8* src;

            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (fn_802FB97C() == 1) {
                src = lbl_80792460;
            } else {
                src = lbl_80792464;
            }
            while (*src != 0xFF) {
                local.show[show_i] = *src;
                src++;
                show_i++;
            }
            if (getItemListSelection() == 0) {
                local.show[show_i] = 1;
                show_i++;
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
        } else if (area == 1) {
            lists = &lbl_80792470;
            if (getItemListSelection() == 1) {
                flag = 1;
            }
        } else if (area == 2) {
            const u8* src;

            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            switch (fn_802FB8EC(0)) {
            case 0:
            case 1:
                local.show[0] = 3;
                local.show[1] = 4;
                show_i = 2;
                break;
            case 2:
                local.show[0] = 1;
                local.show[1] = 4;
                show_i = 2;
                break;
            case 3:
                local.show[0] = 1;
                local.show[1] = 2;
                show_i = 2;
                break;
            }
            switch (fn_802FB8EC(2)) {
            case 0:
                local.show[show_i] = 8;
                show_i++;
                local.show[show_i] = 9;
                show_i++;
                local.show[show_i] = 10;
                show_i++;
                break;
            case 1:
                local.show[show_i] = 9;
                show_i++;
                local.show[show_i] = 10;
                show_i++;
                break;
            case 2:
                local.show[show_i] = 10;
                show_i++;
                break;
            }
            switch (fn_802FB8EC(3)) {
            case 0:
                local.show[show_i] = 11;
                show_i++;
                local.show[show_i] = 12;
                show_i++;
                local.show[show_i] = 13;
                show_i++;
                break;
            case 1:
                local.show[show_i] = 12;
                show_i++;
                local.show[show_i] = 13;
                show_i++;
                break;
            case 2:
                local.show[show_i] = 13;
                show_i++;
                break;
            }
            switch (fn_802FB8EC(1)) {
            case 0:
            case 1:
                local.show[show_i] = 14;
                show_i++;
                local.show[show_i] = 15;
                show_i++;
                break;
            case 2:
                local.show[show_i] = 15;
                show_i++;
                break;
            }
            if (fn_802FB8C4() == 0) {
                local.show[show_i] = 6;
                show_i++;
            } else {
                local.show[show_i] = 5;
                show_i++;
            }
            if (fn_802FB900() == 1) {
                local.show[show_i] = 0x10;
                show_i++;
            }
            if (getItemListSelection() == 0) {
                local.show[show_i] = 7;
                show_i++;
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
        }
        break;
    }
    case 21: {
        u8 area = st->areano;

        switch (area) {
        case 0:
            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (getItemListSelection() == 1) {
                const u8* src;

                src = lbl_805CF6D4;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_80792418;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 0x12;
                    show_i++;
                    local.show[show_i] = 0x13;
                    show_i++;
                }
            } else {
                const u8* src;

                src = lbl_80792418;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_805CF6D4;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 0x10;
                    show_i++;
                    local.show[show_i] = 0x11;
                    show_i++;
                }
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
            break;
        case 1:
            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (getItemListSelection() == 1) {
                const u8* src;

                src = lbl_805CF6E8;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_8079241C;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 0x11;
                    show_i++;
                    local.show[show_i] = 0x12;
                    show_i++;
                }
            } else {
                const u8* src;

                src = lbl_8079241C;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_805CF6E8;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 0x0F;
                    show_i++;
                    local.show[show_i] = 0x10;
                    show_i++;
                }
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
            break;
        case 2:
            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (getItemListSelection() == 1) {
                const u8* src;

                src = lbl_805CF6F4;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_80792420;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 4;
                    show_i++;
                    local.show[show_i] = 0x0C;
                    show_i++;
                    local.show[show_i] = 0x0D;
                    show_i++;
                } else if (fn_802FB9F8() == 1) {
                    local.show[show_i] = 4;
                    show_i++;
                }
            } else {
                const u8* src;

                src = lbl_80792420;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_805CF6F4;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 2;
                    show_i++;
                    local.show[show_i] = 0x0A;
                    show_i++;
                    local.show[show_i] = 0x0B;
                    show_i++;
                } else if (fn_802FB9F8() == 1) {
                    local.show[show_i] = 2;
                    show_i++;
                }
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
            break;
        case 3:
            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (getItemListSelection() == 1) {
                const u8* src;

                src = lbl_80792428;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_80792430;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 7;
                    show_i++;
                    local.show[show_i] = 8;
                    show_i++;
                } else {
                    local.show[show_i] = 4;
                    show_i++;
                }
            } else {
                const u8* src;

                src = lbl_80792430;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_80792428;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 5;
                    show_i++;
                    local.show[show_i] = 6;
                    show_i++;
                } else {
                    local.show[show_i] = 3;
                    show_i++;
                }
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
            break;
        case 5:
            lists = &lbl_80792440;
            if (getItemListSelection() == 1) {
                flag = 1;
            }
            break;
        case 6:
            lists = &lbl_80792450;
            if (getItemListSelection() == 1) {
                flag = 1;
            }
            break;
        case 7:
            lists = &local;
            local.hide = hide_buf;
            local.show = show_buf;
            if (getItemListSelection() == 1) {
                const u8* src;

                src = lbl_805CF700;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_80792458;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 0x19;
                    show_i++;
                    local.show[show_i] = 0x1D;
                    show_i++;
                    local.show[show_i] = 0x1E;
                    show_i++;
                    local.show[show_i] = 0x23;
                    show_i++;
                } else {
                    if (fn_802FB9F8() == 1) {
                        local.show[show_i] = 0x19;
                        show_i++;
                    }
                    local.show[show_i] = 0x17;
                    show_i++;
                    local.show[show_i] = 0x18;
                    show_i++;
                    local.show[show_i] = 0x1F;
                    show_i++;
                }
            } else {
                const u8* src;

                src = lbl_80792458;
                while (*src != 0xFF) {
                    local.show[show_i] = *src;
                    src++;
                    show_i++;
                }
                src = lbl_805CF700;
                while (*src != 0xFF) {
                    local.hide[hide_i] = *src;
                    src++;
                    hide_i++;
                }
                if (::LbCheckKujiraEvent() == 0 && fn_802FB9F8() == 0) {
                    local.show[show_i] = 5;
                    show_i++;
                    local.show[show_i] = 0x1B;
                    show_i++;
                    local.show[show_i] = 0x1C;
                    show_i++;
                    local.show[show_i] = 0x22;
                    show_i++;
                } else {
                    if (fn_802FB9F8() == 1) {
                        local.show[show_i] = 5;
                        show_i++;
                    }
                    local.show[show_i] = 4;
                    show_i++;
                    local.show[show_i] = 0x16;
                    show_i++;
                    local.show[show_i] = 0x1A;
                    show_i++;
                }
            }
            flag = 1;
            local.hide[hide_i] = 0xFF;
            local.show[show_i] = 0xFF;
            break;
        }
        break;
    }
    }
    if (lists != NULL) {
        const u8* shown;
        const u8* hidden;

        if (flag != 0) {
            shown = lists->hide;
            hidden = lists->show;
        } else {
            shown = lists->show;
            hidden = lists->hide;
        }
        while (*shown != 0xFF) {
            chr->setVisibility(*shown, true);
            shown++;
        }
        while (*hidden != 0xFF) {
            chr->setVisibility(*hidden, false);
            hidden++;
        }
    }
}

/* Hides every joint the current area's kind list names. */
extern "C" void fn_802B40D8(StageRuntime* st, MHchar* chr)
{
    u8* p;

    chr->get_joint_num();
    p = fn_802B04A0(st->areano);
    if (p == NULL) {
        return;
    }
    while (*p != 0xFF) {
        chr->setVisibility(0, 0);
        p++;
    }
}

/* Spawns the `eft028` break effects along the map's effect strips. */
extern "C" void fn_802B414C(StageRuntime* st)
{
    nw4r::math::VEC3 pos;
    f32* strip;
    u8* slot;
    f32 limit;
    s32 i;

    VEC3_ctor(&pos);
    if (st->mapno != 0xA || st->areano != 1) {
        return;
    }
    strip = lbl_805CF784;
    slot = lbl_805CF7C0;
    limit = lbl_8079A448;
    for (;;) {
        if (*strip <= limit) {
            return;
        }
        i = 0;
        for (;;) {
            if ((f32)i >= strip[2] / strip[1]) {
                break;
            }
            if (fn_800E16DC((void*)st->area_char[0], 0, 0, *strip + (f32)i * strip[1], lbl_8079A448) != 0) {
                vec_to_mh_vec3(&pos, (struct Vec*)slot);
                ::eft028_set_koware(0xB, &pos, st->areano, 0);
            }
            i++;
        }
        strip += 3;
        slot += 0x0C;
    }
}

/* Spawns the map/area's fixed break effects and sound. */
extern "C" void fn_802B428C(StageRuntime* st)
{
    nw4r::math::VEC3 pos;
    u8 mapno;
    u8 areano;

    VEC3_ctor(&pos);
    fn_802B414C(st);
    mapno = st->mapno;
    if (mapno != 5 && mapno != 0x10) {
        return;
    }
    areano = st->areano;
    switch (areano) {
    case 9:
        if ((s32)(system_w.field_0x0c % 170) == 0) {
            setVector3(&pos, lbl_8079A4A0, lbl_8079A4A4, lbl_8079A4A8);
            ::eft028_set_koware(6, &pos, st->areano, 0);
            return;
        }
        break;
    case 10:
        if ((s32)(system_w.field_0x0c % 160) == 0) {
            setVector3(&pos, lbl_8079A4AC, lbl_8079A4B0, lbl_8079A4B4);
            ::eft028_set_koware(6, &pos, st->areano, 0);
            fn_802BE4FC(0x43);
            fn_800DD38C();
        }
        break;
    }
}

/* Seats the map/area's joint effects on the `shell_set_func_ptr` job table. */
extern "C" void fn_802B43A8(MHchar* chr, u8 kind, u16 flags)
{
    nw4r::math::VEC3 pos;
    s8 order[3];
    f32* thresholds;
    u8* strip;
    u8** joint_tables;
    u8* joints;
    f32 height;
    s32 band;
    u8 index;
    u8 mapno;

    VEC3_ctor(&pos);
    strip = NULL;
    mapno = get_now_mapno();
    if ((mapno == 4 || mapno == 0xF) && kind == 5) {
        strip = lbl_805CF7FC;
        thresholds = lbl_805CF7F0;
        joint_tables = (flags & 1) ? lbl_805CF8E0 : lbl_805CF8F0;
    }
    if (strip == NULL) {
        return;
    }
    band = 0;
    height = chr->pos_0x04.y;
    if (!(height < thresholds[0])) {
        band = 1;
        if (!(height < thresholds[1])) {
            band = 2;
            if (!(height < thresholds[2])) {
                band = 3;
            }
        }
    }
    joints = joint_tables[band];
    index = (s8)(u8)(flags % 3);
    order[0] = index;
    if ((flags & 0x10) != 0) {
        order[1] = index + 1;
        if (order[1] >= 3) {
            order[1] = 0;
        }
        order[2] = order[1] + 1;
        if (order[2] >= 3) {
            order[2] = 0;
        }
    } else {
        order[1] = index - 1;
        if (order[1] < 0) {
            order[1] = 2;
        }
        order[2] = order[1] - 1;
        if (order[2] < 0) {
            order[2] = 2;
        }
    }
    index = 0;
    while ((s8)*joints > 0) {
        vec_to_mh_vec3(&pos, (struct Vec*)(strip + (s8)*joints * 0xC));
        shell_set_func_ptr->method_0x88(&pos, kind, (s16)((s8)order[index] * 0xA), shell_set_func_ptr);
        joints++;
        index++;
    }
}

/* Raises the stage block's one-shot flag byte. */
extern "C" void fn_802B45BC(void)
{
    ((StageRuntime*)stage_w)->field_0x2F91 |= 0x81;
}

/* Returns whether the stage block's one-shot flag byte has bit 0 clear. */
extern "C" s32 fn_802B45D4(void)
{
    return (((StageRuntime*)stage_w)->field_0x2F91 & 1) != 0;
}

/* Arms area `index`'s timer on first use and keeps its 4-second window. */
extern "C" void fn_802B45F4(u8 index)
{
    StageRuntime* st = (StageRuntime*)stage_w;
    u16 bit = 1 << index;
    s32 now;

    if ((st->field_0x2F92 & bit) == 0) {
        st->field_0x2F92 |= bit;
        fn_802FBA94();
    }
    now = quest_time_limit_get();
    st->field_0x2F94[index] = now - quest_time_elapsed_get();
}

/* Tests area bit `index` of the stage block's 16-bit mask and reports whether it is clear. */
extern "C" s32 fn_802B46DC(u8 index)
{
    return (((StageRuntime*)stage_w)->field_0x2F92 & (1 << index)) == 0;
}

/* Dispatches an area-index event to either the local stage or the caller's own handler. */
extern "C" void fn_802B4680(void* plw, u8 index)
{
    if (index >= 0x10U) {
        return;
    }
    if (plw == NULL) {
        fn_802B45F4(index);
        return;
    }
    if (Pl_master_ck((struct _PLW*)plw) == 1U) {
        lb_sub12_send(index);
    }
}

/* Ages the stage block's per-area timers and clears the bits whose 4-second window has lapsed. */
extern "C" void fn_802B4704(StageRuntime* st)
{
    s32 now;
    s32 span;
    u8 index;

    now = quest_time_limit_get();
    span = now - quest_time_elapsed_get();
    if (Pl_motion_input_ck(NULL) == 1U) {
        return;
    }
    for (index = 0; index < 0x10U; index++) {
        if (fn_802B46DC(index) == 0) {
            if (span - (s32)st->field_0x2F94[index] >= 0x2A30) {
                st->field_0x2F92 ^= 1 << index;
                st->field_0x2F94[index] = 0;
            }
        }
    }
}

/* Reports whether every area from 1 to 4 is armed for the current map's stage kind 4. */
extern "C" u32 stage_gate_b_ck(void)
{
    StageRuntime* st = (StageRuntime*)stage_w;
    u8 index;

    if (stage_map_kind_get(st->mapno) != 4) {
        return 0;
    }
    index = 1;
    while (index < 5U) {
        if (fn_802B46DC(index) == 1U) {
            return 0;
        }
        index++;
    }
    return 1;
}

/* Re-drives every live area actor's per-joint frame init for the palette the kind selects. */
extern "C" void fn_802B4824(u8 kind)
{
    f32 scale;
    f32 amount;
    u8 index;
    u8 joint;
    StageRuntime* st = (StageRuntime*)stage_w;

    switch (kind) {
    case 18:
        amount = lbl_8079A4B8;
        break;
    case 33:
        fn_802B4680(NULL, 0);
        return;
    case 47:
        amount = lbl_8079A4BC;
        break;
    default:
        return;
    }
    index = 0;
    scale = lbl_8079A484;
    for (; index < 4U; index++) {
        if (st->area_char[index] != NULL) {
            for (joint = 0; joint < ((StageActorJoints*)st->area_char[index])->joint_count; joint++) {
                st->area_char[index]->frame_init(joint, (u16)joint, amount * scale, 0, lbl_8079A468);
            }
        }
    }
}

/* Arms the map's joint sound effects for the current area. */
extern "C" void fn_802B493C(StageRuntime* st)
{
    nw4r::math::VEC3 pos;
    MHchar* chr;
    u8* list;
    u8* p;
    s32 sound;
    s32 target;

    VEC3_ctor(&pos);
    list = NULL;
    sound = 0;
    target = 0;
    switch (st->mapno) {
    case 21:
        if (getItemListSelection() == 0) {
            switch (st->areano) {
            case 0:
                list = lbl_80792498;
                sound = 1;
                break;
            case 7:
                list = lbl_8079249C;
                break;
            }
        }
        break;
    case 8:
    case 11:
        if (st->areano == 0) {
            list = lbl_807924A0;
        }
        break;
    case 9:
        if (st->areano == 0) {
            list = lbl_807924A4;
        }
        break;
    case 10:
        if (st->areano == 1) {
            list = lbl_807924A8;
            sound = 0;
            target = 1;
        }
        break;
    }
    if (list == NULL) {
        return;
    }
    chr = st->area_char[0];
    for (p = list; (s8)*p > 0; p++) {
        chr->get_joint_wpos((s8)*p, &pos);
        switch (target) {
        case 0:
            shell_se_req(NULL, &pos, 0x10, sound);
            break;
        case 1:
            shell_se_req(NULL, &pos, 0x11, sound);
            break;
        }
    }
}

/* Places the map's demo marker objects on their scripted offsets. */
extern "C" void fn_802B4ABC(StageRuntime* st)
{
    nw4r::math::VEC3 pos;
    StageDemoEntry* entry;
    StageDemoEntry* p;
    int mapno;
    u16 secs;
    s32 now;
    s32 base;

    VEC3_ctor(&pos);
    entry = NULL;
    mapno = get_now_mapno();
    if (mapno != 5 && mapno != 0x10 && mapno != 0xA) {
        return;
    }
    now = quest_time_limit_get();
    base = quest_time_elapsed_get();
    secs = (u16)((now - base) / 300);
    secs += quest_clock_byte_get(0);
    if (--st->field_0x2FDC > 0) {
        return;
    }
    if (((now - base) & 0x40) == 0x40) {
        st->field_0x2FDC = 0;
        return;
    }
    if (((now - base) & 0x30) != 0) {
        st->field_0x2FDC = 0;
        return;
    }
    if (event_demo_ck() == 1U) {
        return;
    }
    for (p = lbl_805CFA38; p->mapno != 0xFF; p++) {
        if (p->mapno == get_now_mapno()) {
            entry = (StageDemoEntry*)p->offsets;
            break;
        }
    }
    if (entry == NULL) {
        return;
    }
    for (;;) {
        if (entry->mapno == 0xFF) {
            break;
        }
        vec_to_mh_vec3(&pos, (struct Vec*)(entry->offsets + (secs % entry->count) * 0xC));
        shell_set_func_ptr->method_0x80(&pos, entry->mapno, 0x8C, shell_set_func_ptr);
        entry++;
    }
    st->field_0x2FDC = 0x12C;
}

/* Re-drives every live area object's own per-frame update. */
extern "C" void fn_802B4C5C(void)
{
    StageRuntime* st = (StageRuntime*)stage_w;
    u8 index;

    if (GameMode_ck() == 1) {
        return;
    }
    index = 0;
    do {
        if (st->area_char[index] != NULL) {
            fn_802B3270(st, st->area_char[index], index);
        }
        index++;
    } while (index < 4U);
}

/* Returns the blend table for the map's current area group. */
extern "C" StageBlendEntry* fn_802B4CD0(void)
{
    StageBlendEntry* table;
    u8 areano;

    areano = ::get_now_areano();
    if ((u32)(areano - 5) <= 2U) {
        return lbl_805CFAE8;
    }
    table = lbl_805CFA68;
    if (areano == 4) {
        table = lbl_805CFB68;
    }
    return table;
}

/* Blends the two areas the 30-second colour cycle straddles into a wrapped 0..1 value. */
extern "C" f32 fn_802B4D24(s16 index, f32 elapsed)
{
    StageBlendEntry* table;
    u32 kind;
    f32 value;

    table = fn_802B4CD0();
    kind = *(u32*)((u8*)table + index * 0x10);
    if (kind < 4U) {
        value = lbl_805CFA58[kind];
    } else if (kind == 5) {
        if (index >= 7) {
            value = lbl_805CFA58[table->kind] - elapsed;
        } else {
            value = lbl_805CFA58[*(u32*)((u8*)table + (index + 1) * 0x10)] - elapsed;
        }
    } else if (index == 0) {
        value = elapsed + lbl_805CFA58[table[1].colour_c];
    } else {
        value = elapsed + lbl_805CFA58[*(u32*)((u8*)table + (index - 1) * 0x10)];
    }
    if (value < lbl_8079A448) {
        return value + lbl_8079A468;
    }
    if (value > lbl_8079A468) {
        value -= lbl_8079A468;
    }
    return value;
}

/* Interpolates the two areas' colour records by the cycle weight and arms the result. */
extern "C" void fn_802B4E58(void)
{
    StageBlendEntry* table;
    u32 colour[6];
    u32 from;
    u32 to;
    f32 peak;
    f32 step;
    f32 best;
    f32 prev;
    f32 frac;
    f32 span;
    f32 weight;
    f32 fa;
    f32 fb;
    f32 fc;
    f32 fd;
    f32 fe;
    f32 ff;
    f32 fg;
    f32 fh;
    u32 count;
    s16 low;
    s16 high;
    s16 i;

    count = fn_8021F218();
    peak = fn_8021F228(count);
    table = fn_802B4CD0();
    step = lbl_8079A4C0 / (f32)count;
    low = 7;
    high = 8;
    prev = 0.0f;
    best = 0.0f;
    for (i = 0; i < 8; i++) {
        best = fn_802B4D24(i, step);
        if (best > peak) {
            high = i;
            break;
        }
        low = i;
        prev = best;
    }
    if (high == 0) {
        prev = fn_802B4D24(low, step);
    }
    if (high >= 8) {
        high = 0;
        best = fn_802B4D24(0, step);
    }
    if (high <= 0) {
        span = (lbl_8079A468 - prev) + best;
        frac = peak - prev;
        if (frac < lbl_8079A448) {
            frac += lbl_8079A468;
        }
    } else {
        span = best - prev;
        frac = peak - prev;
    }
    weight = frac / span;

    from = table[low].colour_a;
    to = table[high].colour_a;
    fa = (f32)(from >> 24);
    fb = (f32)(u8)(from >> 16);
    fc = (f32)(u8)(from >> 8);
    fd = (f32)(u8)from;
    colour[0] = (u8)(s32)(fd + weight * ((f32)(u8)to - fd))
              | ((u8)(s32)(fc + weight * ((f32)(u8)(to >> 8) - fc)) << 8)
              | ((u8)(s32)(fa + weight * ((f32)(to >> 24) - fa)) << 24)
              | ((u8)(s32)(fb + weight * ((f32)(u8)(to >> 16) - fb)) << 16);
    colour[1] = colour[0];

    from = table[low].colour_b;
    to = table[high].colour_b;
    fe = (f32)from;
    ff = (f32)(u8)(from >> 16);
    fg = (f32)(u8)(from >> 8);
    fh = (f32)(u8)from;
    colour[2] = (u8)(s32)(fh + weight * ((f32)(u8)to - fh))
              | ((u8)(s32)(fg + weight * ((f32)(u8)(to >> 8) - fg)) << 8)
              | ((u8)(s32)(fe + weight * ((f32)(to >> 24) - fe)) << 24)
              | ((u8)(s32)(ff + weight * ((f32)(u8)(to >> 16) - ff)) << 16);
    colour[3] = colour[2];

    from = table[low].colour_c;
    to = table[high].colour_c;
    colour[4] = (u8)(s32)((f32)(u8)from + weight * ((f32)(u8)to - (f32)(u8)from))
              | ((u8)(s32)((f32)(u8)(from >> 8) + weight * ((f32)(u8)(to >> 8) - (f32)(u8)(from >> 8))) << 8)
              | ((u8)(s32)((f32)(from >> 24) + weight * ((f32)(to >> 24) - (f32)(from >> 24))) << 24)
              | ((u8)(s32)((f32)(u8)(from >> 16) + weight * ((f32)(u8)(to >> 16) - (f32)(u8)(from >> 16))) << 16);
    colour[5] = colour[4];

    fn_80057DE0(0, 6, colour, lbl_8079A4C4);
}

/* Seeds the three per-area effect seats and the stage block's first area record. */
extern "C" void fn_802B535C(void)
{
    fn_802B53CC((u32)stage_w);
    /* Each seat is its own `lbl_` symbol (0xC apart); a `VEC3[]` view would fold them into one
     * relocation (measured 100 -> 80.86 on fn_802B535C). */
    setVec3((nw4r::math::VEC3*)lbl_806BB7B8, lbl_8079A448, lbl_8079A448, lbl_8079A448);
    setVec3((nw4r::math::VEC3*)lbl_806BB7C4, lbl_8079A4C8, lbl_8079A4CC, lbl_8079A4D0);
    setVec3((nw4r::math::VEC3*)lbl_806BB7D0, lbl_8079A448, lbl_8079A448, lbl_8079A448);
}

/* Runs the stage block's cleanup over every sub-block array it owns. */
extern "C" u32 fn_802B53CC(u32 base)
{
    u32 p;

    p = base + 0x24;
    do {
        fn_802B55E8(p);
        p += 0x5D0;
    } while (p < base + 0xBC4);
    p = base + 0xC08;
    VEC3_ctor((Vec3*)p);
    p = base + 0xC20;
    do {
        fn_802B5538(p);
        p += 0x1DC;
    } while (p < base + 0x2804);
    p = base + 0x2808;
    VEC3_ctor((Vec3*)p);
    p = base + 0x2C2C;
    do {
        fn_802B54C4(p);
        p += 0xA0;
    } while (p < base + 0x2EAC);
    p = base + 0x2EC0;
    do {
        fn_802B5488(p);
        p += 0x34;
    } while (p < base + 0x2F90);
    return base;
}

/* Clears the two 12-byte vector pairs inside one 0x34-byte sub-block. */
extern "C" u32 fn_802B5488(u32 block)
{
    u32 p;

    p = block + 4;
    fn_800FA420((Vec3*)p);
    p = block + 0x14;
    fn_800FA3B8((Vec3*)p);
    return block;
}

/* Clears the 12-byte records of one 0xA0-byte sub-block. */
extern "C" u32 fn_802B54C4(u32 block)
{
    u32 p;

    p = block + 4;
    do {
        VEC3_ctor((Vec3*)p);
        p += 0xC;
    } while (p < block + 0x28);
    p = block + 0x4C;
    do {
        VEC3_ctor((Vec3*)p);
        p += 0xC;
    } while (p < block + 0x70);
    return block;
}

/* Clears the 0x30-byte slots of one 0x1DC-byte area record. */
extern "C" u32 fn_802B5538(u32 rec)
{
    u32 p;

    p = rec + 4;
    do {
        fn_802B5590(p);
        p += 0x30;
    } while (p < rec + 0x124);
    return rec;
}

/* Clears the 12-byte records inside one 0x30-byte slot. */
extern "C" u32 fn_802B5590(u32 slot)
{
    u32 p;

    p = slot + 0xC;
    do {
        VEC3_ctor((Vec3*)p);
        p += 0xC;
    } while (p < slot + 0x30);
    return slot;
}

/* Clears the 0x164-byte records of one 0x5D0-byte sub-block. */
extern "C" u32 fn_802B55E8(u32 block)
{
    u32 p;

    p = block + 4;
    do {
        mhchar_construct((void*)p);
        p += 0x164;
    } while (p < block + 0x594);
    return block;
}
}  /* namespace view_fn_802B2AA0 */
