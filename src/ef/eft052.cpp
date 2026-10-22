/*
 * ef/eft052.cpp - the eft052 effect family (`_EFT` tag 52, two parts flying the area's paths), the cockpit item-page
 *   band (the page counters, the hold strip at `lbl_806BF368`, the item hand-over box `eft052_item_box`), the enemy
 *   part report, and the HUD message queue (`hud_msg_*`, its work at `hud_msg_work`).
 * RANGE. .text 0x80358624-0x8035BAB4 (53 functions); extab 0x8001736C-0x80017494, extabindex 0x80036B88-0x80036D44,
 *   .data 0x805ED0C0-0x805ED370, .bss 0x806BF310-0x806BF3C0, .sdata 0x80793338-0x80793340, .sbss
 *   0x80794BE0-0x80794BE8, .sdata2 0x8079B640-0x8079B668.  The tail from 0x8035BAB4 is `enemy/em033_prog.cpp`.  The
 *   HUD message run (0x8035A9E4-0x8035B998, its own `.sbss` word and the `.sdata2` 0.5f/48.0f/-1.0f/magic run) and
 *   `eft_area_se_req` read as another TU's code; the registered range keeps them here.
 * FLAGS. `cflags_main`; `#pragma peephole off` over every body (retail keeps `clrlwi` + `slwi`/`cmpwi` unfused in
 *   `eft052_part_damage_ck` and `eft052_part_level_even_ck`); `#pragma fp_contract off` over the HUD message run
 *   (retail keeps `fmuls` + `fsubs` in `hud_msg_step` and `hud_msg_sub_set`).
 * NAMES. The file name is a GUESS from the family tag (`eft052_set` seeds `_EFT::field_0x03 = 52`; no `__FILE__`
 *   string is reachable and the dump has only placeholders).  Every definition is a GUESS from its body and callers:
 *   the family (`eft052_set`, `eft052_release`, `eft052_dispatch`, `eft052_place`, `fn_80358B40` the path flight,
 *   `eft052_state_step`, `eft052_pool_release`), the part report (`eft052_part_damage_ck`,
 *   `eft052_part_level_even_ck`, `eft052_part_gauge_add`), the item page (`eft052_item_value_get`,
 *   `eft052_item_half_get`, `eft052_page_counts_get`, `eft052_page_count_add`, `eft052_page_take`, `eft052_page_put`,
 *   `eft052_page_count_ck`), the hold strip (`eft052_hold_row_set`, `eft052_hold_cursor_step`, `eft052_hold_row_get`,
 *   `eft052_hold_entry_set`, `eft052_hold_entry_copy`, `eft052_hold_step`, `eft052_hold_list_draw`, `eft052_hold_draw`),
 *   the item box (`eft052_box_count_sum`, `eft052_box_open`, `eft052_box_grid_copy`, `eft052_item_get_open`,
 *   `eft052_box_value_get`, `eft052_box_recount`, `eft052_box_sell`, `eft052_box_cell_merge`, `eft052_box_sort`,
 *   `eft052_item_get_step`, `eft052_box_menu_draw`, `eft052_box_list_draw`, `eft052_item_box`), the HUD messages
 *   (`hud_msg_init`, `hud_msg_frame`, `hud_msg_alloc`, `hud_msg_free`, `hud_msg_enqueue`, `hud_msg_find`,
 *   `hud_msg_step`, `hud_msg_update`, `hud_msg_open`, `hud_msg_draw`, `hud_msg_push`, `hud_msg_push_two_line`,
 *   `hud_item_msg_push`, `hud_msg_sub_set`, `hud_msg_shown_id_get`, `hud_msg_work`) and `eft_area_se_req`; the data
 *   `eft052_hold_frame_sprites`, `eft052_hold_row_sprites`, `eft052_hold_arrow_sprites`.
 * RESIDUALS. Every row is written; 21 partial rows (`measure.py ef/eft052` lists them), including:
 *  - `eft052_page_take`: retail's parameters start at `r4` (a leading parameter ours lacks), and the `s16`
 *    sign-extensions sit at other points;
 *  - `eft052_place`: ours compares the area unsigned (`cmplwi`) in another order and keeps `+0x04`'s pointer in a
 *    register where retail reloads it per store;
 *  - `eft052_hold_list_draw`, `hud_msg_free`, `hud_msg_enqueue`, `fn_80358B40`: register colouring only;
 *  - `hud_msg_update`: retail steps its first queue walk by 8 bytes (`queue += 2`, kept as written).
 *   flipcheck: `.bss`/`.data`/`.sbss`/`.sdata`/`.sdata2` claimed, not emitted.
 * SHAPES. The pooled `0.0f` is loaded through its label `lbl_8079B640` (a literal emits a 4-byte `.sdata2` of ours).
 *   `get_lsp_data`'s record is read through `EftLspRect` (`hud/layout_types.h` cannot meet `menu/menu_item_page.h`).
 *   `x / 2.0f` keeps retail's `fmuls x, 0.5` operand order where `x * 0.5f` swaps it.
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
#include "Runtime.PPCEABI.H/memcpy.h"
#include "unsplit/unknown.h"
#include "enemy/fn_8011D448.h"
#include "enemy/fn_801251D0.h"
#include "fn_8004CAD8.h"
#include "pl.h"                         /* MHchar */
#include "ef/eft052_box.h"               /* the item box records */
#include "ef/eft052_item_get_open.h"     /* eft052_item_box and the window entry points */
#include "lobby/lb_cmd_pressed_ck.h"     /* lb_cmd_pressed_ck / lb_cmd_repeat_ck / lb_cmd_repeat_get / lb_yes_no_step */
#include "lobby/lobby_w.h"               /* lobby_w */
#include "lobby/lb_list_init.h"          /* the lobby panel helpers */
#include "lobby/LbStr.h"                 /* LbStr */
#include "menu/menu_message.h"           /* the menu cursor helpers */
#include "font/flfnt.h"                  /* flfntStrLen / font_set_size / font_flush */
#include "main.h"                        /* get_ScreenSize */
#include "hud/note_text_max_len.h"       /* note_text_max_len and the note line printers */
#include "lobby/fn_801E0ADC.h"           /* fn_801E72F4 */
#include "sound/fn_800EF7D8.h"           /* se_slot_req */
#include "ef/system_core.h"              /* work_mem_alloc */
#include "sound/fn_800D7F54.h"           /* sysSE_req / shell_se_req */
#include "stage/stg_w.h"                 /* get_now_areano */
#include "ef/fn_800CDB2C.h"               /* ran_suu */
#include "mh3_pad.h"                     /* copyVec3 */
#include "g3d/g3d_calcworld.h"           /* addVec3To */
#include "ef/get_move_work_adrs.h"       /* get_move_work_adrs */
#include "MSL_C/strcat.h"                /* strcat */
#include "lobby/spr_data_copy.h"         /* spr_data_copy */

/* The by-value spellings of the out-pointer vector helpers (the result pointer is the hidden first argument either
 * way): retail builds each result in a compiler temporary, in call order. */
#define VEC3_SUB ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))subVec3)
#define VEC3_SCALE ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, f32))vec3_scale)

/* One 0x1C-byte part slot of the work block below: the pooled model handle `eft_res_model_get` hands back
 * and the byte that says whether the slot has been placed. */
struct EftPartSlot {
    /* +0x00 */ MHchar* model_0x00;     /* `eft_res_model_get`'s pooled model record */
    /* +0x04 */ u8 phase_0x04;          /* 0 pick a path, 1 fly, 2 hide, 3 wait */
    /* +0x05 */ u8 pad_0x05[0x3];
    /* +0x08 */ VEC3 vel_0x08;          /* the step along the path */
    /* +0x14 */ s16 timer_0x14;         /* the frames left on the path, then the wait */
    /* +0x16 */ u8 variant_0x16;        /* which five-joint run the animation shows */
    /* +0x17 */ u8 frame_0x17;          /* the shown joint */
    /* +0x18 */ s8 anim_0x18;           /* the frames to the next joint */
    /* +0x19 */ u8 pad_0x19[0x3];
}; /* size: 0x1C */

