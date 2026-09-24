/* auto/8012BA00_fn_8012BA00.c - one function, .text 0x8012BA00..0x8012BDF4, plus its extab/extabindex
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 * fragments.
 *
 * Picks the frame cost of one of an enemy's motion types: `fn_8012BA00(enemy, motion, other, type,
 * part)` returns the number of frames the motion is worth this tick, which the caller accumulates
 * into the enemy's per-part counters (`lbl_8012B944`/`lbl_8012B9BC`, both clamped to [0, 0x4E20]).
 * `type` selects which of the three motion sets (`r25->+0x00`/`+0x04`/`+0x08` in the caller, the
 * `.data` defaults `lbl_805A0FF8`/`lbl_805A1034`/`lbl_805A106C`); `other` is the enemy record the
 * caller walks (`work_mem_get(2)`/`(3)`, a 0xB18-stride array) and `part` the slot it belongs to.
 *
 * The body is a three-way `switch` that adds a conditional "extra" cost per type, then the common
 * tail: when the enemy is not in its timed state (`+0x43D != 1`) the plain `frames[4]` applies; in
 * it, the cost is the `frames[]` band the distance to `other` falls in (a per-part table for
 * `type == 1`, the vector distance otherwise), plus a fraction of `frames[6]` on the two specially
 * flagged paths.
 *
 * Language: the object's only mangled callee is `em_sleep_ck__FP11_ENEMY_WORKUc`, which a `.c` file
 * reaches by declaring the map's spelling (as `80104BD0` does) - nothing here names a source file or
 * defines a C++ symbol, so the file stays `.c`.
 *
 * Result: `fn_8012BA00` 100 %, `.text` (0x3F4), `extab` (0x8) and `extabindex` (0xC) byte-identical
 * to the target object.
 *
 * Source pragmas, evidenced (the lib's `cflags_main` has the peephole pass on and `-fp_contract on`):
 *   * `#pragma peephole off` - retail keeps the unfused `clrlwi`+`cmpwi` and `clrlwi`+`slwi` pairs
 *     the pass folds into `clrlwi.` and `clrlslwi` (playbook 39).
 *   * `#pragma fp_contract off` - retail keeps `fmuls`+`fadds` where the default contracts into
 *     `fmadds` (playbook 40).
 *   * the `extab`/`extabindex` fragments come from `-Cpp_exceptions on`, already in the lib's flags;
 *     no exception construct is needed in the source.
 *
 * Load-bearing source shapes (each measured; the wrong form costs real points):
 *   * the tail is written negated and first: `if (enemy->timed_0x43D != 1) ... else if (flag == 0 ||
 *     em_sleep_ck(...) == 1) ... else ...`.  With `== 1` first MWCC inverts the branch and the
 *     `frames[4]` body lands at the end of the function instead of after the test.
 *   * `type` and `part` are `u32` parameters narrowed with `(u8)`/`(u16)` at every use: that is what
 *     emits retail's `clrlwi ...,type,24` / `clrlwi ...,part,16`.  As a `u8`/`u16` parameter MWCC
 *     treats the value as already narrowed and emits no mask.
 *   * the last-but-one arm is a **separate, redundant-looking `else if (enemy->state_0x1E2 == 2)` with
 *     an empty body**.  With the equivalent `else if (... != 2 && add == 1)` MWCC drops the `!= 2`
 *     guard and the function is two instructions short; with the empty arm its condition is not
 *     provably true or false, so retail's `cmplwi r0,2; beq <end>` survives.
 *   * `flag` and `add` are `u32`, and `fn_8012D7FC`/`em_sleep_ck` return `u32`: retail's `== 1` tests
 *     are `cmplwi`, and a signed operand gives `cmpwi`.
 *   * the local declaration order `result, thresholds, frames, flag, add` is what colours those five
 *     r31..r27; `result = 0` must precede `fn_80043EA8(&vec)` and `add = 0` must follow it.
 *   * `0.5f * (0.7f * x)` keeps the two `fmuls`s separate - the folded spelling is one `fmadds`.
 *
 * Residuals (relocation *names* only - the section bytes are identical and the official report metric
 * ignores relocation diffs):
 *   * the three `lfd` loads of the signed int -> `f64` conversion magic (`0x4330000080000000`) name
 *     this object's own anonymous `.sdata2` entry where the split names `lbl_80796C68`.  The constant
 *     comes from the conversion idiom and cannot be spelled as a source operand (the same residual as
 *     `80101FA4`'s `fn_80101FA4`); it is also why this object carries 8 bytes of `.sdata2` the target
 *     has not got, which is why the unit stays `NonMatching`.
 *   * the `extabindex` entry's relocation names the compiler-generated local `@114` where dtk's split
 *     named the same extab entry `@etb_8000CBDC` - a splitter name, not a source-reachable one.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/8012BA00_fn_8012BA00.c`.
 * The name is provisional - nothing in the object names the original source file.
 */

