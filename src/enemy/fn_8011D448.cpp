/* enemy/fn_8011D448.cpp - the enemy effect-spawner band, `.text` 0x8011D448..0x801251D0 (101
 * functions, 32136 B).
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>` over the whole range: the dump resolves only
 * `em_parts_damage_level_get(_ENEMY_WORK, u8)` at 0x8011E9DC - which the map already spells
 * `em_parts_damage_level_get__FP11_ENEMY_WORKUc` - and the unrelated `JASSeqCtrl::setIntrMask`
 * at 0x8011F230; every other address is the dump's `FUN_`/`zz_XXXXXXXX_` placeholder).
 *
 * Registration (docs/plan.md 12), decided by evidence class 3+4:
 *   1. No `__FILE__` string.  Every `lis`+`addi`/`@sda21` reference in the range resolves to the
 *      numeric tables `lbl_805A0C10..lbl_805A0DCC` (u16 effect-id tables and four jump tables),
 *      `lbl_807919C8` and `lbl_80796C08..0x48` (`.sdata2` float constants), plus `system_w`.  No
 *      bare source-file name is reachable.
 *   2. The dump's only real in-range name is `em_parts_damage_level_get`, i.e. a FUNCTION name, and
 *      the map already carries it - it names nothing about the file.
 *   3. What the code does: the whole range is the enemy module's effect-spawner band.  The head
 *      (0x8011D448..0x8011E530) allocates pooled `_EFT` records through `fn_800F8788` and installs
 *      `fn_8011D8C0`/`fn_8011D9B8`-style `.release`/`.dispatch` callbacks exactly like
 *      `ef/eft001.cpp`; `_ENEMY_WORK` is the spawning owner everywhere (`self->act_id`,
 *      `self->pos_0x1BC`, `get_em_scale`, `em_get_mot_no`), and the tail
 *      (0x8011E5EC..0x80124C5C) is the enemy action/state machine that calls `get_enemy_data`,
 *      `em_act_ck`, `em_area_ck`, `em_die_ck`, `em_sleep_ck`, `em_frame_check`.  The band's module
 *      is therefore `enemy` (the naming scheme of the registered neighbour above,
 *      `enemy/fn_801251D0.cpp`), not `ef`.  A `fn_XXXXXXXX` unit name is all the evidence supports,
 *      so the map stem is kept (class 4) - no name is invented.
 *   4. The seam is UNPROVEN in both directions (docs/plan.md 8.3):
 *      - left, 0x8011D448: `ef/eft029.cpp` ends there.  Checked for a continuation and rejected -
 *        eft029's data fragment is `lbl_805A06F0..jumptable_805A07AC`, this range's is
 *        `lbl_805A0C10..jumptable_805A0DB4` (disjoint, 1.4 KB apart); no class string exists in
 *        either; eft029's own `.ctors` word (0x8056F314 -> 0x8011AD04) has no successor in this
 *        range; and no `tudiscover` seam signal exists anywhere in the range (`tudiscover.py at`
 *        returns "only weak signals here" for 0x8011D448, 0x8011DA24, 0x8011E138, 0x8011F654,
 *        0x801206C8, 0x801228B8, 0x801236D0, 0x80124C5C and 0x801251D0).
 *      - right, 0x801251D0: the registered `enemy/fn_801251D0.cpp` starts there; also weak.
 *      The range is one maximal unclaimed run, so it is registered once as one unit; its extent
 *      settles as its functions match.
 *
 * Language.  C++: the range references C++-mangled callees (`get_joint_wpos_em__FP11_ENEMY_WORKUl
 * PQ34nw4r4math4VEC3`, `setVector3__FPQ34nw4r4math4VEC3fff`, `change_color_eff__FPQ34nw4r2ef6
 * EffectPQ34nw4r4math4VEC38_GXColor`, `SetRootMtx__Q34nw4r2ef6EffectFRCQ34nw4r4math5MTX34`,
 * `push_eft_effect_heap_num__FPPQ34nw4r2ef6Effectl`) and defines one of its own
 * (`em_parts_damage_level_get__FP11_ENEMY_WORKUc`).  Every definition whose map name is plain
 * (`fn_8011D448`, ...) is `extern "C"` so its emitted name stays the map's stem and objdiff can
 * pair it (playbook row 42).
 *
 * Sections.  `.text` 0x8011D448..0x801251D0; `extab` 0x8000C63C..0x8000C8AC (78 records) and
 * `extabindex` 0x80026844..0x80026BEC (78 x 12 B), the runs the bracketing registered units
 * (`ef/eft029.cpp` below, `enemy/fn_801251D0.cpp` above) leave for this one and whose function
 * addresses were read out of `orig/RMHE08/sys/main.dol` (every record's func is inside this range).
 * No `.ctors`/`.dtors` word.
 *
 * State (this round).  26 of the 101 functions are written; all 26 measure at or above the 80 % bar
 * (21 at 100.0).  In address order, with the official `recompile.py --measure` score:
 *   0x8011D448 fn_8011D448 0x64 100.0   one-shot placer: spawn through fn_8011D558, set the work's
 *                                       target vector from the type table
 *   0x8011D4AC fn_8011D4AC 0x50 100.0   same, fills the target vector through copyVec3
 *   0x8011D4FC fn_8011D4FC 0x5C 100.0   same, two id words + the table vector
 *   0x8011D558 fn_8011D558 0x138 100.0  the `0xA8`-pool creator (area guard, work fill, callbacks)
 *   0x8011D690 fn_8011D690 0x120 92.85  the `0x20`-pool creator owned by an `_ENEMY_WORK`
 *   0x8011D7B0 fn_8011D7B0 0x110 94.10  the `0x20`-pool creator owned by nobody
 *   0x8011D8C0 fn_8011D8C0 0x30 100.0   pool-release dispatcher
 *   0x8011D8F0 fn_8011D8F0 0x8C 89.86   release: every slot of every 0x44-stride group
 *   0x8011D97C fn_8011D97C 0x3C 100.0   release: the flat slot run
 *   0x8011D9B8 fn_8011D9B8 0x6C 84.52   per-frame dispatcher (state_0x05, then the type table)
 *   0x8011DE2C fn_8011DE2C 0x30 100.0   second family's release dispatcher
 *   0x8011E378 fn_8011E378 0x30 100.0   third family's per-frame dispatcher
 *   0x8011E51C fn_8011E51C 0x10 100.0   `state_0x05++` tail
 *   0x8011E52C fn_8011E52C 0x04 100.0   release tail -> fn_800F886C
 *   0x8011E530 fn_8011E530 0xBC 100.0   place one pooled effect at the enemy's joint transform
 *   0x8011E5EC fn_8011E5EC 0x34 100.0   seed `bits_0x824` from the data record
 *   0x8011E620 fn_8011E620 0x10 100.0   set status bits
 *   0x8011E630 fn_8011E630 0x10 100.0   clear status bits
 *   0x8011E640 fn_8011E640 0x18 100.0   test status bits
 *   0x8011E658 fn_8011E658 0x8C 97.14   step + clamp the gauge
 *   0x8011E6E4 fn_8011E6E4 0x08 100.0   step the gauge by the data record's ratio
 *   0x8011E6EC fn_8011E6EC 0x74 100.0   step the gauge every `period` system ticks
 *   0x8011E760 fn_8011E760 0x78 100.0   force the gauge to a value, clamped
 *   0x8011E7D8 fn_8011E7D8 0x1C 100.0   accumulate one part's damage level
 *   0x8011E9DC em_parts_damage_level_get 0x14 100.0  one part's damage level (the map's one real name)
 *   0x8011F230 fn_8011F230 0x10 100.0   OR a mask into `mask_0x80E`
 * Measured (official `report generate` metric): matched bytes 2288.4 of 32136 = 7.12 %.
 *
 * Residuals (every one recorded; none is a `psq_l`/`psq_st` pair and none is a hoisted table base, so
 * none is a stopping-rule case - all five are block-order or register-colouring differences):
 *   fn_8011E658 97.14 - retail loads `amount_0x7A0` a second time for the `+=` (35 instructions to
 *     our 35, ours 136 B to retail's 140 B); every other instruction is identical.  The `if`-clamp
 *     into the ceiling parameter is what permits the CSE here, and every spelling tried that keeps
 *     the clamp reloads once instead.  Recorded, not chased.
 *   fn_8011D7B0 94.10 - same size (272 B) both sides; the difference is the `f32` argument's
 *     register: retail keeps `scale` in f31 and ours reloads it at one store.
 *   fn_8011D690 92.85 - ours is 292 B against retail's 288 B: one extra `fmr`/`stfs` pair, retail
 *     reuses f31 for the scaled store.
 *   fn_8011D8F0 89.86 - the outer loop's two induction pointers: retail hoists `&work[i].slots`
 *     (its r31 = work + 8) out of the loop and advances both by 0x44, MWCC here keeps one pointer
 *     and re-forms `+8` inside the body.  Same 35 instructions, ours 132 B against 140 B.
 *   fn_8011D9B8 84.52 - block ORDER only: retail lays the state-0 body after the four stage tail
 *     calls, MWCC places the inner switch first.  Same instruction set, ours 92 B against 108 B
 *     (the size delta is the padding between the blocks).
 *
 * Shape worth keeping.  Two per-unit levers are needed and both are measured:
 *   - `#pragma peephole off` (below).  With the pass on, `clrlwi r0,r29,24` + `slwi` is fused into
 *     `clrlslwi` and `fn_8011E530` scores 96.60 instead of 100.0, and the `clrlwi` before the
 *     `stb`/`stw` narrowing in `fn_8011D558` disappears (94.23).  The same lever `ef/eft029.cpp`
 *     needs (playbook row 39).
 *   - the argument WIDTHS.  The creators take `u32`, not `u8`, type/timer arguments: retail emits
 *     `clrlwi ...,24` at the store (and `slwi`, never `clrlslwi`, for the type index), which a `u8`
 *     parameter does not produce.  `fn_8011D558`'s last two parameters are `(f32 scale, u8 timer)`
 *     in that order - the prologue's `fmr f31,f1` before `mr r29,r7` is what settles it.
 *   - `event_demo_ck` and `get_em_scale` are declared at C++ scope (their objects reference
 *     `event_demo_ck__Fv` / `get_em_scale__FP11_ENEMY_WORK`); a declaration inside the `extern "C"`
 *     block below would emit the unmangled name and miss the relocation.
 *   - the family skeleton every creator shares: `get_now_areano() == act_id` guard, `fn_800F8788(size)`,
 *     `field_0x03 = 0x1f`, `type_0x02 = type`, `release_0x40`, `dispatch_0x34`,
 *     `event_demo_ck() == 1 -> field_0x07 = 1` - the `ef/eft001.cpp` family shape.  A creator whose
 *     first argument is a bare type (`fn_8011D7B0`) is NOT an `_ENEMY_WORK` owner: it stores
 *     `source_0x30 = NULL` and takes the type, the position, the rotation and the area from its own
 *     arguments.
 *
 * The remaining 75 functions are unwritten and keep their original bytes in the target object; the
 * follow-up queue, in address order, is:
 *   fn_8011DA24 0x150, fn_8011DB74 0x2B8, fn_8011DE5C 0x2DC, fn_8011E138 0x240, fn_8011E3A8 0x174,
 *   fn_8011E898 0x1C, fn_8011E8B4 0xAC, fn_8011E960 0x7C, fn_8011E9F0 0x14, fn_8011EA04 0x8C,
 *   fn_8011EA90 0x2A8, fn_8011ED38 0x14C, fn_8011EE84 0xA0, fn_8011EF24 0x98, fn_8011EFBC 0x190,
 *   fn_8011F14C 0x20, fn_8011F16C 0xC4, fn_8011F240 0x1E0, fn_8011F420 0xB0, fn_8011F4D0 0x50,
 *   fn_8011F520 0x50, fn_8011F570 0x50, fn_8011F5C0 0x94, fn_8011F654 0xB44, fn_80120198 0x2FC,
 *   fn_80120494 0x18, fn_801204AC 0x8, fn_801204B4 0x3C, fn_801204F0 0x40, fn_80120530 0x198,
 *   fn_801206C8 0x4BC, fn_80120B84 0x27C, fn_80120E00 0x3D4, fn_801211D4 0x3D4, fn_801215A8 0x3D4,
 *   fn_8012197C 0xDC, fn_80121A58 0xFC, fn_80121B54 0x40, fn_80121B94 0x1BC, fn_80121D50 0x1BC,
 *   fn_80121F0C 0x98, fn_80121FA4 0x204, fn_801221A8 0x60, fn_80122208 0x170, fn_80122378 0xF8,
 *   fn_80122470 0xF8, fn_80122568 0x44, fn_801225AC 0x6C, fn_80122618 0x184, fn_8012279C 0x9C,
 *   fn_80122838 0x80, fn_801228B8 0x3AC, fn_80122C64 0x48, fn_80122CAC 0xFC, fn_80122DA8 0x2C,
 *   fn_80122DD4 0xB8, fn_80122E8C 0x2C, fn_80122EB8 0x60, fn_80122F18 0xAC, fn_80122FC4 0xFC,
 *   fn_801230C0 0x2C, fn_801230EC 0xA4, fn_80123190 0x24, fn_801231B4 0xE0, fn_80123294 0x134,
 *   fn_801233C8 0x178, fn_80123540 0x44, fn_80123584 0x14C, fn_801236D0 0xA80, fn_80124150 0x450,
 *   fn_801245A0 0x344, fn_801248E4 0xA8, fn_8012498C 0x2D0, fn_80124C5C 0x574.
 * The five the written dispatchers tail-call are already declared below (fn_8011DA24, fn_8011DB74,
 * fn_8011DE5C, fn_8011E138, fn_8011E3A8).  Five patterns to carry into the rest of the queue, each
 * measured here: the dispatchers are a `switch` on `state_0x05` whose first case falls THROUGH to a
 * second `switch` on `lbl_805A0CB8[type_0x02]` (fn_8011D9B8); the release callbacks are the same
 * `switch` with two tail calls; the gauge helpers are `fn_8011E658`/`fn_8011E760` plus their
 * callers; the int->float scale conversions read `field_0x7a4` as SIGNED (`xoris` + `0x4330`); and
 * the per-part table is `parts_0x838[part]`, stride 6.
 *
 * Types.  `_EFT` (0x48, `include/ef.h`) is the pooled effect record; `_EFT::work_0x38` is the
 * per-family work block, so this unit carries the three work views its functions reach:
 * `_EFT_FX_GROUP` (0x44-byte groups), `_EFT_FX_POOL` (flat run) and `_EFT_FX_FULL` (the `0xA8` pool:
 * state, scale, the two id words and the target vector).  `_ENEMY_WORK` (0xB1C, `include/enemy.h`) is
 * the spawning enemy.  Only the offsets the written bodies read are named; every size is stated.
 */