/* This family's work block, the `_EFT::work_0x38` record `eft_res_slot_get` pools with the effect.  The
 * two part slots are walked as a run from the count word (`eft052_set`/`eft052_place` address them
 * with the count word as base), and the tail fields hold the area table the placement pass looks the
 * per-area byte up in. */
struct EftPartWork {
    /* +0x00 */ s32 count_0x00;              /* the slot count this family uses (2) */
    /* +0x04 */ EftPartSlot slots_0x04[2];   /* the two parts the effect is drawn on */
    /* +0x3C */ u8 timer_0x3C;               /* counts the parts that took a path this round */
    /* +0x3D */ s8 path_0x3D;                /* the path the next part flies */
    /* +0x3E */ s8 last_path_0x3E;           /* the path flown last */
    /* +0x3F */ u8 pad_0x3F[0x1];
    /* +0x40 */ Vec* paths_0x40;             /* the area's path ends, two per path; 0 = unusable */
    /* +0x44 */ u32 field_0x44;
    /* +0x48 */ u8 path_count_0x48;          /* the area's path count */
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
extern Vec lbl_805ED0C0[];
extern Vec lbl_805ED120[];
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
        work->slots_0x04[i].model_0x00 = (MHchar*)eft_res_model_get();
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
        eft_res_models_release(&work->slots_0x04[i], 1);
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
    work->paths_0x40 = NULL;
    work->field_0x44 = 0;
    if (get_now_mapno() == 3) {
        table = lbl_805ED168;
        if (self->area_0x44 == 2)
            work->paths_0x40 = lbl_805ED0C0;
        else if (self->area_0x44 == 3)
            work->paths_0x40 = lbl_805ED120;
    }
    if (work->paths_0x40 == NULL) {
        eft052_pool_release(self);
        return;
    }
    for (s32 i = 0; i < work->count_0x00; i++) {
        if (res_eft_model_create_light(work->slots_0x04[i].model_0x00, 76, 8, 2) == NULL) {
            eft052_pool_release(self);
            return;
        }
    }
    self->state_0x05++;
    self->flag_0x01 = 1;
    self->timer_0x0C = 0;
    self->field_0x10 = 0;
    work->path_count_0x48 = table[self->area_0x44];
    for (s32 i = 0; i < work->count_0x00; i++) {
        MHchar* model = work->slots_0x04[i].model_0x00;
        setVector3(&model->scale_0x1C, lbl_8079B640, lbl_8079B640, lbl_8079B640);
        model->field_0x28 = 0;
        model->field_0x2C = 0;
        model->field_0x28 = 0;
        model->ready = 0;
        work->slots_0x04[i].phase_0x04 = 0;
    }
    work->timer_0x3C = 0;
    work->path_0x3D = -1;
    work->last_path_0x3E = -1;
    fn_80358B40(self);
}

/* 0x80358B40 (0x478): Runs both parts along the area's paths: each picks a path (never the last one twice), flies it
 * at a random speed with its joint animation, then hides for a random wait. */
extern "C" void fn_80358B40(_EFT* self)
{
    EftPartWork* work = (EftPartWork*)self->work_0x38;
    s32 reverse = 0;
    VEC3 dir;
    VEC3 from;
    VEC3 to;
    s32 i;
    EftPartSlot* slot;
    s32 j;
    f32 dist;
    u8 speed;

    VEC3_ctor(&dir);
    VEC3_ctor(&from);
    VEC3_ctor(&to);
    for (i = 0, slot = work->slots_0x04; i < work->count_0x00; i++, slot++) {
        switch (slot->phase_0x04) {
        case 0:
            if (work->timer_0x3C == 0) {
                if (work->path_count_0x48 == 1) {
                    work->path_0x3D = 0;
                } else {
                    while (work->last_path_0x3E == work->path_0x3D)
                        work->path_0x3D = (u16)ran_suu(1) % work->path_count_0x48;
                }
                work->last_path_0x3E = work->path_0x3D;
            }
            work->timer_0x3C++;
            if (work->timer_0x3C >= 1)
                work->timer_0x3C = 0;
            if ((u16)ran_suu(1) & 1)
                reverse = 1;
            if (reverse) {
                vec_to_mh_vec3(&from, &work->paths_0x40[work->path_0x3D * 2 + 1]);
                vec_to_mh_vec3(&to, &work->paths_0x40[work->path_0x3D * 2]);
            } else {
                vec_to_mh_vec3(&from, &work->paths_0x40[work->path_0x3D * 2]);
                vec_to_mh_vec3(&to, &work->paths_0x40[work->path_0x3D * 2 + 1]);
            }
            copyVec3(&dir, &VEC3_SUB(&to, &from));
            dist = vec3_distance(&to, &from);
            speed = ((u16)ran_suu(1) % 5 + 18) * 2;
            vec3_normalize_into(&dir, &dir);
            copyVec3(&slot->vel_0x08, &VEC3_SCALE(&dir, (f32)speed));
            slot->timer_0x14 = dist / (f32)speed;
            slot->model_0x00->field_0x2C = calcVecAng2(&from, &to);
            copyVec3(&slot->model_0x00->pos_0x04, &from);
            slot->variant_0x16 = (u16)ran_suu(1) % 2;
            slot->frame_0x17 = slot->variant_0x16 * 5;
            slot->phase_0x04++;
            slot->anim_0x18 = 0;
        case 1:
            if (slot->timer_0x14 < 0)
                slot->phase_0x04++;
            addVec3To(&slot->model_0x00->pos_0x04, &slot->vel_0x08);
            slot->anim_0x18--;
            if (slot->anim_0x18 <= 0) {
                slot->frame_0x17++;
                slot->anim_0x18 = 1;
                if (slot->frame_0x17 >= (slot->variant_0x16 + 1) * 5 + 1)
                    slot->frame_0x17 = slot->variant_0x16 * 5 + 1;
                for (j = 1; j < 11; j++) {
                    if (j == slot->frame_0x17)
                        slot->model_0x00->setVisibility(j, 1);
                    else
                        slot->model_0x00->setVisibility(j, 0);
                }
            }
            break;
        case 2:
            for (j = 1; j < 11; j++)
                slot->model_0x00->setVisibility(j, 0);
            slot->phase_0x04++;
            slot->timer_0x14 = ran_suu(1) & 0x18D;
            break;
        case 3:
            if (slot->timer_0x14 < 0)
                slot->phase_0x04 = 0;
            break;
        }
        slot->timer_0x14--;
        slot->model_0x00->move(0);
        eft_res_models_spawn(self, (void**)slot, 2, 1, 0);
    }
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
        em_data_point_get(self, (u16)((self->area_no & 0xF) * 256), &v);
        near_dist = calcDistanceSqXZ(&self->pos, &v);
        em_data_point_get(self, (u16)(((self->area_no & 0xF) * 256) | 1), &v);
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
    /* +0x00 */ u16 id_0x00;      /* the row's item */
    /* +0x02 */ s16 count_0x02;   /* the count it shows (owned, or the price for kind 1) */
}; /* size: 0x4 */

struct CockpitHoldEntry {
    /* +0x00 */ s32 limit_0x00;         /* the most one pick takes; 0 takes one without a count step */
    /* +0x04 */ s16 msg_0x04;           /* the browse help line, -1 none */
    /* +0x06 */ s16 msg2_0x06;          /* its second line, -1 none */
    /* +0x08 */ s16 title_0x08;         /* the empty strip's line */
    /* +0x0A */ s16 count_msg_0x0A;     /* the count step's help line, -1 none */
    /* +0x0C */ s16 item_count_0x0C;    /* the length of the item list */
    /* +0x0E */ u16 sprite_0x0E;        /* the strip's title sprite */
    /* +0x10 */ u16 highlight_0x10;     /* the item drawn marked, 0 none */
    /* +0x12 */ u16 pad_0x12;
    /* +0x14 */ u16* items_0x14;        /* the item list */
    /* +0x18 */ u8 kind_0x18;           /* 0 owned items, 1 priced items, 2 box room */
    /* +0x19 */ u8 quiet_0x19;          /* no pick sound */
    /* +0x1A */ u16 pad_0x1A;
}; /* size: 0x1C */

