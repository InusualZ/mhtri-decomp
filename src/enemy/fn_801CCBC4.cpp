/* enemy/fn_801CCBC4.cpp - the enemy action/step band `.text` 0x801CCBC4..0x801D428C (58 functions /
 * 0x76C8 bytes), plus the extab run 0x8000FF4C..0x800100D4 and the extabindex run
 * 0x8002BDDC..0x8002C028 its 49 framed functions carry.
 *
 * Registration (proposal/801CCBC4_fn_801CCBC4.cpp).  The range is registered once, here, at its final
 * home.  Which class decided the name and the module:
 *   * class 1 (a `__FILE__` string) fails.  The range's only data references are the `.sdata2` float
 *     pool (0x80799220..0x807993E4), the `.data` jump tables/dispatch tables (0x805B5000..0x805B61E8)
 *     and `.rodata` numeric tables (0x80570450..0x80570500) - no source-file-name literal is loaded
 *     anywhere in the range.  The one `enemy` source name in the image (`enemy_control.cpp`, at
 *     lbl_805A1BB8) is referenced only by the registered `enemy/enemy_control.cpp`, 0x8F000 below.
 *   * class 2 fails too: `python tools/symbols/dumpmap.py lookup` answers a `zz_XXXXXXX_` placeholder
 *     for every address of the range, and the brief states a `zz_` name is not evidence.
 *   * class 3 decides the module: `enemy`.  Both bracketing registered units are `enemy`
 *     (below `enemy/fn_801B7020.cpp` ends at 0x801BD6C0; above `enemy/fn_801D428C.cpp` starts at
 *     0x801D428C), every callee out of the range is an enemy-band function (`_ENEMY_WORK`-based
 *     `fn_8012xxxx`/`fn_8013xxxx` bodies, `em_frame_check`, `em_parts_damage_level_get`,
 *     `get_em_scale`), every function switches on `_ENEMY_WORK::state` (+0x05) or
 *     `_ENEMY_WORK::state_sub` (+0x1E6), and the range's own jump tables (`jumptable_805B5414`,
 *     `_805B546C`, `_805B54F4`, `_805B551C`, `_805B553C`, `_805B5D90`) sit in the enemy `.data` run.
 *   * class 4 keeps the name: nothing supports a file name, so the map's own `fn_801CCBC4` stem is the
 *     file name (the sibling units use the same scheme).  No name was invented.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x801CCBC4` - every address answers the runtime dump's
 * `zz_XXXXXXXX_` placeholder and a bare `fn_XXXXXXXX = .text:0x...` map entry, and no `__FILE__`
 * string is reachable from the range).
 *
 * Seam.  `tudiscover at 0x801CCBC4` reports NO strong boundary at the left edge - every candidate
 * around it is `weak share 0.000`, and the range owns no labelled data at all, so the tool's own
 * answer is "the boundary is unconstrained".  The left edge is the `--max-bytes` cut the brief warns
 * about, and the range below (proposal/801CA004, 0x801CA004..0x801CCBC4) is the same subsystem: this
 * range's dispatchers tail-call into it (`fn_801CBA4C`, `fn_801CBB0C`, `fn_801CBBD8`, `fn_801CBC64`,
 * `fn_801CBD30`, `fn_801CB308`, `fn_801CB9DC`) and its jump tables are contiguous with this range's in
 * the same `.data` run (`jumptable_805B53E4` -> `jumptable_805B5414` -> `jumptable_805B546C`).  The
 * two very probably belong to one TU; the range is worked as one unit and the extent settles as its
 * functions match (invariant 8.3).  The right edge 0x801D428C IS evidence: it is the start of the
 * registered `enemy/fn_801D428C.cpp`, whose extab run begins exactly where this range's ends
 * (0x800100D4) and whose `fn_801D4C3C` dispatch table tail-calls this range's functions.
 * Registration uses the pinned pool range (brief section 2) - extending it left is the orchestrator's
 * re-split decision, recorded as a note in this worker's outbox.
 *
 * Sections claimed: `.text` 0x801CCBC4..0x801D428C, extab 0x8000FF4C..0x800100D4, extabindex
 * 0x8002BDDC..0x8002C028.  No `.ctors`/.dtors word belongs to the range (the `.ctors` words at
 * 0x8056F34C/0x8056F354/0x8056F358 point at `fn_801B985C`, `fn_801CA870` and `fn_801D6FB8`, none of
 * them in this range).
 *
 * Language.  C++: the range reaches mangled callees (`em_frame_check__FP11_ENEMY_WORKUsff`,
 * `setVector3__FPQ34nw4r4math4VEC3fff`, `calcVecAng2__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3`,
 * `get_em_scale__FP11_ENEMY_WORK`), so C++ is settled by class 1 of the language probe.  Rule 9: each
 * is declared at C++ scope with the signature its mangling encodes and called through it; every plain
 * `fn_XXXXXXXX` definition stays `extern "C"` so the map name is emitted.
 *
 * STATUS (measured in this worktree with `python tools/units/recompile.py
 * enemy/fn_801CCBC4.cpp --measure <symbol>`; the worktree also passes a full `ninja
 * build/RMHE08/ok` -> `build/RMHE08/main.dol: OK`).  This is a PARTIAL landing: 45 of the range's 58
 * functions are reconstructed and EVERY one of them is at or above the 80 % bar; 27 of them are
 * byte-identical (100.00 %).  The report's own unit-level number for the whole range is
 * `fuzzy_match_percent` 46.47 (6364 of 30408 bytes matched, 28 of 58 functions) - it is byte-weighted,
 * and the 13 unwritten rows are 16044 of the range's 30408 bytes, which is what holds it down; the
 * mean over the 45 written rows is 98.10.  The 13 unwritten ones are the residual this worker hands on (address,
 * size in bytes): `fn_801CF0C0` (1416), `fn_801CF840` (948), `fn_801CFE04` (2408), `fn_801D0B94`
 * (1100), `fn_801D1110` (1152), `fn_801D1590` (844), `fn_801D18DC` (1316), `fn_801D1E00` (836),
 * `fn_801D2144` (1992), `fn_801D320C` (856), `fn_801D3564` (828), `fn_801D38A0` (920),
 * `fn_801D3CF8` (1428) - they are the range's remaining state machines and the two biggest
 * parameter-table bodies, so they are incremental work, not a blocked seam.
 *
 * Residuals of the 45 written functions (all are codegen shapes, not comprehension):
 *   * ONE INSTRUCTION / REGISTER CHOICE - `fn_801CFBF4` 97.40, `fn_801CE4A0` 96.88,
 *     `fn_801CEE74` 96.15, `fn_801CEF44` 95.79, `fn_801D0FE0` 94.74: the control flow and every call
 *     match; a single register or the compare's operand order differs (e.g. `cmplw a,b` for the
 *     target's `cmplw b,a`).  Nothing to change without moving a landed owner's header.
 *   * `fn_801CD71C` 84.43 (388 B vs our 392 B) - the target reuses `r3` for the (short-lived) area
 *     target pointer that `fn_80131034` returns, so it needs no third callee-saved register; every
 *     spelling tried (a named local, a ternary, an early `break`) allocates one, costing the extra
 *     `stw`/`lwz` pair.  Recorded rather than restructured.
 *   * `fn_801CD400` 93.55, `fn_801CD944` 99.13, `fn_801CEC44` 99.81, `fn_801CD068` 99.81,
 *     `fn_801CDCDC` 98.70, `fn_801CE2B0` 97.35, `fn_801D08A8` 99.95, `fn_801D2B10` 99.83,
 *     `fn_801D2ED0` 97.78, `fn_801CDF10` 96.92 - one hoisted `li`/`mr`, one argument's materialisation
 *     order (the int before the pool float, or the reverse) or one commutative `fmuls` operand order.
 *
 * Pragma.  The whole unit is compiled with `#pragma peephole off` AND `#pragma fp_contract off`.
 * `peephole off` is load-bearing: retail keeps the unfused `clrlwi`/`rlwinm` + `cmpwi` pairs `-O3`
 * folds into their record forms (measured on `fn_801CCBC4`'s mode test, `fn_801CD2EC` and the whole
 * band - the same finding `enemy/fn_801D428C.cpp` recorded).  `fp_contract off` is measured too: with
 * the lib's `-fp_contract on` the three sites that multiply an effect scale and then add it
 * (`fn_801CD068` case 2/3, `fn_801CD944` state 2, `fn_801CEC44` state 2) fuse into `fmadds` where
 * retail keeps `fmuls` + `fadds`; turning it off moved exactly those three up (97.81 -> 99.81,
 * 98.41 -> 99.13, 98.29 -> 99.81) and left the other 42 scores unchanged.  Both are per-unit
 * `#pragma`s, the scoped deviation invariant 8.2 allows, not a lib flag.
 */

#include "types.h"
#include "nw4r/math.h"

/* The whole band is compiled with the peephole optimizer OFF: retail keeps the unfused
 * `clrlwi`/`rlwinm` + `cmpwi` pairs that `-O3` fuses into their record forms (`clrlwi.`),
 * measured on this range's own functions - the same finding `enemy/fn_801D428C.cpp` and
 * `enemy/fn_80147CE0.cpp` recorded.  A per-unit `#pragma` is the scoped deviation invariant 8.2
 * allows for one unit. */
#pragma peephole off
#pragma fp_contract off

#include "enemy/ENEMY_WORK.h"
#include "fn_8004CAD8.h"

/* The band's owner headers are NOT included: several of them publish spellings that do not match the
 * callees' own bodies (`fn_8012EC60(void)` where the body reads +0x8AA, `fn_80126278(u16,VEC3*)`
 * where the body takes `self` in r3), and MWCC rejects two C-linkage declarations of the same name in
 * one TU (`10197 illegal function overloading`).  This unit therefore declares its foreign callees
 * itself - the same shape the neighbouring `enemy/fn_801D428C.cpp` uses - and the owner-header
 * corrections are recorded as a `shared-file` request in this worker's outbox. */

