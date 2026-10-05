/*
 * ef/eft052.cpp - unit, `.text` 0x80358624..0x8035BAB4 (53 functions, 13456 bytes).
 *
 * 20 of 53 functions have a body here.
 *
 * FLAGS.  `cflags_main`.  The tail (0x8035BAB4..) is `enemy/em033_prog.cpp` (no bodies came from this source).
 *
 * Sections: the unit's block in config/RMHE08/splits.txt (.bss, .data, .sbss, .sdata, .sdata2, .text, extab,
 * extabindex).
 */
/* ---- header inherited from src/ef/eft052.cpp (written against its pre-phase-4 range) ---- */
/* ef/eft052.cpp - the `eft052` effect family (the `_EFT` tag 52) and the cockpit item-page band it
 * draws, `.text` 0x80358624..0x8035E034 (92 functions / 23056 B).
 *
 * WHAT IT IS.  An effect family plus the cockpit hold/item band it draws from, exactly the shape the
 * registered neighbour `ef/eft050.cpp` documents: `eft052_set` pools the effect record
 * (`eft_res_slot_get(76)`, the 76-byte block `ef/eft035.cpp` measured) and installs its
 * `dispatch_0x34`/`release_0x40` hooks, `eft052_dispatch` is the `state_0x05` machine whose arms are
 * the model-create pass (`res_eft_model_create_light`), the placement/alive body and the pool release
 * (`eft_res_slot_release`), and the rest of the range is the cockpit item layer: the page counters
 * (`eft052_page_counts_get`, `eft052_page_take`, `eft052_page_put`), the hold block at
 * `.bss:0x806BF368`, the item/monster strings (`LbStr`, `ItemName`, `GetItemData`) and the sprite
 * sheet (`draw_sprite_*`, `get_lsp_data`, `PutPageArrow`, `font_*`).  Three bodies read the enemy work
 * record (`em_parts_damage_level_get`, `fn_80126278`, `calcDistanceSqXZ`), which is what the effect
 * family reports on.
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string is reachable: every
 * `lbl_` reference in the 92 split objects resolves to the `.sdata2` float pool 0x8079B640..0x8079B704,
 * to the `.data` run 0x805ED0C0..0x805ED938 or to a call - never to a source-file-name literal.
 * 2. `dumpmap.py lookup` answers only `zz_` placeholders (two stubs carry Ghidra's library-signature
 * match, `DBClose` at 0x8035BC24 and `J3DColorBlockLightOff::getColorChanNum` at 0x8035DD40, which is
 * signature noise on a 4- and an 8-byte stub, not a name).  3. Class 3, the module: `ef` - the range
 * is the `eft` family shape above (the `+0x34` dispatch / `+0x40` release hook pair, the `state_0x05`
 * machine, `_EFT::area_0x44`, the pooled-model handle) with the cockpit band's callee profile, and its
 * callers are the `ef` and `lobby` bands (`ef/ef_emitter.cpp` calls `fn_8035B998`, `ef/eft050.cpp`
 * calls `fn_8035A7D8`, `lobby/fn_801E7530.cpp`/`lobby/fn_801EC9F8.cpp` call
 * `eft052_hold_entry_set`/`eft052_item_value_get`, `lobby/fn_801E0ADC.cpp` calls
 * `eft052_page_count_add`/`eft052_hold_cursor_step`/`eft052_hold_row_get`).  4. The family's own tag
 * named it: `eft052_set` seeds `_EFT::field_0x03 = 52` (in the target object, byte-identical to ours:
 * `li r0,52` / `stb r0,3(r30)`), and that tag is the file name of every registered sibling whose name
 * the dump knows - `ef/eft001.cpp` seeds 1, `eft002` 2, `eft009` 9, `eft019` 19 ("the eft019
 * family tag"), `eft035` 35 and `eft050` 50 (0x32).  The file name `eft052` is therefore DERIVED from
 * the unit's own tag and stays a GUESS (no dump name exists for the family - the runtime dump has only
 * `zz_XXXXXXXX_` for 0x80358624 and no `__FILE__` string is reachable), so a later pass may refine it.
 *
 * NAMES.  Every symbol this file DEFINES is named from its own body - the family tag `eft052` plus
 * what the body does - and each is a guess a later pass may refine (the evidence is the body, not the
 * map); the 72 entries this file does not reconstruct yet keep the map's `fn_` stems, because they are
 * absent or referenced, never defined:
 *   * the family: `eft052_set` (pool the record with `eft_res_slot_get(76)`, install the two hooks, seed
 *     both part slots), `eft052_release` (the `+0x40` hook - hand every part slot's handle back),
 *     `eft052_dispatch` (the `+0x34` hook - one handler per `state_0x05`), `eft052_place` (the
 *     placement pass - pick the per-area table, create both models, advance), `eft052_state_step`
 *     (advance the state) and `eft052_pool_release` (`eft_res_slot_release`);
 *   * the enemy part report the family draws from: `eft052_part_damage_ck` (one flag per queried part
 *     index), `eft052_part_level_even_ck` (whether the part's damage level is even) and
 *     `eft052_part_gauge_add` (step the part gauge and clamp it to 0..500);
 *   * the item page (`lobby_world_block`): `eft052_item_value_get`/`eft052_item_half_get` (the item
 *     record's +0x010 value / its +0x00C value halved, floored at 1), `eft052_page_counts_get` (the
 *     three counts of one id, through optional out pointers), `eft052_page_count_add` (move one id's
 *     count by `delta`), `eft052_page_take`/`eft052_page_put` (the two moves between the page and the
 *     caller's hand) and `eft052_page_count_ck` (one of the page's two counts for an id - the cabinet
 *     or the hand);
 *   * the hold block (`.bss:0x806BF368`): `eft052_hold_row_set` (re-point the block at a row - the one
 *     owned symbol whose body is still unwritten, so its declaration below stays a reference),
 *     `eft052_hold_cursor_step` (re-point at the clamped cursor row and clear the dirty byte),
 *     `eft052_hold_row_get` (the cursor row's table value and the block's +0x0A word),
 *     `eft052_hold_entry_set` (hand the caller's entry to the block and re-seed it) and
 *     `eft052_hold_entry_copy` (copy one entry field by field).
 *
 * Naming note: every remaining `fn_XXXXXXXX` in this file is a REFERENCE to a symbol ANOTHER unit
 * owns (the rule's tolerated half, checked with `python tools/symbols/symedit.py refs` on each of the
 * file's names): the effect pool/manager (`eft_res_slot_get`, `eft_res_model_get`, `eft_res_slot_release`, `eft_state_flags_set`,
 * `fn_800F8A44`), the enemy band (`fn_80126278`, `em_parts_damage_level_get`) and the lobby item API
 * (`fn_8004Axxx`/`fn_8004Bxxx`).  `symedit.py range` shows the map spells none of those ranges with
 * anything but their `fn_` stems, so this file cannot name them.  No symbol this unit owns stays
 * generated.
 *
 * SEAM.  The brief's range is one `attribute.py` `--max-bytes` cut of the unclaimed
 * 0x8034C1D0..0x8035E034 run and no `__FILE__` string exists anywhere in it, so the decisive class-1
 * test cannot fire.  Both edges are recorded as candidate re-draws, not as settled: the `.sdata2`
 * pool run continues in referrer order across the lower edge (0x8079B638 -> 0x8079B640) and
 * `em033_prog_tbl` (0x805ED370) sits in the previous proposal's own `.data` span while listing eight
 * of this range's functions; at the upper edge the vtable-like table `lbl_805ED808` (inside this
 * range's `.data` span) mixes this range's functions with `fn_8035F004`/`fn_8035EF50`/`fn_8035EF58`
 * of the registered `enemy/fn_8035E034.cpp`.  Neither edge can be proven false in-session.
 *
 * LANGUAGE AND SECTIONS.  C++ (the map's mangled callees `GetItemData__FUs`, `LbStr__FUcUs`,
 * `res_eft_model_create_light__FP6MHcharUsUll`, `calcDistanceSqXZ__FP...`, and the class whose
 * constructor `fn_8035BBE8` installs `lbl_805ED808`).  Every plain `fn_` definition is `extern "C"` so
 * it keeps the map's name (playbook 42).  dtk appended the unit's `extab` 0x8001736C..0x80017574 and
 * `extabindex` 0x80036B88..0x80036E94 lines to the splits block itself, from the gaps the bracketing
 * registrations leave; they are not yet reproduced byte for byte.  The `.data` run
 * 0x805ED0C0..0x805ED938, the `.sdata2` pool and the tables are declared, never defined (playbook 29).
 *
 * FLAGS: the `ef` lib's `cflags_main` (Wii/1.3, `-inline noauto`) plus `#pragma peephole off` for this
 * file - the file keeps the unfused forms retail has, exactly like the `menu` and `hud` libs
 * (`cflags_menu`/`cflags_hud` are `cflags_main` + `-opt nopeephole`).  Measured: with the peephole on,
 * `eft052_part_damage_ck` reads 93.82 % and `eft052_part_level_even_ck` 91.92 % (the fused
 * `clrlslwi`/`clrlwi.` forms where retail keeps `clrlwi`+`slwi`/`clrlwi`+`cmpwi`); with it off, both
 * are byte-identical.  A lib-level `-opt nopeephole` needs two agreeing units (policy 8.2) - this is
 * the first, and it is scoped to the file until a second unit of the band is written.
 *
 * STATUS / RESIDUALS (measured 2026-09-27, official report metric; 23056 B total).  The rename to
 * `eft052` moved no row (all 92 rows, and every other unit's, are identical before and after).
 *   * 20 of the 92 functions are reconstructed: 2568 `.text` bytes, unit 10.37 % fuzzy, 94.05 % mean
 *     over the 20 scored rows.  Byte-identical: `eft052_part_damage_ck`, `eft052_part_level_even_ck`,
 *     `eft052_part_gauge_add`, `eft052_set`, `eft052_dispatch`, `eft052_state_step`,
 *     `eft052_pool_release`, `eft052_item_value_get`, `eft052_page_count_add`,
 *     `eft052_hold_entry_set`.
 *   * 89-99 %: `eft052_item_half_get` 99.29, `eft052_release` 98.60, `eft052_page_counts_get` 95.72,
 *     `eft052_page_count_ck` 93.33, `eft052_hold_entry_copy` 91.30, `eft052_place` 89.30 (the
 *     `lbl_805ED0C0`/`lbl_805ED120`/`lbl_805ED168` table selection's colouring), `eft052_page_put`
 *     88.15.
 *   * 56-86 %: `eft052_page_take` 85.43, `eft052_hold_cursor_step` 83.77, `eft052_hold_row_get` 56.07
 *     (the +0x10 table row lookup's stride addressing).
 *   * the other 72 functions (20488 B) are unwritten, not residual - the two biggest bodies
 *     (`fn_8035A034` 1684 B, `fn_80358B40` 1144 B) are drafted from `tools/m2c` but not yet
 *     transcribed, and `eft052_hold_row_set`'s body is unwritten too.
 *   * DATA: `datagap.py --unit ef/eft052` reports **no ours-extra row** and the expected target-extra
 *     rows for the unwritten part (`.text` 23056/2568 B, extab 520/104 B, extabindex 780/156 B,
 *     `.rela.text` 10392/1176 B, `.relaextabindex` 1560/312 B).  The 4-byte `.sdata2` ours-extra this
 *     unit had at first write was cleared by loading the pooled `0.0f` through its own label
 *     (`lbl_8079B640`, declared never defined) instead of emitting a literal.
 */

