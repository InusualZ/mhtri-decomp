/*
 * ef/eft052.cpp - the eft052 effect family (`_EFT` tag 52) and the cockpit item-page band it draws: the page
 *   counters, the hold block at `lbl_806BF368` and the enemy part report the family shows.
 * RANGE. .text 0x80358624-0x8035BAB4 (53 functions); extab 0x8001736C-0x80017494, extabindex 0x80036B88-0x80036D44,
 *   .data 0x805ED0C0-0x805ED370, .bss 0x806BF310-0x806BF3C0, .sdata 0x80793338-0x80793340, .sbss
 *   0x80794BE0-0x80794BE8, .sdata2 0x8079B640-0x8079B668.  The tail from 0x8035BAB4 is `enemy/em033_prog.cpp`.
 * FLAGS. `cflags_main`; `#pragma peephole off` over every body (retail keeps
 *   `clrlwi` + `slwi`/`cmpwi` unfused in `eft052_part_damage_ck` and `eft052_part_level_even_ck`).
 * NAMES. The file name is a GUESS from the family tag (`eft052_set` seeds `_EFT::field_0x03 = 52`, the scheme of
 *   every sibling the dump names; no `__FILE__` string is reachable and the dump has only placeholders).  Every
 *   definition is a GUESS `eft052_<what the body does>`: the family (`eft052_set`, `eft052_release`,
 *   `eft052_dispatch`, `eft052_place`, `eft052_state_step`, `eft052_pool_release`), the part report
 *   (`eft052_part_damage_ck`, `eft052_part_level_even_ck`, `eft052_part_gauge_add`), the item page
 *   (`eft052_item_value_get`, `eft052_item_half_get`, `eft052_page_counts_get`, `eft052_page_count_add`,
 *   `eft052_page_take`, `eft052_page_put`, `eft052_page_count_ck`) and the hold block (`eft052_hold_row_set`,
 *   `eft052_hold_cursor_step`, `eft052_hold_row_get`, `eft052_hold_entry_set`, `eft052_hold_entry_copy`).
 *   GUESS (from each body and its callers): eft052_item_get_open, eft052_item_get_step, eft052_box_list_draw
 *   GUESS: eft052_item_box
 *   GUESS (from each body and its lobby caller): eft052_hold_step, eft052_hold_draw
 * RESIDUALS. 33 rows unwritten: 0x80358B40-0x80358FB8 (`fn_80358B40`), 0x80359334-0x803594C8 (`eft052_hold_row_set`),
 *   0x80359628-0x8035BAB4 (31 rows, `eft052_hold_step` to `fn_8035B998`).
 *   10 partial rows, including:
 *  - `eft052_hold_row_get`, `eft052_hold_cursor_step`: retail materialises `lbl_806BF368` once and keeps it in a
 *    register, ours re-materialises it at each use;
 *  - `eft052_page_take`: retail's parameters start at `r4` (a leading parameter ours lacks), and the `s16`
 *    sign-extensions sit at other points;
 *  - `eft052_place`: ours compares the area unsigned (`cmplwi`) in another order and keeps `+0x04`'s pointer in a
 *    register where retail reloads it per store.
 *   The other 6 partial rows have no recorded cause (`symdiff.py -u ef/eft052 --all`).
 *   flipcheck: `.bss`/`.data`/`.sbss`/`.sdata`/`.sdata2` claimed, not emitted; `.text` (0xA08 of 0x3490), extab (0x68
 *   of 0x128) and extabindex (0x9C of 0x1BC) short of the claim and differing.
 * SHAPES. The pooled `0.0f` is loaded through its label `lbl_8079B640` (a literal emits a 4-byte `.sdata2` of ours).
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

/* The pooled literal `eft052_place` passes to `setVector3` as 0.0f, in the unit's own `.sdata2` claim
 * (0x8079B640-0x8079B668); declared, never defined (playbook 29). */
extern f32 lbl_8079B640;

/* The per-area tables the placement pass chooses between, in the unit's claimed `.data` run; declared, not
 * defined (our object emits no `.data`). */
extern u8 lbl_805ED0C0[];
extern u8 lbl_805ED120[];
extern u8 lbl_805ED168[];

#pragma peephole off

/* 0x80358834 (0xD4): Pools the family's record, installs its two hooks and seeds both part slots. */
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

/* 0x80358908 (0x64): Hands every part slot's pooled handle back (the release hook). */
extern "C" void eft052_release(_EFT* self)
{
    EftPartWork* work = (EftPartWork*)self->work_0x38;
    for (s32 i = 0; i < work->count_0x00; i++)
        fn_800F8A44(&work->slots_0x04[i], 1);
}

/* 0x8035896C (0x3C): Dispatches `state_0x05` to its state handler (the dispatch hook). */
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

/* 0x803589A8 (0x198): Picks the per-area table, creates both parts' models and advances to the alive state. */
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

/* 0x80358624 (0x198): Reports one damage flag per queried part index. */
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

/* 0x803587BC (0x34): Tells whether the part's damage level is even. */
extern "C" s32 eft052_part_level_even_ck(_ENEMY_WORK* self, u32 part)
{
    u8 level = em_parts_damage_level_get(self, (u8)part);

    return !(level & 1);
}

/* 0x803587F0 (0x44): Adds a delta to the part gauge while the monster is in state 2, clamped to 0..500. */
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

/* 0x80358FB8 (0x10): Advances the record's state. */
extern "C" void eft052_state_step(_EFT* self)
{
    self->state_0x05++;
}