/* ----------------------------------------------------------------------------------------------------
 * The `.sdata2` float pool this range loads (0x80799220..0x807993E4).  Its labels are unsplit - no
 * registered unit claims a `.sdata2` range - so they are declared here and never defined: the target
 * addresses the pool.  This is the counted rule-2 "address band interleaves modules" gap.
 * -------------------------------------------------------------------------------------------------- */

extern f32 lbl_80799220;
extern f32 lbl_80799240;
extern f32 lbl_8079924C;
extern f32 lbl_80799250;
extern f32 lbl_80799258;
extern f32 lbl_8079925C;
extern f32 lbl_80799264;
extern f32 lbl_80799268;
extern f32 lbl_8079926C;
extern f32 lbl_80799270;
extern f32 lbl_80799274;
extern f32 lbl_80799278;
extern f32 lbl_8079927C;
extern f32 lbl_80799280;
extern f32 lbl_80799284;
extern f32 lbl_80799288;
extern f32 lbl_8079928C;
extern f32 lbl_80799294;
extern f32 lbl_80799298;
extern f32 lbl_807992A0;
extern f32 lbl_807992A8;
extern f32 lbl_807992B0;
extern f32 lbl_807992B8;
extern f32 lbl_807992BC;
extern f32 lbl_807992C0;
extern f32 lbl_807992C4;
extern f32 lbl_807992C8;
extern f32 lbl_807992CC;
extern f32 lbl_807992D0;
extern f32 lbl_807992D4;
extern f32 lbl_807992D8;
extern f32 lbl_807992DC;
extern f32 lbl_807992E0;
extern f32 lbl_807992E4;
extern f32 lbl_807992E8;
extern f32 lbl_807992EC;
extern f32 lbl_807992F0;
extern f32 lbl_807992F4;
extern f32 lbl_807992F8;
extern f32 lbl_807992FC;
extern f32 lbl_80799300;
extern f32 lbl_80799304;
extern f32 lbl_80799308;
extern f32 lbl_8079930C;
extern f32 lbl_80799310;
extern f32 lbl_80799314;
extern f32 lbl_80799318;
extern f32 lbl_8079931C;
extern f32 lbl_80799320;
extern f32 lbl_80799324;
extern f32 lbl_80799328;
extern f32 lbl_8079932C;
extern f32 lbl_80799330;
extern f32 lbl_80799334;
extern f32 lbl_80799338;
extern f32 lbl_8079933C;
extern f32 lbl_80799340;
extern f32 lbl_80799344;
extern f32 lbl_80799348;
extern f32 lbl_8079934C;
extern f32 lbl_80799350;
extern f32 lbl_80799354;
extern f32 lbl_80799358;
extern f32 lbl_8079935C;
extern f32 lbl_80799360;
extern f32 lbl_80799364;
extern f32 lbl_80799368;
extern f32 lbl_8079936C;
extern f32 lbl_80799370;
extern f32 lbl_80799374;
extern f32 lbl_80799378;
extern f32 lbl_8079937C;
extern f32 lbl_80799380;
extern f32 lbl_80799384;
extern f32 lbl_80799388;
extern f32 lbl_8079938C;
extern f32 lbl_80799390;
extern f32 lbl_80799394;
extern f32 lbl_80799398;
extern f32 lbl_8079939C;
extern f32 lbl_807993A0;
extern f32 lbl_807993A4;
extern f32 lbl_807993A8;
extern f32 lbl_807993AC;
extern f32 lbl_807993B0;
extern f32 lbl_807993B4;
extern f32 lbl_807993B8;
extern f32 lbl_807993BC;
extern f32 lbl_807993C0;
extern f32 lbl_807993C4;
extern f32 lbl_807993C8;
extern f32 lbl_807993CC;
extern f32 lbl_807993D0;
extern f32 lbl_807993D4;
extern f32 lbl_807993D8;
extern f32 lbl_807993DC;
extern f32 lbl_807993E0;
extern f32 lbl_807993E4;

/* The three-word scratch record `fn_801354F4` takes: its other call sites hand it
 * `&_ENEMY_WORK::field_0x1BC` and a `VEC3*`, and `fn_801CEF44` builds one from the latched
 * rotation word and two zero words.  No function reads its fields by name, so only the layout and
 * the size the target's own stores spell are stated.
 * size: 0x0C */
struct EmWord3 {
    /* +0x0 */ u32 x;
    /* +0x4 */ u32 y;
    /* +0x8 */ u32 z;
};

/* ----------------------------------------------------------------------------------------------------
 * The C++-mangled callees (rule 9): declared at C++ scope with the signature the mangling encodes,
 * so the compiler mangles them back to exactly these map names, and called through them.
 * -------------------------------------------------------------------------------------------------- */

f32 get_em_scale(struct _ENEMY_WORK* self);              /* get_em_scale__FP11_ENEMY_WORK */
f32 get_em_chg_scale(struct _ENEMY_WORK* self);          /* get_em_chg_scale__FP11_ENEMY_WORK */
u32 em_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
                                                         /* em_frame_check__FP11_ENEMY_WORKUsff */
u32 em_after_frame_check(struct _ENEMY_WORK* self, u16 a, f32 b, f32 c);
                                                         /* em_after_frame_check__FP11_ENEMY_WORKUsff */
u8 em_parts_damage_level_get(struct _ENEMY_WORK* self, u8 part);
                                                         /* em_parts_damage_level_get__FP11_ENEMY_WORKUc */

/* `.data` objects with no registered owner (the same counted rule-2 gap): `lbl_805B5088` is the
 * motion/parameter record `fn_80135644` reads and the `jumptable_*`s are the band's dispatch tables. */
extern u8 lbl_805B5088[];
extern u8 jumptable_805B5414[];
extern u8 jumptable_805B546C[];
extern u8 jumptable_805B54F4[];
extern u8 jumptable_805B551C[];
extern u8 jumptable_805B553C[];
extern u8 jumptable_805B5D90[];
extern u8 lbl_805704D0[];
extern u8 lbl_805B5000[];
extern u8 lbl_805B5050[];
extern u8 lbl_80570490[];
extern u8 lbl_805B50C0[];
extern u8 lbl_805B50F8[];
extern u8 lbl_805B6078[];
extern u8 lbl_805B6108[];
extern u8 lbl_805B6170[];
extern u8 lbl_805B61B8[];
extern u8 lbl_805B61E8[];
extern u8 lbl_805B5618[];
extern u8 lbl_805B5640[];
extern u8 lbl_805B5668[];
extern u8 lbl_805B5690[];
extern u8 lbl_805B56D0[];
extern u8 lbl_805B5730[];
extern u8 lbl_805B57A0[];
extern u8 lbl_805B5840[];
extern u8 lbl_805B58B0[];
extern u8 lbl_805B58D8[];
extern u8 lbl_805B5918[];
extern u8 lbl_805B5948[];
extern u8 lbl_805B59A0[];
extern u8 lbl_805B5A08[];
extern u8 lbl_805B5A40[];
extern u8 lbl_805B5AB0[];
extern u8 lbl_805B5B10[];
extern u8 lbl_805B5B38[];
extern u8 lbl_805B5B60[];
extern u8 lbl_805B5B88[];
extern u8 lbl_805B5BF8[];
extern u8 lbl_805B5C58[];
extern u8 lbl_805B5C80[];
extern u8 lbl_805B5CA8[];
extern u8 lbl_805B5CE8[];
extern u8 lbl_805B5D60[];