#include "types.h"

#include "nw4r/math.h"
#include "ef.h"
#include "enemy.h"

#include "ef/effect.h"
#include "ef/eft001.h"
#include "ef/eft_res.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012BDF4.h"
#include "fn_8004CAD8.h"
#include "g3d/g3d_calcworld.h"
#include "unsplit/unknown.h"
#include "unsplit/enemy.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* Retail keeps the unfused `clrlwi`+`slwi` and `clrlwi`+`stb`/`stw` pairs the `-O3` peephole pass
 * folds into `clrlslwi` - `fn_8011E530`'s part index and `fn_8011D558`'s type/timer narrowing both show
 * it.  The pass off is the same per-unit lever `ef/eft029.cpp` needs (playbook row 39). */
#pragma peephole off

/* The range's own `.data`/`.sdata` pool (unsplit: no registered unit owns the fragments, so the
 * names are declared here and never defined - playbook 23/29). */
extern "C" {
u16 lbl_805A0C10[]; /* type -> effect id block, 0x8011DE5C's default arm */
u16 lbl_805A0C20[];
u16 lbl_805A0C30[];
u16 lbl_805A0C40[];
u16 lbl_805A0C50[];
u8 lbl_805A0C60[];  /* type -> work state word (the 0x20-pool creators) */
f32 lbl_805A0C88[]; /* type -> target vector component */
u8 lbl_805A0CB8[];  /* type -> family tag (0 = pool, 1 = flat run) */
u8 lbl_805A0CC8[];
u8 lbl_807919C8[];  /* family tag -> re-arm delay */
f32 lbl_80796C08;   /* 0.0f */
}

