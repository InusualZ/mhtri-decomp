/*
 * ef/eft046.cpp - the eft046 `_EFT` effect family (`_EFT::field_0x03 = 0x2E`), the menu target marker: one model
 *   that follows a player's joint and bobs, dispatched on `state_0x05` with its `+0x38` work record (`MenuFxWorkB`).
 * RANGE. .text 0x8031F304-0x8031F77C (7 functions); extab 0x8001609C-0x800160BC, extabindex 0x80034F50-0x80034F80,
 *   .sdata2 0x8079AED8-0x8079AEF8.  One TU: its pool (10.0f, 50.0f, 200.0f, 2*pi, 65536.0f, the int-to-float double)
 *   is loaded only by `menu_target_marker_start`/`menu_target_marker_follow`, and the extabindex records 0x80034F50
 *   and 0x80034F80 name 0x8031F304 and 0x8031F77C as the first functions of this unit and the next.
 * FLAGS. `cflags_menu` (configure.py, the group of the range it was cut from); file-scope `#pragma pool_data off`,
 *   `#pragma fp_contract off` and `#pragma optimization_level 4`.
 * NAMES. The file name is a GUESS from the `field_0x03` family tag (sibling scheme `ef/eft044.cpp`); no `__FILE__`
 *   string covers the range.  GUESS: the `menu_target_marker_*` family, `MenuFxWorkB`, `MenuPlJoints`.
 * RESIDUALS. none known.
 */

#include "ef/eft046.h"
#include "ef/eft_state_flags_set.h" /* eft_state_flags_set (rule 2: the owner's header) */
#include "ef/eft_rot_vec_copy.h" /* eft_rot_vec_copy (rule 2: the owner's header) */
#include "types.h"
#include "lobby/lb_npc.h" /* lb_party_state_reset (rule 2: its owner) */
#include "ef/eft_res.h" /* eft_res_slot_get / eft_res_models_spawn / res_eft_model_create */
#include "lobby/menu_scratch.h" /* LbMenuScratch */
#include "lobby/lb_npc_model_height_get.h"
#include "lobby/lb_menu_pos_tbl.h" /* getItemListSelection */
#include "quest/quest_entry.h" /* quest_record_find / quest_result_field_text_get */
#include "ef/math_sin_rad.h"
/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_state_flags_set_c1 ((void (*)(_EFT*, u32, u32))eft_state_flags_set)

/* retail addresses each table of this file with its own lis/addi pair, and keeps every float multiply and add
 * apart (fmuls + fadds, no fmadds) */
#pragma pool_data off
#pragma fp_contract off
#pragma optimization_level 4

/* This unit's own forward declarations (defined below). */
extern "C" {
void menu_target_marker_release(_EFT* self);
void menu_target_marker_dispatch(_EFT* self);
void menu_target_marker_start(_EFT* self);
void menu_target_marker_follow(_EFT* self);
void menu_target_marker_state_next(_EFT* self);
void menu_target_marker_retire(_EFT* self);
} /* extern "C" */

/* The mangled callees (real C++ signatures - rule 9). */
void setVector3(nw4r::math::VEC3* v, f32 x, f32 y, f32 z);

/* 0x8031F304 - spawn the second effect family over `arg`. */
extern "C" void menu_target_marker_set(void* arg) {
    _EFT* self = (_EFT*)eft_res_slot_get(0xC);
    MenuFxWork* work;
    s32 i;

    if (self == NULL) {
        return;
    }

    work = (MenuFxWork*)self->work_0x38;
    work->count = 1;
    ((MenuFxWorkB*)work)->bob_phase_0x08 = 0;
    self->type_0x02 = 0;
    self->release_0x40 = menu_target_marker_release;
    self->dispatch_0x34 = menu_target_marker_dispatch;

    for (i = 0; i < work->count; i++) {
        work->models[i] = (MHchar*)eft_res_model_get();
        if (work->models[i] == NULL) {
            eft_res_slot_release(self);
            return;
        }
    }

    self->rot_0x24.x = 0;
    self->rot_0x24.y = 0;
    self->rot_0x24.z = 0;
    eft_state_flags_set_c1(self, 1, 0);
    self->field_0x03 = 0x2E;
    self->source_0x30 = arg;
}