extern "C" {

/* ----------------------------------------------------------------------------------------------------
 * The enemy-band callees.  Signatures are the call sites' registers, cross-checked against the
 * bodies the neighbouring registered units already wrote; where an owner header disagrees with the
 * body, the body wins and the correction is recorded in this worker's outbox.
 * -------------------------------------------------------------------------------------------------- */

/* enemy/fn_801251D0.cpp (0x801251D0..0x8012BA00) */
void fn_801251D0(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b);void fn_801251D8(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b);
void fn_801277F4(struct _ENEMY_WORK* self, u32 a);
void fn_80127F48(struct _ENEMY_WORK* self);
void fn_80127FE4(struct _ENEMY_WORK* self);
void fn_801280AC(struct _ENEMY_WORK* self);
void fn_80128A14(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80128A70(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80128BF8(struct _ENEMY_WORK* self, u32 a);
void fn_8012933C(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void fn_80129668(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80129724(struct _ENEMY_WORK* self, u32 a);
void fn_80129744(struct _ENEMY_WORK* self, u32 a);

/* enemy/fn_8012BDF4.cpp (0x8012BDF4..0x8012E968) */
void fn_8012B380(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void CancelFade(struct _ENEMY_WORK* self);
void fn_8012CF20(struct _ENEMY_WORK* self);
u32 fn_8012D1A0(struct _ENEMY_WORK* self);

/* enemy/fn_8012E968.cpp (0x8012E968..0x8012EC74) */
u32 fn_8012EC3C(struct _ENEMY_WORK* self);

/* enemy/fn_8012EC74.cpp (0x8012EC74..0x80137604) */
void fn_8012F5B8(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void fn_8012F7D4(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c, f32 d);
void fn_8012F810(struct _ENEMY_WORK* self);
void fn_8012F860(struct _ENEMY_WORK* self, f32 a, f32 b);
void fn_8012F62C(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void fn_8012F8C8(struct _ENEMY_WORK* self, f32 a);
f32 fn_8012F8E4(struct _ENEMY_WORK* self);
f32 fn_8012F8EC(struct _ENEMY_WORK* self);
u32 fn_8012F93C(struct _ENEMY_WORK* self);
s32 fn_8012F948(struct _ENEMY_WORK* self);
u32 fn_80130008(struct _ENEMY_WORK* self, f32 a);
f32 fn_80130248(struct _ENEMY_WORK* self);
f32 fn_8013032C(struct _ENEMY_WORK* self);
void fn_801303EC(struct _ENEMY_WORK* self, f32 a);
void fn_801303FC(struct _ENEMY_WORK* self, f32 a);
void fn_80130478(struct _ENEMY_WORK* self, u32 a);
void fn_801305C4(struct _ENEMY_WORK* self);
void fn_80130CDC(struct _ENEMY_WORK* self, u32 a);
struct _ENEMY_WORK* fn_80131034(struct _ENEMY_WORK* self, u8 kind, u8 distance_check);
void fn_80131BD4(struct _ENEMY_WORK* self);
void fn_80131D84(struct _ENEMY_WORK* self);
void fn_80131D9C(struct _ENEMY_WORK* self);
void fn_80131E00(struct _ENEMY_WORK* self);
void fn_80131E74(struct _ENEMY_WORK* self);
void fn_80133C3C(struct _ENEMY_WORK* self);
f32 fn_802B0430(u8 area);
void* fn_80041E40(void* dst, const void* src); /* owner `src/mh3_pad.cpp`; its header is
    * unreachable from an `include/ef.h` consumer (`fn_80043EA8`/`fn_80041E8C` conflict), so the
    * shape here is the owner body's (`mr r3,r31` -> returns `dst`) */
void fn_80043EA8(void* out);
u32 fn_80133C50(struct _ENEMY_WORK* self, u32 a);
void fn_80133CC8(struct _ENEMY_WORK* self, u32 a, u32 b);
void fn_80133DB0(struct _ENEMY_WORK* self, u32 a);
void fn_80133E3C(struct _ENEMY_WORK* self, s32 a, f32 b, f32 c);
void fn_80134004(struct _ENEMY_WORK* self, u32 a, f32 b);
u32 fn_80134114(struct _ENEMY_WORK* self, s32 a, s32 b);
void fn_80134964(struct _ENEMY_WORK* self, void* tbl, u32 a, u32 b, u32 c);
u32 fn_80134B0C(struct _ENEMY_WORK* self, void* tbl);
void fn_80134DF4(struct _ENEMY_WORK* self);
void fn_80134E28(struct _ENEMY_WORK* self);
void fn_80134E8C(struct _ENEMY_WORK* self);
void fn_80134F18(struct _ENEMY_WORK* self);
void fn_80134F70(struct _ENEMY_WORK* self, void* tbl);
void fn_80135000(struct _ENEMY_WORK* self, u32 a, void* tbl);
void fn_801353E4(struct _ENEMY_WORK* self);
void fn_801353F8(struct _ENEMY_WORK* self);
void fn_80135418(struct _ENEMY_WORK* self);
void fn_801354F4(struct _ENEMY_WORK* self, void* p);
void fn_80135584(struct _ENEMY_WORK* self, void* p);
void fn_801355C8(struct _ENEMY_WORK* self, void* p);
u32 fn_80135600(struct _ENEMY_WORK* self, void* p);
f32 fn_80135644(struct _ENEMY_WORK* self, void* tbl);
f32 fn_801356A8(struct _ENEMY_WORK* self, f32 a, f32 b, f32 c);
void fn_80136D14(struct _ENEMY_WORK* self);

/* enemy/fn_8013ACC4.cpp (0x8013ACC4..0x8013BE60) */
void fn_8013AAC4(struct _ENEMY_WORK* self);
u32 fn_802B0668(u8 kind);

/* enemy/enemy_control.cpp (0x801411B8..0x80147CE0) */
void fn_80149788(struct _ENEMY_WORK* self, u32 a);

/* the unclaimed enemy action band below this range (0x801CA004..0x801CCBC4), which this range's
 * dispatchers tail-call.  Its owner is not registered yet, so the declarations sit here; the band
 * header `include/unsplit/enemy.h` is where they move once those ranges land (rule 2). */
void fn_801CAF70(struct _ENEMY_WORK* self);
void fn_801CAFBC(struct _ENEMY_WORK* self);
void fn_801CB008(struct _ENEMY_WORK* self);
void fn_801CB050(struct _ENEMY_WORK* self);
void fn_801CBA4C(struct _ENEMY_WORK* self, u32 a);
void fn_801CBB0C(struct _ENEMY_WORK* self, u32 a);
void fn_801CBBD8(struct _ENEMY_WORK* self);
void fn_801CBC64(struct _ENEMY_WORK* self, u32 a);
void fn_801CBD30(struct _ENEMY_WORK* self, u32 a, u32 b, u32 c);
void fn_801CC5DC(struct _ENEMY_WORK* self, u32 a);
void fn_801CCB6C(struct _ENEMY_WORK* self, u32 a, u32 b);

/* enemy/fn_801D428C.cpp (0x801D428C..0x801D80EC) - the unit above; it declares this range's
 * dispatchers, so the two units agree on their signatures. */
void fn_801D6548(struct _ENEMY_WORK* self, u8 a);
void fn_801D6EDC(void* rec, u8 a, u16 b, u16 c);
s32 fn_801D80EC();


} /* extern "C" */

/* ----------------------------------------------------------------------------------------------------
 * The range's own step functions, called by its own state machines and by the unit above.  Declared
 * as the target's bodies are (one `_ENEMY_WORK*`; `fn_801D3CF8` also takes a mode word).
 * -------------------------------------------------------------------------------------------------- */

extern "C" {
void fn_801CCBC4(struct _ENEMY_WORK* self, u32 mode);
void fn_801CCCE8(struct _ENEMY_WORK* self);
void fn_801CCE10(struct _ENEMY_WORK* self);
void fn_801CCF50(struct _ENEMY_WORK* self, u8 mode);
void fn_801CD068(struct _ENEMY_WORK* self, u8 mode);
void fn_801CD2E8(struct _ENEMY_WORK* self);
void fn_801CD2EC(struct _ENEMY_WORK* self);
void fn_801CD400(struct _ENEMY_WORK* self, u8 mode);
void fn_801CD71C(struct _ENEMY_WORK* self, u8 mode);
void fn_801CD8A0(struct _ENEMY_WORK* self, u8 mode);
void fn_801CD944(struct _ENEMY_WORK* self);
void fn_801CDCDC(struct _ENEMY_WORK* self, u8 mode);
void fn_801CDD94(struct _ENEMY_WORK* self);
void fn_801CDE34(struct _ENEMY_WORK* self);
void fn_801CDF10(struct _ENEMY_WORK* self, u8 mode);
void fn_801CE014(struct _ENEMY_WORK* self);
void fn_801CE0C0(struct _ENEMY_WORK* self);
void fn_801CE190(struct _ENEMY_WORK* self);
void fn_801CE2B0(struct _ENEMY_WORK* self, u8 mode);
void fn_801CE3F0(struct _ENEMY_WORK* self);
void fn_801CE4A0(struct _ENEMY_WORK* self);
void fn_801CE5A0(struct _ENEMY_WORK* self, u8 mode);
void fn_801CE71C(struct _ENEMY_WORK* self, u8 mode);
void fn_801CE898(struct _ENEMY_WORK* self);
void fn_801CE99C(struct _ENEMY_WORK* self);
void fn_801CEA68(struct _ENEMY_WORK* self);
void fn_801CEB28(struct _ENEMY_WORK* self);
void fn_801CEC44(struct _ENEMY_WORK* self);
void fn_801CEDE8(struct _ENEMY_WORK* self);
void fn_801CEE74(struct _ENEMY_WORK* self);
void fn_801CEF44(struct _ENEMY_WORK* self, u8 mode);
void fn_801CF0C0(struct _ENEMY_WORK* self, u8 mode);
void fn_801CF648(struct _ENEMY_WORK* self);
void fn_801CF6A8(struct _ENEMY_WORK* self, u8 mode);
void fn_801CF840(struct _ENEMY_WORK* self, u8 mode);
void fn_801CFBF4(struct _ENEMY_WORK* self, u8 mode);
void fn_801CFD28(struct _ENEMY_WORK* self, u8 mode);
void fn_801CFE04(struct _ENEMY_WORK* self, u8 mode, u8 sub);
void fn_801D076C(struct _ENEMY_WORK* self, u8 mode);
void fn_801D08A8(struct _ENEMY_WORK* self);
void fn_801D0B94(struct _ENEMY_WORK* self, u8 mode);
void fn_801D0FE0(struct _ENEMY_WORK* self, u8 mode);
void fn_801D1110(struct _ENEMY_WORK* self, u8 mode);
void fn_801D1590(struct _ENEMY_WORK* self, u8 mode, u8 sub);
void fn_801D18DC(struct _ENEMY_WORK* self, u8 mode, u8 sub);
void fn_801D1E00(struct _ENEMY_WORK* self, u8 mode);
void fn_801D2144(struct _ENEMY_WORK* self, u8 mode, u8 sub);
void fn_801D290C(struct _ENEMY_WORK* self);
void fn_801D2B10(struct _ENEMY_WORK* self);
void fn_801D2E0C(struct _ENEMY_WORK* self);
void fn_801D2EB4(struct _ENEMY_WORK* self);
void fn_801D2EBC(struct _ENEMY_WORK* self);
void fn_801D2ED0(struct _ENEMY_WORK* self);
void fn_801D320C(struct _ENEMY_WORK* self);
void fn_801D3564(struct _ENEMY_WORK* self);
void fn_801D38A0(struct _ENEMY_WORK* self);
void fn_801D3C38(struct _ENEMY_WORK* self);
void fn_801D3CF8(struct _ENEMY_WORK* self, u32 mode);
} /* extern "C" */

extern "C" {

/* 0x801CCBC4 - the state machine of one enemy action: state 0 arms the motion and derives the aim
 * angle from the two positions, state 1 waits on the motion (`em_frame_check`) and closes it.  Called
 * by `fn_801CCCE8` with mode 0/1. */
void fn_801CCBC4(struct _ENEMY_WORK* self, u32 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xD1, 4, 0);
        {
            u32 angle = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
            if (angle >= 0x8000) {
                angle = 0x10000 - angle;
            }
            self->timer_0x020 = (s16)angle;
            if ((u32)self->timer_0x020 > 0x1555) {
                self->timer_0x020 = 0x1555;
            }
        }
        break;
    case 1:
        if ((u8)mode == 0 && em_frame_check(self, 3, lbl_80799220, lbl_807992C0) == 1) {
            f32 v = fn_8012F8E4(self);
            u16 angle = (u16)(s32)((lbl_8079927C * (lbl_807992C4 * v)) / lbl_80799278 + lbl_80799280);
            fn_80133C50(self, angle);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}


/* 0x801CCCE8 - the `state_sub` (+0x1E6) dispatcher, 22 ways.  Each arm is a tail call into the
 * neighbouring action band below this range; the range check the compiler emits (`cmplwi 0x15` +
 * `bgtlr`) is the switch's default (implicit return). */
void fn_801CCCE8(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801CBA4C(self, 0); break;
    case 1: fn_801CBA4C(self, 1); break;
    case 2: fn_801CBB0C(self, 0); break;
    case 3: fn_801CBB0C(self, 1); break;
    case 4: fn_801CBBD8(self); break;
    case 5: fn_801CBC64(self, 0); break;
    case 6: fn_801CBD30(self, 0, 0, 0); break;
    case 7: fn_801CBD30(self, 1, 0, 0); break;
    case 8: fn_801CC5DC(self, 0); break;
    case 9: fn_801CC5DC(self, 1); break;
    case 10: fn_801CCB6C(self, 0, 0); break;
    case 11: fn_801CBD30(self, 2, 0, 0); break;
    case 12: fn_801CBD30(self, 3, 0, 0); break;
    case 13: fn_801CBC64(self, 1); break;
    case 14: fn_801CBD30(self, 0, 1, 0); break;
    case 15: fn_801CBD30(self, 1, 1, 0); break;
    case 16: fn_801CBD30(self, 2, 1, 0); break;
    case 17: fn_801CBD30(self, 3, 1, 0); break;
    case 18: fn_801CCBC4(self, 0); break;
    case 19: fn_801CCBC4(self, 1); break;
    case 20: fn_801CBD30(self, 0, 1, 1); break;
    case 21: fn_801CBD30(self, 1, 1, 1); break;
    }
}

/* 0x801CCE10 - the four-state open/close action: state 0 arms, state 1 waits and closes, state 2
 * aims at the reference position, state 3 releases the part pair. */
void fn_801CCE10(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x29, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130248(self);
            fn_801305C4(self);
            fn_8012F5B8(self, 0x43, 0, 0);
            fn_801353E4(self);
        }
        break;
    case 2:
        self->field_0x314 = fn_80135644(self, lbl_805B5088);
        fn_80130248(self);
        fn_80135584(self, &self->field_0x1BC);
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x1A, 6, 0);
        }
        break;
    case 3:
        fn_80133C50(self, 0x100);
        if (fn_8012F93C(self) == 1) {
            fn_80128A70(self, 3, 2);
        }
        break;
    }
}

/* 0x801CCF50 - the same action shape as `fn_801CCE10` with the mode-1 prelude (`fn_8012CF20` +
 * `fn_80131E74`) and a shorter close (state 2 ends through `fn_80127FE4`). */
void fn_801CCF50(struct _ENEMY_WORK* self, u8 mode) {
    if (mode == 1) {
        fn_8012CF20(self);
        fn_80131E74(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x29, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130248(self);
            fn_801305C4(self);
            fn_8012F5B8(self, 0x43, 0, 0);
            fn_801353E4(self);
        }
        break;
    case 2:
        self->field_0x314 = fn_80135644(self, lbl_805B5088);
        fn_80130248(self);
        fn_80135584(self, &self->field_0x1BC);
        if (fn_8012F93C(self) == 1) {
            fn_80127FE4(self);
        }
        break;
    }
}

/* 0x801CD068 - the five-state landing action: arm, wait under an `fn_80130008` height test, then
 * descend onto the effect height (`field_0x20C`) at the effect scale, then release. */
void fn_801CD068(struct _ENEMY_WORK* self, u8 mode) {
    if (mode == 1) {
        fn_8012CF20(self);
        fn_80131E74(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F62C(self, 0x1A, 6, 0);
        fn_80134E8C(self);
        break;
    case 1:
        fn_80134F18(self);
        if (fn_80130008(self, lbl_807992C8) == 1) {
            f32 scale;
            self->state++;
            fn_8012F5B8(self, 0x1D, 6, 0);
            scale = fn_8012F8E4(self);
            self->field_0x314 = -(lbl_807992CC * get_em_scale(self) / lbl_80799284) * scale;
        }
        break;
    case 2:
        if (fn_8012F948(self) == 0) {
            fn_801354F4(self, &self->field_0x1BC);
        }
        {
            f32 scale = get_em_scale(self);
            if (self->pos.y - self->field_0x20C < fn_80130248(self) * scale) {
                self->pos.y = self->field_0x20C + fn_80130248(self) * get_em_scale(self);
            }
        }
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x2E, 6, 0);
        }
        break;
    case 3:
        fn_801354F4(self, &self->field_0x1BC);
        {
            f32 scale = get_em_scale(self);
            if (self->pos.y - self->field_0x20C < fn_80130248(self) * scale) {
                self->pos.y = self->field_0x20C + fn_80130248(self) * get_em_scale(self);
            }
        }
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0x41, 0, 0);
        }
        break;
    case 4:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801CD2E8 - an empty body (the map's 4-byte `blr`). */
void fn_801CD2E8(struct _ENEMY_WORK* self) {
}

/* 0x801CD2EC - the two-state "motion + ::UpdateValue" action: state 0 arms the motion, state 1
 * writes the aim-angle-derived value and hands the motion its angle pair. */
void fn_801CD2EC(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F810(self);
        fn_8012F5B8(self, 0x1F, 0, 0);
        break;
    case 1: {
        f32 angle;
        u16 raw;
        u16 delta;
        fn_80128BF8(self, 0);
        raw = (u16)calcVecAng2(&self->pos, &self->vec_0x36C);
        delta = raw - self->field_0x1C0;
        if (delta == 0) {
            angle = lbl_80799220;
        } else {
            angle = (f32)(s16)delta * lbl_80799278 / lbl_8079927C / lbl_80799284;
        }
        fn_8012F860(self, angle, lbl_807992D0);
        fn_8012F7D4(self, 0x23, 0x24, (u32)(s32)fn_8012F8EC(self), self->field_0x464);
        break;
    }
    }
}

/* 0x801CD400 - the "target search and approach" action: a mode-parameterised step machine whose
 * state 1 runs two sub-steps (`state_0x006` then `state_0x007`) and picks its target group through
 * `fn_80131034`; the arms that finish early leave through `return`, exactly as the target's shared
 * epilogue (`b .L_801CD704`) does. */
void fn_801CD400(struct _ENEMY_WORK* self, u8 mode) {
    fn_8012CF20(self);
    fn_80131D9C(self);
    if ((u8)fn_802B0668(self->field_0x1E0) == 4) {
        if ((u32)(self->area_no - 4) <= 2) {
            fn_80136D14(self);
        }
    }
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        fn_80134F70(self, lbl_805704D0);
        fn_80134004(self, 0x19, lbl_80799220);
        fn_80130248(self);
        fn_80135584(self, &self->field_0x1BC);
        break;
    case 1:
        if (mode == 1) {
            switch (self->state_0x006) {
            case 0:
                if (self->field_0x1F9 == 0) {
                    struct _ENEMY_WORK* target;
                    self->state_0x006++;
                    target = 0;
                    if (fn_8012EC3C(self) == 1 || self->field_0x8A2 >= 0xFA) {
                        target = fn_80131034(self, 0x1C, 0);
                    }
                    if (target != 0) {
                        self->state_0x007 = 1;
                        break;
                    }
                    if (self->field_0x43D == 1) {
                        fn_80131BD4(self);
                    }
                }
                break;
            case 1:
                switch (self->state_0x007) {
                case 1: {
                    struct _ENEMY_WORK* target = fn_80131034(self, 0x1C, 1);
                    if (target != 0) {
                        fn_8012B380(self, 3, 2, target->group);
                        fn_80128A14(self, 0x0D, 0);
                        fn_8013AAC4(self);
                        return;
                    }
                    break;
                }
                case 2:
                    if (calcDistanceSqXZ(&self->pos, &self->vec_0x36C) <= lbl_807992D4) {
                        fn_80128A14(self, 3, 8);
                        fn_8013AAC4(self);
                        return;
                    }
                    break;
                }
                break;
            }
        }
        if (fn_80134114(self, 0, 0) == 1) {
            switch (mode) {
            case 1:
                if (self->area_no == self->field_0x9F8 || self->field_0x9F8 == 0xFF) {
                    fn_801CAF70(self);
                } else {
                    fn_80128A14(self, 3, 9);
                }
                return;
            case 2:
                if (self->field_0x1E7 == 0) {
                    fn_801CAF70(self);
                } else {
                    fn_801277F4(self, 0);
                    fn_80128A70(self, 3, 0x0E);
                }
                return;
            default:
                fn_801CAF70(self);
                return;
            }
        }
        switch (mode) {
        case 2:
            fn_80135000(self, 2, lbl_805704D0);
            break;
        case 3:
            fn_80135000(self, 3, lbl_805704D0);
            break;
        default:
            fn_80135000(self, 2, lbl_805704D0);
            break;
        }
        fn_80130248(self);
        fn_80135584(self, &self->field_0x1BC);
        break;
    }
}

/* 0x801CD71C - the two-sub-step "aim at the reference height" action: `state_0x006` 0 runs the
 * reference record (`lbl_805B5000`) twice, sub-step 1 hands over to `fn_80134E28`; the mode picks
 * the height the work record's y is compared against (area height, own vector y, or `fn_802B0430`). */
void fn_801CD71C(struct _ENEMY_WORK* self, u8 mode) {
    f32 limit;
    fn_8012CF20(self);
    fn_80131D9C(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x1A, 6, 0);
        fn_801353E4(self);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            self->field_0x314 = fn_80135644(self, lbl_805B5000);
            fn_80135418(self);
            if (fn_8012F93C(self) == 1) {
                u8 count = self->state_0x007 + 1;
                self->state_0x007 = count;
                if (count >= 2) {
                    self->state_0x006++;
                    fn_80134DF4(self);
                }
            }
            break;
        case 1:
            fn_80134E28(self);
            break;
        }
        switch (mode) {
        case 1:
            limit = lbl_807992E0 + self->vec_0x36C.y;
            break;
        case 2:
            limit = self->vec_0x36C.y;
            break;
        default:
            limit = fn_802B0430(self->area_no) - lbl_807992D8;
            if (limit - self->field_0x20C < lbl_807992DC) {
                limit = lbl_807992DC + self->field_0x20C;
            }
            break;
        }
        if (self->pos.y >= limit) {
            fn_80127FE4(self);
        }
        break;
    }
}

