/*
 * hud/cockpit_quest.cpp - the quest cockpit HUD band (0x802E7408..0x802EBED8, 64 functions, 19152 B).
 *
 * Home and name, evidence class 1 (a `__FILE__` string): `.data` 0x805D5D08 is the bare source name
 * "cockpit_quest.cpp" (18 B; `config/RMHE08/symbols.txt`'s `lbl_805D5D08`, `type:object size:0x12
 * data:string`), the runtime dump spells it `_802e4e00s_cockpit_quest.cpp_805d5d08`, and it is
 * referenced from *inside this range* - `fn_802E7408` (assert at 0x802E74DE, line 2040) and
 * `fn_802E7548` (0x802E7650, line 2088) pass it to `nw4r::db::Panic` with the message at
 * 0x805D5D20, and no other code in the image references either string (`grep -rl 805D5D08
 * build/RMHE08/obj/auto_fn_*` returns only these two objects).  So the translation unit is
 * `cockpit_quest.cpp` and the file name is decided by the string's own suffix.  Module `hud`: the
 * band is the cockpit HUD's quest half - it drives `lbl_806BDCC8` (the same two 0x194 B work
 * records the registered `hud/fn_80324F7C.c` and `hud/layout.cpp` bands know), it calls the
 * `hud` 2D element library (`draw_sprite*`, `drawshape_*`, `get_lsp_data`) and the `_PLW` getters,
 * and the two source files below it in the band are `cockpit.cpp` (0x802D9EB4..0x802E0740) and
 * `layout.cpp` (0x802E0740..0x802E4978), both registered in the `hud` lib.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `tools/symbols/symedit.py --section .text range 0x802E7408 0x802EBED8`: 63 of the 64 rows are
 * bare `fn_` stems and the dump answers `zz_02exxxxx_` for all of them; the single real name,
 * `PitTrapSet__FPCQ34nw4r4math4VEC3Uc` at 0x802EBB58, is written as the real function
 * `PitTrapSet(const nw4r::math::VEC3*, u8)` - rule 9/row 50 - not as its mangling).
 *
 * Seam (unproven, as the brief says).  `python tools/splits/tudiscover.py at 0x802E7408` must-links
 * only `fn_802E7408`/`fn_802E7548` (the `cockpit_quest.cpp` string anchor) and offers a *weak* left
 * boundary at 0x802E7408 and a *strong* one at 0x802E932C (`.sdata2` run jump lbl_8079A944 ->
 * lbl_8079A948).  Read as a whole, the pool evidence says the opposite of a cut at 0x802E7408: the
 * `.sdata2` run 0x8079A8E0..0x8079A944 is shared across the boundary (lbl_8079A8E8 is read by
 * `fn_802E4C5C` *and* by `fn_802E8A20`/`fn_802EBBD0`; lbl_8079A8FC by 15 functions on both sides),
 * i.e. the two bands are one object's pool, while the run jump at 0x802E932C is the real allocation
 * break.  This unit therefore claims the brief's range as registered and records the finding: the
 * true TU is probably 0x802E4978..0x802EBBD0 (`cockpit_quest.cpp`, whose string is emitted by
 * 0x802E4E00, inside the *lower* band).  Both halves are filed as one `range` config request for
 * the orchestrator's re-split; the sibling's `menu/fn_802E4978.cpp` branch is the lower half.
 *
 * Sections this unit owns: .text 0x802E7408..0x802EBED8 (64 functions), extab
 * 0x800150C4..0x8001529C and extabindex 0x8003378C..0x80033A50 - both runs are 59 records (8 and
 * 12 bytes), one per framed function of the range, and both start exactly where the band below's
 * runs end (`fn_802E7408`'s own `.note.split` says extab 0x800150C4 / extabindex 0x8003378C /
 * .text 0x802E7408; the band below's last record is 0x800150C4-8).
 *
 * Flags.  `cflags_hud` (Wii/1.3, `-O3`, `-inline noauto`, `-Cpp_exceptions on`, `-opt nopeephole`): this
 * band keeps the unfused `clrlwi` + `cmpwi`/`cmplwi` pairs retail has, and the peephole pass is off for
 * the whole `hud` lib as of 2026-09-27 (three independently measured `hud` units agree on the flag, so the
 * per-file pragma this unit was landed with is gone).  It is the whole story on four of the ten bodies
 * below - `fn_802E7408` 93.63 -> 100.00,
 * `fn_802E7690` 95.65 -> 100.00, `fn_802E884C` 96.25 -> 100.00, `fn_802E8BA4` 74.52 -> 91.00 (and
 * `fn_802E7548` 95.12 -> 99.21) - measured with `recompile.py --measure` around the pragma, everything
 * else unchanged.
 *
 * Playbook 29: the `.sdata2`/`.data` pool words, the `u16` table and the `HudBlend` tables this
 * range reads are **declared** and never defined, which is what keeps the .text/extab claims
 * linkable; no `.data`/`.sdata`/`.sdata2` range is claimed by this unit (a partial pool claim is not
 * linkable - `hud/layout.cpp`'s header records the mwld error).  The one table that *is* this unit's
 * own compiler output is `jumptable_805D62C4` (`fn_802EAAB4`'s switch, playbook 53): it stays
 * unclaimed until that body is written, and is recorded in the outbox.
 *
 * Score at this commit (the official report metric, one `recompile.py --measure` per symbol against
 * MAIN's own retired `auto_fn_*_text.o` split of the same addresses): 8.10 % fuzzy (1551 of 19152
 * `.text` bytes), **10 of the 64 bodies written, 8 of those at or above 99 % and 6 byte-identical**:
 *   100.00   fn_802E7408 (320 B), fn_802E7690 (248 B), fn_802E884C (112 B), fn_802E8F54 (104 B),
 *            fn_802E9070 (80 B), fn_802E90C0 (60 B), fn_802E90FC (52 B)
 *    99.21   fn_802E7548 (328 B)
 *    91.00   fn_802E8BA4 (232 B)
 *    35.74   fn_802E9130 (108 B)   <- residual, below
 * The 54 functions without a body score 0 and dominate the unit percentage.
 *
 * Residuals of the written bodies:
 *  - `fn_802E9130` 35.74 % (108 B target, 104 B ours).  Every instruction is present in the
 *    target's order except that MWCC hoists the u8 -> f32 widening of `value` above the fraction's
 *    `fdivs` (ours: `clrlwi`/magic-double pair, then `fsubs`/`fsubs`/`fdivs`; retail: `fdivs`
 *    first, then the widening), and retail's `add` keeps the high half of the incoming `value`
 *    (`clrrwi r3,r4,8` + `add`) where ours returns the widened product directly.  Four spellings
 *    were measured - `value = (u8)(...)` under the peephole (46.48), the same with the pragma
 *    (33.52), `value += (u8)(...)` (35.74), the fraction in a named `f32` local (35.74), the
 *    operands swapped (35.74).  Playbook 22 territory (the scheduler's choice, not the shape):
 *    kept the best, 35.74.
 *  - `fn_802E8BA4` 91.00 % (232 B target, 240 B ours): two instructions over.  Ours folds the
 *    `u16 count` loop bound into a fused compare + `bne` where retail keeps `subi`/`clrlwi`/
 *    `cmpwi` separate; the `1 << target->index` and the `|` are one instruction heavier each.
 *  - `fn_802E7548` 99.21 %: one argument register differs (`mr r3`/`mr r4` order in the tail call);
 *    the mask `(-off | off) >> 31` and both switches are byte-identical.
 *
 * Not written (54, in address order, with what each needs): the rest of the band.  The largest are
 * `fn_802E7FA0` (0x8AC), `fn_802E796C` (0x390), `fn_802E7CFC` (0x2A4), `fn_802E98A8` (0x2A0),
 * `fn_802EBBD0` (0x25C), `fn_802EA3B4` (0x240), `fn_802EB034` (0x234), `fn_802E9CB4` (0x1E8),
 * `fn_802EA5F4` (0x1E0), `fn_802E7788` (0x1E4) - all of them read the same `lbl_806BDCC8` record
 * this unit now views, and their shape oracle (`tools/units/m2cinput.py` + `tools/m2c`) output is
 * kept for the next lane in `build/tmp/scratch/m2c/` (throwaway, gitignored).
 */