/* The unwritten members of this unit that the written dispatchers tail-call, and the two callees
 * whose owner unit is unsplit (rule 2's named gap: no registered range brackets them, so no module
 * header can own the declaration). */
extern "C" {
void fn_8011DA24(_EFT* self);
void fn_8011DB74(_EFT* self);
void fn_8011DE2C(_EFT* self);
void fn_8011DE5C(_EFT* self);
void fn_8011E138(_EFT* self);
void fn_8011E378(_EFT* self);
void fn_8011E3A8(_EFT* self);
void fn_8011E51C(_EFT* self);
void fn_8011E52C(_EFT* self);
void fn_8011E658(_ENEMY_WORK* self, s32 step, s32 limit);
void fn_8011D8F0(_EFT* self);
void fn_8011D97C(_EFT* self);
void fn_8011D8C0(_EFT* self);
void fn_8011D9B8(_EFT* self);
_EFT* fn_8011D558(_ENEMY_WORK* self, u32 kind, u32 id0, u32 id1, f32 scale, u8 timer);
s32 em_get_mot_no(_ENEMY_WORK* enemy);
void get_camera_pos(nw4r::math::VEC3* out);


/* `src/mh3_pad.cpp` owns the three 3-float helpers; `include/mh3_pad.h` and `include/ef.h` now
 * spell them with the same record type (`nw4r::math::VEC3*`, docs/plan.md 6.5 rule 11), so both
 * headers can be included here and neither needs a local copy of the declaration. */
}

