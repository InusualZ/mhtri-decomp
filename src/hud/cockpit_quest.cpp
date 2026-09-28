/*
 * hud/cockpit_quest.cpp - the quest cockpit HUD band (0x802E7408..0x802EBED8, 64 functions, 19152 B).
 *
 * Home and name, evidence class 1 (a `__FILE__` string): `.data` 0x805D5D08 is the bare source name
 * "cockpit_quest.cpp" (18 B; `config/RMHE08/symbols.txt`'s `lbl_805D5D08`, `type:object size:0x12
 * data:string`), the runtime dump spells it `_802e4e00s_cockpit_quest.cpp_805d5d08`, and it is
 * referenced from *inside this range* - `quest_bar_a_next_id` (assert at 0x802E74DE, line 2040) and
 * `quest_bar_b_next_id` (0x802E7650, line 2088) pass it to `nw4r::db::Panic` with the message at
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
 * only `quest_bar_a_next_id`/`quest_bar_b_next_id` (the `cockpit_quest.cpp` string anchor) and offers a *weak* left
 * boundary at 0x802E7408 and a *strong* one at 0x802E932C (`.sdata2` run jump lbl_8079A944 ->
 * lbl_8079A948).  Read as a whole, the pool evidence says the opposite of a cut at 0x802E7408: the
 * `.sdata2` run 0x8079A8E0..0x8079A944 is shared across the boundary (lbl_8079A8E8 is read by
 * `fn_802E4C5C` *and* by `fn_802E8A20`/`quest_marker_draw_faded`; lbl_8079A8FC by 15 functions on both sides),
 * i.e. the two bands are one object's pool, while the run jump at 0x802E932C is the real allocation
 * break.  This unit therefore claims the brief's range as registered and records the finding: the
 * true TU is probably 0x802E4978..0x802EBBD0 (`cockpit_quest.cpp`, whose string is emitted by
 * 0x802E4E00, inside the *lower* band).  Both halves are filed as one `range` config request for
 * the orchestrator's re-split; the sibling's `menu/fn_802E4978.cpp` branch is the lower half.
 *
 * Sections this unit owns: .text 0x802E7408..0x802EBED8 (64 functions), extab
 * 0x800150C4..0x8001529C and extabindex 0x8003378C..0x80033A50 - both runs are 59 records (8 and
 * 12 bytes), one per framed function of the range, and both start exactly where the band below's
 * runs end (`quest_bar_a_next_id`'s own `.note.split` says extab 0x800150C4 / extabindex 0x8003378C /
 * .text 0x802E7408; the band below's last record is 0x800150C4-8).
 *
 * Flags.  `cflags_hud` (Wii/1.3, `-O3`, `-inline noauto`, `-Cpp_exceptions on`, `-opt nopeephole`): this
 * band keeps the unfused `clrlwi` + `cmpwi`/`cmplwi` pairs retail has, and the peephole pass is off for
 * the whole `hud` lib as of 2026-09-27 (three independently measured `hud` units agree on the flag, so the
 * per-file pragma this unit was landed with is gone).  It is the whole story on four of the ten bodies
 * below - `quest_bar_a_next_id` 93.63 -> 100.00,
 * `quest_blend_lerp` 95.65 -> 100.00, `quest_target_visible_ck` 96.25 -> 100.00, `quest_targets_update_a` 74.52 -> 91.00 (and
 * `quest_bar_b_next_id` 95.12 -> 99.21) - measured with `recompile.py --measure` around the pragma, everything
 * else unchanged.
 *
 * Playbook 29: the `.sdata2`/`.data` pool words, the `u16` table and the `HudBlend` tables this
 * range reads are **declared** and never defined, which is what keeps the .text/extab claims
 * linkable; no `.data`/`.sdata`/`.sdata2` range is claimed by this unit (a partial pool claim is not
 * linkable - `hud/layout.cpp`'s header records the mwld error).  Two things this unit's compiler
 * emits are extras the target object does not carry, and both are `flipcheck.py` refusals:
 * `jumptable_805D62C4` (`fn_802EAAB4`'s switch, playbook 53), unclaimed until that body is written
 * and recorded in the outbox, and an 8-byte `.sdata2` word - 0x4330000000000000, the int -> double
 * magic `quest_mark_fade_value`'s `(f32)value` widens through where the target loads the shared pool
 * (`lbl_8079A910`) instead - claimable only while this unit is its sole referencer (playbook 58), or
 * removable by a source shape that does not synthesise it.
 *
 * Names.  Every `fn_XXXXXXXX` this file defines or calls was renamed through `symedit.py` and swept
 * (38 rows, 2026-09-28): 26 of them are this band's own bodies, the other 12 are callees other
 * registered units own (`Pl_Skill_slot_item_get`, `Pl_act_state_ck`, `pl_act_kind_get`,
 * `anim_tick_cos`, `anim_tick_angle`, `math_sincos_idx`, `screen_projection_get`, `quest_bar_id_keep`,
 * `ai_npc_hold_item_arm`, `uv_pair_copy`, `quest_marker_draw`, `menu_item_frame_update`).  The names
 * are derived from the call sites and each callee's own body - none of them comes from the runtime
 * dump (which answers `zz_02exxxxx_` for this whole band), so the derived ones are **GUESS**es in the
 * sense of the brief and a later pass may refine them.
 *
 * Score at this commit (official report metric; review lane: `python tools/objdiff/unitscore.py
 * hud/cockpit_quest`): 8.099 -> **14.448622 %** fuzzy, matched code 976 -> **1948 of 19152** `.text`
 * bytes, matched functions 7 -> **17 of 64**.  The rows with a body, 21 (17 of them at 100 %):
 *   100.00   quest_bar_a_next_id (320 B), quest_blend_lerp (248 B), quest_target_visible_ck (112 B),
 *            quest_screen_project (104 B), quest_rot_pair (80 B), quest_mark_dist (60 B),
 *            quest_mark_on_screen_ck (52 B), quest_mark_visible_ck (184 B), quest_mark_toggle (40 B),
 *            quest_mark_flag_get (8 B), quest_mark_dist_sq (180 B), quest_slot_arm_all_a (124 B),
 *            quest_slot_arm_a (108 B), quest_slot_arm_all_b (124 B), quest_slot_arm_b (80 B),
 *            quest_mark_append (116 B), quest_target_usable0_ck (8 B)
 *    99.21   quest_bar_b_next_id (328 B)
 *    96.50   quest_targets_update_b (240 B)
 *    96.38   quest_targets_update_a (232 B)
 *    35.74   quest_mark_fade_value (108 B)   <- residual, below
 * The 43 functions without a body score 0 and dominate the unit percentage.
 *
 * Residuals of the written bodies:
 *  - `quest_mark_fade_value` 35.74 % (108 B target, 104 B ours).  Every instruction is present in the
 *    target's order except that MWCC hoists the u8 -> f32 widening of `value` above the fraction's
 *    `fdivs` (ours: `clrlwi`/magic-double pair, then `fsubs`/`fsubs`/`fdivs`; retail: `fdivs`
 *    first, then the widening), and retail's `add` keeps the high half of the incoming `value`
 *    (`clrrwi r3,r4,8` + `add`) where ours returns the widened product directly.  Five spellings
 *    were measured - `value = (u8)(...)` under the peephole (46.48), the same with the pragma
 *    (33.52), `value += (u8)(...)` (35.74), the fraction in a named `f32` local (35.74), the
 *    operands swapped (35.74).  Playbook 22 territory (the scheduler's choice, not the shape):
 *    kept the best, 35.74.
 *  - `quest_targets_update_a` 96.38 % / `quest_targets_update_b` 96.50 % (232/240 B target, same
 *    sizes ours): one instruction of shape, the `+0xCF` state byte.  Retail keeps `4` in a *scratch*
 *    (`li r0,4` after the `quest_mark_visible_ck` call, then `beq`/`li r0,0`/`stb`), ours keeps the
 *    value in a callee-saved register live across the call (`li r29,4` in the preheader).  Measured
 *    spellings: `state = 4; if (call != 0) state = 0;` (the best, kept), `if (call == 0) state = 4;
 *    else state = 0;` (both 96.38 -> 93.92), the ternary `call == 0 ? 4 : 0` (branchless
 *    `cntlzw`/`extrwi`, 95.78).  All three shapes are correct; the register the allocator picks is
 *    playbook 22's residual, so the best-scoring one stays.
 *    Both improved this pass from 91.00/68.03 by the loop shape alone: `s32 count` (not `u16`),
 *    the `count--`/`target++` pair at the *end of the body* in a `while (count > 0)`, and
 *    `quest_target_rank_get(work, work->plw)` - the callee's own body reads `r3+0x48`/`+0x184` as a
 *    `CockpitWork*` and `r4` as the `_PLW*`, so its first parameter is the work record.
 *  - `quest_bar_b_next_id` 99.21 %: one argument register differs (`mr r3`/`mr r4` order in the tail call);
 *    the mask `(-off | off) >> 31` and both switches are byte-identical.
 *
 * NOT WRITTEN - BLOCKED ON THE ONE-RECORD FOLD: `quest_marker_arm` (0x802E8E64, 240 B) is **100 % in
 * the working state of 2026-09-28** (branch `worker/ui-cockpit-quest-8c20`, commit e6d85ecbd) and is
 * deliberately left unwritten: its target calls the *neighbour* body `quest_marker_draw`
 * (0x802EBED8) once, whose owner header `include/hud/fn_802EBED8.h` cannot be included from this
 * translation unit (it redefines `CockpitWork`/`lbl_806BDCC8`, gives `lbl_806BDFF0` a second
 * vocabulary `QuestBlink`, and declares `get_now_areano` inside `extern "C"` against the map's
 * mangled row), and a local declaration of it here is a rule 2 finding the land gate refuses.
 * **Restore it after the fold** (see below); the body is one address away and its comment block is
 * still in this file, at 0x802E8E64's slot.  The body text exists only in commit `e6d85ecbd`, which
 * the landing orphans (`367671cff` removes it) - recover it with
 * `git show e6d85ecbd:src/hud/cockpit_quest.cpp` (or the rescue ref a release parks the branch at,
 * `refs/rescue/cockpit-quest-a8ab`) rather than re-deriving the 240 B.
 *
 * THE FOLD (a `merge` item, filed in the outbox - three units, three headers, four rulings already
 * taken by the owner 2026-09-28): the shared `.bss` records `lbl_806BDCC8` (0x328 B, stride 0x194)
 * and `lbl_806BDFF0` (0x88 B) currently have three vocabularies -
 * `include/menu/fn_802E4978.h` (the storage owner: `CockpitWork`, `CockpitState`),
 * `include/hud/fn_802EBED8.h` (`CockpitWork`, `extern _PLW* lbl_806BDCC8[]`, `QuestBlink`) and this
 * header (`CockpitWork`, `CockpitState`, the members this band reads).  The fold gives the records
 * one owner header; the rulings are: (1) `CockpitWork::+0x000` is `_PLW*` - the call sites decide
 * (`Pl_Skill_ck`, `Pl_master_ck`, `Pl_Skill_slot_item_get` all take `_PLW*`), and the owner's
 * `CockpitMove*` view disappears; (2) the owner's `CockpitState` names win and its extent is the
 * map/split size 0x88, not `QuestBlink`'s "0x100 approximate"; (3) the width of `+0x5BC` is decided
 * by the access - this band's `quest_mark_visible_ck` loads it with `lbz`, so it is `u8` and belongs
 * to `pl.h`'s `_PLW`; (4) `get_now_areano` drops `extern "C"` (the map row is the mangling, so the
 * C++ declaration is the correct one) with `hud/fn_802EBED8` re-measured around it and that part
 * reverted alone if a row drops.
 *
 * Playbook 29: the `.sdata2`/`.data` pool words, the `u16` table and the `HudBlend` tables this
 * range reads are **declared** and never defined, which is what keeps the .text/extab claims
 * linkable; no `.data`/`.sdata`/`.sdata2` range is claimed by this unit (a partial pool claim is not
 * linkable - `hud/layout.cpp`'s header records the mwld error).  Two things this unit's compiler
 * emits are extras the target object does not carry, and both are `flipcheck.py` refusals:
 * `jumptable_805D62C4` (`fn_802EAAB4`'s switch, playbook 53), unclaimed until that body is written
 * and recorded in the outbox, and an 8-byte `.sdata2` word - 0x4330000000000000, the int -> double
 * magic `quest_mark_fade_value`'s `(f32)value` widens through where the target loads the shared pool
 * (`lbl_8079A910`) instead - claimable only while this unit is its sole referencer (playbook 58), or
 * removable by a source shape that does not synthesise it.  The header declares only the
 * pool entries a *written* body reads (rule 12): the rest arrive with the body that consumes them.
 *
 * Not written (43, in address order, with what each needs): the rest of the band.  The largest are
 * `fn_802E7FA0` (0x8AC), `fn_802E796C` (0x390), `fn_802E7CFC` (0x2A4), `fn_802E98A8` (0x2A0),
 * `quest_marker_draw_faded` (0x25C), `fn_802EA3B4` (0x240), `fn_802EB034` (0x234), `fn_802E9CB4` (0x1E8),
 * `quest_target_rank_get` (0x1E0), `fn_802E7788` (0x1E4) - all of them read the same `lbl_806BDCC8` record
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
#include "menu/menu_item.h"
#include "menu/get_pop_dat_ptr.h"
#include "unsplit/unknown.h"
#include "ai/fn_802D44F4.h"
#include "ef/fn_800CDB2C.h"
#include "lobby/lb_quest_screen.h"
#include "enemy/fn_8012EC74.h"
#include "mh3_pad.h"

/* Retail keeps the unfused `clrlwi` + `cmpwi` pairs this band is full of
 * (`quest_targets_update_a` 0x802E8BA4+0x5C) - `-opt nopeephole`, which the `hud` lib carries as `cflags_hud`. */

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
 * "no change" - the caller's previous value is kept by `quest_bar_id_keep`.  The `li r4,0 / cmplwi /
 * bne / li r4,1` sequence is the target's shape for the "slot is 0x7D" test, so the cancel flag is a
 * materialised local, not a folded comparison. */
u16 quest_bar_a_next_id(u16 id, u8 kind, _PLW* plw) {
    u16 off = 0;

    if (plw->field_0x464 == 0) {
        s32 row = 0;
        u8 slot = Pl_Skill_slot_item_get(plw, 45);
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
    return quest_bar_id_keep(id, plw);
}

/* 0x802E7548 (0x148).  The second bar's blend parameter - `quest_bar_a_next_id`'s twin one slot family
 * over (slot 46, table values 0x80/0x82/0x83, kinds 0/2/4, line 2088), and its "off is zero" case
 * falls out as the target's mask `(-off | off) >> 31` rather than a branch (the caller then keeps
 * the previous value by adding it).  Its gate is `_PLW`'s +0x466 (`field_0x466`; `Pl/pl_act.cpp`
 * clamps the same field to 0x2328). */
u16 quest_bar_b_next_id(s32 id, u8 kind, _PLW* plw) {
    u16 off = 0;

    if (plw->field_0x466 == 0) {
        s32 row = 0;
        u8 slot = Pl_Skill_slot_item_get(plw, 46);
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
 * otherwise they are interpolated between `a` and `b` by `lbl_8079A920 * (1.0f - anim_tick_cos(t))`
 * clamped to [0.0f, 1.0f]; the fourth word is always copied.  `anim_tick_cos` maps the blend clock to
 * a fraction (`lbl_8079A8FC` is 1.0f, `lbl_8079A8F8` 0.0f). */
void quest_blend_lerp(HudBlend* out, const HudBlend* a, const HudBlend* b, u16 t) {
    if (t == 0) {
        out->x = a->x;
        out->y = a->y;
        out->z = a->z;
    } else {
        f32 f = lbl_8079A920 * (lbl_8079A8FC - anim_tick_cos(t));

        if (f < lbl_8079A8F8) {
            f = lbl_8079A8F8;
        }
        if (f > lbl_8079A8FC) {
            f = lbl_8079A8FC;
        }
        out->x = color_lerp(a->x, b->x, f);
        out->y = color_lerp(a->y, b->y, f);
        out->z = color_lerp(a->z, b->z, f);
    }
    out->tag = a->tag;
}

/* 0x802E884C (0x70).  "Is this quest target usable by the player?": `quest_target_usable_ck` is asked twice -
 * a global gate first (the same arguments with flags 0), then the per-target test when the gate
 * returns 1.  The result is the boolean, materialised as 1/0 by the branch. */
s32 quest_target_visible_ck(CockpitWork* work, QuestTarget* target) {
    u8 ok = quest_target_usable_ck(work, target, 0);

    if (ok == 1) {
        ok = quest_target_usable_ck(work, target, 1);
    }
    if (ok != 0) {
        return 1;
    }
    return 0;
}

/* 0x802E8BA4 (0xE8).  Fill the quest-target bitmask: every live record of the kind-3 move table
 * whose `quest_target_visible_ck` predicate holds raises the bit at its own index, and the two quest-view
 * fields are reset before the count.  The target's `loop_5` is a `for` over the table
 * (`get_move_work_max(3)` entries, stride 0xB18); the residual is in the file header. */
void quest_targets_update_a(CockpitWork* work) {
    QuestTarget* target;
    s32 count;
    s8 state;

    work->field_0x0D0 = quest_target_rank_get(work, work->plw);
    work->field_0x0BC = 0;
    target = (QuestTarget*)get_move_work_adrs(3);
    count = get_move_work_max(3);
    while (count > 0) {
        if ((target->flags & 1) != 0 && quest_target_visible_ck(work, target) != 0) {
            work->field_0x0BC |= (u16)(1 << target->index);
        }
        count--;
        target++;
    }
    state = 4;
    if (quest_mark_visible_ck(work) != 0) {
        state = 0;
    }
    work->field_0x0CF = state;
    quest_view_frame_task(work, 0);
}

/* 0x802E8C8C (0xF0).  `quest_targets_update_a`'s twin for the second cockpit view: the same target scan and
 * countdown, with the per-frame task argument 1, and the player state's pending flag latched into
 * its live one at the end. */
void quest_targets_update_b(CockpitWork* work) {
    CockpitState* state = &lbl_806BDFF0;
    QuestTarget* target;
    s32 count;
    s8 view_state;

    work->field_0x0D0 = quest_target_rank_get(work, work->plw);
    work->field_0x0BC = 0;
    target = (QuestTarget*)get_move_work_adrs(3);
    count = get_move_work_max(3);
    while (count > 0) {
        if ((target->flags & 1) != 0 && quest_target_visible_ck(work, target) != 0) {
            work->field_0x0BC |= (u16)(1 << target->index);
        }
        count--;
        target++;
    }
    view_state = 4;
    if (quest_mark_visible_ck(work) != 0) {
        view_state = 0;
    }
    work->field_0x0CF = view_state;
    quest_view_frame_task(work, 1);
    state->field_0x059 = state->field_0x058;
    state->field_0x058 = 0;
}

/* 0x802E8D7C (0xB8).  "May this view's quest mark be shown?": the view gate first, then the option
 * configuration (`get_option_cfg(14)` in normal play, `get_arena_cfg(player chunk, 14)` in VS
 * mode), then the item menu's frame, and finally the player work record's own +0x5BC byte. */
s32 quest_mark_visible_ck(CockpitWork* work) {
    if (work->field_0x0CE != 0) {
        return 0;
    }
    if (system_w.vs_mode_0x8b0 == 0) {
        if (get_option_cfg(14) == 1) {
            return 0;
        }
    } else if (get_arena_cfg(work->plw->chunk_ofs, 14) == 1) {
        return 0;
    }
    /* `menu/menu_item.h` names the record `menu_item_frame_update` walks `MenuFrameWork`; it is the same
     * player work record this header calls `_PLW` (both views name +0x008 and +0x5BC), and the
     * target's call site passes the `CockpitWork::plw` pointer unchanged. */
    if (menu_item_frame_update((MenuFrameWork*)work->plw) == 1) {
        return 0;
    }
    return work->plw->field_0x5BC == 0;
}

/* 0x802E8E34 (0x28).  Toggle the view's +0xCE gate and report its new value. */
u8 quest_mark_toggle(CockpitWork* work) {
    if (work->field_0x0CE == 0) {
        work->field_0x0CE = 1;
    } else {
        work->field_0x0CE = 0;
    }
    return work->field_0x0CE;
}

/* 0x802E8E5C (0x8).  The view's +0xCE gate. */
u8 quest_mark_flag_get(CockpitWork* work) {
    return work->field_0x0CE;
}

/* 0x802E8E64 (0xF0) - NOT WRITTEN, blocked on the one-record fold.  It measures 240 B at 100 % in
 * the working state of 2026-09-28 (branch `worker/ui-cockpit-quest-8c20`, commit e6d85ecbd, which
 * holds the body text: `git show e6d85ecbd:src/hud/cockpit_quest.cpp`) and its only obstacle is
 * that the target calls the *neighbour* body `quest_marker_draw` (0x802EBED8) once:
 *
 *     work->draw_id_0x0BA = 0xBE5;  mark = pos;
 *     if (work->field_0x0CE == 0) quest_marker_draw(work, 0xBE4, 0, &mark);
 *     else { pos.x += 0x20C; pos.y = 0xC6; work->field_0x0A0/0xA4/0xA8 = 0x8079A930/34/38; ... quest_marker_draw_faded(...); }
 *
 * `quest_marker_draw` is owned by `src/hud/fn_802EBED8.cpp`, so its declaration belongs in that
 * unit's header - and that header cannot be included from this translation unit (it redefines
 * `CockpitWork`/`lbl_806BDCC8`, gives `lbl_806BDFF0` a second vocabulary `QuestBlink`, and declares
 * `get_now_areano` inside an `extern "C"` block against the map's mangled row).  Declaring it here
 * instead is a rule 2 finding the land gate refuses, so the body stays unwritten with its evidence
 * until the fold lands (file header) - it is one address away from being restored verbatim.
 *
 * `quest_marker_draw_faded` (0x802EBBD0) is THIS unit's own body and is itself still unwritten (the
 * not-written list in the file header; the report scores it 0.00000); only the `quest_marker_draw`
 * arm blocks this one, which is why the two arms cannot be split.
 */

/* 0x802E8F54 (0x68).  Project a world position into the screen space `screen_projection_get` describes:
 * x and z are divided by the two scales after the offsets are taken out. */
void quest_screen_project(f32* out, const f32* pos) {
    const QuestScreen* scr = screen_projection_get();

    out[0] = (pos[0] - scr->ofs_x) / scr->scale_x;
    out[1] = (pos[2] - scr->ofs_y) / scr->scale_y;
}

/* 0x802E8FBC (0xB4).  The squared screen distance of the marker: the offset is rotated by the
 * view's rotation clock and carried by the two scales of +0xAC/+0xB0, and the two squares are
 * added. */
#pragma fp_contract off
f32 quest_mark_dist_sq(CockpitWork* work, const f32* pos) {
    f32 rot[2];
    f32 dx = pos[0] - work->marker_ref_x_0x098;
    f32 dy = pos[1] - work->marker_ref_y_0x09C;
    f32 x;
    f32 y;

    quest_rot_pair(&rot[1], &rot[0], work->field_0x0B8);
    x = dx * rot[0] - dy * rot[1];
    y = dx * rot[1] + dy * rot[0];
    x *= work->field_0x0AC;
    y *= work->field_0x0B0;
    return x * x + y * y;
}
#pragma fp_contract on

/* 0x802E9070 (0x50).  The quest marker's screen position, scaled by the blend fraction of the
 * clock the caller passes (`anim_tick_angle` maps ticks to a fraction, `lbl_8079A928` is the gain). */
void quest_rot_pair(f32* a, f32* b, u16 t) {
    math_sincos_idx(a, b, lbl_8079A928 * anim_tick_angle(t));
}

/* 0x802E90C0 (0x3C).  Project and hand the result to `quest_mark_dist_sq`, which turns it into the
 * squared screen distance of the marker. */
f32 quest_mark_dist(CockpitWork* work, const f32* pos) {
    f32 sp8[2];

    quest_screen_project(sp8, pos);
    return quest_mark_dist_sq(work, sp8);
}

/* 0x802E90FC (0x34).  "Is the marker inside the screen?" - the projection is compared against the
 * 1.0f at `lbl_8079A8FC`, and the target's `cror eq,lt,eq` + `mfcr`/`rlwinm` is MWCC's `<=` in an
 * integer context. */
s32 quest_mark_on_screen_ck(CockpitWork* work, const f32* pos) {
    return quest_mark_dist(work, pos) <= lbl_8079A8FC;
}

/* 0x802E9130 (0x6C).  Fade the marker's value by how far the target is: inside
 * `work->field_0x0B4` squared the value stands, outside it is raised by the remaining fraction of
 * 1.0f (`lbl_8079A8FC`).  The residual - the scheduler's ordering of the u8 -> f32 widening and the
 * `.sdata2` magic that widening synthesises - is in the file header. */
u8 quest_mark_fade_value(CockpitWork* work, u8 value, f32 dist) {
    f32 fade = work->field_0x0B4;
    f32 near = fade * fade;

    if (dist < near) {
        return value;
    }
    value += (u8)(((lbl_8079A8FC - dist) / (lbl_8079A8FC - near)) * (f32)value);
    return value;
}

/* 0x802EA050 (0x7C).  Every live player's `quest_slot_arm_a` arm must have taken for the caller's
 * action to pass; the result is the AND over the players `lbl_806BDFF0` counts. */
s32 quest_slot_arm_all_a(s8 value) {
    s32 ok = 1;
    CockpitState* state = &lbl_806BDFF0;
    s32 i;

    for (i = 0; i < state->field_0x000; i++) {
        if (quest_slot_arm_a(&lbl_806BDCC8[i], value) == 0) {
            ok = 0;
        }
    }
    return ok;
}

/* 0x802EA0CC (0x6C).  Arm one player's slot: an armed (negative) slot timer is cleared, the
 * player's bit goes into the shared flag byte and the slot's own byte is cleared. */
s32 quest_slot_arm_a(CockpitWork* work, s8 slot) {
    if (work->slot_timer_0x0C0[slot] >= 0) {
        return 0;
    }
    work->slot_timer_0x0C0[slot] = 0;
    work->slot_flags_0x0C8 |= (u8)(1 << slot);
    work->slot_state_0x0C9[slot] = 0;
    ai_npc_hold_item_arm();
    return 1;
}

/* 0x802EA138 (0x7C).  `quest_slot_arm_all_a`'s twin over `quest_slot_arm_b`. */
s32 quest_slot_arm_all_b(s8 value) {
    s32 ok = 1;
    CockpitState* state = &lbl_806BDFF0;
    s32 i;

    for (i = 0; i < state->field_0x000; i++) {
        if (quest_slot_arm_b(&lbl_806BDCC8[i], value) == 0) {
            ok = 0;
        }
    }
    return ok;
}

/* 0x802EA1B4 (0x50).  `quest_slot_arm_a` without the follow-up call: the slot's own byte is set
 * instead of cleared. */
s32 quest_slot_arm_b(CockpitWork* work, s8 slot) {
    if (work->slot_timer_0x0C0[slot] >= 0) {
        return 0;
    }
    work->slot_timer_0x0C0[slot] = 0;
    work->slot_flags_0x0C8 |= (u8)(1 << slot);
    work->slot_state_0x0C9[slot] = 1;
    return 1;
}

/* 0x802EA204 (0x74).  Append one mark to the player's 8-entry table: the texture coordinate pair
 * is copied and the three values ride behind it, and the count only advances while there is room. */
void quest_mark_append(CockpitWork* work, const _mh_tex_uv_* uv, u8 tex_idx, u32 coord, u16 size) {
    QuestMark* mark;

    if (work->mark_count_0x0CD >= 8) {
        return;
    }
    mark = &work->marks_0x0D4[work->mark_count_0x0CD];
    uv_pair_copy(&mark->uv, uv);
    mark->field_0x04 = coord;
    mark->field_0x08 = size;
    mark->field_0x0A = tex_idx;
    work->mark_count_0x0CD++;
}

/* 0x802EA7D4 (0x8).  `quest_target_usable_ck` with no flags. */
u8 quest_target_usable0_ck(CockpitWork* work, QuestTarget* target) {
    return quest_target_usable_ck(work, target, 0);
}