#include "types.h"
#include "nw4r/math.h"
#include "Pl/pl_master.h"
#include "enemy/fn_8012BDF4.h"
#include "unsplit/enemy.h"

/* ---------------------------------------------------------------------------------------------------
 * types
 * ------------------------------------------------------------------------------------------------- */

/* A 3-float engine vector (`nw4r::math::VEC3`, the C spelling `VEC3`) comes from `nw4r/math.h`
 * (same layout: three `f32` at +0x00/+0x04/+0x08, size 0x0C). */

/* One motion set, the record `fn_8012BA00`'s second argument points at.  The three `.data` defaults
 * the callers fall back to fix the arrays' shapes: `lbl_805A0FF8` -> thresholds `lbl_805A0FB0`
 * (1000/2000/3000/5000), frames `lbl_805A0FC0` (5/3/2/1/0/-5/-30), extra `lbl_805A0FDC`
 * (0xC8/0/0/0/0/0/0), i.e. four band bounds and seven frame entries each.  size: 0x0C */
typedef struct MOTION_SET {
    /* +0x00 */ const f32* thresholds;   /* 4 entries: the upper bound of each distance band */
    /* +0x04 */ const s32* frames;       /* 7 entries: the frame delta of each band */
    /* +0x08 */ const s32* extra_frames; /* 7 entries: the deltas the per-type tests add */
} MOTION_SET;

/* The enemy record.  Both pointer arguments have this shape: the first is what `em_sleep_ck` takes
 * as `_ENEMY_WORK*`, the second is what the caller's `0xB18`-stride enemy array holds (it compares
 * the two records' `+0x003` monster byte).  Only the bytes this unit reads are named.  The engine's
 * own header (Ghidra's, `D:/WiiExperiment/MH3Disassembly/_ENEMY_WORK.h`) stops at 0x300 which this
 * unit reads past, so the size here comes from the caller's stride.  size: 0xB18 - lower bound */
typedef struct _ENEMY_WORK {
    /* +0x000 */ u8 pad_0x000[0x009];
    /* +0x009 */ u8 state_0x009;    /* `== 3` marks type 1's slot for the extra cost */
    /* +0x00A */ u8 pad_0x00A[0x170 - 0x00A];
    /* +0x170 */ u8 state_0x170;    /* `== 2` marks type 2's slot for the extra cost */
    /* +0x171 */ u8 pad_0x171[0x178 - 0x171];
    /* +0x178 */ VEC3 pos_0x178;    /* the position type 2 measures the distance to */
    /* +0x184 */ u8 pad_0x184[0x188 - 0x184];
    /* +0x188 */ VEC3 pos_0x188;    /* the enemy's own position */
    /* +0x194 */ u8 pad_0x194[0x1E1 - 0x194];
    /* +0x1E1 */ u8 area_no;        /* the same value on both records means "same area" */
    /* +0x1E2 */ u8 state_0x1E2;    /* `== 2` selects the fractional cost */
    /* +0x1E3 */ u8 pad_0x1E3[0x1E5 - 0x1E3];
    /* +0x1E5 */ u8 action_0x1E5;   /* the action the `+0x380`/`+0x382` pair belongs to */
    /* +0x1E6 */ u8 pad_0x1E6[0x380 - 0x1E6];
    /* +0x380 */ u8 special_type_0x380; /* the `(type, part)` pair the fractional cost is scoped to */
    /* +0x381 */ u8 pad_0x381[0x382 - 0x381];
    /* +0x382 */ u8 special_part_0x382;
    /* +0x383 */ u8 pad_0x383[0x420 - 0x383];
    /* +0x420 */ u8 flag_0x420;     /* `== 1` and a positive `count_0x422` add type 2's extra cost */
    /* +0x421 */ u8 pad_0x421;
    /* +0x422 */ s16 count_0x422;
    /* +0x424 */ u8 pad_0x424[0x43D - 0x424];
    /* +0x43D */ u8 timed_0x43D;    /* `!= 1`: the plain per-distance-table path */
    /* +0x43E */ u8 pad_0x43E[0x454 - 0x43E];
    /* +0x454 */ f32 frames_0x454[8]; /* per-part frame cost, read for `type == 1`; lower bound */
    /* +0x474 */ u8 pad_0x474[0x5C4 - 0x474];
    /* +0x5C4 */ u8 flags_0x5C4;    /* bit 0-3 set adds type 1's second extra cost */
    /* +0x5C5 */ u8 pad_0x5C5[0xB18 - 0x5C5];
} _ENEMY_WORK;

/* ---------------------------------------------------------------------------------------------------
 * callees (a plain name is what `symbols.txt` spells; the mangled one is written verbatim, and the
 * file is compiled `-lang=c`, so the identifier and the relocation pair with the map's symbol)
 * ------------------------------------------------------------------------------------------------- */