/* `get_em_scale` (0x80135940) is in the unsplit enemy band, so its home is `include/unsplit/enemy.h` -
 * which is read-only for a worker and carries only the mangled spelling
 * (`get_em_scale__FP11_ENEMY_WORK`), which rule 9 forbids calling.  Declared at C++ scope with the
 * plain spelling so it mangles back to the map name (`ef/eft009.cpp` and `ef/fn_80105314.cpp` do the
 * same); moving it into that header is the outbox's `shared-file` request. */
f32 get_em_scale(_ENEMY_WORK* enemy);

/* `event_demo_ck` (0x803C4814, in the still-unclaimed 0x803Cxxxx band) is C++ in the target: the
 * objects reference `event_demo_ck__Fv`, so the plain spelling is declared at C++ scope, where MWCC
 * mangles it to exactly that name.  Its result is compared UNSIGNED (`cmplwi r3, 0x1` in the
 * target), so it returns `u32`. */
u32 event_demo_ck();

/* ---------------------------------------------------------------------------------------------------
 * the effect record's per-family work blocks
 * ------------------------------------------------------------------------------------------------- */

/* One 0x44-byte group of the pooled family: entry 0's `entry_count` is the number of groups, every
 * entry's `slot_count` the number of live slots that follow it. size: 0x44 */
