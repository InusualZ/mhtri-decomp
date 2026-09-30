/* enemy/fn_8012EC74.cpp - the enemy per-motion frame-window/area set, `.text` 0x8012EC74..0x80137604.
 * Naming note: the symbol map spells 266 of this range's 281 functions as bare `fn_XXXXXXXX` rows
 * (checked `python tools/symbols/symedit.py range 0x8012EC74 0x80137604`); those keep the map's stem.
 * The 15 rows that carry real names (`em_sleep_ck`, `em_get_mot_no`, `em_frame_check`,
 * `em_after_frame_check`, `em_water_check`, `em_magma_check`, `UpdateValue`, `shuffle1`..`shuffle6`,
 * `CancelFade`, `get_joint_wmat_em`, `get_joint_wpos_em`, `get_em_scale`, `get_em_chg_scale`) are
 * defined through those names.
 *
 * Run state.  This is the run discovery proposed (`proposal/8012EC74_fn_8012EC74.cpp`, 281 functions /
 * 0x8990 bytes).  It is registered once, at its final home.  Bodies are being filled in address order
 * (brief section 5); the functions below are the first block and the rest of the range is the named
 * follow-up queue in the residual section at the end of this header.
 *
 * Seam (re-proved from this range's own evidence, not assumed).  The left edge at 0x8012EC74 is real:
 * `tudiscover at 0x8012EC74` reports the one strong observation in the band, the `.sdata2` pool-run
 * jump `lbl_80796CB4 -> lbl_80796CB8` (referrer sets disjoint and ordered), whose legal `.text` cuts
 * are 0x8012EC74 and the two function starts inside the landed left neighbour; the landed
 * `enemy/fn_8012E968.cpp` owns 0x8012E968..0x8012EC74 and its own pool run is 0x80796C9C..0x80796CB4,
 * while this range's first functions read the next run (0x80796CB8 0.3f, 0x80796CBC 0.2f, 0x80796CC0
 * 0.18f, 0x80796CC4 0.15f, 0x80796CC8 0.1f, 0x80796CCC 0.05f, 0x80796CD0 0.9f) - i.e. a *different*
 * MWCC pool fragment, so this range does not continue that unit and is registered as its own.  (The
 * extab/extabindex runs tile it too: this unit's extab starts where fn_8012E968's ends, 0x8000CD34,
 * and its extabindex starts where that unit's ends, 0x800272B8.)  The right edge at 0x80137604 is the
 * landed `enemy/fn_80137604.cpp`'s own start, and this unit's extab ends at 0x8000D284 where that
 * unit's begins.
 *
 * What it is.  The enemy per-motion support set of `_ENEMY_WORK`: the motion-frame window helpers
 * (fn_8012EC74's 0.3/0.2/0.18/0.15 scaling, fn_8012ECF0/em_motion_window_ck's elapsed-frame ratio tests,
 * fn_8012ED68/fn_8012EE80's team-wide window scans), the in-area gates (fn_8012EFDC's status/team
 * test, fn_8012F110's program-mode test, fn_8012F39C's height-vs-scale test), the sleep gate
 * (`em_sleep_ck`) and, further up, the motion/effect helpers the map already names (`em_get_mot_no`,
 * `em_frame_check`, `get_joint_wmat_em`, the `shuffle*` table builders).
 *
 * Sections.  `.text` plus `extab` 0x8000CD34..0x8000D284 and `extabindex` 0x800272B8..0x80027AB0,
 * read out of the retired per-function `auto_*_text.o` records (the first function's `@etb_8000CD34` /
 * `@eti_800272B8`, the last's 0x8000D27C / 0x80027AA4 + their extab/extabindex sizes).  No `.ctors`
 * word in the range.  The `.sdata2`/`.bss` labels this unit reads (`lbl_80796C50`..`lbl_80796CD0`, the
 * move-work records) are *read*, never defined, so nothing but `.text`/`extab`/`extabindex` is claimed
 * (playbook 29: pool literals stay with the data pass).
 *
 * Types.  `_ENEMY_WORK` is the shared 0xB18 record in `include/enemy/ENEMY_WORK.h` (rule 1).  This
 * unit named the bytes it measures that the header still had as padding, at their measured offsets:
 * `+0x00B` (the effect-queue argument byte), `+0x1AC` (a height compared against 0.9 * the model
 * scale), `+0x43B` (the per-motion kind gate), `+0x7AC`/`0x7B0` (the second window counter and its
 * threshold), `+0x7BC`/`0x7C0`/`0x7C4` (the effect radius pair and the value F504 clears), `+0x818` and
 * `+0x938` (two signed motion timers).  Every insertion keeps the existing offsets exact.
 *
 * Language / declarations.  The unit calls both C-linkage `fn_*` helpers and C++ mangled helpers
 * (`em_area_ck`, `em_die_ck`, `em_sleep_ck`), so it is C++ (rule 9: a caller never spells a mangling).
 * `em_area_ck`/`em_die_ck` and the `get_enemy_data` accessor are declared in their owners' headers
 * (`enemy/fn_8012BDF4.h`, `enemy/fn_801251D0.h`), which this file includes (rule 2); `em_area_ck`'s C++
 * spelling was added to its owner's header in the same landing as this file because the header only
 * carried the mangled `em_area_ck__FP11_ENEMY_WORK` spelling before.  `fn_8012D8D0`, `fn_8012DB3C` and
 * `fn_8012E21C` are declared in their owner's header for the same reason.  The in-range helpers this
 * block calls (`em_get_rank`, `fn_80130134`, `fn_8013032C`, `fn_801322CC`, `fn_80132270`) are
 * forward-declared here and are part of the follow-up queue.
 *
 * Flags.  The unit's command line is the `enemy` lib's (`configure.py`), and it carries the same
 * scoped `#pragma peephole off` as its landed neighbours `enemy/fn_8012BDF4.cpp` and
 * `enemy/fn_8012E968.cpp`: retail keeps `clrlwi`/`rlwinm` + `cmpwi` separate where the `-O3` peephole
 * would fuse them into a record form.
 *
 * Status and residual.  Measured with `python tools/units/recompile.py enemy/fn_8012EC74.cpp --measure
 * <symbol>` against the retired per-function `auto_*_text.o` (the same original bytes the split object
 * will carry).  First block (0x8012EC74..0x8012F39C):
 *   * 100.00 % - fn_8012EC74 (124 B), fn_8012ECF0 (120 B), fn_8012EF98 (68 B), fn_8012EFDC (308 B),
 *     fn_8012F110 (200 B), fn_8012F2A4 (96 B);
 *   * em_motion_window_ck 97.65 % (204/204 B) - the final `field_0x7A0/0x7A4 <= rate+pad` is materialised by
 *     this build's allocator branchlessly (`mfcr` + `extrwi`) where retail keeps the branch
 *     (`bne ret0` + `li r3,1`); an `if (...) return 1; return 0;` shape is worse (212 B, 95.98 %), so
 *     the value-return form is applied.  Register-colouring residual, docs/matching.md row 22;
 *   * fn_8012ED68 96.13 % (272/280 B) and fn_8012EE80 96.99 % (272/280 B) - every instruction matches
 *     except the two-instruction dead loop counter (`li r4,0` + `addi r4,r4,1`); this build drops an
 *     unused `i`, retail keeps it.  The same residual `enemy/fn_8012E968.cpp` records for its walk
 *     (“a fresh local does not reproduce it”).  Recorded, not chased;
 *   * em_sleep_ck 89.21 % (168/152 B) - the two-arm gate is otherwise identical; this build folds the
 *     second `== 1` test into a branchless `(x==1)` and materialises a `li r3,0` per arm where retail
 *     shares one `ret0` and falls through both `bne ret0` arms.  The `&&`/nested/`||` shapes all score
 *     65-89 %; the best (89.21 %) is applied.  Recorded;
 *   * fn_8012F39C 87.50 % (88/88 B, size exact) - the height/scale comparison's register pair is
 *     retail's (`f2`/`f0`) here `f0`/`f1` (the load of the constant is hoisted before the field); both
 *     orders and the local-temp form were measured and this is the best.  Register-colouring residual.
 *
 * Follow-up queue (the rest of the range, in address order): every symbol from 0x8012F3F4
 * (`fn_8012F3F4`) to 0x801373D0 (`fn_801373D0`) - 270 functions, including the named
 * `em_get_mot_no` (0x8012F8FC), `em_frame_check`/`em_after_frame_check` (0x8012F91C/0x8012F92C),
 * `UpdateValue` (0x8012FDA0), `em_water_check`/`em_magma_check` (0x80130104/0x8013011C),
 * `shuffle1`..`shuffle6` (0x801325A4..0x80133344), `CancelFade` (0x80135428),
 * `get_joint_wmat_em`/`get_joint_wpos_em`/`get_em_scale`/`get_em_chg_scale`
 * (0x80135930/0x80135938/0x80135940/0x80135950) and the large blocks `fn_801363F8` (0x5A8 B) /
 * `fn_80136E38` (0x598 B) / `fn_80135000` (0x3E4 B) / `fn_80131150` (0x32C B) first.
 */