/* 0x801CD8A0 - the mode-1 prelude plus a two-state hold that turns the work record by 0x200 and
 * ends through `fn_80127FE4`. */
void fn_801CD8A0(struct _ENEMY_WORK* self, u8 mode) {
    if (mode == 1) {
        fn_8012CF20(self);
        fn_80131D9C(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x1A, 6, 0);
        break;
    case 1:
        if (fn_80133C50(self, 0x200) == 1) {
            fn_80127FE4(self);
        }
        break;
    }
}

/* 0x801CD944 - the six-state "fly up, hover and throw" action.  State 2 runs two sub-steps through
 * `fn_80135600` (whose return selects the state-4 hand-over) and state 3/4/5 close the motion. */
void fn_801CD944(struct _ENEMY_WORK* self) {
    fn_8012CF20(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x2D, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x3D, 6, 0);
            self->vec_0x36C.y += lbl_807992E4 * get_em_chg_scale(self);
            self->state_0x006 = 0;
        }
        break;
    case 2:
        fn_80133C3C(self);
        switch (self->state_0x006) {
        case 0:
            fn_80133CC8(self, 0x100, 0x100);
            if (em_frame_check(self, 1, lbl_80799294, lbl_80799220) == 1) {
                self->state_0x006++;
                fn_801353F8(self);
                self->field_0x318 = lbl_807992E8;
                self->field_0x324 = lbl_807992EC;
                fn_80134004(self, 0x10, lbl_807992F0);
            }
            break;
        case 1:
            fn_80130248(self);
            fn_80135600(self, &self->field_0x1BC);
            fn_80134114(self, 0, 0x80);
            if (self->field_0x318 > lbl_807992F4) {
                self->field_0x318 = lbl_807992F4;
            }
            break;
        }
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x3E, 6, 0);
        }
        break;
    case 3:
        fn_80133C3C(self);
        if (fn_80134114(self, 0, 0x80) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x2D, 6, 0);
            self->state_0x006 = 0;
            self->field_0x314 = lbl_807992F8;
            self->field_0x324 = lbl_807992FC;
            fn_80130248(self);
            fn_80135600(self, &self->field_0x1BC);
        } else {
            fn_80130248(self);
            fn_80135600(self, &self->field_0x1BC);
            if (self->field_0x318 > lbl_807992F4) {
                self->field_0x318 = lbl_807992F4;
            }
        }
        break;
    case 4: {
        u32 forward;
        switch (self->state_0x006) {
        case 0:
            fn_80130248(self);
            forward = fn_80135600(self, &self->field_0x1BC);
            if (em_frame_check(self, 1, lbl_80799294, lbl_80799220) == 1) {
                self->state_0x006++;
                self->field_0x324 = lbl_80799300;
            }
            break;
        case 1:
            fn_80130248(self);
            forward = fn_80135600(self, &self->field_0x1BC);
            if (self->field_0x318 < lbl_80799220) {
                self->field_0x318 = lbl_80799220;
            }
            break;
        }
        if (fn_8012F93C(self) == 1) {
            if (forward == 1) {
                self->state++;
                fn_80130478(self, 3);
                fn_8012F5B8(self, 0x1D, 6, 0);
            } else if (fn_8012D1A0(self) == 1) {
                fn_80127FE4(self);
            }
        }
        break;
    }
    case 5:
        if (fn_8012F93C(self) == 1) {
            fn_80130478(self, 3);
            fn_801280AC(self);
        }
        break;
    }
}