struct CockpitHoldBlock {
    /* +0x00 */ u8 mode_0x00;           /* 0 browse, 1 count step */
    /* +0x01 */ u8 row_0x01;            /* the cursor row of the page */
    /* +0x02 */ s16 page_0x02;          /* the shown page */
    /* +0x04 */ s16 pages_0x04;         /* the page count (at least 1) */
    /* +0x06 */ u16 moved_0x06;         /* the cursor helpers' moved flags */
    /* +0x08 */ s16 rows_0x08;          /* the rows on the page */
    /* +0x0A */ s16 amount_0x0A;        /* the picked count */
    /* +0x0C */ s32 max_0x0C;           /* the count step's limit */
    /* +0x10 */ CockpitHoldRow rows_0x10[8];
    /* +0x30 */ u8 pad_0x30[0x8];
    /* +0x38 */ CockpitHoldEntry entry_0x38;
    /* +0x54 */ u8 pad_0x54[0x4];
}; /* size: 0x58 */

extern CockpitHoldBlock lbl_806BF368;

/* This unit's own out-of-order definitions. */
extern "C" s32 eft052_page_count_ck(u16 id, u8 use_rows);
extern "C" void eft052_hold_row_set(CockpitHoldBlock* block, s16 row);
extern "C" void eft052_hold_entry_copy(CockpitHoldEntry* dst, CockpitHoldEntry* src);

/* One HUD message entry: its state and kind, its id, the second-line state, the slide phase and hold timer, the
 * position and box sizes, the font size, the slide's stop row and speed, and its two text lines.  size: 0xBC */
struct HudMsg {
    /* +0x00 */ u8 state_0x00;          /* 0 free, 1 queued, 2 shown, 3 done */
    /* +0x01 */ u8 kind_0x01;           /* 1 plays the item jingle, 5 lays the text out as a note */
    /* +0x02 */ u8 id_0x02;             /* the entry's own index, handed back to the pusher */
    /* +0x03 */ u8 sub_on_0x03;         /* the second line is shown */
    /* +0x04 */ s8 phase_0x04;          /* 0 lay out, 1 slide in, 2 hold, 3 slide out */
    /* +0x05 */ u8 pad_0x05[0x3];
    /* +0x08 */ s32 timer_0x08;         /* the hold frames */
    /* +0x0C */ u8 unused_0x0C[0xC];
    /* +0x18 */ f32 x_0x18;
    /* +0x1C */ f32 y_0x1C;
    /* +0x20 */ s16 half_w_0x20;        /* half the text width */
    /* +0x22 */ s16 margin_0x22;
    /* +0x24 */ s16 width_0x24;         /* the text width */
    /* +0x26 */ s16 height_0x26;
    /* +0x28 */ s16 font_w_0x28;
    /* +0x2A */ s16 font_h_0x2A;
    /* +0x2C */ u8 pad_0x2C[0x2];
    /* +0x2E */ s16 stop_y_0x2E;        /* the row the slide-in stops at */
    /* +0x30 */ f32 speed_0x30;
    /* +0x34 */ char text_0x34[0x40];
    /* +0x74 */ s16 sub_x_0x74;
    /* +0x76 */ s16 sub_y_0x76;
    /* +0x78 */ s16 sub_w_0x78;
    /* +0x7A */ s16 sub_h_0x7A;
    /* +0x7C */ char sub_0x7C[0x40];
};

/* One of the sixteen records the HUD message work keeps after its queue; only its first byte is cleared here.
 * size: 0x40 */
struct HudMsgSlot {
    /* +0x00 */ u8 used_0x00;
    /* +0x01 */ u8 unused_0x01[0x3F];
};

/* The HUD message work `hud_msg_init` allocates: the entries, the queue in show order and the ids shown this frame.
 * size: 0x1068 */
struct HudMsgWork {
    /* +0x0000 */ u8 unused_0x0000[0x4];
    /* +0x0004 */ s32 field_0x04;
    /* +0x0008 */ s32 count_0x08;           /* the queued entries */
    /* +0x000C */ HudMsg msgs_0x0C[16];
    /* +0x0BCC */ HudMsg* queue_0xBCC[16];
    /* +0x0C0C */ s8 shown_0xC0C[16];       /* the shown ids, -1 past the last */
    /* +0x0C1C */ s32 field_0xC1C;
    /* +0x0C20 */ s32 field_0xC20;
    /* +0x0C24 */ s32 field_0xC24;
    /* +0x0C28 */ s16 field_0xC28[16];
    /* +0x0C48 */ s16 field_0xC48[16];
    /* +0x0C68 */ HudMsgSlot slots_0xC68[16];
};

/* The layout record `get_lsp_data` hands back, as far as the hold strip reads it (`hud/layout_types.h`'s `_SPR_DATA_`,
 * whose header cannot meet `menu/menu_item_page.h`'s `_mh_ivec2_`).  size: 0x24 */
struct EftLspRect {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
    /* +0x04 */ s16 ofs_x;
    /* +0x06 */ s16 ofs_y;
    /* +0x08 */ s16 width;
    /* +0x0A */ s16 height;
    /* +0x0C */ u8 unused_0x0C[0x18];
};

/* The HUD message work pointer, in the unit's `.sbss` claim; declared, not defined. */
extern HudMsgWork* hud_msg_work;

/* The hold strip's frame sprites, its row layouts and its page-arrow sprites, in the unit's `.data` claim; declared,
 * not defined. */
extern u16 eft052_hold_frame_sprites[16];
extern u16 eft052_hold_row_sprites[8];
extern u16 eft052_hold_arrow_sprites[8];

/* This unit's own out-of-order definitions. */
extern "C" void eft052_hold_list_draw(void);
extern "C" void eft052_box_grid_copy(Eft052BoxGrid* dst, Eft052BoxGrid* src);
extern "C" void hud_msg_update(HudMsgWork* work);
extern "C" void hud_msg_draw(void);


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
extern "C" s32 eft052_page_count_ck(u16 id, u8 use_rows)
{
    if (use_rows == 0)
        return fn_8004B624(lobby_world_block, id);
    return item_slots_room_get(id, &lobby_world_block->field_0x0180, userdata_box_capacity(lobby_world_block));
}

/* 0x80359334 (0x194): Fills the block's rows with page `page` of the entry's shown items (owned ones, or every listed
 * one for the box-room kind) and recounts the pages. */
extern "C" void eft052_hold_row_set(CockpitHoldBlock* block, s16 page)
{
    s32 first = page * 8;
    s32 last = first + 7;
    s32 shown;
    s32 count;
    s32 i;
    u16 id;
    s32 have;

    block->rows_0x08 = 0;
    block->page_0x02 = page;
    memset(block->rows_0x10, 0, sizeof(block->rows_0x10));
    shown = 0;
    count = block->entry_0x38.item_count_0x0C;
    for (i = 0; i < count; i++) {
        id = block->entry_0x38.items_0x14[i];
        if (block->entry_0x38.kind_0x18 == 2)
            have = id != 0 ? 1 : 0;
        else
            have = userdata_item_count_total(id, lobby_world_block);
        if (have == 0 && block->entry_0x38.kind_0x18 != 2)
            continue;
        if (shown >= first && shown <= last && shown < count) {
            block->rows_0x10[block->rows_0x08].id_0x00 = id;
            switch (block->entry_0x38.kind_0x18) {
            case 0:
            case 2:
                block->rows_0x10[block->rows_0x08].count_0x02 = have;
                break;
            case 1:
                block->rows_0x10[block->rows_0x08].count_0x02 = item_price_get(id);
                break;
            }
            block->rows_0x08++;
        }
        shown++;
    }
    if (block->row_0x01 >= block->rows_0x08)
        block->row_0x01 = block->rows_0x08 - 1;
    block->pages_0x04 = menu_page_count(shown, 8);
    if (block->pages_0x04 == 0)
        block->pages_0x04 = 1;
}