#include "types.h"

#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"

#pragma peephole off

/* The `.sdata2` pool labels this block reads, in address order.  They belong to the data pass and are
 * only referenced here (the target object's own relocations name them), never defined. */
extern "C" f32 lbl_80796C50;   /* 1.0f  - the seed `fn_8012F474` starts from */
extern "C" f32 lbl_80796C58;   /* 0.0f  - `fn_8012EFDC`'s `team == 0xf` threshold */
extern "C" f32 lbl_80796C5C;   /* 100.0f - the frame-window denominator */
extern "C" f64 lbl_80796C68;   /* the u32->double magic pair */
extern "C" f64 lbl_80796C80;   /* the s32->double magic */
extern "C" f32 lbl_80796CB8;   /* 0.3f  */
extern "C" f32 lbl_80796CBC;   /* 0.2f  */
extern "C" f32 lbl_80796CC0;   /* 0.18f */
extern "C" f32 lbl_80796CC4;   /* 0.15f */
extern "C" f32 lbl_80796CC8;   /* 0.1f  */
extern "C" f32 lbl_80796CCC;   /* 0.05f */
extern "C" f32 lbl_80796CD0;   /* 0.9f  */

/* The work-record accessors `src/ef/fn_800CDB2C.cpp` owns (rule 2: the owner's header does not carry
 * them yet, so they are declared here at C++ scope with the enemy-record view `enemy/fn_8012E968.cpp`
 * also uses). */