/* 0x801CDCDC - the mode-gated turn: state 0 arms, state 1 turns by 0x180 (mode 1) and ends. */
void fn_801CDCDC(struct _ENEMY_WORK* self, u8 mode) {
    fn_8012CF20(self);
    fn_80131D9C(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x2D, 6, 0);
        break;
    case 1:
        if (mode == 1) {
            fn_80133C50(self, 0x180);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127FE4(self);
        }
        break;
    }
}

/* 0x801CDD94 - the timer action: state 0 arms a 0x3C-frame wait, state 1 counts it down and hands
 * control back to `fn_801CAF70` when it expires. */
void fn_801CDD94(struct _ENEMY_WORK* self) {
    fn_8012CF20(self);
    fn_80131D9C(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F62C(self, 0x1F, 4, 0);
        self->timer_0x020 = 0x3C;
        break;
    case 1: {
        s32 timer = self->timer_0x020 - 1;
        self->timer_0x020 = timer;
        if (timer <= 0) {
            fn_801CAF70(self);
        }
        break;
    }
    }
}

/* 0x801CDE34 - the effect-scale action: state 0 arms with the two effect scales, state 1 clamps the
 * scale and ends through `fn_801CAF70`. */
void fn_801CDE34(struct _ENEMY_WORK* self) {
    fn_8012CF20(self);
    fn_80131D9C(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x20, 6, 0);
        fn_801353F8(self);
        self->field_0x318 = lbl_80799304;
        self->field_0x324 = lbl_807992EC;
        break;
    case 1:
        fn_80130248(self);
        fn_80135600(self, &self->field_0x1BC);
        if (self->field_0x318 > lbl_80799308) {
            self->field_0x318 = lbl_80799308;
        }
        if (fn_8012F93C(self) == 1) {
            fn_801CAF70(self);
        }
        break;
    }
}

/* 0x801CDF10 - the mode-1 turn/clamp action. */
void fn_801CDF10(struct _ENEMY_WORK* self, u8 mode) {
    if (mode == 1) {
        fn_8012CF20(self);
        fn_80131E74(self);
        fn_80131D84(self);
        fn_80131D9C(self);
    }
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F62C(self, 0x1A, 6, 0);
        fn_80134004(self, 0, lbl_80799220);
        fn_801353F8(self);
        self->field_0x318 = lbl_807992B0;
        self->field_0x324 = lbl_80799240;
        break;
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1) {
            fn_80127FE4(self);
        } else {
            fn_801355C8(self, &self->field_0x1BC);
            if (self->field_0x318 > lbl_8079930C) {
                self->field_0x318 = lbl_8079930C;
            }
        }
        break;
    }
}

/* 0x801CE014 - the mode-1 clamps plus a two-state open/close whose close hands over to
 * `fn_801CAFBC`. */
void fn_801CE014(struct _ENEMY_WORK* self) {
    fn_8012CF20(self);
    fn_80131E74(self);
    fn_80131D84(self);
    fn_80131D9C(self);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F62C(self, 0x1B, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_801CAFBC(self);
        }
        break;
    }
}

/* 0x801CE0C0 - the three-state aim-and-close action. */
void fn_801CE0C0(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x2A, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130248(self);
            fn_801305C4(self);
            fn_8012F5B8(self, 0x44, 0, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 1, lbl_80799310, lbl_80799220) == 1) {
            fn_801CAFBC(self);
        }
        break;
    }
}

/* 0x801CE190 - the four-state height-gated close: state 1 waits on `fn_80130008` (which reads the
 * `fn_80130248` return in f1), state 2 re-arms the motion, state 3 closes it. */
void fn_801CE190(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F62C(self, 0x1B, 6, 0);
        fn_80134E8C(self);
        break;
    case 1:
        fn_80134F18(self);
        if (fn_80130008(self, fn_80130248(self)) == 1) {
            self->state++;
            fn_80130478(self, 3);
            fn_8012F5B8(self, 0x2E, 6, 0);
        }
        break;
    case 2:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0x41, 0, 0);
        }
        break;
    case 3:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801CE2B0 - the mode-1 clamp set plus a two-sub-step hold that closes when the work record
 * reaches its own vector's height. */
void fn_801CE2B0(struct _ENEMY_WORK* self, u8 mode) {
    fn_8012CF20(self);
    fn_80131E74(self);
    fn_80131D84(self);
    fn_80131D9C(self);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        self->state_0x006 = 0;
        self->state_0x007 = 0;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x1B, 6, 0);
        fn_801353E4(self);
        break;
    case 1:
        switch (self->state_0x006) {
        case 0:
            self->field_0x314 = fn_80135644(self, lbl_805B5050);
            fn_80135418(self);
            if (fn_8012F93C(self) == 1) {
                u8 count = self->state_0x007 + 1;
                self->state_0x007 = count;
                if (count >= 2) {
                    self->state_0x006++;
                    fn_80134DF4(self);
                }
            }
            break;
        case 1:
            fn_80134E28(self);
            break;
        }
        if (self->pos.y >= self->vec_0x36C.y) {
            fn_801CAFBC(self);
        }
        break;
    }
}

/* 0x801CE3F0 - the mode-1 clamps plus a two-state turn (state 1 turns by 0x200 and closes). */
void fn_801CE3F0(struct _ENEMY_WORK* self) {
    fn_8012CF20(self);
    fn_80131E74(self);
    fn_80131D84(self);
    fn_80131D9C(self);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F62C(self, 0x1B, 6, 0);
        break;
    case 1:
        if (fn_80133C50(self, 0x200) == 1) {
            fn_801CAFBC(self);
        }
        break;
    }
}

/* 0x801CE4A0 - the mode-1 clamps plus an `fn_80134114`-gated two-state hold. */
void fn_801CE4A0(struct _ENEMY_WORK* self) {
    fn_8012CF20(self);
    fn_80131E74(self);
    fn_80131D84(self);
    fn_80131D9C(self);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F62C(self, 0x1B, 6, 0);
        fn_80134004(self, 0, lbl_80799220);
        fn_801353F8(self);
        self->field_0x318 = lbl_807992B0;
        self->field_0x324 = lbl_80799240;
        break;
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1) {
            fn_801CAFBC(self);
        } else {
            fn_801355C8(self, &self->field_0x1BC);
            if (self->field_0x318 > lbl_8079930C) {
                self->field_0x318 = lbl_8079930C;
            }
        }
        break;
    }
}