#include "types.h"
#include "nw4r/math.h"
#include "enemy/ENEMY_WORK.h"
#include "ef.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "menu/menu_item.h"
#include "menu/menu_item_page.h"
#include "fn_80047398.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/unknown.h"
#include "enemy/fn_8011D448.h"
#include "enemy/fn_801251D0.h"
#include "fn_8004CAD8.h"

/* One 0x1C-byte part slot of the work block below: the pooled model handle `eft_res_model_get` hands back
 * and the byte that says whether the slot has been placed. */
struct EftPartSlot {
    /* +0x00 */ EftModel* model_0x00;   /* `eft_res_model_get`'s pooled model record */
    /* +0x04 */ u8 field_0x04;          /* cleared once the slot is placed */
    /* +0x05 */ u8 pad_0x05[0x17];
}; /* size: 0x1C */

/* This family's work block, the `_EFT::work_0x38` record `eft_res_slot_get` pools with the effect.  The
 * two part slots are walked as a run from the count word (`eft052_set`/`eft052_place` address them
 * with the count word as base), and the tail fields hold the area table the placement pass looks the
 * per-area byte up in. */
struct EftPartWork {
    /* +0x00 */ s32 count_0x00;              /* the slot count this family uses (2) */
    /* +0x04 */ EftPartSlot slots_0x04[2];   /* the two parts the effect is drawn on */
    /* +0x3C */ u8 field_0x3C;
    /* +0x3D */ u8 field_0x3D;
    /* +0x3E */ u8 field_0x3E;
    /* +0x3F */ u8 pad_0x3F[0x1];
    /* +0x40 */ u8* table_0x40;              /* one of the three per-area tables, 0 = unusable */
    /* +0x44 */ u32 field_0x44;
    /* +0x48 */ u8 field_0x48;               /* the per-area byte the table hands back */
    /* +0x49 */ u8 pad_0x49[0x3];
}; /* size: 0x4C */