_ENEMY_WORK* get_move_work_adrs(u8 kind);
u16 get_move_work_max(u8 kind);

/* In-range helpers this first block calls (defined later in the follow-up queue), plus the forward
 * declaration of the first definition below. */
extern "C" f32 fn_8012EC74(_ENEMY_WORK* self);
extern "C" u32 em_get_rank(_ENEMY_WORK* self);
extern "C" u32 fn_80130134(_ENEMY_WORK* self, u32 flag);
extern "C" f32 fn_8013032C(_ENEMY_WORK* self);
extern "C" u32 fn_801322CC(_ENEMY_WORK* self, u32 flag);
extern "C" u32 fn_80132270(_ENEMY_WORK* self);

/* The other in-range mangled entry point this block calls (rule 9: the owner's real spelling). */
u32 em_magma_check(_ENEMY_WORK* self);

/* Other-unit / unsplit C-linkage callees. */
extern "C" s32 fn_8011E640(_ENEMY_WORK* self, u32 mask);
extern "C" u8 stage_map_kind_get(u32 map_no);
extern "C" u32 fn_803B5CA4(u32 id);
extern "C" u32 quest_entry_active_ck(void);

/* Whether the enemy's area/group state admits the record (see the file header). */
extern "C" s32 fn_8012ECF0(_ENEMY_WORK* self)
{
    f32 rate;

    rate = fn_8012EC74(self);
    return (f32)(s32)self->field_0x7A0 / (f32)(s32)self->field_0x7A4 <= rate;
}

/* The mode-selected base window: 0.3/0.2/0.18/0.15 s by the motion's sub-window count
 * `em_get_rank` returns.  The target re-reads the count in each arm, so the call is written per arm. */
extern "C" f32 fn_8012EC74(_ENEMY_WORK* self)
{
    if ((u8)em_get_rank(self) <= 1) return lbl_80796CB8;
    if ((u8)em_get_rank(self) <= 2) return lbl_80796CBC;
    if ((u8)em_get_rank(self) <= 3) return lbl_80796CC0;
    return lbl_80796CC4;
}

/* The first team-wide window scan: "(100 - kind) % of the motion has run on a live record of `team`".
 * `team`/`kind` are the two bytes the callers at 0x803B5990 mask out of their table row. */
extern "C" s32 fn_8012ED68(u8 team, u8 kind)
{
    _ENEMY_WORK* work;
    u16 max;
    s32 i;
    f32 want;

    want = (f32)(100 - kind) / lbl_80796C5C;
    work = get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++) {
        if (work->active != 0 && work->team == team) {
            if ((f32)(s32)work->field_0x7A0 / (f32)(s32)work->field_0x7A4 <= want) {
                return 1;
            }
        }
        work++;
    }
    return 0;
}

/* The second team-wide window scan: "the taken part `(field_0x7AC - field_0x7A0) / field_0x7A4` is at
 * least `kind` %". */