/* 0x801CE5A0 - the effect-pair action: state 0 measures the height difference into `value_0x378`
 * (scaled down when it is negative) and seeds the two effect scales by which side of the own
 * vector's y the record is on; state 1 clamps `field_0x314` to the mode's limit and runs
 * `value_0x378` down by the measured length of the +0x310 triple. */
void fn_801CE5A0(struct _ENEMY_WORK* self, u8 mode) {
    fn_8012CF20(self);
    fn_80131E74(self);
    fn_80131D84(self);
    fn_80131D9C(self);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F62C(self, 0x1B, 6, 0);
        self->value_0x378 = self->pos.y - self->vec_0x36C.y;
        if (self->value_0x378 < lbl_80799220) {
            self->value_0x378 = self->value_0x378 * lbl_80799314;
        }
        fn_801353F8(self);
        if (self->pos.y <= self->vec_0x36C.y) {
            self->field_0x314 = lbl_807992B0;
            self->field_0x320 = lbl_80799240;
        } else {
            self->field_0x314 = lbl_807992F8;
            self->field_0x320 = lbl_80799314;
        }
        break;
    case 1: {
        f32 limit;
        fn_801355C8(self, &self->field_0x1BC);
        limit = (mode == 0) ? lbl_8079930C : lbl_80799284;
        if (self->field_0x314 > limit) {
            self->field_0x314 = limit;
        } else if (self->field_0x314 < -limit) {
            self->field_0x314 = -limit;
        }
        self->value_0x378 = self->value_0x378 - fn_80050F24(&self->offset_0x30C.vec_0x310.x);
        if (self->value_0x378 <= lbl_80799220) {
            fn_801CAFBC(self);
        }
        break;
    }
    }
}

/* 0x801CE71C - `fn_801CE5A0`'s sibling with the other close (`fn_80127FE4`) and mode 1's limit. */
void fn_801CE71C(struct _ENEMY_WORK* self, u8 mode) {
    fn_8012CF20(self);
    fn_80131E74(self);
    fn_80131D84(self);
    fn_80131D9C(self);
    fn_80131E00(self);
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F62C(self, 0x1A, 6, 0);
        self->value_0x378 = self->pos.y - self->vec_0x36C.y;
        if (self->value_0x378 < lbl_80799220) {
            self->value_0x378 = self->value_0x378 * lbl_80799314;
        }
        fn_801353F8(self);
        if (self->pos.y <= self->vec_0x36C.y) {
            self->field_0x314 = lbl_807992B0;
            self->field_0x320 = lbl_80799240;
        } else {
            self->field_0x314 = lbl_807992F8;
            self->field_0x320 = lbl_80799314;
        }
        break;
    case 1: {
        f32 limit;
        fn_801355C8(self, &self->field_0x1BC);
        limit = (mode == 0) ? lbl_8079930C : lbl_80799284;
        if (self->field_0x314 > limit) {
            self->field_0x314 = limit;
        } else if (self->field_0x314 < -limit) {
            self->field_0x314 = -limit;
        }
        self->value_0x378 = self->value_0x378 - fn_80050F24(&self->offset_0x30C.vec_0x310.x);
        if (self->value_0x378 <= lbl_80799220) {
            fn_80127FE4(self);
        }
        break;
    }
    }
}

/* 0x801CE898 - the `state_sub` (+0x1E6) dispatcher that owns this whole band: 34 ways, each a tail
 * call into one of the range's step functions (case 19 is the empty arm). */
void fn_801CE898(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801CCE10(self); break;
    case 1: fn_801CCF50(self, 0); break;
    case 2: fn_801CD068(self, 0); break;
    case 3: fn_801CD2E8(self); break;
    case 4: fn_801CD2EC(self); break;
    case 5: fn_801CD400(self, 0); break;
    case 6: fn_801CD71C(self, 0); break;
    case 7: fn_801CD8A0(self, 0); break;
    case 8: fn_801CD944(self); break;
    case 9: fn_801CD400(self, 1); break;
    case 10: fn_801CDCDC(self, 0); break;
    case 11: fn_801CDD94(self); break;
    case 12: fn_801CDE34(self); break;
    case 13: fn_801CD71C(self, 1); break;
    case 14: fn_801CD400(self, 2); break;
    case 15: fn_801CDCDC(self, 1); break;
    case 16: fn_801CD71C(self, 2); break;
    case 17: fn_801CDF10(self, 0); break;
    case 18: fn_801CD400(self, 3); break;
    case 19: break;
    case 20: fn_801CE014(self); break;
    case 21: fn_801CE0C0(self); break;
    case 22: fn_801CE190(self); break;
    case 23: fn_801CE2B0(self, 0); break;
    case 24: fn_801CE3F0(self); break;
    case 25: fn_801CE4A0(self); break;
    case 26: fn_801CE5A0(self, 0); break;
    case 27: fn_801CD8A0(self, 1); break;
    case 28: fn_801CCF50(self, 1); break;
    case 29: fn_801CD068(self, 1); break;
    case 30: fn_801CDF10(self, 1); break;
    case 31: fn_801CE5A0(self, 1); break;
    case 32: fn_801CE71C(self, 0); break;
    case 33: fn_801CE71C(self, 1); break;
    }
}

/* 0x801CE99C - the three-state aim/close action that ends through `fn_801280AC`. */
void fn_801CE99C(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x2A, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130478(self, 3);
            fn_8012F5B8(self, 0x44, 0, 0);
        }
        break;
    case 2:
        if (em_frame_check(self, 1, lbl_80799310, lbl_80799220) == 1) {
            fn_801280AC(self);
        }
        break;
    }
}

/* 0x801CEA68 - the sibling of `fn_801CE99C` that closes through `fn_80127F48`. */
void fn_801CEA68(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 3);
        fn_8012F5B8(self, 0x2E, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0x41, 0, 0);
        }
        break;
    case 2:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801CEB28 - the fade action: state 1 seeds the two effect scales from the model scale, state 2
 * switches between `fn_80135418` and `CancelFade` on a frame window and closes. */
void fn_801CEB28(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 3);
        fn_8012F5B8(self, 0x1E, 6, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80799318, lbl_80799220) == 1) {
            self->state++;
            fn_80130248(self);
            fn_801305C4(self);
            fn_801353F8(self);
            self->field_0x314 = lbl_8079931C * fn_8012F8E4(self);
            self->field_0x320 = lbl_80799320 * fn_8012F8E4(self);
        }
        break;
    case 2:
        if (em_frame_check(self, 1, lbl_80799324, lbl_80799220) == 0) {
            fn_80135418(self);
        } else {
            CancelFade(self);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127FE4(self);
        }
        break;
    }
}

/* 0x801CEC44 - the "descend onto the effect height" action (`fn_801CD068`'s sibling): state 1 seeds
 * the effect scale from the model scale, state 2 walks `pos.y` down to `field_0x20C` and ends either
 * through `fn_801280AC` (once it has reached it) or by re-arming at 0x1B. */
void fn_801CEC44(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130248(self);
        fn_801305C4(self);
        fn_8012F5B8(self, 0x1A, 6, 0);
        fn_80134E8C(self);
        break;
    case 1:
        fn_80134F18(self);
        if (fn_80130008(self, lbl_807992C8) == 1) {
            f32 scale;
            self->state++;
            fn_8012F5B8(self, 0x1D, 6, 0);
            scale = fn_8012F8E4(self);
            self->field_0x314 = -(lbl_807992CC * get_em_scale(self) / lbl_80799284) * scale;
        }
        break;
    case 2:
        if (fn_8012F948(self) == 0) {
            fn_801354F4(self, &self->field_0x1BC);
        }
        {
            f32 scale = get_em_scale(self);
            if (self->pos.y - self->field_0x20C < fn_80130248(self) * scale) {
                self->pos.y = self->field_0x20C + fn_80130248(self) * get_em_scale(self);
                if (fn_8012F93C(self) == 1) {
                    fn_801280AC(self);
                }
            } else if (fn_8012F93C(self) == 1) {
                fn_8012F62C(self, 0x1B, 6, 0);
            }
        }
        break;
    }
}

/* 0x801CEDE8 - the two-state file-row action: state 0 copies the `lbl_80570490` row, state 1 waits on
 * it through `fn_80134B0C` and closes with `fn_801280AC`. */
void fn_801CEDE8(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 3);
        fn_80134964(self, lbl_80570490, 0, 0, 0);
        break;
    case 1:
        if (fn_80134B0C(self, lbl_80570490) == 1) {
            fn_801280AC(self);
        }
        break;
    }
}

/* 0x801CEE74 - the mode-1 clamp action with `fn_80134004` and the `fn_80134114` gate. */
void fn_801CEE74(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 3);
        fn_8012F5B8(self, 0x1B, 6, 0);
        fn_80134004(self, 0, lbl_80799328);
        fn_801353F8(self);
        self->field_0x318 = lbl_80799310;
        self->field_0x324 = lbl_80799240;
        break;
    case 1:
        if (fn_80134114(self, 0, 0x80) == 1) {
            fn_801280AC(self);
        } else {
            fn_801355C8(self, &self->field_0x1BC);
            if (self->field_0x318 > lbl_807992F4) {
                self->field_0x318 = lbl_807992F4;
            }
        }
        break;
    }
}

/* 0x801CEF44 - the two-state "turn and re-seat" action: state 0 arms and latches the rotation word
 * into `field_0x37C`, state 1 turns by +/-0x4000 (the sign from the mode), seeds `field_0x310` and
 * `field_0x318`, hands the latched word back through `fn_801354F4` and closes with `fn_801280AC`. */