struct _EFT_FX_GROUP {
    /* +0x00 */ s32 entry_count;
    /* +0x04 */ s32 slot_count;
    /* +0x08 */ nw4r::ef::Effect* slots[15];
};

/* The flat family: one count, then that many slots. size: 0x04 + 4n */
struct _EFT_FX_POOL {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* slots[1];
};

/* The `0xA8` pool's work block.  `state_0x00` is the family tag the creator writes, `ids_0x90` the
 * two id words the same creator passes and `fn_8011E530` indexes by the spawner's part number.
 * size: 0xA4 (lower bound: +0x98 is the highest offset any function of this round reads) */
struct _EFT_FX_FULL {
    /* +0x00 */ u32 state_0x00;
    /* +0x04 */ u8 pad_0x04[0x88];
    /* +0x8C */ f32 scale_0x8C;
    /* +0x90 */ u32 ids_0x90[2];
    /* +0x98 */ nw4r::math::VEC3 target_0x98;
};

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

/* 0x8011D448 - spawn one pooled effect through fn_8011D558 and point its target vector along the
 * type table's z.  The z component comes from `lbl_805A0C88[type]`; x and y stay at 0.0f. */
extern "C" void fn_8011D448(_ENEMY_WORK* self, u32 type, u32 id, u32 timer, f32 scale)
{
    _EFT* effect = fn_8011D558(self, type, id, id, scale, timer);
    _EFT_FX_FULL* work;

    if (effect != NULL) {
        work = (_EFT_FX_FULL*)effect->work_0x38;
        setVector3(&work->target_0x98, lbl_80796C08, lbl_80796C08, lbl_805A0C88[type]);
    }
}

/* 0x8011D4AC - the same spawn, with the target vector copied from the caller's record instead of the
 * table. */
extern "C" void fn_8011D4AC(_ENEMY_WORK* self, u32 type, u32 id, u32 timer, const nw4r::math::VEC3* pos,
                            f32 scale)
{
    _EFT* effect = fn_8011D558(self, type, id, id, scale, timer);
    _EFT_FX_FULL* work;

    if (effect != NULL) {
        work = (_EFT_FX_FULL*)effect->work_0x38;
        copyVec3(&work->target_0x98, pos);
    }
}

/* 0x8011D4FC - the same spawn, target vector from the type table only.  Distinct from 0x8011D448
 * because the id pair is taken from the caller's two words. */