extern "C" s32 fn_8012EE80(u8 team, u8 kind)
{
    _ENEMY_WORK* work;
    u16 max;
    u32 i;
    f32 want;

    want = (f32)kind / lbl_80796C5C;
    work = get_move_work_adrs(3);
    max = get_move_work_max(3);
    for (i = 0; i < max; i++) {
        if (work->active != 0 && work->team == team) {
            if ((f32)(s32)(work->field_0x7AC - work->field_0x7A0) / (f32)(s32)work->field_0x7A4 >= want) {
                return 1;
            }
        }
        work++;
    }
    return 0;
}

/* "The record's team id is a live map id and the map's frame gate is the first frame". */
extern "C" s32 fn_8012EF98(_ENEMY_WORK* self)
{
    if (fn_803B5CA4(self->team) != 0) {
        if (quest_entry_active_ck() == 1) return 1;
    }
    return 0;
}

/* The area/status gate that guards the per-motion block: the `field_0x1C8` bit-0 request, the area and
 * magma tests, then the team-specific height/map tests. */
extern "C" s32 fn_8012EFDC(_ENEMY_WORK* self)
{
    if (self->field_0x1C8 & 1) {
        if (em_area_ck(self) == 0) return 0;
        if (fn_80130134(self, 1) == 1) return 0;
        if (em_magma_check(self) == 1) return 0;
        if (fn_803B5CA4(self->team) == 0x404) return 0;
    }
    switch (self->team) {
    case 0xf:
        if (self->field_0x7B0 > lbl_80796C58) return 0;
        break;
    case 0x19:
        if (stage_map_kind_get(self->field_0x1E0) == 6 && self->area_no == 2) return 1;
        return 0;
    case 0x14:
        if (stage_map_kind_get(self->field_0x1E0) == 7 && self->area_no == 3) return 1;
        return 0;
    }
    return 1;
}

/* The program-mode gate for one of the two window masks (0xa / 0xb); the "die" test is the only path
 * that keeps the record alive. */
extern "C" s32 fn_8012F110(_ENEMY_WORK* self, u16 mode)
{
    if (self->field_0x818 > 0) return 0;
    switch (mode) {
    case 0xa:
        if (fn_8011E640(self, 0x20) != 0) {
            if (self->field_0x1E2 == 0) {
                if ((u32)em_die_ck(self) != 1) break;
            }
        }
        return 0;
    case 0xb:
        if (fn_8011E640(self, 0x40) != 0) {
            if (self->field_0x1E2 == 0 || self->field_0x1E2 == 2) {
                if ((u32)em_die_ck(self) != 1) break;
            }
        }
        return 0;
    }
    return 1;
}

/* The window test with the sub-window padding `em_get_rank` selects (0.1 s or 0.05 s). */
extern "C" u32 em_motion_window_ck(_ENEMY_WORK* self)
{
    if (self->field_0x938 > 0) {
        f32 rate = fn_8012EC74(self);
        f32 limit;

        if ((u8)em_get_rank(self) <= 1) {
            limit = rate + lbl_80796CC8;
        } else {
            limit = rate + lbl_80796CCC;
        }
        return (f32)(s32)self->field_0x7A0 / (f32)(s32)self->field_0x7A4 <= limit;
    }
    return 0;
}

/* The window test behind the two landed cross-unit gates: either of the two "in this motion" tests,
 * then the padded window test. */
extern "C" s32 fn_8012F2A4(_ENEMY_WORK* self)
{
    if (fn_8012D8D0(self) == 1 || fn_8012DB3C(self) == 1) {
        if (em_motion_window_ck(self) == 1) return 1;
    }
    return 0;
}

/* The sleep gate (`u32`): kind 0 is "can sleep this frame", kind 1 the `em_sleep_ck` action lookup. */
u32 em_sleep_ck(_ENEMY_WORK* self, u8 kind)
{
    switch (kind) {
    case 0:
        if (fn_801322CC(self, 1) == 1 || fn_80132270(self) == 1) return 1;
        return 0;
    case 1:
        if (fn_8012E21C(self->action, self->state_sub) == 1 || fn_80132270(self) == 1) return 1;
        return 0;
    }
    return 0;
}

/* "The record's kind is 4 and its height is above 0.9 * the model scale `fn_8013032C` returns". */
extern "C" s32 fn_8012F39C(_ENEMY_WORK* self)
{
    if (self->field_0x1E2 == 4) {
        if (self->field_0x1AC > lbl_80796CD0 * fn_8013032C(self)) return 1;
    }
    return 0;
}