void fn_801CEF44(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 3);
        if (mode == 0) {
            fn_8012F5B8(self, 0xCB, 6, 0);
        } else {
            fn_8012F5B8(self, 0xCC, 6, 0);
        }
        fn_801353F8(self);
        self->field_0x37C = self->field_0x1C0;
        self->timer_0x328.field_0x328 =
            fn_801356A8(self, lbl_80799240, lbl_8079932C, lbl_80799330);
        fn_80130CDC(self, (u32)-0x0A);
        break;
    case 1: {
        struct EmWord3 rec;
        if (mode == 0) {
            fn_80133E3C(self, -0x4000, lbl_80799284, lbl_80799250);
            self->offset_0x30C.vec_0x310.x = fn_80135644(self, lbl_805B50C0);
        } else {
            fn_80133E3C(self, 0x4000, lbl_80799284, lbl_80799250);
            self->offset_0x30C.vec_0x310.x = -fn_80135644(self, lbl_805B50C0);
        }
        self->field_0x318 = self->timer_0x328.field_0x328 * fn_80135644(self, lbl_805B50F8);
        rec.x = 0;
        rec.y = self->field_0x37C;
        rec.z = 0;
        fn_801354F4(self, &rec);
        if (fn_8012F93C(self) == 1) {
            fn_801280AC(self);
        }
        break;
    }
    }
}

/* 0x801CF6A8 - the mode-selected "hold the part pair" action: state 0 arms one of four motion/part
 * sets, state 1 closes on `fn_8012F93C`. */
void fn_801CF6A8(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        switch (mode) {
        case 0:
            fn_8012F5B8(self, 0xDC, 4, 0);
            fn_8012933C(self, 0, 1, 8);
            fn_8012933C(self, 1, 0x12, 0x10);
            break;
        case 1:
            fn_8012F5B8(self, 0xDD, 4, 0);
            fn_8012933C(self, 0, 2, 8);
            fn_8012933C(self, 1, 0x13, 0x10);
            break;
        case 2:
            fn_8012F5B8(self, 0xDC, 4, 0);
            fn_8012933C(self, 0, 0x1E, 8);
            fn_8012933C(self, 1, 0x12, 0x10);
            break;
        case 3:
            fn_8012F5B8(self, 0xDD, 4, 0);
            fn_8012933C(self, 0, 0x1F, 8);
            fn_8012933C(self, 1, 0x13, 0x10);
            break;
        }
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801CFBF4 - the mode-paired "arm and wait" action: state 0 arms 0xDE/0xE1 and latches the aim
 * angle into `timer_0x020`, state 1 turns by it and hands over to the mode's band function. */
void fn_801CFBF4(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        switch (mode) {
        case 0:
            fn_8012F5B8(self, 0xDE, 2, 0);
            break;
        case 1:
            fn_8012F5B8(self, 0xE1, 2, 0);
            break;
        }
        self->timer_0x020 = (s16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        break;
    case 1:
        fn_80133E3C(self, self->timer_0x020, lbl_8079930C, lbl_80799310);
        if (em_frame_check(self, 1, lbl_80799354, lbl_80799220) == 1) {
            self->state++;
            switch (mode) {
            case 0:
            case 2:
                fn_801CB008(self);
                break;
            case 1:
            case 3:
                fn_801CB050(self);
                break;
            }
        }
        break;
    }
}

/* 0x801CFD28 - `fn_801CFBF4`'s timer-based sibling (the same mode pair with `fn_8012F62C`). */
void fn_801CFD28(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        switch (mode) {
        case 0:
            fn_8012F62C(self, 0xDE, 0, 0x48);
            break;
        case 1:
            fn_8012F62C(self, 0xE1, 0, 0x48);
            break;
        }
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            switch (mode) {
            case 0:
                fn_801CB008(self);
                break;
            case 1:
                fn_801CB050(self);
                break;
            }
        }
        break;
    }
}

/* 0x801D2EB4 - the tail call into `fn_801CD068(self, 0)` (the small thunk the unit above's
 * dispatcher uses). */
void fn_801D2EB4(struct _ENEMY_WORK* self) {
    return fn_801CD068(self, 0);
}

/* 0x801D2EBC - `state_sub == 0` gates the same thunk. */
void fn_801D2EBC(struct _ENEMY_WORK* self) {
    if (self->state_sub != 0) {
        return;
    }
    fn_801D2EB4(self);
}

/* 0x801D2E0C - the `state_sub`-keyed file-row selector: five sub-states (plus the default) each
 * hand `fn_801251D8` one of the band's `.data` rows and the row's own sub-state pair. */
void fn_801D2E0C(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: return fn_801251D8(self, lbl_805B6078, 0, 0);
    case 5: return fn_801251D8(self, lbl_805B6108, 1, 5);
    case 0x0F: return fn_801251D8(self, lbl_805B6170, 0, 0x0F);
    case 0x1A: return fn_801251D8(self, lbl_805B61B8, 0, 0x1A);
    case 0x1C: return fn_801251D8(self, lbl_805B61E8, 0, 0x1C);
    default: return fn_801251D8(self, lbl_805B6078, 0, 0);
    }
}

/* 0x801D3C38 - the three-state "advance by 0x3E8 and close" action. */
void fn_801D3C38(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0x13, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_8012F5B8(self, 0x14, 8, 0);
            fn_80130CDC(self, 0x3E8);
        }
        break;
    case 2:
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801CF648 - the `state_sub` dispatcher over this range's 0x801CE99C.. band (10 ways). */
void fn_801CF648(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801CE99C(self); break;
    case 1: fn_801CEA68(self); break;
    case 2: fn_801CEB28(self); break;
    case 3: fn_801CEC44(self); break;
    case 4: fn_801CEDE8(self); break;
    case 5: fn_801CEE74(self); break;
    case 6: fn_801CEF44(self, 0); break;
    case 7: fn_801CEF44(self, 1); break;
    case 8: fn_801CF0C0(self, 0); break;
    case 9: fn_801CF0C0(self, 1); break;
    }
}