/* 0x803594C8 (0x68): Re-points the hold block at its clamped cursor row and clears its dirty byte. */
extern "C" void eft052_hold_cursor_step()
{
    CockpitHoldBlock* block = &lbl_806BF368;

    eft052_hold_row_set(block, block->page_0x02);
    if (block->page_0x02 >= block->pages_0x04)
        block->page_0x02 = block->pages_0x04 - 1;
    eft052_hold_row_set(block, block->page_0x02);
    block->mode_0x00 = 0;
}

/* 0x80359530 (0x38): Returns the cursor row's table value and the block's +0x0A word through optional pointers. */
extern "C" void eft052_hold_row_get(u16* out_row, s32* out_value)
{
    CockpitHoldBlock* block = &lbl_806BF368;

    if (out_row != NULL)
        *out_row = block->rows_0x10[block->row_0x01].id_0x00;
    if (out_value != NULL)
        *out_value = block->amount_0x0A;
}

/* 0x80359568 (0x64): Hands the caller's entry to the block (its +0x19 byte first) and re-seeds the block. */
extern "C" void eft052_hold_entry_set(CockpitHoldEntry* entry, u8 flag)
{
    entry->quiet_0x19 = flag;
    memset(&lbl_806BF368, 0, 0x54);
    eft052_hold_entry_copy(&lbl_806BF368.entry_0x38, entry);
    eft052_hold_row_set(&lbl_806BF368, 0);
}

/* 0x803595CC (0x5C): Copies one hold entry field by field. */
extern "C" void eft052_hold_entry_copy(CockpitHoldEntry* dst, CockpitHoldEntry* src)
{
    dst->limit_0x00 = src->limit_0x00;
    dst->msg_0x04 = src->msg_0x04;
    dst->msg2_0x06 = src->msg2_0x06;
    dst->title_0x08 = src->title_0x08;
    dst->count_msg_0x0A = src->count_msg_0x0A;
    dst->item_count_0x0C = src->item_count_0x0C;
    dst->sprite_0x0E = src->sprite_0x0E;
    dst->highlight_0x10 = src->highlight_0x10;
    dst->items_0x14 = src->items_0x14;
    dst->kind_0x18 = src->kind_0x18;
    dst->quiet_0x19 = src->quiet_0x19;
}

/* 0x80359628 (0x248): Steps the hold strip's input: picks a row (with a count step when the entry has a limit), moves
 * the row and page cursors, or cancels; 1 once an entry is taken, 2 when cancelled. */
extern "C" s32 eft052_hold_step(void)
{
    CockpitHoldBlock* block = &lbl_806BF368;
    s32 result = 0;
    s32 have;
    s16 room;

    block->moved_0x06 = 0;
    switch (block->mode_0x00) {
    case 0:
        if (block->rows_0x08 != 0 && lb_cmd_pressed_ck(0x10)) {
            if (block->entry_0x38.kind_0x18 == 2) {
                room = eft052_page_count_ck(block->rows_0x10[block->row_0x01].id_0x00, 1);
                if (room < 0)
                    room = 0;
                if (room > 0) {
                    sysSE_req(0);
                    block->amount_0x0A = 1;
                    result = 1;
                } else {
                    sysSE_req(2);
                }
            } else {
                if (block->entry_0x38.quiet_0x19 == 0)
                    sysSE_req(0);
                block->amount_0x0A = 1;
                if (block->entry_0x38.limit_0x00 == 0) {
                    result = 1;
                } else {
                    block->mode_0x00 = 1;
                    have = userdata_item_count_total(block->rows_0x10[block->row_0x01].id_0x00, lobby_world_block);
                    if (have > block->entry_0x38.limit_0x00)
                        block->max_0x0C = block->entry_0x38.limit_0x00;
                    else
                        block->max_0x0C = have;
                }
            }
        } else if (lb_cmd_pressed_ck(0x20)) {
            sysSE_req(1);
            result = 2;
        } else if (lb_cmd_repeat_ck(3)) {
            block->row_0x01 = menu_cursor_step(block->row_0x01, block->rows_0x08, lb_cmd_repeat_get(), 1, 2);
        } else if (lb_cmd_repeat_ck(0xC)) {
            block->page_0x02 = menu_cursor_step_fixed_tail(block->page_0x02, block->pages_0x04, lb_cmd_repeat_get(), 4, 8,
                                                           &block->moved_0x06);
            eft052_hold_row_set(block, block->page_0x02);
        }
        break;
    case 1:
        switch (menu_cursor_page_move(&block->amount_0x0A, block->max_0x0C, lobby_w.cmd_mask_0x084[0][0],
                                      lb_cmd_repeat_get(), &block->moved_0x06)) {
        case 1:
            result = 1;
            sysSE_req(0);
            break;
        case 2:
            block->mode_0x00 = 0;
            sysSE_req(1);
            break;
        }
        break;
    }
    return result;
}

/* 0x80359870 (0x290): Draws the hold strip: its frame and title sprite, the page arrows and one row per item of the
 * page (or the entry's empty line). */
extern "C" void eft052_hold_list_draw(void)
{
    CockpitHoldBlock* block = &lbl_806BF368;
    s32 active;
    u16 flags;
    u8 arrow_mode;
    s16 lines;
    _mh_ivec2_ base;
    _mh_ivec2_ pos;
    EftLspRect spr;
    s32 i;
    const u16* lsp;
    EftLspRect* cell;
    s32 picked;
    s32 enabled;
    s16 room;
    s32 marked;

    if (block->mode_0x00 == 0) {
        active = 1;
        flags = block->moved_0x06;
        arrow_mode = 0;
    } else {
        active = 0;
        flags = 0;
        arrow_mode = 0x80;
    }
    get_lsp_data(0x13D4, &base);
    draw_sprite_ary(eft052_hold_frame_sprites, &base);
    draw_sprite_idx(block->entry_0x38.sprite_0x0E, &base);
    if (block->rows_0x08 != 0)
        PutPageArrow(eft052_hold_arrow_sprites, block->page_0x02, block->pages_0x04, flags, &base, arrow_mode);
    spr_data_copy((_SPR_DATA_*)&spr, (const _SPR_DATA_*)get_lsp_data(0x13D4, &base));
    if (block->rows_0x08 == 0) {
        spr_data_copy((_SPR_DATA_*)&spr, (const _SPR_DATA_*)get_lsp_data(0x13DC, NULL));
        pos.x = menu_text_block_center_x((char*)LbStr(3, block->entry_0x38.title_0x08),
                                         (s16)(spr.x + ((spr.width >> 1) + base.x)), 18, &lines);
        pos.y = (s16)(spr.y + ((spr.height >> 1) + base.y)) - (s16)(lines * 9);
        font_set_size(18, 18);
        font_print_ex(pos.x, pos.y, 2, (s8*)LbStr(3, block->entry_0x38.title_0x08));
        return;
    }
    for (i = 0, lsp = eft052_hold_row_sprites; i < block->rows_0x08; i++, lsp++) {
        cell = (EftLspRect*)get_lsp_data(*lsp, NULL);
        pos.x = base.x + cell->x;
        pos.y = base.y + cell->y;
        if (i == block->row_0x01) {
            menu_hold_row_draw_by_lsp(0x13F6, (u16*)&block->rows_0x10[i]);
            picked = 1;
        } else {
            picked = 0;
        }
        enabled = 1;
        if (block->entry_0x38.kind_0x18 == 2) {
            room = eft052_page_count_ck(block->rows_0x10[i].id_0x00, 1);
            if (room < 0)
                room = 0;
            if (room <= 0)
                enabled = 0;
        }
        marked = 0;
        if (block->entry_0x38.highlight_0x10 != 0 && block->entry_0x38.highlight_0x10 == block->rows_0x10[i].id_0x00)
            marked = 1;
        lb_item_row_draw(block->rows_0x10[i].id_0x00, block->rows_0x10[i].count_0x02, (s32)&pos, enabled, picked, active, marked, 0);
    }
}