/* 0x8031F3E4 - release the second family's model list. */
extern "C" void menu_target_marker_release(_EFT* self) {
    MenuFxWork* work = (MenuFxWork*)self->work_0x38;

    eft_res_models_release(&work->models[0], work->count);
    work->count = 0;
}

/* 0x8031F420 - the second family's four-state dispatch on `state_0x05`. */
extern "C" void menu_target_marker_dispatch(_EFT* self) {
    switch (self->state_0x05) {
    case 0:
        menu_target_marker_start(self);
        return;
    case 1:
        menu_target_marker_follow(self);
        return;
    case 2:
        menu_target_marker_state_next(self);
        return;
    case 3:
        menu_target_marker_retire(self);
        return;
    }
}

/* 0x8031F45C - the second family's state 0: create each model and seed its scale. */
extern "C" void menu_target_marker_start(_EFT* self) {
    s32 i;
    MenuFxWork* work = (MenuFxWork*)self->work_0x38;

    self->state_0x05++;

    for (i = 0; i < work->count; i++) {
        if (res_eft_model_create(work->models[i], 0x3B, 0) == NULL) {
            menu_target_marker_retire(self);
            return;
        }
        setVector3(&work->models[i]->scale_0x1C, 10.0f, 10.0f, 10.0f);
    }

    menu_target_marker_follow(self);
}

/* 0x8031F510 (0x258): the second family's state 1 - floats the marker over the menu's current target (the
 * local player, another player or an NPC) with a sine bob and a spin. */
extern "C" void menu_target_marker_follow(_EFT* self) {
    nw4r::math::VEC3 pos;
    MenuFxWorkB* work;
    _PLW* plw;
    LbMenuScratch* scratch;
    s32 i;

    VEC3_ctor(&pos);
    work = (MenuFxWorkB*)self->work_0x38;
    scratch = (LbMenuScratch*)self->source_0x30;
    plw = scratch->plw_0x000;
    if (GameMode_ck() != 2) {
        self->state_0x05++;
        self->flag_0x01 = 0;
        return;
    }
    self->flag_0x01 = plw->field_0x001;
    self->area_0x44 = plw->area_0x16;
    if (scratch->hidden_0x155 != 0) {
        return;
    }
    switch (scratch->index_0x14C) {
    case -1:
        return;
    case -2:
        if (plw->slot_active == 0) {
            return;
        }
        ((MenuPlJoints*)plw->physics_0x13C)->joint_0x004.get_joint_wpos(13, &pos);
        pos.y += 50.0f;
        break;
    default: {
        LbMenuRow* row = &scratch->rows_0x00C[scratch->index_0x14C];
        _PLW* other = (_PLW*)row->table_0x00;

        if (other != NULL) {
            if (other->slot_active == 0) {
                return;
            }
            copyVec3(&pos, (VEC3*)&other->field_0x03C);
            pos.y += 200.0f;
        } else {
            _LB_NPC* npc = (_LB_NPC*)row->model_0x04;

            if (npc != NULL && npc->field_0x000 != 0) {
                copyVec3(&pos, &npc->pos_0x10);
                pos.y += lb_npc_model_height_get(
                    ((_LB_NPC*)scratch->rows_0x00C[scratch->index_0x14C].model_0x04)->model_0x050->model_0x002);
                pos.y += 50.0f;
            } else {
                return;
            }
        }
        break;
    }
    }
    pos.y += 10.0f * math_sin_rad(6.2831855f * ((work->bob_phase_0x08 * 2) & 0xFFFF) / 65536.0f);
    work->bob_phase_0x08 += 0x200;
    self->rot_0x24.y += 0xFF;
    for (i = 0; i < work->count; i++) {
        copyVec3(&((MenuFxWork*)work)->models[i]->pos_0x04, copyVec3(&self->pos_0x18, &pos));
        eft_rot_vec_copy(&((MenuFxWork*)work)->models[i]->rot_0x28, &self->rot_0x24);
        ((MenuFxWork*)work)->models[i]->move(0);
    }
    eft_res_models_spawn(self, (void**)&work->model_0x04, 2, work->count, 0);
}


/* 0x8031F768 - advance the second family's state. */
extern "C" void menu_target_marker_state_next(_EFT* self) {
    self->state_0x05++;
}

/* 0x8031F778 - retire the second family's effect. */
extern "C" void menu_target_marker_retire(_EFT* self) {
    eft_res_slot_release(self);
}