extern void fn_80041E40(VEC3* dst, const VEC3* src); /* vector copy */
extern void fn_80043EA8(VEC3* v);                    /* v = (0, 0, 0) */
extern f32 fn_80050EF4(const VEC3* a, const VEC3* b); /* the distance between two positions */
extern s32 fn_802D2B78(_ENEMY_WORK* other, u16 mask); /* tests a bit of other's `+0x1EC` */

/* The constants the source reaches, all in the pool run the split leaves unowned
 * (0x80796C58..0x80796C90): declared, never defined (playbook 29).  The int -> f64 conversion magic
 * at 0x80796C68 is *not* declared - the compiler emits its own `.sdata2` entry for it (see the
 * residual in the header). */
extern f32 lbl_80796C58; /* 0.0f */
extern f32 lbl_80796C70; /* 0.5f */
extern f32 lbl_80796C8C; /* 0.7f */

/* ---------------------------------------------------------------------------------------------------
 * body
 * ------------------------------------------------------------------------------------------------- */

/* The unit's two deviations from the lib defaults; the evidence is in the header. */
#pragma peephole off
#pragma fp_contract off

/* The frame cost of motion `type` for slot `part`: the per-type extra cost, then the distance band
 * the distance to `other` falls in, plus a fraction of the last entry on the two flagged paths. */
s32 fn_8012BA00(_ENEMY_WORK* enemy, const MOTION_SET* motion, _ENEMY_WORK* other, u32 type, u32 part)
{
    s32 result = 0;
    const f32* thresholds;
    const s32* frames;
    u32 flag; /* "add the extra cost", set by each type's test; unset for any other type */
    u32 add;
    VEC3 vec;
    const s32* extra;
    f32 dist;

    fn_80043EA8(&vec);
    add = 0;

    thresholds = motion->thresholds;
    frames = motion->frames;

    switch ((u8)type) {
    case 1:
        flag = (fn_8012D0B4(enemy, other) == 1);
        extra = motion->extra_frames;
        if (extra != 0) {
            if (fn_8026FE98(other, 0x200) != 0) {
                result = extra[0];
            }
            if ((other->flags_0x5C4 & 0xF) != 0) {
                result += extra[1];
            }
            if (flag == 1 && fn_8012D7FC(other) == 1) {
                result += extra[2];
            }
        }
        if (other->state_0x009 == 3) {
            add = 1;
        }
        break;

    case 2:
        flag = (fn_8012D188(enemy, other) == 1);
        fn_80041E40(&vec, &other->pos_0x178);
        extra = motion->extra_frames;
        if (extra != 0) {
            if (fn_802D2B78(other, 1) != 0) {
                result = extra[0];
            }
            if (other->flag_0x420 == 1 && other->count_0x422 > 0) {
                result += extra[1];
            }
        }
        if (other->state_0x170 == 2) {
            add = 1;
        }
        break;

    case 3:
        flag = (enemy->area_no == other->area_no);
        fn_80041E40(&vec, &other->pos_0x188);
        if (other->state_0x1E2 == 2) {
            add = 1;
        }
        break;
    }

    if (enemy->timed_0x43D != 1) {
        result += frames[4];
    } else if (flag == 0 || em_sleep_ck__FP11_ENEMY_WORKUc(enemy, 0) == 1) {
        result += frames[5];
    } else {
        if ((u8)type == 1) {
            dist = enemy->frames_0x454[(u16)part];
        } else {
            dist = fn_80050EF4(&enemy->pos_0x188, &vec);
        }

        if (dist < lbl_80796C58) {
            result += frames[5];
        } else if (dist <= thresholds[0]) {
            result += frames[0];
        } else if (dist <= thresholds[1]) {
            result += frames[1];
        } else if (dist <= thresholds[2]) {
            result += frames[2];
        } else if (dist <= thresholds[3]) {
            result += frames[3];
        } else {
            result += frames[4];
        }

        if (enemy->action_0x1E5 == 7) {
            if (enemy->special_type_0x380 == (u8)type && enemy->special_part_0x382 == (u16)part) {
                if (enemy->state_0x1E2 == 2) {
                    result = (s32)((f32)result + lbl_80796C8C * (f32)frames[6]);
                } else {
                    result += frames[6];
                }
            }
        } else if (enemy->state_0x1E2 == 2 && add == 0) {
            result = (s32)((f32)result + lbl_80796C70 * (lbl_80796C8C * (f32)frames[6]));
        } else if (enemy->state_0x1E2 == 2) {
            /* the state is 2 and the slot is not flagged: nothing to add */
        } else if (add == 1) {
            result = (s32)((f32)result + lbl_80796C70 * (f32)frames[6]);
        }
    }

    return result;
}