/* 0x801D0FE0 - the two-mode "arm the part pair and turn" action. */
void fn_801D0FE0(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        if (mode == 0) {
            fn_8012F5B8(self, 0xD5, 4, 0);
            fn_8012933C(self, 0, 8, 8);
            fn_8012933C(self, 1, 9, 0x18);
        } else {
            fn_8012F5B8(self, 0xD9, 4, 0);
            fn_8012933C(self, 0, 0x11, 8);
            fn_8012933C(self, 1, 0x24, 0x18);
        }
        break;
    case 1:
        if (mode == 0) {
            fn_80133E3C(self, 0x4000, lbl_80799380, lbl_80799384);
        } else {
            fn_80133E3C(self, -0x4000, lbl_80799380, lbl_80799384);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801D076C - the two-frame-window action: state 0 arms the part pair, state 1 opens the 1/2
 * windows (the second one turns the work record) and closes. */
void fn_801D076C(struct _ENEMY_WORK* self, u8 mode) {
    switch (self->state) {
    case 0:
        self->state++;
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xD4, 4, 0);
        fn_8012933C(self, 0, 3, 8);
        fn_8012933C(self, 1, 4, 0x18);
        break;
    case 1:
        if (em_frame_check(self, 0, lbl_8079935C, lbl_80799220) == 1) {
            fn_80129724(self, 1);
            fn_8012933C(self, 1, 0x10, 0x10);
        }
        if (em_frame_check(self, 2, lbl_80799270, lbl_80799220) == 1) {
            u16 angle = (u16)(s32)((lbl_8079927C * (lbl_80799360 * fn_8012F8E4(self)))
                                       / lbl_80799278
                                   + lbl_80799280);
            fn_80133C50(self, angle);
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801D290C - the `state_sub` dispatcher over this range's upper action band (48 ways; the table is
 * `jumptable_805B553C`).  Each arm is a tail call, so each case is a `return`. */
void fn_801D290C(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0: fn_801CF6A8(self, 0); break;
    case 1: fn_801CF6A8(self, 1); break;
    case 2: fn_801CF840(self, 0); break;
    case 3: fn_801CFBF4(self, 0); break;
    case 4: fn_801CFBF4(self, 1); break;
    case 5: fn_801CFD28(self, 0); break;
    case 6: fn_801CFD28(self, 1); break;
    case 7: fn_801CFE04(self, 0, 0); break;
    case 8: fn_801CFE04(self, 1, 0); break;
    case 9: fn_801CFE04(self, 2, 0); break;
    case 10: fn_801CFE04(self, 3, 0); break;
    case 11: fn_801CFE04(self, 4, 0); break;
    case 12: fn_801CFE04(self, 5, 0); break;
    case 13: fn_801D076C(self, 0); break;
    case 14: fn_801D08A8(self); break;
    case 15: fn_801D0B94(self, 0); break;
    case 16: fn_801D0FE0(self, 0); break;
    case 17: fn_801D1110(self, 0); break;
    case 18: fn_801D1590(self, 0, 0); break;
    case 19: fn_801D1590(self, 1, 0); break;
    case 20: fn_801D18DC(self, 0, 0); break;
    case 21: fn_801D1E00(self, 0); break;
    case 22: fn_801CFE04(self, 0, 1); break;
    case 23: fn_801CFE04(self, 1, 1); break;
    case 24: fn_801CFE04(self, 2, 1); break;
    case 25: fn_801CFE04(self, 3, 1); break;
    case 26: fn_801CFE04(self, 4, 1); break;
    case 27: fn_801CFE04(self, 5, 1); break;
    case 28: fn_801D0B94(self, 1); break;
    case 29: fn_801D1110(self, 1); break;
    case 30: fn_801D18DC(self, 1, 0); break;
    case 31: fn_801CF840(self, 1); break;
    case 32: fn_801D1E00(self, 1); break;
    case 33: fn_801CF6A8(self, 2); break;
    case 34: fn_801CF6A8(self, 3); break;
    case 35: fn_801D2144(self, 0, 0); break;
    case 36: fn_801D2144(self, 1, 0); break;
    case 37: fn_801D2144(self, 0, 1); break;
    case 38: fn_801D2144(self, 1, 1); break;
    case 39: fn_801D1590(self, 2, 0); break;
    case 40: fn_801D076C(self, 1); break;
    case 41: fn_801D1590(self, 0, 1); break;
    case 42: fn_801D1590(self, 2, 1); break;
    case 43: fn_801D0FE0(self, 1); break;
    case 44: fn_801CF840(self, 2); break;
    case 45: fn_801CF840(self, 3); break;
    case 46: fn_801D1E00(self, 2); break;
    case 47: fn_801D1590(self, 3, 0); break;
    }
}

/* 0x801D2B10 - the `state_sub` dispatcher that carries this range's whole action parameter table:
 * 180 ways (0x17..0xCA), each handing `fn_801251D0` the `.data` cell that belongs to that sub-state
 * plus its own index; every other sub-state closes the action through `fn_80127F48` (the target's
 * shared default block). */
void fn_801D2B10(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0x17: return fn_801251D0(self, lbl_805B5668, 0x0, 0x17);
    case 0x18: return fn_801251D0(self, lbl_805B5618, 0x0, 0x18);
    case 0x19: return fn_801251D0(self, lbl_805B5730, 0x0, 0x19);
    case 0x1A: return fn_801251D0(self, lbl_805B57A0, 0x0, 0x1A);
    case 0x1B: return fn_801251D0(self, lbl_805B5618, 0x0, 0x1B);
    case 0x1C: return fn_801251D0(self, lbl_805B5730, 0x0, 0x1C);
    case 0x1D: return fn_801251D0(self, lbl_805B57A0, 0x0, 0x1D);
    case 0x1E: return fn_801251D0(self, lbl_805B5640, 0x0, 0x1E);
    case 0x23: return fn_801251D0(self, lbl_805B5840, 0x1, 0x23);
    case 0x38: return fn_801251D0(self, lbl_805B56D0, 0x0, 0x38);
    case 0x58: return fn_801251D0(self, lbl_805B5690, 0x0, 0x58);
    case 0x78: return fn_801251D0(self, lbl_805B5B10, 0x0, 0x78);
    case 0x7A: return fn_801251D0(self, lbl_805B58B0, 0x0, 0x7A);
    case 0x7B: return fn_801251D0(self, lbl_805B58D8, 0x0, 0x7B);
    case 0x7C: return fn_801251D0(self, lbl_805B5948, 0x0, 0x7C);
    case 0x7E: return fn_801251D0(self, lbl_805B59A0, 0x0, 0x7E);
    case 0x7F: return fn_801251D0(self, lbl_805B5A08, 0x0, 0x7F);
    case 0x84: return fn_801251D0(self, lbl_805B5AB0, 0x1, 0x84);
    case 0x8D: return fn_801251D0(self, lbl_805B5A40, 0x0, 0x8D);
    case 0x8E: return fn_801251D0(self, lbl_805B5A40, 0x0, 0x8E);
    case 0x9F: return fn_801251D0(self, lbl_805B5918, 0x0, 0x9F);
    case 0xA0: return fn_801251D0(self, lbl_805B5918, 0x0, 0xA0);
    case 0xA8: return fn_801251D0(self, lbl_805B5B38, 0x0, 0xA8);
    case 0xA9: return fn_801251D0(self, lbl_805B5840, 0x1, 0xA9);
    case 0xAF: return fn_801251D0(self, lbl_805B5618, 0x0, 0xAF);
    case 0xB0: return fn_801251D0(self, lbl_805B5840, 0x1, 0xB0);
    case 0xB6: return fn_801251D0(self, lbl_805B5B60, 0x0, 0xB6);
    case 0xB7: return fn_801251D0(self, lbl_805B5B88, 0x0, 0xB7);
    case 0xB8: return fn_801251D0(self, lbl_805B5BF8, 0x1, 0xB8);
    case 0xB9: return fn_801251D0(self, lbl_805B5C58, 0x0, 0xB9);
    case 0xBA: return fn_801251D0(self, lbl_805B5C80, 0x0, 0xBA);
    case 0xBB: return fn_801251D0(self, lbl_805B5CA8, 0x0, 0xBB);
    case 0xBC: return fn_801251D0(self, lbl_805B5CA8, 0x0, 0xBC);
    case 0xBF: return fn_801251D0(self, lbl_805B5CE8, 0x0, 0xBF);
    case 0xC1: return fn_801251D0(self, lbl_805B5A40, 0x0, 0xC1);
    case 0xCA: return fn_801251D0(self, lbl_805B5D60, 0x0, 0xCA);
    default: return fn_80127F48(self);
    }
}

/* 0x801D08A8 - the "lean over and slide" action: state 0 seeds the effect scale and latches the aim
 * angle clamped to 0x4000, state 1 runs three frame windows (the 2-window turns the record by the
 * scaled angle) and hands over to state 2, which closes through `fn_80127F48`. */
void fn_801D08A8(struct _ENEMY_WORK* self) {
    fn_8012CF20(self);
    switch (self->state) {
    case 0: {
        u32 diff;
        self->state++;
        fn_80130478(self, 3);
        fn_8012F5B8(self, 0xD6, 6, 0);
        fn_801353F8(self);
        self->field_0x318 = lbl_80799364 * fn_8012F8E4(self);
        self->field_0x318 =
            self->field_0x318 * fn_801356A8(self, lbl_80799368, lbl_8079932C, lbl_8079926C);
        diff = (u16)(calcVecAng2(&self->pos, &self->vec_0x36C) - self->field_0x1C0);
        if (diff >= 0x8000) {
            diff = 0x10000 - diff;
        }
        self->timer_0x020 = (s16)diff;
        if (self->timer_0x020 > 0x4000) {
            self->timer_0x020 = 0x4000;
        }
        break;
    }
    case 1:
        if (em_frame_check(self, 0, lbl_8079936C, lbl_80799220) == 1) {
            fn_8012933C(self, 0, 0x0D, 0x0B);
        }
        if (em_frame_check(self, 2, lbl_80799294, lbl_80799220) == 1) {
            f32 scaled = (f32)self->timer_0x020 * lbl_80799278 / lbl_8079927C;
            u16 angle = (u16)(s32)((lbl_8079927C
                                    * (scaled / lbl_80799294 * fn_8012F8E4(self)))
                                       / lbl_80799278
                                   + lbl_80799280);
            fn_80133C50(self, angle);
        }
        fn_801354F4(self, &self->field_0x1BC);
        if (fn_8012F93C(self) == 1) {
            self->state++;
            fn_80130478(self, 0);
            fn_8012F5B8(self, 0xD7, 0, 0);
            self->field_0x324 = -self->field_0x318 / lbl_80799294 * fn_8012F8E4(self);
            fn_8012933C(self, 1, 0x19, 0x10);
        }
        break;
    case 2:
        if (em_frame_check(self, 0, lbl_80799370, lbl_80799220) == 1) {
            fn_80129724(self, 0);
        }
        if (em_frame_check(self, 0, lbl_8079930C, lbl_80799220) == 1) {
            fn_8012F8C8(self, lbl_80799280);
        }
        if (em_frame_check(self, 2, lbl_80799294, lbl_80799220) == 1) {
            fn_801355C8(self, &self->field_0x1BC);
            if (self->field_0x318 < lbl_80799220) {
                self->field_0x318 = lbl_80799220;
            }
        }
        if (fn_8012F93C(self) == 1) {
            fn_80127F48(self);
        }
        break;
    }
}

/* 0x801D2ED0 - the "step onto the area's offset point" action: state 0 picks a per-area offset
 * vector out of the map kind, saves the target point and re-seats on the offset; states 1/2 slide
 * there and state 3 lands (the part levels choose the landing sub-step). */
void fn_801D2ED0(struct _ENEMY_WORK* self) {
    nw4r::math::VEC3 spot;
    u32 have_spot = 0;
    fn_80043EA8(&spot);
    switch (self->state) {
    case 0:
        if ((u8)fn_802B0668(self->field_0x1E0) == 4) {
            switch (self->area_no) {
            case 1:
                setVector3(&spot, lbl_807993B0, lbl_807993B4, lbl_807993B8);
                have_spot = 1;
                break;
            case 2:
                setVector3(&spot, lbl_807993BC, lbl_807993B4, lbl_807993C0);
                have_spot = 1;
                break;
            case 3:
                setVector3(&spot, lbl_807993C4, lbl_807993B4, lbl_807993C8);
                have_spot = 1;
                break;
            }
        }
        if (have_spot == 1) {
            self->state++;
            fn_80130248(self);
            fn_801305C4(self);
            fn_80041E40(&self->action_0x328.vec_0x334, &self->vec_0x36C);
            fn_80041E40(&self->vec_0x36C, &spot);
            fn_80134F70(self, lbl_805704D0);
            fn_80134004(self, 0x19, lbl_80799220);
            fn_80130248(self);
            fn_80135584(self, &self->field_0x1BC);
        } else {
            self->state = 3;
            fn_80130248(self);
            fn_801305C4(self);
            fn_8012F5B8(self, 0x1A, 0x0A, 0);
            fn_80134E8C(self);
            self->timer_0x020 = 0x12C;
        }
        break;
    case 1:
        if (fn_80134114(self, 0, 0) == 1) {
            self->state++;
            fn_80041E40(&self->vec_0x36C, &self->action_0x328.vec_0x334);
            fn_8012F5B8(self, 0x1A, 0x0A, 0);
        } else {
            fn_80135000(self, 2, lbl_805704D0);
            fn_80130248(self);
            fn_80135584(self, &self->field_0x1BC);
        }
        break;
    case 2:
        if (fn_80133C50(self, 0x200) == 1) {
            self->state++;
            fn_80134E8C(self);
            self->timer_0x020 = 0x12C;
        }
        break;
    case 3: {
        f32 gap = self->vec_0x36C.y - self->pos.y;
        s32 timer;
        if (gap < lbl_807993CC) {
            fn_80134F18(self);
        } else if (gap > lbl_807993D0) {
            self->pos.y = self->pos.y + lbl_807992B0;
        }
        gap = self->vec_0x36C.y - self->pos.y;
        timer = self->timer_0x020 - 1;
        self->timer_0x020 = timer;
        if (timer <= 0
            || (fn_80133C50(self, 0x180) == 1 && gap >= lbl_807993CC && gap <= lbl_807993D0)) {
            if (em_parts_damage_level_get(self, 2) >= 1
                && em_parts_damage_level_get(self, 3) >= 1) {
                fn_80128A14(self, 0x0D, 6);
            } else {
                fn_80128A14(self, 0x0D, 5);
            }
        }
        break;
    }
    }
}

} /* extern "C" */