extern "C" void fn_8011D4FC(_ENEMY_WORK* self, u32 type, u32 id0, u32 id1, u32 timer, f32 scale)
{
    _EFT* effect = fn_8011D558(self, type, id0, id1, scale, timer);
    _EFT_FX_FULL* work;

    if (effect != NULL) {
        work = (_EFT_FX_FULL*)effect->work_0x38;
        setVector3(&work->target_0x98, lbl_80796C08, lbl_80796C08, lbl_805A0C88[type]);
    }
}

/* 0x8011D558 - the `0xA8`-pool creator.  Refuses to spawn outside the enemy's own area, fills the
 * work block (family tag, scale multiplied by the enemy's scale, the two id words), stamps the
 * record (`field_0x03` = 0x1f, type, timer, owner, area, the enemy's rotation) and installs the
 * family's release/dispatch pair.  `event_demo_ck() == 1` marks the record as demo-owned. */
extern "C" _EFT* fn_8011D558(_ENEMY_WORK* self, u32 type, u32 id0, u32 id1, f32 scale, u8 timer)
{
    _EFT* effect;
    _EFT_FX_FULL* work;

    if (get_now_areano() != self->act_id) {
        return NULL;
    }

    effect = (_EFT*)fn_800F8788(0xA8);
    if (effect == NULL) {
        return NULL;
    }

    work = (_EFT_FX_FULL*)effect->work_0x38;
    work->state_0x00 = 2;
    work->scale_0x8C = scale * get_em_scale(self);
    work->ids_0x90[0] = id0;
    work->ids_0x90[1] = id1;

    effect->field_0x03 = 0x1f;
    effect->type_0x02 = type;
    effect->timer_0x0C = timer;
    effect->source_0x30 = self;
    effect->area_0x44 = self->act_id;
    effect->rot_0x24 = self->pos_0x1BC;
    fn_800F9DF4(effect, 0, 0);
    effect->demo_flag_0x08 = self->team;
    effect->release_0x40 = fn_8011D8C0;
    effect->dispatch_0x34 = fn_8011D9B8;
    if (event_demo_ck() == 1) {
        effect->field_0x07 = 1;
    }
    return effect;
}

/* 0x8011D690 - the `0x20`-pool creator for the same family: the enemy's own anchor, its pose
 * rotation and the table's work state word. */
extern "C" void fn_8011D690(_ENEMY_WORK* self, u32 type, u32 id, f32 scale)
{
    _EFT* effect;
    _EFT_FX_FULL* work;

    if (get_now_areano() != self->act_id) {
        return;
    }

    effect = (_EFT*)fn_800F8788(0x20);
    if (effect == NULL) {
        return;
    }

    work = (_EFT_FX_FULL*)effect->work_0x38;
    work->state_0x00 = lbl_805A0C60[type];
    work->scale_0x8C = scale * get_em_scale(self);
    work->ids_0x90[0] = id;
    work->ids_0x90[1] = 0;

    effect->field_0x03 = 0x1f;
    effect->type_0x02 = type;
    effect->timer_0x0C = 0;
    effect->source_0x30 = self;
    effect->area_0x44 = self->act_id;
    effect->rot_0x24 = self->pos_0x1BC;
    fn_800F9DF4(effect, 0, 0);
    effect->demo_flag_0x08 = self->team;
    if (event_demo_ck() == 1) {
        effect->field_0x07 = 1;
    }
    effect->release_0x40 = fn_8011D8C0;
    effect->dispatch_0x34 = fn_8011D9B8;
}

/* 0x8011D7B0 - the `0x20`-pool creator that belongs to nobody: the caller hands in the type, the
 * position, the rotation and the area directly, so `source_0x30` stays NULL and the area guard is
 * the caller's word, not the enemy's. */
extern "C" void fn_8011D7B0(u32 type, const nw4r::math::VEC3* pos, const _CP_VECTOR* rot, u8 area,
                            u8 demo, f32 scale)
{
    _EFT* effect;
    _EFT_FX_FULL* work;

    if (get_now_areano() != area) {
        return;
    }

    effect = (_EFT*)fn_800F8788(0x20);
    if (effect == NULL) {
        return;
    }

    work = (_EFT_FX_FULL*)effect->work_0x38;
    work->state_0x00 = lbl_805A0C60[type];
    work->scale_0x8C = scale;

    effect->field_0x03 = 0x1f;
    effect->type_0x02 = type;
    effect->source_0x30 = NULL;
    effect->area_0x44 = area;
    effect->rot_0x24 = *rot;
    copyVec3(&effect->pos_0x18, pos);
    effect->timer_0x0C = 0;
    fn_800F9DF4(effect, 0, 0);
    effect->demo_flag_0x08 = demo;
    if (event_demo_ck() == 1) {
        effect->field_0x07 = 1;
    }
    effect->release_0x40 = fn_8011D8C0;
    effect->dispatch_0x34 = fn_8011D9B8;
}