/* 0x80358FC8 (0x4): Retires the effect record to the pool. */
extern "C" void eft052_pool_release(_EFT* self)
{
    eft_res_slot_release(self);
}

/* 0x80358FCC (0x28): Returns the item record's per-item value. */
extern "C" u32 eft052_item_value_get(u16 id)
{
    return GetItemData(id)->field_0x010;
}

/* 0x80358FF4 (0x38): Returns half the item record's +0x0C value, floored at 1. */
extern "C" s32 eft052_item_half_get(u16 id)
{
    s32 value = GetItemData(id)->field_0x00C >> 1;

    if (value == 0)
        value = 1;
    return value;
}

/* The cockpit hold block at `.bss:0x806BF368` (0x58 B), in the unit's own `.bss` claim (0x806BF310-0x806BF3C0):
 * the row cursor `eft052_hold_cursor_step` advances, the per-row table `eft052_hold_row_get` reads and the
 * current-entry copy `eft052_hold_entry_copy` fills; declared, never defined. */
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

/* 0x8035902C (0x114): Returns the three item-page counts of one item id through optional out pointers. */
extern "C" u32 eft052_page_counts_get(u16 id, u32* out1, u32* out2, u32* out3)
{
    u32 a = fn_8004AF20(lobby_world_block);
    u32 b = item_count_find(id, userdata_equip_item_slots_get(lobby_world_block), a);
    u32 c = item_slots_count_sum(id, &lobby_world_block->field_0x0180, userdata_box_capacity(lobby_world_block));
    u32 d;

    if (userdata_gunner_ck(lobby_world_block) == 1)
        d = item_count_find(id, userdata_pouch_get(lobby_world_block, 0), userdata_pouch_size(0));
    else
        d = item_count_find(id, userdata_pouch_get(lobby_world_block, 1), userdata_pouch_size(1));
    if (out1 != NULL)
        *out1 = b;
    if (out2 != NULL)
        *out2 = c;
    if (out3 != NULL)
        *out3 = d;
    return c + (d + b);
}

/* 0x80359140 (0x24): Moves the item page's count for one id by `delta` (a negative `delta` adds). */
extern "C" void eft052_page_count_add(u16 id, s16 delta)
{
    if (id != 0)
        fn_8004B200(lobby_world_block, id, -delta);
}

/* 0x80359164 (0xB8): Moves `count` of an item from the page's own count into the caller's hand. */
extern "C" void eft052_page_take(u16 id, s16 count, u8 flag)
{
    u8 sp8[8];
    s16 avail;

    GetItemData(id);
    avail = eft052_page_count_ck(id, 0);
    if (avail > 0) {
        if (avail < count) {
            userdata_item_give(lobby_world_block, id, avail, 1);
        } else {
            userdata_item_give(lobby_world_block, id, count, 1);
            return;
        }
    }
    if (flag == 1 && count != 0)
        item_box_store(id, count, sp8);
}

/* 0x8035921C (0xB8): Moves `count` of an item from the caller's hand into the page. */
extern "C" void eft052_page_put(u16 id, s16 count, u8 flag)
{
    u8 sp8[8];
    s16 left = count;
    s16 avail;

    avail = eft052_page_count_ck(id, 1);
    if (avail > 0) {
        if (avail >= left) {
            item_box_store(id, left, sp8);
            return;
        }
        item_box_store(id, avail, sp8);
        left -= avail;
    }
    if (flag == 1 && left != 0)
        userdata_item_give(lobby_world_block, id, left, 1);
}

/* 0x803592D4 (0x60): Returns one of the item page's two counts for an id, from the cabinet or the hand. */
extern "C" s16 eft052_page_count_ck(u16 id, u8 use_rows)
{
    if (use_rows == 0)
        return fn_8004B624(lobby_world_block, id);
    return item_slots_room_get(id, &lobby_world_block->field_0x0180, userdata_box_capacity(lobby_world_block));
}

/* 0x803594C8 (0x68): Re-points the hold block at its clamped cursor row and clears its dirty byte. */
extern "C" void eft052_hold_cursor_step()
{
    eft052_hold_row_set(&lbl_806BF368, lbl_806BF368.cursor_0x02);
    if (lbl_806BF368.cursor_0x02 >= lbl_806BF368.rows_0x04)
        lbl_806BF368.cursor_0x02 = lbl_806BF368.rows_0x04 - 1;
    eft052_hold_row_set(&lbl_806BF368, lbl_806BF368.cursor_0x02);
    lbl_806BF368.field_0x00 = 0;
}

/* 0x80359530 (0x38): Returns the cursor row's table value and the block's +0x0A word through optional pointers. */
extern "C" void eft052_hold_row_get(u16* out_row, s32* out_value)
{
    if (out_row != NULL)
        *out_row = lbl_806BF368.rows_0x10[lbl_806BF368.row_0x01].value_0x00;
    if (out_value != NULL)
        *out_value = lbl_806BF368.field_0x0A;
}

/* 0x80359568 (0x64): Hands the caller's entry to the block (its +0x19 byte first) and re-seeds the block. */
extern "C" void eft052_hold_entry_set(CockpitHoldEntry* entry, u8 flag)
{
    entry->field_0x19 = flag;
    memset(&lbl_806BF368, 0, 0x54);
    eft052_hold_entry_copy(&lbl_806BF368.entry_0x38, entry);
    eft052_hold_row_set(&lbl_806BF368, 0);
}

/* 0x803595CC (0x5C): Copies one hold entry field by field. */
extern "C" void eft052_hold_entry_copy(CockpitHoldEntry* dst, CockpitHoldEntry* src)
{
    *dst = *src;
}