/* This unit's own forward declarations (definitions follow in address order). */
extern "C" void eft052_set();
extern "C" void eft052_release(_EFT* self);
extern "C" void eft052_dispatch(_EFT* self);
extern "C" void eft052_place(_EFT* self);
extern "C" void fn_80358B40(_EFT* self);
extern "C" void eft052_state_step(_EFT* self);
extern "C" void eft052_pool_release(_EFT* self);

/* The pooled literal `eft052_place` passes to `setVector3` as 0.0f (the pool run
 * 0x8079B640..0x8079B704 is the band's, so it is declared, never defined - playbook 29). */
extern f32 lbl_8079B640;

/* The per-area tables the placement pass chooses between; declared, never defined (playbook 29 - the
 * `.data` run 0x805ED0C0..0x805ED938 belongs to the auto chunk until this unit's data is claimed). */
extern u8 lbl_805ED0C0[];
extern u8 lbl_805ED120[];
extern u8 lbl_805ED168[];

#pragma peephole off

/* ---------------------------------------------------------------------------------------------- *
 * 0x80358834 - pools this effect family's record, installs its two hooks and seeds both part slots.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void eft052_set()
{
    _EFT* eft = (_EFT*)eft_res_slot_get(76);
    if (eft == NULL)
        return;

    eft->dispatch_0x34 = eft052_dispatch;
    eft->release_0x40 = eft052_release;
    EftPartWork* work = (EftPartWork*)eft->work_0x38;
    work->count_0x00 = 2;
    for (s32 i = 0; i < work->count_0x00; i++) {
        work->slots_0x04[i].model_0x00 = (EftModel*)eft_res_model_get();
        if (work->slots_0x04[i].model_0x00 == NULL) {
            eft_res_slot_release(eft);
            return;
        }
    }
    eft->source_0x30 = NULL;
    eft->field_0x03 = 52;
    eft_state_flags_set(eft, 8, 0);
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x80358908 - the release hook: hands every part slot's pooled handle back, one at a time.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void eft052_release(_EFT* self)
{
    EftPartWork* work = (EftPartWork*)self->work_0x38;
    for (s32 i = 0; i < work->count_0x00; i++)
        fn_800F8A44(&work->slots_0x04[i], 1);
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035896C - the dispatch hook: one state handler per `state_0x05` value.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void eft052_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        eft052_place(self);
        return;
    case 1:
        fn_80358B40(self);
        return;
    case 2:
        eft052_state_step(self);
        return;
    case 3:
        eft052_pool_release(self);
        return;
    }
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x803589A8 - the placement pass: picks the per-area table, creates both parts' models, then
 * advances the state machine to the alive state.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void eft052_place(_EFT* self)
{
    u8* table;
    EftPartWork* work = (EftPartWork*)self->work_0x38;

    self->area_0x44 = get_now_areano();
    work->table_0x40 = NULL;
    work->field_0x44 = 0;
    if (get_now_mapno() == 3) {
        table = lbl_805ED168;
        if (self->area_0x44 == 2)
            work->table_0x40 = lbl_805ED0C0;
        else if (self->area_0x44 == 3)
            work->table_0x40 = lbl_805ED120;
    }
    if (work->table_0x40 == NULL) {
        eft052_pool_release(self);
        return;
    }
    for (s32 i = 0; i < work->count_0x00; i++) {
        if (res_eft_model_create_light((struct MHchar*)work->slots_0x04[i].model_0x00, 76, 8, 2) == NULL) {
            eft052_pool_release(self);
            return;
        }
    }
    self->state_0x05++;
    self->flag_0x01 = 1;
    self->timer_0x0C = 0;
    self->field_0x10 = 0;
    work->field_0x48 = table[self->area_0x44];
    for (s32 i = 0; i < work->count_0x00; i++) {
        EftModel* model = work->slots_0x04[i].model_0x00;
        setVector3(&model->pos_0x1C, lbl_8079B640, lbl_8079B640, lbl_8079B640);
        model->field_0x28 = 0;
        model->field_0x2C = 0;
        model->field_0x28 = 0;
        model->field_0x35 = 0;
        work->slots_0x04[i].field_0x04 = 0;
    }
    work->field_0x3C = 0;
    work->field_0x3D = 0xFF;
    work->field_0x3E = 0xFF;
    fn_80358B40(self);
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x80358624 - the part-damage report: one flag per queried part index.
 * ---------------------------------------------------------------------------------------------- */