/* 0x80359B00 (0xE0): Draws the hold strip and the help lines of its mode (the count step's line only while idle). */
extern "C" void eft052_hold_draw(s32 busy)
{
    CockpitHoldBlock* block = &lbl_806BF368;

    switch (block->mode_0x00) {
    case 0:
        eft052_hold_list_draw();
        if (block->entry_0x38.msg_0x04 != -1) {
            lb_panel_msg_draw(0x1877, block->entry_0x38.msg_0x04);
            if (block->entry_0x38.msg2_0x06 != -1)
                lb_panel_str_print(0x1877, block->entry_0x38.msg2_0x06, 5);
        }
        break;
    case 1:
        eft052_hold_list_draw();
        if (block->entry_0x38.count_msg_0x0A != -1 && (u8)busy == 0) {
            lb_panel_msg_draw(0x1877, block->entry_0x38.count_msg_0x0A);
            lb_count_window_draw(0x14CA, 0x14D5, block->amount_0x0A, (s16)block->max_0x0C,
                        item_price_get(block->rows_0x10[block->row_0x01].id_0x00), 1, block->moved_0x06);
        }
        break;
    }
}

/* 0x80359BE0 (0xC8): Returns the summed counts of `n` box cells. */
extern "C" s32 eft052_box_count_sum(Eft052BoxCell* cells, s32 n)
{
    s32 sum = 0;
    s32 i;

    for (i = 0; i < n; i++, cells++)
        sum += cells->count_0x02;
    return sum;
}

/* 0x80359CA8 (0xA4): Opens the item box on `cells` (with the sort buffer `sorted`) and the window data `grid`;
 * an empty box starts on the leave option. */
extern "C" void eft052_box_open(Eft052BoxCell* cells, Eft052BoxCell* sorted, Eft052BoxGrid* grid)
{
    Eft052ItemBox* box = &eft052_item_box;

    memset(box, 0, sizeof(Eft052ItemBox));
    eft052_box_grid_copy(&box->grid_0x38, grid);
    box->cells_0x24 = cells;
    box->sorted_0x28 = sorted;
    box->cell_count_0x2C = grid->columns_0x00 * grid->rows_0x02;
    box->total_0x1C = eft052_box_count_sum(cells, box->cell_count_0x2C);
    if (box->total_0x1C == 0)
        box->menu_0x02 = 2;
}

/* 0x80359D4C (0x4C): Copies the window data field by field. */
extern "C" void eft052_box_grid_copy(Eft052BoxGrid* dst, Eft052BoxGrid* src)
{
    dst->columns_0x00 = src->columns_0x00;
    dst->rows_0x02 = src->rows_0x02;
    dst->field_0x04 = src->field_0x04;
    dst->labels_0x08 = src->labels_0x08;
    dst->help_0x14 = src->help_0x14;
}

/* 0x80359D98 (0xC): Opens the item hand-over window on the request's cells, with no sort buffer. */
extern "C" void eft052_item_get_open(Eft052ItemGetReq* req, u8* data)
{
    eft052_box_open((Eft052BoxCell*)req, NULL, (Eft052BoxGrid*)data);
}

/* 0x80359DA4 (0x8C): Returns the box's worth: each cell's item value times its count. */
extern "C" s32 eft052_box_value_get(Eft052ItemBox* box)
{
    s32 sum = 0;
    s32 i;

    for (i = 0; i < box->cell_count_0x2C; i++)
        sum += GetItemData(box->cells_0x24[i].item_0x00)->field_0x010 * box->cells_0x24[i].count_0x02;
    return sum;
}

/* 0x80359E30 (0x50): Recounts the box; an emptied box returns to the menu on the leave option. */
extern "C" void eft052_box_recount(Eft052ItemBox* box)
{
    box->total_0x1C = eft052_box_count_sum(box->cells_0x24, box->cell_count_0x2C);
    if (box->total_0x1C == 0) {
        box->menu_0x02 = 2;
        box->state_0x00 = 0;
    }
}

/* 0x80359E80 (0x54): Sells the whole box: adds its worth to the money and empties the cells. */
extern "C" void eft052_box_sell(Eft052ItemBox* box)
{
    score_add_clamped(eft052_box_value_get(box), &lobby_world_block->money_0x0018);
    sysSE_req(9);
    memset(box->cells_0x24, 0, box->cell_count_0x2C * sizeof(Eft052BoxCell));
}

/* 0x80359ED4 (0xE0): Merges one cell into the `n` cells of `dst` (stacks of at most 99, then free cells); returns
 * the count left over. */
extern "C" u16 eft052_box_cell_merge(Eft052BoxCell* src, Eft052BoxCell* dst, s16 n)
{
    u16 left;
    s32 i;
    Eft052BoxCell* cell;
    u32 room;

    if (src->item_0x00 == 0)
        return 0;
    cell = dst;
    left = src->count_0x02;
    for (i = 0; i < n; i++, cell++) {
        if (cell->item_0x00 == src->item_0x00 && cell->count_0x02 < 99) {
            room = 99 - cell->count_0x02;
            if (room >= left) {
                cell->count_0x02 += (s16)left;
                return 0;
            }
            left -= (u16)room;
            cell->count_0x02 = 99;
        }
    }
    for (i = 0, cell = dst; i < n; i++, cell++) {
        if (cell->item_0x00 == 0) {
            cell->item_0x00 = src->item_0x00;
            if (left <= 99) {
                cell->count_0x02 = (s16)left;
                return 0;
            }
            cell->count_0x02 = 99;
            left -= 99;
        }
    }
    return left;
}

/* 0x80359FB4 (0x80): Builds the sorted copy of the box: every cell merged into the cleared sort buffer. */
extern "C" void eft052_box_sort(Eft052ItemBox* box)
{
    s16 i;

    memset(box->sorted_0x28, 0, box->cell_count_0x2C * sizeof(Eft052BoxCell));
    for (i = 0; i < box->cell_count_0x2C; i++)
        eft052_box_cell_merge(&box->cells_0x24[i], box->sorted_0x28, box->cell_count_0x2C);
}

/* 0x8035A034 (0x694): One step of the item hand-over window: the menu (take, store, leave), moving one cell to the
 * pouch or the box, the sort question and the leave question; 1 once the window closes. */