#include "types.h"
#include "pl.h"
#include "nw4r/math.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_act.h"
#include "Pl/pl_master.h"
#include "hud/layout.h"
#include "hud/cockpit_quest.h"
#include "main.h"
#include "fn_80429B94.h"

/* Retail keeps the unfused `clrlwi` + `cmpwi` pairs this band is full of
 * (`fn_802E8BA4` 0x802E8BA4+0x5C) - `-opt nopeephole`, which the `hud` lib carries as `cflags_hud`. */

/* nw4r's debug panic - the map's mangling is `Panic__Q24nw4r2dbFPCciPCce` (rule 9: the owner is
 * `nw4r::db`, so the declaration is the real one and the front-end reproduces the map name). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace db

extern "C" {

/* The one string table this unit indexes itself: `lbl_805D5C74`'s two rows (5 entries each,
 * `lbl_805D5C74` + 0x14 = the first `HudBlend` table 0x805D5C88). */
extern u16 lbl_805D5C74[2][5];

/* The two cockpit work records live in the band below (see include/hud/cockpit_quest.h). */
extern CockpitWork lbl_806BDCC8[2];

}

/* 0x802E7408 (0x140).  The first bar's blend parameter: the player's slot 45 state picks a row of
 * the 2x5 `u16` table at 0x805D5C74 and the argument picks the column pair, and a zero result means
 * "no change" - the caller's previous value is kept by `fn_802E73B8`.  The `li r4,0 / cmplwi /
 * bne / li r4,1` sequence is the target's shape for the "slot is 0x7D" test, so the cancel flag is a
 * materialised local, not a folded comparison. */