/* 0x8011D8C0 - the pooled family's `.release` callback: hand the type's slot run back. */
extern "C" void fn_8011D8C0(_EFT* self)
{
    switch (lbl_805A0CB8[self->type_0x02]) {
    case 0:
        fn_8011D8F0(self);
        break;
    case 1:
        fn_8011D97C(self);
        break;
    }
}

/* 0x8011D8F0 - the grouped release: every slot of every 0x44-stride group, one call each, then the
 * group count is cleared. */
extern "C" void fn_8011D8F0(_EFT* self)
{
    _EFT_FX_GROUP* work = (_EFT_FX_GROUP*)self->work_0x38;
    s32 i;
    s32 j;

    for (i = 0; i < work[0].entry_count; i++) {
        for (j = 0; j < work[i].slot_count; j++) {
            push_eft_effect_heap_num(&work[i].slots[j], 1);
        }
    }
    work[0].entry_count = 0;
}

/* 0x8011D97C - the flat release: the whole run in one call, then the count is cleared. */
extern "C" void fn_8011D97C(_EFT* self)
{
    _EFT_FX_POOL* work = (_EFT_FX_POOL*)self->work_0x38;

    push_eft_effect_heap_num(work->slots, work->count);
    work->count = 0;
}

/* 0x8011D9B8 - the family's `.dispatch` callback: `state_0x05` selects the stage; stage 0 falls out of
 * the switch and posts on to the type table's first/second handler (retail lays that second switch
 * after the first one's exit, with stage 0 branching over the other stages' tail calls). */
extern "C" void fn_8011D9B8(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        break;
    case 1:
        fn_8011DE2C(self);
        return;
    case 2:
        fn_8011E378(self);
        return;
    case 3:
        fn_8011E52C(self);
        return;
    }
    switch (lbl_805A0CB8[self->type_0x02]) {
    case 0:
        fn_8011DA24(self);
        break;
    case 1:
        fn_8011DB74(self);
        break;
    }
}

/* 0x8011DE2C - the second family's release dispatcher. */
extern "C" void fn_8011DE2C(_EFT* self)
{
    switch (lbl_805A0CB8[self->type_0x02]) {
    case 0:
        fn_8011DE5C(self);
        break;
    case 1:
        fn_8011E138(self);
        break;
    }
}

/* 0x8011E378 - the third family's per-frame dispatcher. */
extern "C" void fn_8011E378(_EFT* self)
{
    switch (lbl_805A0CB8[self->type_0x02]) {
    case 0:
        fn_8011E3A8(self);
        break;
    case 1:
        fn_8011E51C(self);
        break;
    }
}

/* 0x8011E51C - the terminal stage of the shared state machine. */
extern "C" void fn_8011E51C(_EFT* self)
{
    self->state_0x05++;
}

/* 0x8011E52C - the terminal release: hand the whole pooled record back. */
extern "C" void fn_8011E52C(_EFT* self)
{
    fn_800F886C(self);
}

/* 0x8011E530 - place `effect` at the enemy's `part` joint: build the rotation matrix from the
 * record's own rotation, carry the work's target vector through it, add the joint's world position
 * and push the result into the pooled effect. */
extern "C" void fn_8011E530(_EFT* self, nw4r::ef::Effect* effect, u8 part)
{
    nw4r::math::VEC3 joint;
    nw4r::math::VEC3 target;
    nw4r::math::MTX34 mtx;
    _ENEMY_WORK* owner;
    _EFT_FX_FULL* work;

    VEC3_ctor(&joint);
    VEC3_ctor(&target);
    MTX34_ctor(&mtx);

    owner = (_ENEMY_WORK*)self->source_0x30;
    work = (_EFT_FX_FULL*)self->work_0x38;

    cpSetRotMatrix(&self->rot_0x24, &mtx);
    copyVec3(&target, &work->target_0x98);
    mulVecMat(&target, &mtx);
    get_joint_wpos_em(owner, work->ids_0x90[part], &joint);
    fn_80073F68(&joint, &target);
    fn_800FBB90(&mtx, &joint);
    effect->SetRootMtx(mtx);
}