extern "C" s32 eft052_item_get_step(void)
{
    Eft052ItemBox* box = &eft052_item_box;
    s32 done = 0;
    s32 cell;
    s32 room;
    u8 tmp[8];

    if (++box->frame_0x08 > 40)
        box->frame_0x08 = 0;
    if (box->picked_0x0C != 0) {
        if (++box->frame_0x0A > 12) {
            box->frame_0x0A = 0;
            box->picked_0x0C = 0;
        }
    }
    switch (box->state_0x00) {
    case 0:
        if (lb_cmd_pressed_ck(0x10)) {
            switch (box->menu_0x02) {
            case 0:
                box->state_0x00 = 1;
                sysSE_req(5);
                break;
            case 2:
                if (box->total_0x1C != 0) {
                    box->state_0x00 = 3;
                    box->confirm_0x18 = 0;
                    sysSE_req(0);
                } else {
                    done = 1;
                    sysSE_req(0);
                }
                break;
            case 1:
                box->state_0x00 = 2;
                sysSE_req(5);
                break;
            }
        } else if (lb_cmd_pressed_ck(0x20)) {
            if (box->menu_0x02 != 2) {
                box->menu_0x02 = 2;
                sysSE_req(1);
            } else if (box->total_0x1C == 0) {
                done = 1;
                sysSE_req(1);
            }
        } else if (lb_cmd_repeat_ck(3)) {
            if (box->total_0x1C != 0 || box->menu_0x02 != 2)
                box->menu_0x02 = menu_cursor_step(box->menu_0x02, 3, lb_cmd_repeat_get(), 1, 2);
        }
        break;
    case 1:
        if (lb_cmd_pressed_ck(0x10)) {
            cell = box->column_0x01 + box->row_0x12 * box->grid_0x38.columns_0x00;
            room = fn_8004B624(lobby_world_block, box->cells_0x24[cell].item_0x00);
            if (room > 0 && box->cells_0x24[cell].count_0x02 != 0) {
                if (room < box->cells_0x24[cell].count_0x02) {
                    userdata_item_give(lobby_world_block, box->cells_0x24[cell].item_0x00, room, 1);
                    box->cells_0x24[cell].count_0x02 -= (s16)room;
                } else {
                    userdata_item_give(lobby_world_block, box->cells_0x24[cell].item_0x00,
                                       box->cells_0x24[cell].count_0x02, 1);
                    box->cells_0x24[cell].item_0x00 = 0;
                    box->cells_0x24[cell].count_0x02 = 0;
                }
                box->picked_0x0C = 1;
                box->frame_0x0A = 0;
                box->cursor_0x0E = cell;
                sysSE_req(8);
            } else {
                sysSE_req(2);
            }
            eft052_box_recount(box);
        } else if (lb_cmd_pressed_ck(0x20)) {
            box->state_0x00 = 0;
            sysSE_req(1);
        } else if (lb_cmd_pressed_ck(0x40)) {
            if (box->sorted_0x28 != NULL) {
                box->state_0x00 = 4;
                box->confirm_0x18 = 0;
                eft052_box_sort(box);
                sysSE_req(0xD);
            }
        } else if (lb_cmd_repeat_ck(0xC)) {
            box->column_0x01 = menu_cursor_step(box->column_0x01, box->grid_0x38.columns_0x00, lb_cmd_repeat_get(), 4, 8);
            box->hover_0x10 = box->column_0x01 + box->row_0x12 * box->grid_0x38.columns_0x00;
        } else if (lb_cmd_repeat_ck(3)) {
            box->row_0x12 = menu_cursor_step(box->row_0x12, box->grid_0x38.rows_0x02, lb_cmd_repeat_get(), 1, 2);
            box->hover_0x10 = box->column_0x01 + box->row_0x12 * box->grid_0x38.columns_0x00;
        }
        break;
    case 2:
        if (lb_cmd_pressed_ck(0x10)) {
            cell = box->column_0x01 + box->row_0x12 * box->grid_0x38.columns_0x00;
            room = item_slots_room_get(box->cells_0x24[cell].item_0x00, &lobby_world_block->field_0x0180,
                                       userdata_box_capacity(lobby_world_block));
            if (room > 0 && box->cells_0x24[cell].count_0x02 != 0) {
                if (room < box->cells_0x24[cell].count_0x02) {
                    item_box_store(box->cells_0x24[cell].item_0x00, room, tmp);
                    box->cells_0x24[cell].count_0x02 -= (s16)room;
                } else {
                    item_box_store(box->cells_0x24[cell].item_0x00, box->cells_0x24[cell].count_0x02, tmp);
                    box->cells_0x24[cell].item_0x00 = 0;
                    box->cells_0x24[cell].count_0x02 = 0;
                }
                box->picked_0x0C = 1;
                box->frame_0x0A = 0;
                box->cursor_0x0E = cell;
                sysSE_req(8);
            } else {
                sysSE_req(2);
            }
            eft052_box_recount(box);
        } else if (lb_cmd_pressed_ck(0x20)) {
            box->state_0x00 = 0;
            sysSE_req(1);
        } else if (lb_cmd_pressed_ck(0x40)) {
            if (box->sorted_0x28 != NULL) {
                box->state_0x00 = 4;
                box->confirm_0x18 = 0;
                eft052_box_sort(box);
                sysSE_req(0xD);
            }
        } else if (lb_cmd_repeat_ck(0xC)) {
            box->column_0x01 = menu_cursor_step(box->column_0x01, box->grid_0x38.columns_0x00, lb_cmd_repeat_get(), 4, 8);
            box->hover_0x10 = box->column_0x01 + box->row_0x12 * box->grid_0x38.columns_0x00;
        } else if (lb_cmd_repeat_ck(3)) {
            box->row_0x12 = menu_cursor_step(box->row_0x12, box->grid_0x38.rows_0x02, lb_cmd_repeat_get(), 1, 2);
            box->hover_0x10 = box->column_0x01 + box->row_0x12 * box->grid_0x38.columns_0x00;
        }
        break;
    case 3:
        switch (toggle_word_step_dpad(&box->confirm_0x18, lobby_cmd_trig_get(), 4, 8)) {
        case 1:
            done = 1;
            eft052_box_sell(box);
            break;
        case 2:
            box->state_0x00 = 0;
            break;
        }
        break;
    case 4:
        switch (lb_yes_no_step(&box->confirm_0x18)) {
        case 1:
            memcpy(box->cells_0x24, box->sorted_0x28, box->cell_count_0x2C * sizeof(Eft052BoxCell));
        case 2:
            switch (box->menu_0x02) {
            case 0:
                box->state_0x00 = 1;
                break;
            case 1:
                box->state_0x00 = 2;
                break;
            }
            break;
        }
        break;
    }
    return done;
}

/* 0x8035A6C8 (0x110): Draws the box menu at layout `lsp`: its frame and the three options (leave greys out while the
 * box is empty, and shows the close line). */
extern "C" void eft052_box_menu_draw(Eft052ItemBox* box, u16 lsp, const u16* frame, const u16* rows, u8 flag)
{
    Eft052BoxGrid* grid = &box->grid_0x38;
    s32 i;
    s8* text;
    bool enabled;
    _mh_ivec2_ base;
    u32 color;
    bool active;
    bool picked;

    get_lsp_data(lsp, &base);
    draw_sprite_ary(frame, &base);
    active = box->state_0x00 == 0;
    for (i = 0; i < 3; i++) {
        picked = i == box->menu_0x02;
        if (box->total_0x1C != 0 || i == 2)
            enabled = true;
        else
            enabled = false;
        color = GetMenuFontColor(enabled, picked, active, 0);
        if (!active)
            picked = 0;
        if (box->total_0x1C == 0 && i == 2)
            text = (s8*)get_str_tbl(31)[8];
        else
            text = grid->labels_0x08.text[i];
        lb_frame_draw_at(lsp, rows[i], text, picked, color, flag);
    }
}

/* 0x8035A7D8 (0x20C): Draws the item box: the menu, or the pointed cell; then the help line of the state. */
extern "C" void eft052_box_list_draw(u16 panel, const u16* sprites, const u16* rows, u16 help, u8 flag)
{
    Eft052ItemBox* box = &eft052_item_box;
    Eft052BoxGrid* grid = &box->grid_0x38;
    _mh_ivec2_ pos;
    s32 room;

    switch (box->state_0x00) {
    case 0:
    case 3:
        eft052_box_menu_draw(box, panel, sprites, rows, flag);
        break;
    default:
        menu_hold_row_draw_by_lsp(help, (u16*)&box->cells_0x24[box->hover_0x10]);
        if (lobby_w.state_0x000 == 0x17 && userdata_box_item_ck(lobby_world_block, box->cells_0x24[box->hover_0x10].item_0x00))
            get_lsp_data(help, &pos);
        break;
    }
    if (box->total_0x1C != 0) {
        switch (box->state_0x00) {
        case 0:
            lb_help_text_put(0x1878, grid->help_0x14.text[box->menu_0x02]);
            break;
        case 3:
            lb_help_num_put(0x1878, 0x37, eft052_box_value_get(box));
            lb_panel_yes_no_draw(0x1893, box->confirm_0x18);
            break;
        case 4:
            lb_panel_msg_draw(0x1878, 0xBC);
            lb_panel_yes_no_draw(0x1893, box->confirm_0x18);
            break;
        default:
            lb_help_text_put(0x1878, (s8*)LbStr(3, box->menu_0x02 + 0x1DC));
            if (box->cells_0x24[box->hover_0x10].item_0x00 != 0) {
                room = eft052_page_count_ck(box->cells_0x24[box->hover_0x10].item_0x00, box->menu_0x02);
                if (room <= 0) {
                    if (box->menu_0x02 == 0) {
                        if (room < 0)
                            lb_panel_line_draw(0x1878, 0x2F, 2, 2);
                        else
                            lb_panel_line_draw(0x1878, 0x1DF, 2, 2);
                    } else {
                        lb_panel_line_draw(0x1878, 0x30, 2, 2);
                    }
                }
            }
            break;
        }
    } else {
        lb_panel_msg_draw(0x1878, 0x1DE);
    }
}