u16 fn_802E7408(u16 id, u8 kind, _PLW* plw) {
    u16 off = 0;

    if (plw->field_0x464 == 0) {
        s32 row = 0;
        u8 slot = fn_802715A0(plw, 45);
        s32 cancel = 0;

        if (slot == 0x7D) {
            cancel = 1;
        }
        if (cancel == 0) {
            switch (slot) {
            case 0x7C:
                off = 0;
                break;
            case 0x7E:
                off = 3;
                break;
            case 0x7F:
                off = 4;
                break;
            default:
                off = 2;
                break;
            }
            switch (kind) {
            case 0:
                row = 0;
                off = 0;
                break;
            case 1:
                row = 0;
                break;
            case 3:
                row = 1;
                break;
            default:
                nw4r::db::Panic(lbl_805D5D08, 0x7F8, lbl_805D5D20);
                break;
            }
            off = lbl_805D5C74[row][off];
        }
    }
    if (off != 0) {
        return (u16)(id + off);
    }
    return fn_802E73B8(id, plw);
}

/* 0x802E7548 (0x148).  The second bar's blend parameter - `fn_802E7408`'s twin one slot family
 * over (slot 46, table values 0x80/0x82/0x83, kinds 0/2/4, line 2088), and its "off is zero" case
 * falls out as the target's mask `(-off | off) >> 31` rather than a branch (the caller then keeps
 * the previous value by adding it).  Its gate is `_PLW`'s +0x466 (`field_0x466`; `Pl/pl_act.cpp`
 * clamps the same field to 0x2328). */
u16 fn_802E7548(s32 id, u8 kind, _PLW* plw) {
    u16 off = 0;

    if (plw->field_0x466 == 0) {
        s32 row = 0;
        u8 slot = fn_802715A0(plw, 46);
        s32 cancel = 0;

        if (slot == 0x81) {
            cancel = 1;
        }
        if (cancel == 0) {
            switch (slot) {
            case 0x80:
                off = 0;
                break;
            case 0x82:
                off = 3;
                break;
            case 0x83:
                off = 4;
                break;
            default:
                off = 2;
                break;
            }
            switch (kind) {
            case 0:
                row = 0;
                off = 0;
                break;
            case 2:
                row = 0;
                break;
            case 4:
                row = 1;
                break;
            default:
                nw4r::db::Panic(lbl_805D5D08, 0x828, lbl_805D5D20);
                break;
            }
            off = lbl_805D5C74[row][off];
        }
    }
    return off != 0 ? (u16)(id + off) : 0;
}

/* 0x802E7690 (0xF8).  One 0x10-byte blend record: with `t == 0` the three components are copied,
 * otherwise they are interpolated between `a` and `b` by `lbl_8079A920 * (1.0f - fn_800501B4(t))`
 * clamped to [0.0f, 1.0f]; the fourth word is always copied.  `fn_800501B4` maps the blend clock to
 * a fraction (`lbl_8079A8FC` is 1.0f, `lbl_8079A8F8` 0.0f). */