extern "C" u8 eft052_part_damage_ck(_ENEMY_WORK* self, u32 part)
{
    VEC3 v;
    f32 near_dist;
    f32 far_dist;

    VEC3_ctor(&v);

    switch ((u8)part) {
    case 0:
        return self->field_0x1E4;
    case 1:
        if (self->field_0x1E4 == 1 && self->part_values_0x32C.part_value_0x32E <= 0)
            return 1;
        break;
    case 2:
        if (self->field_0x1E4 == 2 && self->part_values_0x32C.part_value_0x32C <= 0)
            return 1;
        break;
    case 3:
        if (self->field_0x1E4 == 0 && em_parts_damage_level_get(self, 0) < 4 &&
            self->part_values_0x32C.part_value_0x330 <= 0)
            return 1;
        break;
    case 4:
        if (self->part_values_0x32C.part_value_0x332 <= 0)
            return 1;
        break;
    case 5:
        fn_80126278(self, (u16)((self->area_no & 0xF) * 256), &v);
        near_dist = calcDistanceSqXZ(&self->pos, &v);
        fn_80126278(self, (u16)(((self->area_no & 0xF) * 256) | 1), &v);
        far_dist = calcDistanceSqXZ(&self->pos, &v);
        if (near_dist > far_dist)
            return 1;
        break;
    case 6:
        if (self->field_0x32A != 0)
            return 1;
        break;
    default:
        break;
    }
    return 0;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x803587BC - whether the part's damage level is even.
 * ---------------------------------------------------------------------------------------------- */
extern "C" s32 eft052_part_level_even_ck(_ENEMY_WORK* self, u32 part)
{
    u8 level = em_parts_damage_level_get(self, (u8)part);

    return !(level & 1);
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x803587F0 - adds a delta to the part gauge while the monster is in state 2, clamped to 0..500.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void eft052_part_gauge_add(_ENEMY_WORK* self, s16 delta)
{
    if (self->field_0x1E4 != 2)
        return;

    self->part_values_0x32C.part_value_0x32C += delta;
    if (self->part_values_0x32C.part_value_0x32C > 500) {
        self->part_values_0x32C.part_value_0x32C = 500;
        return;
    }
    if (self->part_values_0x32C.part_value_0x32C < 0)
        self->part_values_0x32C.part_value_0x32C = 0;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x80358FB8 - the state machine's advance: the next state handler is the incremented `state_0x05`.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void eft052_state_step(_EFT* self)
{
    self->state_0x05++;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x80358FC8 - retires the effect record back to the pool.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void eft052_pool_release(_EFT* self)
{
    eft_res_slot_release(self);
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x80358FCC - the item record's per-item value.
 * ---------------------------------------------------------------------------------------------- */
extern "C" u32 eft052_item_value_get(u16 id)
{
    return GetItemData(id)->field_0x010;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x80358FF4 - half the item record's +0x0C value, floored at 1.
 * ---------------------------------------------------------------------------------------------- */
extern "C" s32 eft052_item_half_get(u16 id)
{
    s32 value = GetItemData(id)->field_0x00C >> 1;

    if (value == 0)
        value = 1;
    return value;
}

/* The cockpit hold block at `.bss:0x806BF368` (0x58 B): the row cursor `eft052_hold_cursor_step`
 * advances, the per-row table `eft052_hold_row_get` reads and the current-entry copy
 * `eft052_hold_entry_copy` fills.  Its owner band is unclaimed, so the declaration stays here
 * (rule 2's named gap, the shape `ef/eft050.cpp` uses for the neighbouring block at 0x806BF310). */
struct CockpitHoldRow {
    /* +0x00 */ u16 value_0x00;   /* the value `eft052_hold_row_get` hands back for the row */
    /* +0x02 */ u16 pad_0x02;
}; /* size: 0x4 */

struct CockpitHoldEntry {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ s16 field_0x04;
    /* +0x06 */ s16 field_0x06;
    /* +0x08 */ s16 field_0x08;
    /* +0x0A */ s16 field_0x0A;
    /* +0x0C */ s16 field_0x0C;
    /* +0x0E */ u16 field_0x0E;
    /* +0x10 */ u16 field_0x10;
    /* +0x12 */ u16 pad_0x12;
    /* +0x14 */ u32 field_0x14;
    /* +0x18 */ u8 field_0x18;
    /* +0x19 */ u8 field_0x19;
}; /* size: 0x1A */

struct CockpitHoldBlock {
    /* +0x00 */ u8 field_0x00;          /* cleared by `eft052_hold_cursor_step` */
    /* +0x01 */ u8 row_0x01;            /* the row `eft052_hold_row_get` indexes the +0x10 table with */
    /* +0x02 */ s16 cursor_0x02;        /* the row the entry copy is made for */
    /* +0x04 */ s16 rows_0x04;          /* the row count `eft052_hold_cursor_step` clamps against */
    /* +0x06 */ u8 pad_0x06[0x4];
    /* +0x0A */ s16 field_0x0A;         /* the value `eft052_hold_row_get` writes to its second out */
    /* +0x0C */ u8 pad_0x0C[0x4];
    /* +0x10 */ CockpitHoldRow rows_0x10[10];
    /* +0x38 */ CockpitHoldEntry entry_0x38;
    /* +0x52 */ u8 pad_0x52[0x6];
}; /* size: 0x58 */

extern CockpitHoldBlock lbl_806BF368;

/* This unit's own out-of-order definitions. */
extern "C" s16 eft052_page_count_ck(u16 id, u8 use_rows);
extern "C" void eft052_hold_row_set(CockpitHoldBlock* block, s16 row);
extern "C" void eft052_hold_entry_copy(CockpitHoldEntry* dst, CockpitHoldEntry* src);

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035902C - the three item-page counts of one item id, handed back through optional out pointers.
 * ---------------------------------------------------------------------------------------------- */
extern "C" u32 eft052_page_counts_get(u16 id, u32* out1, u32* out2, u32* out3)
{
    u32 a = fn_8004AF20(lobby_world_block);
    u32 b = item_count_find(id, userdata_equip_item_slots_get(lobby_world_block), a);
    u32 c = fn_8004B70C(id, &lobby_world_block->field_0x0180, fn_8004AE70(lobby_world_block));
    u32 d;

    if (userdata_gunner_ck(lobby_world_block) == 1)
        d = item_count_find(id, fn_8004AF60(lobby_world_block, 0), fn_8004AF0C(0));
    else
        d = item_count_find(id, fn_8004AF60(lobby_world_block, 1), fn_8004AF0C(1));
    if (out1 != NULL)
        *out1 = b;
    if (out2 != NULL)
        *out2 = c;
    if (out3 != NULL)
        *out3 = d;
    return c + (d + b);
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x80358FCC - the item page's count for one item id, moved by `delta` (a negative `delta` adds).
 * ---------------------------------------------------------------------------------------------- */
extern "C" void eft052_page_count_add(u16 id, s16 delta)
{
    if (id != 0)
        fn_8004B200(lobby_world_block, id, -delta);
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x80359164 - moves `count` of an item out of the page's own count into the caller's hand.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void eft052_page_take(u16 id, s16 count, u8 flag)
{
    u8 sp8[8];
    s16 avail;

    GetItemData(id);
    avail = eft052_page_count_ck(id, 0);
    if (avail > 0) {
        if (avail < count) {
            fn_8004BCBC(lobby_world_block, id, avail, 1);
        } else {
            fn_8004BCBC(lobby_world_block, id, count, 1);
            return;
        }
    }
    if (flag == 1 && count != 0)
        fn_8004BEA4(id, count, sp8);
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x8035921C - the opposite move: takes `count` from the caller's hand into the page.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void eft052_page_put(u16 id, s16 count, u8 flag)
{
    u8 sp8[8];
    s16 left = count;
    s16 avail;

    avail = eft052_page_count_ck(id, 1);
    if (avail > 0) {
        if (avail >= left) {
            fn_8004BEA4(id, left, sp8);
            return;
        }
        fn_8004BEA4(id, avail, sp8);
        left -= avail;
    }
    if (flag == 1 && left != 0)
        fn_8004BCBC(lobby_world_block, id, left, 1);
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x803592D4 - one of the item page's two counts for an id, from the cabinet or the hand.
 * ---------------------------------------------------------------------------------------------- */
extern "C" s16 eft052_page_count_ck(u16 id, u8 use_rows)
{
    if (use_rows == 0)
        return fn_8004B624(lobby_world_block, id);
    return fn_8004B7B0(id, &lobby_world_block->field_0x0180, fn_8004AE70(lobby_world_block));
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x803594C8 - re-points the hold block at its clamped cursor row and clears its dirty byte.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void eft052_hold_cursor_step()
{
    eft052_hold_row_set(&lbl_806BF368, lbl_806BF368.cursor_0x02);
    if (lbl_806BF368.cursor_0x02 >= lbl_806BF368.rows_0x04)
        lbl_806BF368.cursor_0x02 = lbl_806BF368.rows_0x04 - 1;
    eft052_hold_row_set(&lbl_806BF368, lbl_806BF368.cursor_0x02);
    lbl_806BF368.field_0x00 = 0;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x80359530 - the cursor row's table value and the block's +0x0A word, through optional out pointers.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void eft052_hold_row_get(u16* out_row, s32* out_value)
{
    if (out_row != NULL)
        *out_row = lbl_806BF368.rows_0x10[lbl_806BF368.row_0x01].value_0x00;
    if (out_value != NULL)
        *out_value = lbl_806BF368.field_0x0A;
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x80359568 - hands the caller's entry to the block (its +0x19 byte first) and re-seeds the block.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void eft052_hold_entry_set(CockpitHoldEntry* entry, u8 flag)
{
    entry->field_0x19 = flag;
    memset(&lbl_806BF368, 0, 0x54);
    eft052_hold_entry_copy(&lbl_806BF368.entry_0x38, entry);
    eft052_hold_row_set(&lbl_806BF368, 0);
}

/* ---------------------------------------------------------------------------------------------- *
 * 0x803595CC - copies one hold entry field by field.
 * ---------------------------------------------------------------------------------------------- */
extern "C" void eft052_hold_entry_copy(CockpitHoldEntry* dst, CockpitHoldEntry* src)
{
    *dst = *src;
}