#pragma fp_contract off

/* 0x8035A9E4 (0x16C): Allocates and clears the HUD message work: sixteen free entries, each keeping its own id, and an
 * empty queue. */
extern "C" void hud_msg_init(void)
{
    HudMsgWork* work = (HudMsgWork*)work_mem_alloc(sizeof(HudMsgWork));
    s32 i;

    hud_msg_work = work;
    memset(work, 0, sizeof(HudMsgWork));
    for (i = 0; i < 16; i++) {
        memset(&work->msgs_0x0C[i], 0, sizeof(HudMsg));
        work->msgs_0x0C[i].id_0x02 = (u8)i;
        work->msgs_0x0C[i].sub_on_0x03 = 0;
        work->queue_0xBCC[i] = NULL;
    }
    work->field_0x04 = 0;
    work->count_0x08 = 0;
    work->field_0xC1C = 0;
    work->field_0xC20 = 0;
    work->field_0xC24 = 0;
    for (i = 0; i < 16; i++) {
        work->field_0xC28[i] = 0;
        work->field_0xC48[i] = 0;
        work->slots_0xC68[i].used_0x00 = 0;
    }
}

/* 0x8035AB50 (0x14): Runs the HUD messages, once the work exists. */
extern "C" void hud_msg_frame(void)
{
    if (hud_msg_work != NULL)
        hud_msg_update(hud_msg_work);
}

/* 0x8035AB64 (0xB4): Returns a free message entry (NULL when all sixteen are taken or the work is missing). */
extern "C" HudMsg* hud_msg_alloc(void)
{
    HudMsg* msg;
    s32 i;

    if (hud_msg_work == NULL)
        return NULL;
    for (msg = hud_msg_work->msgs_0x0C, i = 0; i < 16; msg++, i++) {
        if (msg->state_0x00 == 0)
            return msg;
    }
    return NULL;
}

/* 0x8035AC18 (0x190): Takes `msg` off the queue (closing the gap), clears it and drops the queued count. */
extern "C" void hud_msg_free(HudMsg* msg)
{
    HudMsgWork* work = hud_msg_work;
    s32 i;
    HudMsg** queue;

    if (work == NULL)
        return;
    for (i = 0, queue = work->queue_0xBCC; i < 16; i++, queue++) {
        if (*queue == msg)
            break;
    }
    if (i >= 16)
        return;
    for (; i < 15; i++, queue++)
        queue[0] = queue[1];
    *queue = NULL;
    memset(msg, 0, sizeof(HudMsg));
    work->count_0x08--;
}

/* 0x8035ADA8 (0xF0): Puts `msg` on the first free queue place; 0 when the queue is full or the work is missing. */
extern "C" s32 hud_msg_enqueue(HudMsg* msg)
{
    HudMsgWork* work = hud_msg_work;
    HudMsg** queue;
    s32 i;

    if (work == NULL)
        return 0;
    for (i = 0, queue = work->queue_0xBCC; i < 16; i++, queue++) {
        if (*queue == NULL)
            break;
    }
    if (i >= 16)
        return 0;
    *queue = msg;
    work->count_0x08++;
    return 1;
}

/* 0x8035AE98 (0x4C): Returns the queued message with id `id`, or NULL. */
extern "C" HudMsg* hud_msg_find(u8 id)
{
    HudMsgWork* work = hud_msg_work;
    HudMsg** queue;
    s32 i;

    if (work == NULL)
        return NULL;
    for (i = 0, queue = work->queue_0xBCC; i < work->count_0x08; i++, queue++) {
        if ((*queue)->id_0x02 == id)
            return *queue;
    }
    return NULL;
}

/* 0x8035AEE4 (0x35C): Steps one message: lays it out centred above the screen, slides it down, holds it (shorter
 * while others wait), then slides it back up and marks it done. */
extern "C" void hud_msg_step(HudMsg* msg)
{
    HudMsgWork* work = hud_msg_work;
    _MH_VEC2 screen;
    s32 len;
    f32 width;
    f32 y;

    switch (msg->phase_0x04) {
    case 0:
        get_ScreenSize(&screen);
        msg->font_w_0x28 = msg->font_h_0x2A = 18;
        if (msg->kind_0x01 == 5)
            len = note_text_max_len(18, msg->text_0x34);
        else
            len = flfntStrLen(msg->text_0x34);
        width = (f32)len * ((f32)msg->font_w_0x28 / 2.0f);
        msg->half_w_0x20 = width / 2.0f;
        msg->margin_0x22 = 8;
        msg->height_0x26 = msg->font_h_0x2A;
        if (msg->kind_0x01 == 5)
            msg->height_0x26 += msg->height_0x26;
        msg->width_0x24 = width;
        msg->x_0x18 = screen.x / 2.0f - (f32)msg->half_w_0x20;
        msg->y_0x1C = (f32)(-msg->height_0x26);
        if (msg->sub_on_0x03 != 0) {
            len = flfntStrLen(msg->sub_0x7C);
            if (len > 0) {
                width = (f32)len * ((f32)msg->font_w_0x28 / 2.0f);
                msg->sub_h_0x7A = msg->height_0x26;
                msg->sub_w_0x78 = msg->width_0x24;
                msg->sub_x_0x74 = screen.x / 2.0f - width / 2.0f;
                msg->sub_y_0x76 = -msg->height_0x26;
            } else {
                msg->sub_on_0x03 = 0;
            }
        }
        msg->timer_0x08 = 60;
        msg->speed_0x30 = 48.0f;
        msg->stop_y_0x2E = screen.y / 2.0f - (f32)msg->height_0x26;
        msg->phase_0x04++;
        sysSE_req(4);
        if (msg->kind_0x01 == 1)
            se_req_item_jingle();
        msg->state_0x00 = 2;
    case 1:
        y = msg->y_0x1C + msg->speed_0x30;
        if ((s16)y >= msg->stop_y_0x2E) {
            msg->y_0x1C = (f32)msg->stop_y_0x2E;
            msg->speed_0x30 = msg->speed_0x30 * -1.0f;
            msg->phase_0x04++;
            if (msg->kind_0x01 == 1)
                se_req_item_jingle();
        } else {
            msg->y_0x1C = y;
        }
        break;
    case 2:
        if (--msg->timer_0x08 <= 0 || (msg->timer_0x08 <= 30 && work->count_0x08 > 0))
            msg->phase_0x04++;
        break;
    case 3:
        y = msg->y_0x1C + msg->speed_0x30;
        if ((s16)y < -msg->height_0x26)
            msg->state_0x00 = 3;
        else
            msg->y_0x1C = y;
        break;
    }
}

/* 0x8035B240 (0x140): Steps the head of the queue, frees the finished messages, lists the shown ids and queues the
 * draw. */