/* 0x8011E5EC - seed the enemy's status bits from its static data record. */
extern "C" void fn_8011E5EC(_ENEMY_WORK* self)
{
    self->bits_0x824 = get_enemy_data(self)->bits_0x30;
}

/* 0x8011E620 - set status bits. */
extern "C" void fn_8011E620(_ENEMY_WORK* self, u32 mask)
{
    self->bits_0x824 |= mask;
}

/* 0x8011E630 - clear status bits. */
extern "C" void fn_8011E630(_ENEMY_WORK* self, u32 mask)
{
    self->bits_0x824 &= ~mask;
}

/* 0x8011E640 - are all of `mask`'s bits set?  (retail emits the `cntlzw` of
 * `(bits & mask) - mask`, which is this comparison and nothing else.) */
extern "C" u32 fn_8011E640(_ENEMY_WORK* self, u32 mask)
{
    return (self->bits_0x824 & mask) == mask;
}

/* 0x8011E658 - step the gauge and clamp it: never below 0, never above the caller's ceiling, and
 * never below the data record's own per-model ratio (`field_0x7a4 * field_0x7B0`).  The two reads of
 * `amount_0x7A0` before the step (one for the ceiling, one for the add) are retail's. */
extern "C" void fn_8011E658(_ENEMY_WORK* self, s32 step, s32 ceiling)
{
    s32 value;
    s32 low;

    if (self->amount_0x7A0 > ceiling) {
        ceiling = self->amount_0x7A0;
    }
    value = self->amount_0x7A0 + step;
    self->amount_0x7A0 = value;
    if (value < 0) {
        self->amount_0x7A0 = 0;
    } else if (value > ceiling) {
        self->amount_0x7A0 = ceiling;
    }
    low = (s32)((f32)(s32)self->field_0x7a4 * self->field_0x7B0);
    if (self->amount_0x7A0 < low) {
        self->amount_0x7A0 = low;
    }
}

/* 0x8011E6E4 - step the gauge by `step` with the data record's ratio as the ceiling. */
extern "C" void fn_8011E6E4(_ENEMY_WORK* self, s32 step)
{
    fn_8011E658(self, step, self->field_0x7a4);
}

/* 0x8011E6EC - step the gauge on every `period`-th system tick.  The modulo is UNSIGNED
 * (`divwu` on the system counter), which is what a `u32` counter against a sign-extended `s16`
 * produces. */
extern "C" void fn_8011E6EC(_ENEMY_WORK* self, s32 step, s16 period, f32 scale)
{
    if (system_w.field_0x0c % period == 0) {
        fn_8011E658(self, step, (s32)((f32)(s32)self->field_0x7a4 * scale));
    }
}

/* 0x8011E760 - force the gauge to `value`, clamped the same way `fn_8011E658` clamps it. */
extern "C" void fn_8011E760(_ENEMY_WORK* self, s32 value)
{
    s32 ceiling;
    s32 low;

    if (value < 0) {
        self->amount_0x7A0 = 0;
        return;
    }
    ceiling = (s32)self->field_0x7a4;
    if (value > ceiling) {
        self->amount_0x7A0 = ceiling;
        return;
    }
    low = (s32)((f32)ceiling * self->field_0x7B0);
    if (value < low) {
        self->amount_0x7A0 = low;
    } else {
        self->amount_0x7A0 = value;
    }
}

/* 0x8011E7D8 - accumulate one part's damage level. */
extern "C" void fn_8011E7D8(_ENEMY_WORK* self, u8 part, u8 add)
{
    self->parts_0x838[part].damage_level += add;
}

/* 0x8011E9DC - one part's damage level.  Defined at C++ scope: the map's name is the mangling
 * `em_parts_damage_level_get__FP11_ENEMY_WORKUc`, which is what `(_ENEMY_WORK*, u8)` mangles to. */
u8 em_parts_damage_level_get(_ENEMY_WORK* self, u8 part)
{
    return self->parts_0x838[part].damage_level;
}

/* 0x8011F230 - OR a mask into the record's own u16 mask. */
extern "C" void fn_8011F230(_ENEMY_WORK* self, u16 mask)
{
    self->mask_0x80E |= mask;
}