void fn_802E7690(HudBlend* out, const HudBlend* a, const HudBlend* b, u16 t) {
    if (t == 0) {
        out->x = a->x;
        out->y = a->y;
        out->z = a->z;
    } else {
        f32 f = lbl_8079A920 * (lbl_8079A8FC - fn_800501B4(t));

        if (f < lbl_8079A8F8) {
            f = lbl_8079A8F8;
        }
        if (f > lbl_8079A8FC) {
            f = lbl_8079A8FC;
        }
        out->x = fn_802E270C(a->x, b->x, f);
        out->y = fn_802E270C(a->y, b->y, f);
        out->z = fn_802E270C(a->z, b->z, f);
    }
    out->tag = a->tag;
}

/* 0x802E884C (0x70).  "Is this quest target usable by the player?": `fn_802EA7DC` is asked twice -
 * a global gate first (the same arguments with flags 0), then the per-target test when the gate
 * returns 1.  The result is the boolean, materialised as 1/0 by the branch. */
s32 fn_802E884C(CockpitWork* work, QuestTarget* target) {
    u8 ok = fn_802EA7DC(work, target, 0);

    if (ok == 1) {
        ok = fn_802EA7DC(work, target, 1);
    }
    if (ok != 0) {
        return 1;
    }
    return 0;
}

/* 0x802E8F54 (0x68).  Project a world position into the screen space `fn_802B0230` describes:
 * x and z are divided by the two scales after the offsets are taken out. */
void fn_802E8F54(f32* out, const f32* pos) {
    const QuestScreen* scr = fn_802B0230();

    out[0] = (pos[0] - scr->ofs_x) / scr->scale_x;
    out[1] = (pos[2] - scr->ofs_y) / scr->scale_y;
}

/* 0x802E9070 (0x50).  The quest marker's screen position, scaled by the blend fraction of the
 * clock the caller passes (`fn_800501E4` maps ticks to a fraction, `lbl_8079A928` is the gain). */
void fn_802E9070(f32* a, f32* b, u16 t) {
    fn_80500EF4(a, b, lbl_8079A928 * fn_800501E4(t));
}

/* 0x802E90C0 (0x3C).  Project and hand the result to `fn_802E8FBC`, which turns it into the
 * squared screen distance of the marker. */
f32 fn_802E90C0(CockpitWork* work, const f32* pos) {
    f32 sp8[2];

    fn_802E8F54(sp8, pos);
    return fn_802E8FBC(work, sp8);
}

/* 0x802E90FC (0x34).  "Is the marker inside the screen?" - the projection is compared against the
 * 1.0f at `lbl_8079A8FC`, and the target's `cror eq,lt,eq` + `mfcr`/`rlwinm` is MWCC's `<=` in an
 * integer context. */
s32 fn_802E90FC(CockpitWork* work, const f32* pos) {
    return fn_802E90C0(work, pos) <= lbl_8079A8FC;
}

/* 0x802E9130 (0x6C).  Fade the marker's value by how far the target is: inside
 * `work->field_0x0B4` squared the value stands, outside it is raised by the remaining fraction of
 * 1.0f (`lbl_8079A8FC`).  Residual: see the file header (the scheduler's ordering of the u8 -> f32
 * widening, 35.74 %). */
u8 fn_802E9130(CockpitWork* work, u8 value, f32 dist) {
    f32 fade = work->field_0x0B4;
    f32 near = fade * fade;

    if (dist < near) {
        return value;
    }
    value += (u8)(((lbl_8079A8FC - dist) / (lbl_8079A8FC - near)) * (f32)value);
    return value;
}

/* 0x802E8BA4 (0xE8).  Fill the quest-target bitmask: every live record of the kind-3 move table
 * whose `fn_802E884C` predicate holds raises the bit at its own index, and the two quest-view
 * fields are reset before the count.  The target's `loop_5` is a `for` over the table
 * (`get_move_work_max(3)` entries, stride 0xB18); residual in the file header (91.00 %). */
void fn_802E8BA4(CockpitWork* work) {
    QuestTarget* target;
    u16 count;
    s8 state;

    work->field_0x0D0 = fn_802EA5F4(work->plw);
    work->field_0x0BC = 0;
    target = (QuestTarget*)get_move_work_adrs(3);
    count = get_move_work_max(3);
    for (; count > 0; count--) {
        if ((target->flags & 1) != 0 && fn_802E884C(work, target) != 0) {
            work->field_0x0BC |= (u16)(1 << target->index);
        }
        target++;
    }
    state = 4;
    if (fn_802E8D7C(work) != 0) {
        state = 0;
    }
    work->field_0x0CF = state;
    fn_802E9E9C(work, 0);
}