extern "C" void hud_msg_update(HudMsgWork* work)
{
    s32 i;
    HudMsg* msg;
    HudMsg** queue;

    for (queue = work->queue_0xBCC, i = 0; i < work->count_0x08; queue += 2, i++) {
        if (*queue != NULL) {
            hud_msg_step(*queue);
            break;
        }
    }
    for (msg = work->msgs_0x0C, i = 0; i < 16; msg++, i++) {
        if (msg != NULL && msg->state_0x00 == 3)
            hud_msg_free(msg);
    }
    for (i = 0; i < 16; i++)
        work->shown_0xC0C[i] = -1;
    for (i = 0; i < work->count_0x08; i++) {
        if (work->queue_0xBCC[i] != NULL && work->queue_0xBCC[i]->state_0x00 == 2)
            work->shown_0xC0C[i] = (s8)work->queue_0xBCC[i]->id_0x02;
    }
    subTransSetPrio(8, (u32)hud_msg_draw, 0, 0);
}

/* 0x8035B380 (0xB0): Takes a free entry for text `text` of kind `kind` and queues it; NULL when none is free. */
extern "C" HudMsg* hud_msg_open(s32 kind, const char* text)
{
    HudMsg* msg;

    if (hud_msg_work == NULL)
        return NULL;
    msg = hud_msg_alloc();
    if (msg == NULL)
        return NULL;
    if (!hud_msg_enqueue(msg)) {
        hud_msg_free(msg);
        return NULL;
    }
    msg->state_0x00 = 1;
    msg->kind_0x01 = kind;
    strcpy((s8*)msg->text_0x34, (const s8*)text);
    msg->sub_on_0x03 = 0;
    msg->sub_0x7C[0] = 0;
    return msg;
}

/* 0x8035B430 (0x1CC): Draws every shown message (and its second line once it holds). */
extern "C" void hud_msg_draw(void)
{
    HudMsgWork* work = hud_msg_work;
    HudMsg* msg;
    s32 i;

    if (work == NULL)
        return;
    font_flush();
    for (i = 0; i < work->count_0x08; i++) {
        msg = work->queue_0xBCC[i];
        if (msg != NULL && msg->state_0x00 == 2) {
            font_color_set(0);
            font_set_size(msg->font_w_0x28, msg->font_h_0x2A);
            font_locate((s16)msg->x_0x18, (s16)msg->y_0x1C);
            flfntFontPuts(msg->text_0x34);
            if (msg->kind_0x01 == 5) {
                note_text_draw((s16)msg->x_0x18, (s16)msg->y_0x1C, msg->font_w_0x28, msg->text_0x34);
            } else {
                note_line_draw((s16)msg->x_0x18, (s16)msg->y_0x1C, msg->font_w_0x28, msg->text_0x34);
                if (msg->phase_0x04 == 2 && msg->sub_on_0x03 != 0) {
                    msg->sub_y_0x76 = msg->y_0x1C + (f32)(msg->height_0x26 * 2);
                    font_locate(msg->sub_x_0x74, msg->sub_y_0x76);
                    flfntFontPuts(msg->sub_0x7C);
                    note_line_draw(msg->sub_x_0x74, msg->sub_y_0x76, msg->font_w_0x28, msg->sub_0x7C);
                }
            }
        }
    }
    font_flush();
}

/* 0x8035B5FC (0x4C): Queues text `text` of kind `ch` and returns the new entry's id (-1 when none is free). */
extern "C" s32 hud_msg_push(u32 ch, const char* text)
{
    HudMsg* msg;

    if (hud_msg_work == NULL)
        return -1;
    msg = hud_msg_open(ch, text);
    if (msg == NULL)
        return -1;
    return (s8)msg->id_0x02;
}

/* 0x8035B648 (0xB8): Queues text `text` of kind `kind` with room for a second line; returns its id or -1. */
extern "C" s32 hud_msg_push_two_line(s32 kind, const char* text)
{
    HudMsg* msg;

    if (hud_msg_work == NULL)
        return -1;
    msg = hud_msg_alloc();
    if (msg == NULL)
        return -1;
    if (!hud_msg_enqueue(msg)) {
        hud_msg_free(msg);
        return -1;
    }
    msg->state_0x00 = 1;
    msg->kind_0x01 = kind;
    strcpy((s8*)msg->text_0x34, (const s8*)text);
    msg->sub_on_0x03 = 2;
    msg->sub_0x7C[0] = 0;
    return (s8)msg->id_0x02;
}

/* 0x8035B700 (0x178): Builds one item message line from the string table (`kind` 1 plain, 2 with the item name, 3
 * after a hunter's name, 4 with a number) and queues it; returns its id or -1. */
extern "C" s32 hud_item_msg_push(s32 kind, s32 id, u16 arg)
{
    char name[0x20];
    char line[0x100];
    char** table = (char**)get_str_tbl(4);
    s32 result = -1;

    if (hud_msg_work == NULL)
        return result;
    switch (kind) {
    case 1:
        strcpy((s8*)line, (const s8*)table[id]);
        result = hud_msg_push(0, line);
        break;
    case 2:
        if (arg == 0) {
            strcpy((s8*)line, (const s8*)table[9]);
        } else {
            strcpy((s8*)name, (const s8*)ItemName(arg));
            sprintf((s8*)line, table[id], name);
        }
        result = hud_msg_push(0, line);
        if (id == 18)
            se_slot_req(2);
        break;
    case 3:
        strcpy((s8*)line, (const s8*)((u8*)get_move_work_adrs(2) + arg * 0xB20 + 0x5CA));
        strcat((char*)line, (const char*)table[id]);
        result = hud_msg_push(0, line);
        break;
    case 4:
        sprintf((s8*)line, "%d%s", arg, table[id]);
        result = hud_msg_push(0, line);
        break;
    }
    return result;
}

/* 0x8035B878 (0x104): Gives the queued message `id` its second line `text`, centred under it; returns the id or -1. */
extern "C" s32 hud_msg_sub_set(u8 id, const char* text)
{
    HudMsg* msg = hud_msg_find(id);
    s32 len;
    _MH_VEC2 screen;
    f32 width;

    if (msg == NULL)
        return -1;
    msg->sub_on_0x03 = 1;
    strcpy((s8*)msg->sub_0x7C, (const s8*)text);
    len = flfntStrLen(msg->sub_0x7C);
    if (len > 0) {
        get_ScreenSize(&screen);
        width = (f32)len * ((f32)msg->font_w_0x28 / 2.0f);
        msg->sub_h_0x7A = msg->height_0x26;
        msg->sub_w_0x78 = msg->width_0x24;
        msg->sub_x_0x74 = screen.x / 2.0f - width / 2.0f;
        msg->sub_y_0x76 = -msg->height_0x26;
    }
    return (s8)msg->id_0x02;
}

/* 0x8035B97C (0x1C): Returns the id of the first shown message (0xFF when none), -1 without the work. */
extern "C" s32 hud_msg_shown_id_get(void)
{
    if (hud_msg_work == NULL)
        return -1;
    return (u8)hud_msg_work->shown_0xC0C[0];
}

/* 0x8035B998 (0x11C): Plays the effect's sound while the player stands in its area: the tag-3 splash (one of one to
 * four), and the tag-10/14 shell sounds of their types. */
extern "C" void eft_area_se_req(_EFT* self)
{
    if (self->area_0x44 != get_now_areano())
        return;
    switch (self->field_0x03) {
    case 3:
        switch (self->demo_flag_0x08) {
        case 1:
            se_req_splash(0, &self->pos_0x18);
            break;
        case 2:
            se_req_splash((ran_suu(0) & 1) + 1, &self->pos_0x18);
            break;
        case 3:
            se_req_splash((ran_suu(0) & 3) + 1, &self->pos_0x18);
            break;
        }
        break;
    case 10:
        if ((u8)(self->type_0x02 - 9) <= 1)
            shell_se_req(((_ENEMY_WORK*)self->source_0x30)->se_0xB14, &self->pos_0x18, 2, (u32)self);
        break;
    case 14:
        if (self->type_0x02 == 1)
            shell_se_req(((_ENEMY_WORK*)self->source_0x30)->se_0xB14, &self->pos_0x18, 2, (u32)self);
        break;
    }
}
