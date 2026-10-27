/* ef/eft018.cpp - the player and NPC light effects: the area effect (`eft_spawn_pos_in_area` builds an `_EFT`
 *   record whose `eft_area_effect_*` states create, colour and retire one nw4r effect) and the light family
 *   (`eft_light_spawn` pools the light models, `eft_light_dispatch`/`eft_light_update` run one init and one update
 *   body per variant: 0 the lantern, 2 the dropped light, 3 the NPC's lantern, 4 the lobby lantern).
 * RANGE. .text 0x8010D2B0-0x801121DC (34 functions); extab 0x8000C264-0x8000C32C, extabindex 0x80026280-0x800263AC,
 *   .data 0x8059F588-0x8059F670, .sdata 0x80791828-0x80791840, .sdata2 0x80796848-0x807969D8.
 *   Seam: the three functions before 0x8010D2B0 are called only from `ef/eft016.cpp`, whose object owns them, the
 *   `.sdata2` run 0x80796838..0x80796848 and the `.data` tables through 0x8059F588.
 *   Name: GUESS - the ef units are named by the family tag their allocator stores at `_EFT+0x03`; `eft_light_spawn`
 *   stores 0x12 (and `eft_spawn_pos_in_area` 0x11, riding along as in `ef/eft002.cpp`).
 * FLAGS. `cflags_main`; C++ (every callee it reaches is a C++ mangling - `MHchar` members, nw4r functions,
 *   `Pl_frame_check` - and the target carries extab/extabindex for its functions).  The unit's own definitions are
 *   `extern "C"`: the map has only plain names for them.
 *   `#pragma peephole off` around `eft_spawn_pos_in_area`, `eft_area_effect_create`, `eft_light_spawn`,
 *   `eft_light_release`, `eft_light_drop_init`, `eft_light_trail_update`, `eft_light_lantern_update`,
 *   `eft_light_drop_update`, `eft_light_npc_update`, `eft_light_lobby_update` and `eft_light_ring_colors_get` (retail
 *   keeps the unfused `clrlwi`/`extsb` + `cmpwi` pairs, `rlwinm` + `srawi`, function addresses staged in `r0`, the
 *   loop counter stepped last and the reloads after a store); `#pragma fp_contract off` around
 *   `eft_light_drop_update` (retail keeps `fmuls` + `fadds`); `#pragma optimization_level 4` around
 *   `eft_light_drop_init`, `eft_light_lantern_update` and `eft_light_drop_update` (retail's `subi`/`cmplwi` switch
 *   ranges and prologue-hoisted int-to-float magic, playbook 108).
 * NAMES. GUESS, from each body and its callers: the unit's file name, every `eft_area_effect_*` and `eft_light_*`
 *   function, `eft_spawn_pos_in_area`, and the data `eft_area_effect_groups`, `eft_area_effect_ids`,
 *   `eft_light_colors_a`/`_b`, `eft_light_tint_color`, `eft_light_fade_keys`, `eft_light_model_ids`.  Registry: `se_req_pos_id27`, `se_req_pos_id59`, `eft025_pl_set`,
 *   `lb_pl_act_off_ck` (foreign rows, GUESS for the last two).
 * RESIDUALS. Every row is written.
 *  - `eft_light_lantern_update`: the 40/47 case's yaw (`angle`) and pitch (`beam`) take `r29`/`r30` swapped against
 *    retail (a separate pitch local, a `u32` yaw, the declaration orders and an early `angle` declaration do not
 *    reach it);
 *  - `eft_light_beam_aim_job`: `angle = (u16)(yaw + 0xC000)` builds its `addis`/`subi` in `r3`/`r0` where retail
 *    builds it in `r30` (peephole off, `- 0x4000` and a `(u16)` constant do not reach it);
 *  - `eft_light_dispatch`: retail keeps a `b` stub as case 0's body (the compare branches to it, it jumps to the
 *    type switch after the outer exit); ours branches straight to the type switch (the type switch inside case 0,
 *    an inline init helper and `break` exits measure worse).
 *   flipcheck: `.text` differs (the rows above).
 * SHAPES. `eft_area_effect_create`'s colour split uses explicit `(color & mask) >> shift` terms: `(color >> shift) &
 *   0xFF` folds into one `rlwinm`.  A one-case compare on a `u8`/`u16` value is a `switch` with the other path as
 *   `default` (retail's `cmpwi`); a colour index is a `u8` local (retail's `lbzx` from precomputed offsets).
 *   `eft_spawn_pos_in_area`'s area and kind parameters are `u8`, as are `eft_light_spawn`'s variant; its last two
 *   parameters are `s32` narrowed with `(u8)`: the masks come from the casts.  `eft_light_beam_bind`'s motion is
 *   an `s32` narrowed with `(u16)` (retail's `clrlwi` + `cmpwi`) and its source is loaded first.
 *   `eft_light_motion_part_get` stores its answer in a local through `if`/`else if` chains (the `return` form is
 *   if-converted to `neg`/`or`/`srawi`); `ef_pl_effects_init`'s spawn tests `return` (retail keeps the
 *   `cmpwi`/`bne`); `eft_light_beam_aim_job`'s angle is a `u32` holding `(u16)` values (masked once at the store).
 *   `EftWorkHead`, `EftWorkSlots` and `EftWorkBody` are unions: the variants reuse the same bytes for different
 *   payloads.  `eft_light_lantern_update`'s early exits are `continue` inside one `do { } while (0)` whose end is
 *   the shared `eft_light_trail_update` call; the light-kind blocks fill both `color_a` and `color_b` (the dead
 *   `color_a` stores go, the shared `+1`/`+2` offsets keep retail's `lbzx`).
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "ef.h"
#include "pl.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* MTX34_ctor */
#include "fn_8004CAD8.h" /* mtx34_identity, mtx34_concat_assign, cpSetRotMatrix, rotVecY, rotMatrixY (rule 2) */
#include "scaleMat34W.h"
#include "rotMatrixXYZ.h"
#include "ef/pRoot.h"
#include "ef/eft004.h" /* mtx34_trans_add / mtx34_trans_get */
#include "ef/eft001.h" /* mtx34_set_trans */
#include "g3d/g3d_calcworld.h" /* addVec3To */
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode */
#include "g3d/res_node_mtx_id_get.h"
#include "rotLocalMatX.h" /* rotLocalMatZ */
#include "MSL/mathf.h" /* atan2f */
#include "Pl/Pl_master_ck.h" /* Pl_master_ck, Pl_act_ck */
#include "Pl/coll_wall_slide.h"
#include "ai/ai_npc.h" /* _AINPC_W */
#include "ai/ai_get_motion_no.h"
#include "ai/ai_npc_frame_ck.h"
#include "ef/eft019.h" /* eft019_set_core, eft019_set */
#include "stage/shell_set_func_ptr.h"
#include "stage/get_now_mapno.h"
#include "ef/eft_stage_spawn_flag.h"
#include "ef/eft025_pl_set.h"
#include "lobby/lb_pl_act_off_ck.h"
#include "sound/fn_800D7F54.h" /* se_req_pos_id63 */
#include "sound/se_req_pos_id27.h"
#include "sound/se_req_pos_id59.h"
#include "light/light_point_scale_set.h"
#include "sound/mhchar_wld_mtx_array_get.h"
#include "ef/eft_res.h" /* eft_res_slot_get, eft_res_slot_release, eft_res_models_spawn, res_eft_create (rule 2) */
#include "ef/res_eft_model_create_light.h"
#include "ef/effect.h" /* eft_state_flags_set, effect_move, change_paramscale_eff (rule 2) */
#include "ef/change_color_eff.h"
#include "ef/push_eft_effect_heap_num.h"
#include "ef/GameMode_ck.h"
#include "stage/stg_w.h" /* get_now_areano */
#include "stage/get_stg_eft_col.h"
#include "lobby/lb_server_sel_trans.h" /* event_demo_ck */
#include "light/light.h" /* light_id_next */
#include "Pl/fn_802693C4.h" /* Get_motion_no, Pl_frame_check, Pl_frame_check_alt */
#include "g3d/g3d_root_model_bind.h"
#include "g3d/g3d_calcmaterial.h" /* res_tex_srt_copy_ctor (rule 2) */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResTexSrt */
#include "nw4r/g3d/scnmdl.h" /* nw4r::g3d::ScnMdl::CopiedMatAccess */
#include "sound/mhchar_joint_mtx_get.h"
#include "sound/copied_mat_access_valid_ck.h"
#include "sound/sound_job_request.h"

/* signatures the calls below use, when they differ from the owner header's: a cast call is the same direct call. */
#define eft_state_flags_set_c1 ((void (*)(_EFT*, s32, s32))eft_state_flags_set)
#define coll_wall_slide_c1 ((s32 (*)(VEC3*, VEC3*, f32, VEC3*, u32, u16, u8, u8))coll_wall_slide)

/* ---- the per-family work block `_EFT::work_0x38` points at ---- */

/* The record's +0x04 run: the spawned effect, its colour and scale for the area effect; the light scale vector for
 * the light family. size: 0x0C */
typedef union EftWorkHead {
    /* +0x00 */ struct {
        /* +0x00 */ nw4r::ef::Effect* effect;
        /* +0x04 */ _GXColor color_0x08;
        /* +0x08 */ f32 scale_0x0C;
    } fx; /* size: 0x0C */
    /* +0x00 */ VEC3 scale_0x04;
} EftWorkHead; /* size: 0x0C */

/* The work block's slot array at +0x10: the light models the light family walks, or the effect handles the
 * teardown frees. size: 0x1C */
typedef union EftWorkSlots {
    /* +0x00 */ struct {
        /* +0x00 */ MHchar* items_0x10[5];
    } items; /* size: 0x14 */
    /* +0x00 */ struct {
        /* +0x00 */ u8 pad_0x10[8];
        /* +0x08 */ nw4r::ef::Effect* handles_0x18[5];
    } handles; /* size: 0x1C */
} EftWorkSlots; /* size: 0x1C */

/* The work block's payload at +0x34: a joint matrix for the follow effects, a scale for the day-cycle
 * effect. size: 0x30 */
typedef union EftWorkBody {
    /* +0x00 */ MTX34 matrix_0x34;
    /* +0x00 */ struct {
        /* +0x00 */ u8 pad_0x00[0x10];
        /* +0x10 */ f32 scale_0x44;
    } as_scaled; /* size: 0x30 */
} EftWorkBody; /* size: 0x30 */

/* The work block both families share the front of. size: 0x70 (the light family's `eft_light_spawn` allocation) */
typedef struct EftWork {
    /* +0x00 */ s32 count;
    /* +0x04 */ EftWorkHead head;
    /* +0x10 */ EftWorkSlots slots_0x10;
    /* +0x2C */ u16 light_id_0x2C;
    /* +0x2E */ u8 area_0x2E;
    /* +0x2F */ s8 field_0x2F;
    /* +0x30 */ u32 motion_0x30;
    /* +0x34 */ EftWorkBody body_0x34;
    /* +0x64 */ s8 effect_count_0x64;
    /* +0x65 */ u8 field_0x65;
    /* +0x66 */ u8 color_index_0x66;
    /* +0x67 */ u8 field_0x67;
    /* +0x68 */ f32 field_0x68;
    /* +0x6C */ f32 field_0x6C;
} EftWork; /* size: 0x70 */

#define EFT_WORK(eft) ((EftWork*)(eft)->work_0x38)

/* The player body `_PLW::physics_0x13C` points at: the player's `MHchar` sits at +0x04.  A view: the record is
 * `ef/fn_80114E34_types.h`'s `_PLW_PHYSICS`, whose header cannot be included beside `ef.h`. size: 0x144 - lower
 * bound */
typedef struct EftPlBody {
    /* +0x000 */ u8 unused_0x00[4];
    /* +0x004 */ MHchar chr_0x04;
} EftPlBody; /* size: 0x144 - lower bound */

#define EFT_PL_CHR(pl) (&((EftPlBody*)(pl)->physics_0x13C)->chr_0x04)

/* The `_PLW` byte at +0x5BB, which `Pl/plw.h` spells `unk5BB` and `pl_item_menu_open_ck` returns: the player has the
 * item menu open.  A view of the record's head. size: 0x5BC - lower bound */
typedef struct EftPlMenu {
    /* +0x000 */ u8 unused_0x000[0x5BB];
    /* +0x5BB */ u8 item_menu_0x5BB;
} EftPlMenu; /* size: 0x5BC - lower bound */

#define EFT_PL_MENU(pl) ((EftPlMenu*)(pl))

/* The record `sound_job_request` hands back; its +0x2C word carries the owning effect for the callback.
 * size: 0x30 - lower bound */
typedef struct EftLightJob {
    /* +0x00 */ u8 unused_0x00[0x2C];
    /* +0x2C */ _EFT* owner_0x2C;
} EftLightJob; /* size: 0x30 - lower bound */

/* The pooled g3d work `MHchar::g3d_0x10C` points at; only the model handle it carries is read.  A view: the record
 * is `stage/shell.cpp`'s `_g3d_work`. size: 0x08 - lower bound */
typedef struct EftG3dWork {
    /* +0x00 */ u8 unused_0x00[4];
    /* +0x04 */ u32 model_0x04;
} EftG3dWork; /* size: 0x08 - lower bound */

/* A three-channel colour entry of the effect-colour tables. size: 0x03 */
typedef struct Rgb3 {
    /* +0x00 */ u8 r;
    /* +0x01 */ u8 g;
    /* +0x02 */ u8 b;
} Rgb3; /* size: 0x03 */

extern "C" {

/* The resource group and effect id `eft_area_effect_create` creates the area effect from, per record type. */
u16 eft_area_effect_groups[10] = {0x2F, 0x12A, 0x12B, 0x12C, 0xF9, 0x12D, 0x131, 0x122, 0x123, 0};
u16 eft_area_effect_ids[10] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0, 0, 0};
/* The two material colours of the lantern light, three bytes per colour index. */
u8 eft_light_colors_a[8 * 3] = {
    0xE2, 0xED, 0xD1, 0xE2, 0xED, 0xD1, 0xAE, 0x9A, 0x78, 0xAE, 0x9A, 0x78,
    0x52, 0x4C, 0x43, 0xFF, 0xD2, 0xC0, 0xFF, 0xD2, 0xC0, 0xFF, 0xD2, 0xC0,
};
u8 eft_light_colors_b[8 * 3] = {
    0xFF, 0xD2, 0xC0, 0xD8, 0xB4, 0x8D, 0xB1, 0x96, 0x5A, 0xB1, 0x96, 0x5A,
    0x63, 0x4F, 0x41, 0xC0, 0x10, 0xC0, 0xFF, 0xFF, 0x10, 0x10, 0xC0, 0xC0,
};
/* The weapon light's material tint (three bytes used). */
u8 eft_light_tint_color[8] = {0x3F, 0x3F, 0x3F};
/* The alpha keys the thrown light fades out by: (frame, alpha) pairs ended by 0xFF. */
u8 eft_light_fade_keys[8] = {0, 0xFF, 12, 0xFF, 20, 0, 0xFF, 0};
/* The light-model resource id per light item. */
u16 eft_light_model_ids[4] = {7, 4, 0, 0};

}

extern "C" {

_EFT* eft_light_spawn(_PLW* source, u8 arg1, s32 arg2, s32 arg3);
void eft_area_effect_release(_EFT* self);
void eft_area_effect_dispatch(_EFT* self);
void eft_area_effect_create(_EFT* self);
void eft_area_effect_move(_EFT* self);
void eft_area_effect_state_next(_EFT* self);
void eft_area_effect_retire(_EFT* self);
void eft_light_release(_EFT* self);
void eft_light_dispatch(_EFT* self);
void eft_light_lantern_init(_EFT* self);
void eft_light_drop_init(_EFT* self);
void eft_light_npc_init(_EFT* self);
void eft_light_lobby_init(_EFT* self);
void eft_light_update(_EFT* self);
void eft_light_lantern_update(_EFT* self);
void eft_light_drop_update(_EFT* self);
void eft_light_npc_update(_EFT* self);
void eft_light_lobby_update(_EFT* self);
void eft_light_state_next(_EFT* self);
void eft_light_retire(_EFT* self);
void eft_light_main_part_show(_EFT* self, u32 arg1);
void eft_light_sub_part_show(_EFT* self, u32 arg1);
void eft_light_npc_part_show(_EFT* self, u32 arg1);
void eft_light_lobby_part_show(_EFT* self, u32 arg1);
void eft_light_beam_aim_job(struct PrimRec* job);
void eft_light_beam_bind(_EFT* self, MHchar* model, s32 arg2);
void eft_light_ring_colors_get(u8 arg0, s32 arg1, _GXColor* arg2, _GXColor* arg3, _GXColor* arg4, _GXColor* arg5);
s32 eft_light_motion_part_get(_PLW* self);
void eft_light_trail_update(_EFT* self, u8 active);
void eft_light_blade_place(_EFT* self);

#pragma peephole off
void eft_spawn_pos_in_area(VEC3* arg0, u8 arg1, u8 arg2, u32 arg3, f32 farg0)
{
    _EFT* eft;
    EftWork* work;

    if (arg1 == get_now_areano()) {
        eft = (_EFT*)eft_res_slot_get(0x10);
        if (eft != NULL) {
            work = EFT_WORK(eft);
            work->count = 1;
            work->head.fx.scale_0x0C = farg0;
            eft->field_0x03 = 0x11;
            eft->type_0x02 = arg2;
            copyVec3(&eft->pos_0x18, arg0);
            eft->rot_0x24.y = arg3;
            eft->area_0x44 = arg1;
            eft->release_0x40 = eft_area_effect_release;
            eft->dispatch_0x34 = eft_area_effect_dispatch;
            eft_state_flags_set_c1(eft, 0, 0);
            if (event_demo_ck() == 1) {
                eft->demo_flag_0x08 = 1;
            }
        }
    }
}
#pragma peephole reset

void eft_area_effect_release(_EFT* self)
{
    EftWork* work;

    work = EFT_WORK(self);
    push_eft_effect_heap_num(&work->head.fx.effect, work->count);
    work->count = 0;
}

void eft_area_effect_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        eft_area_effect_create(self);
        return;
    case 1:
        eft_area_effect_move(self);
        return;
    case 2:
        eft_area_effect_state_next(self);
        return;
    case 3:
        eft_area_effect_retire(self);
        return;
    }
}

/* Creates the effect this state runs and orients it by the record's rotation and position. */
#pragma peephole off /* the pool-block store keeps the target's reload - playbook 39 */
void eft_area_effect_create(_EFT* self)
{
    MTX34 mtx;
    EftWork* work;
    u32 color;

    MTX34_ctor(&mtx);
    work = EFT_WORK(self);
    self->state_0x05++;
    work->head.fx.effect = res_eft_create(eft_area_effect_groups[self->type_0x02], eft_area_effect_ids[self->type_0x02], 0);
    if (work->head.fx.effect == 0) {
        eft_area_effect_retire(self);
        return;
    }
    cpSetRotMatrix(&self->rot_0x24, &mtx);
    mtx34_trans_add(&mtx, &self->pos_0x18);
    work->head.fx.effect->SetRootMtx(mtx);
    self->flag_0x01 = 1;
    color = get_stg_eft_col(self->area_0x44, 0);
    work->head.fx.color_0x08.r = (color & 0xFF000000) >> 24;
    work->head.fx.color_0x08.g = (color & 0x00FF0000) >> 16;
    work->head.fx.color_0x08.b = (color & 0x0000FF00) >> 8;
    work->head.fx.color_0x08.a = 0xFF;
    change_paramscale_eff(work->head.fx.effect, work->head.fx.scale_0x0C);
    eft_area_effect_move(self);
}
#pragma peephole reset

/* Advances the state while the effect lives, recolouring it from the record's colour. */
void eft_area_effect_move(_EFT* self)
{
    EftWork* work;

    work = EFT_WORK(self);
    if (self->demo_flag_0x08 == 1 && event_demo_ck() == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (self->area_0x44 != get_now_areano()) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (effect_move(work->head.fx.effect) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    change_color_eff(work->head.fx.effect, &self->pos_0x18, work->head.fx.color_0x08);
    eft_res_models_spawn(self, (void**)&work->head.fx.effect, 1, work->count, 0);
}

void eft_area_effect_state_next(_EFT* self)
{
    self->state_0x05++;
}

void eft_area_effect_retire(_EFT* self)
{
    eft_res_slot_release(self);
}

/* Spawns the sub-effect whose variant depends on the current game mode. */
void ef_pl_effects_init(_PLW* source)
{
    if (GameMode_ck() == 2) {
        if (eft_light_spawn(source, 4, 0, 1) == NULL) {
            return;
        }
    } else {
        if (eft_light_spawn(source, 0, 0, 2) == NULL) {
            return;
        }
    }
}

/* Spawns the third sub-effect variant. */
void eft_npc_model_set_create(_PLW* source)
{
    eft_light_spawn(source, 3, 2, 1);
}

/* Spawns a sub-effect that follows the player's weapon joint. */
void eft_pl_weapon_follow_spawn(_PLW* self)
{
    _EFT* eft;
    EftWork* work;

    eft = eft_light_spawn(self, 2, 0, 1);
    if (eft != NULL) {
        work = EFT_WORK(eft);
        work->field_0x68 = self->ground_y_0x060;
        work->field_0x6C = self->motion_vel_0x7C;
        mhchar_joint_mtx_get(EFT_PL_CHR(self), 7, &work->body_0x34.matrix_0x34);
        eft_state_flags_set_c1(eft, 1, 4);
    }
}

/* Allocates the sub-effect record and its per-variant handles. */
#pragma peephole off
_EFT* eft_light_spawn(_PLW* source, u8 arg1, s32 arg2, s32 arg3)
{
    _EFT* eft;
    EftWork* work;
    s32 i;

    eft = (_EFT*)eft_res_slot_get(0x70);
    if (eft == NULL) {
        return NULL;
    }
    eft->type_0x02 = arg1;
    eft->release_0x40 = eft_light_release;
    work = EFT_WORK(eft);
    work->count = (u8)arg3;
    for (i = 0; i < work->count; i++) {
        work->slots_0x10.items.items_0x10[i] = (MHchar*)eft_res_model_get();
        if (work->slots_0x10.items.items_0x10[i] == 0) {
            eft_res_slot_release(eft);
            return NULL;
        }
    }
    work->field_0x67 = 0;
    eft->flag_0x01 = 1;
    eft->field_0x03 = 0x12;
    eft->timer_0x0C = 0;
    eft_state_flags_set_c1(eft, 1, 0);
    switch ((u8)arg2) {
    case 0:
        eft->source_0x30 = source;
        break;
    case 2:
        eft->source_0x30 = source;
        break;
    }
    eft->dispatch_0x34 = eft_light_dispatch;
    return eft;
}
#pragma peephole reset

/* Releases the sub-effect's handles and clears its count. */
#pragma peephole off
void eft_light_release(_EFT* self)
{
    EftWork* work;
    s32 i;

    work = EFT_WORK(self);
    eft_res_models_release(&work->slots_0x10.items.items_0x10[0], work->count);
    for (i = 0; i < 5; i++) {
        if (work->slots_0x10.handles.handles_0x18[i] != 0) {
            push_eft_effect_heap_num(&work->slots_0x10.handles.handles_0x18[i], 1);
        }
    }
    work->count = 0;
}
#pragma peephole reset

/* Advances the state: a per-variant handler while the state is 0, then the generic ones. */
void eft_light_dispatch(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        break;
    case 1:
        eft_light_update(self);
        return;
    case 2:
        eft_light_state_next(self);
        return;
    case 3:
        eft_light_retire(self);
        return;
    default:
        return;
    }
    switch (self->type_0x02) {
    case 0:
        eft_light_lantern_init(self);
        return;
    case 2:
        eft_light_drop_init(self);
        return;
    case 3:
        eft_light_npc_init(self);
        return;
    case 4:
        eft_light_lobby_init(self);
        return;
    }
}

/* Creates the light per item and seeds the day-cycle state. */
void eft_light_lantern_init(_EFT* self)
{
    s32 i;
    s32 joint;
    _PLW* source;
    EftWork* work;

    source = (_PLW*)self->source_0x30;
    work = EFT_WORK(self);
    self->state_0x05++;
    joint = source->chunk_ofs + 3;
    for (i = 0; i < work->count; i++) {
        if (res_eft_model_create_light(work->slots_0x10.items.items_0x10[i], eft_light_model_ids[i], 0x12C, joint) == 0) {
            eft_light_retire(self);
            return;
        }
    }
    eft_light_main_part_show(self, 0);
    eft_light_sub_part_show(self, 0);
    for (i = 0; i < work->count; i++) {
        setVector3(&work->slots_0x10.items.items_0x10[i]->scale_0x1C, 1.0f, 1.0f, 1.0f);
        copyVec3(&work->head.scale_0x04, &work->slots_0x10.items.items_0x10[i]->scale_0x1C);
        work->slots_0x10.items.items_0x10[i]->ready = 0;
    }
    work->light_id_0x2C = light_id_next();
    work->area_0x2E = source->area_0x16;
    eft_light_update(self);
}

/* Creates the light per item for the weapon-follow variant, then picks its shape from the player's weapon state
 * and places it on the weapon joint. */
#pragma peephole off
#pragma optimization_level 4
void eft_light_drop_init(_EFT* self)
{
    EftWork* work;
    VEC3 offset;
    MTX34 unused;
    MTX34 local;
    MTX34 tex_mtx;
    s32 i;
    s32 joint;
    u16 id;
    _PLW* source;
    _GXColor color;

    source = (_PLW*)self->source_0x30;
    work = EFT_WORK(self);
    VEC3_ctor(&offset);
    MTX34_ctor(&unused);
    MTX34_ctor(&local);
    MTX34_ctor(&tex_mtx);
    self->state_0x05++;
    if (GameMode_ck() == 2) {
        id = 0xA7;
    } else {
        id = eft_light_model_ids[0];
    }
    joint = source->chunk_ofs + 3;
    for (i = 0; i < work->count; i++) {
        if (res_eft_model_create_light(work->slots_0x10.items.items_0x10[i], id, 0x12C, joint) == 0) {
            eft_light_retire(self);
            return;
        }
    }
    switch ((s32)(source->field_0x5C4 & 0xF0) >> 4) {
    case 1:
    case 2:
        if (GameMode_ck() == 2) {
            eft_light_lobby_part_show(self, 1);
        } else {
            eft_light_main_part_show(self, 6);
        }
        self->rot_0x24.x = 0xD99B;
        self->rot_0x24.y = 0x7A51;
        self->rot_0x24.z = 0x53E9;
        setVector3(&offset, 5.5f, -28.0f, 12.0f);
        setVector3(&work->head.scale_0x04, 1.0f, 1.3f, 1.0f);
        for (i = 0; i < work->count; i++) {
            work->slots_0x10.items.items_0x10[i]->ready = 1;
        }
        break;
    case 3:
        if (GameMode_ck() == 2) {
            eft_light_lobby_part_show(self, 1);
        } else {
            eft_light_main_part_show(self, 6);
        }
        {
            nw4r::g3d::ScnMdl::CopiedMatAccess access(
                (nw4r::g3d::ScnMdl*)work->slots_0x10.items.items_0x10[0]->field_0x118, 6);
            if (copied_mat_access_valid_ck(&access) != 0) {
                nw4r::g3d::ResTexSrt srt;
                res_tex_srt_copy_ctor(&srt, access.GetResTexSrt(false));
                srt.GetEffectMtx(0, &tex_mtx);
                tex_mtx.m[0][3] = 0.123f;
                srt.SetEffectMtx(0, &tex_mtx);
            }
        }
        self->rot_0x24.x = 0xD99B;
        self->rot_0x24.y = 0x7A51;
        self->rot_0x24.z = 0x53E9;
        setVector3(&offset, 8.9f, -25.0f, 4.6f);
        setVector3(&work->head.scale_0x04, 1.0f, 1.0f, 1.0f);
        for (i = 0; i < work->count; i++) {
            work->slots_0x10.items.items_0x10[i]->ready = 0;
        }
        break;
    case 4:
        eft_light_main_part_show(self, 14);
        self->rot_0x24.x = 0xF60C;
        self->rot_0x24.y = 0x749F;
        self->rot_0x24.z = 0x3333;
        setVector3(&offset, 3.3f, -24.0f, 4.0f);
        setVector3(&work->head.scale_0x04, 2.5f, 3.0f, 2.0f);
        color.r = eft_light_tint_color[0];
        color.g = eft_light_tint_color[1];
        color.b = eft_light_tint_color[2];
        color.a = 0xFF;
        work->slots_0x10.items.items_0x10[0]->setMatColor(9, GX_COLOR0A0, color, false);
        for (i = 0; i < work->count; i++) {
            work->slots_0x10.items.items_0x10[i]->ready = 0;
        }
        break;
    }
    mulVecMat(&offset, &work->body_0x34.matrix_0x34);
    mtx34_trans_get(&work->body_0x34.matrix_0x34, &self->pos_0x18);
    self->pos_0x18.x += offset.x;
    self->pos_0x18.y += offset.y;
    self->pos_0x18.z += offset.z;
    mtx34_identity(&local);
    scaleMat34W(&local, &work->head.scale_0x04);
    rotMatrixXYZ(&self->rot_0x24, &local);
    mtx34_concat_assign(&work->body_0x34.matrix_0x34, &local);
    work->body_0x34.matrix_0x34.m[0][3] = self->pos_0x18.x;
    work->body_0x34.matrix_0x34.m[1][3] = self->pos_0x18.y;
    work->body_0x34.matrix_0x34.m[2][3] = self->pos_0x18.z;
    eft_light_update(self);
}
#pragma optimization_level reset
#pragma peephole reset

/* Creates one light per item at the fixed joint and seeds the vectors. */
void eft_light_npc_init(_EFT* self)
{
    EftWork* work;
    s32 i;

    work = EFT_WORK(self);
    self->state_0x05++;
    for (i = 0; i < work->count; i++) {
        if (res_eft_model_create_light(work->slots_0x10.items.items_0x10[i], 4, 0x2C, 6) == 0) {
            eft_light_retire(self);
            return;
        }
    }
    eft_light_npc_part_show(self, 0);
    for (i = 0; i < work->count; i++) {
        setVector3(&work->slots_0x10.items.items_0x10[i]->scale_0x1C, 1.0f, 1.0f, 1.0f);
        copyVec3(&work->head.scale_0x04, &work->slots_0x10.items.items_0x10[i]->scale_0x1C);
        work->slots_0x10.items.items_0x10[i]->ready = 0;
    }
    work->light_id_0x2C = light_id_next();
    eft_light_update(self);
}

/* Creates the single light at the named joint and seeds the vectors. */
void eft_light_lobby_init(_EFT* self)
{
    s32 i;
    EftWork* work;

    work = EFT_WORK(self);
    self->state_0x05++;
    if (res_eft_model_create_light(work->slots_0x10.items.items_0x10[0], 0xA7, 0x12C, 0) == 0) {
        eft_light_retire(self);
        return;
    }
    eft_light_lobby_part_show(self, 0);
    for (i = 0; i < work->count; i++) {
        setVector3(&work->slots_0x10.items.items_0x10[i]->scale_0x1C, 1.0f, 1.0f, 1.0f);
        copyVec3(&work->head.scale_0x04, &work->slots_0x10.items.items_0x10[i]->scale_0x1C);
        work->slots_0x10.items.items_0x10[i]->ready = 0;
    }
    eft_light_update(self);
}

/* Runs the per-variant body of the effect this record was created for. */
void eft_light_update(_EFT* self)
{
    switch (self->type_0x02) {
    case 0:
        eft_light_lantern_update(self);
        return;
    case 2:
        eft_light_drop_update(self);
        return;
    case 3:
        eft_light_npc_update(self);
        return;
    case 4:
        eft_light_lobby_update(self);
        return;
    }
}

/* Drops the trail effects when the area changed; otherwise spawns a trail effect at the player's front every eight
 * frames while `active` and keeps the live ones moving. */
#pragma peephole off
void eft_light_trail_update(_EFT* self, u8 active)
{
    EftWork* work;
    _PLW* source;
    MTX34 mtx;
    VEC3 pos;
    VEC3 offset;
    s32 i;

    work = EFT_WORK(self);
    source = (_PLW*)self->source_0x30;
    MTX34_ctor(&mtx);
    VEC3_ctor(&pos);
    VEC3_ctor(&offset);
    if (work->area_0x2E != self->area_0x44) {
        for (i = 0; i < 5; i++) {
            if (work->slots_0x10.handles.handles_0x18[i] != 0) {
                push_eft_effect_heap_num(&work->slots_0x10.handles.handles_0x18[i], 1);
            }
        }
        work->area_0x2E = self->area_0x44;
        return;
    }
    if (active != 0 && self->field_0x07 != 0) {
        if (work->field_0x2F < 0) {
            switch (self->field_0x07) {
            case 3:
                cpSetRotMatrix(&source->rot_0x54, &mtx);
                copyVec3(&pos, &source->vec_0x03C);
                setVector3(&offset, 0.0f, 0.0f, 130.0f);
                rotVecY(&offset, source->field_0x058);
                break;
            default:
                cpSetRotMatrix(&source->rot_0x54, &mtx);
                copyVec3(&pos, &source->vec_0x03C);
                setVector3(&offset, 0.0f, 30.0f, 145.0f);
                rotVecY(&offset, source->field_0x058);
                break;
            }
            addVec3To(&pos, &offset);
            mtx34_set_trans(&mtx, &pos);
            for (i = 0; i < 5; i++) {
                if (work->slots_0x10.handles.handles_0x18[i] == 0) {
                    work->slots_0x10.handles.handles_0x18[i] = res_eft_create(0xDF, 0x19, 0);
                    if (work->slots_0x10.handles.handles_0x18[i] != 0) {
                        work->slots_0x10.handles.handles_0x18[i]->SetRootMtx(mtx);
                        work->effect_count_0x64++;
                    }
                    break;
                }
            }
            work->field_0x2F = 8;
        }
        work->field_0x2F--;
    }
    if (work->effect_count_0x64 > 0) {
        for (i = 0; i < 5; i++) {
            if (work->slots_0x10.handles.handles_0x18[i] != 0) {
                if (effect_move(work->slots_0x10.handles.handles_0x18[i]) == 0) {
                    push_eft_effect_heap_num(&work->slots_0x10.handles.handles_0x18[i], 1);
                    work->effect_count_0x64--;
                } else {
                    eft_res_models_spawn(self, (void**)&work->slots_0x10.handles.handles_0x18[i], 1, 1, 0);
                }
            }
        }
    }
    work->area_0x2E = self->area_0x44;
}
#pragma peephole reset

/* The player's held light: picks the joint, offset and rotation for the current weapon or motion (the lantern
 * motions also animate its beams and colours), places the light models, then updates the trail. */
#pragma peephole off
#pragma optimization_level 4
void eft_light_lantern_update(_EFT* self)
{
    VEC3 offset;
    VEC3 pos;
    VEC3 pivot;
    VEC3 light_pos;
    VEC3 sound_pos;
    MTX34 mtx;
    MTX34 local;
    MTX34 pose;
    u32 show = 0;
    s32 placed = 0;
    u8 trail = 0;
    u32 beam = 0;
    s32 start;
    s32 mode;
    s32 i;
    u16 motion;
    _PLW* source;
    EftWork* work;
    u16 angle;
    f32 y;
    f32 x;
    f32 z;
    VEC3 diff;
    _GXColor color_a;
    _GXColor color_b;
    _GXColor ring_a;
    _GXColor ring_b;
    _GXColor core_a;
    _GXColor core_b;

    VEC3_ctor(&offset);
    VEC3_ctor(&pos);
    VEC3_ctor(&pivot);
    VEC3_ctor(&light_pos);
    VEC3_ctor(&sound_pos);
    MTX34_ctor(&mtx);
    MTX34_ctor(&local);
    MTX34_ctor(&pose);
    work = EFT_WORK(self);
    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    if (event_demo_ck() == 1) {
        return;
    }
    source = (_PLW*)self->source_0x30;
    self->flag_0x01 = source->field_0x001;
    self->area_0x44 = source->area_0x16;
    motion = Get_motion_no(source);
    if ((u32)(motion - 346) > 4 && (u32)(motion - 333) > 3) {
        work->field_0x67 = 12;
        self->field_0x06 = 0;
        self->field_0x07 = 0;
        self->demo_flag_0x08 = 0;
        work->field_0x2F = -1;
    }
    do {
        if ((source->field_0x5C4 & 0xF) != 0) {
            if (Pl_act_ck(source, 12, 9) != 0 && motion == 51 && EFT_PL_CHR(source)->field_0x44 > 0.0f) {
                continue;
            }
            switch ((s32)(source->field_0x5C4 & 0xF0) >> 4) {
            case 1:
            case 2:
                eft_light_main_part_show(self, 6);
                eft_light_sub_part_show(self, 0xFF);
                work->head.scale_0x04.x = 1.0f;
                work->head.scale_0x04.y = 1.3f;
                work->head.scale_0x04.z = 1.0f;
                self->rot_0x24.x = 0xD99B;
                self->rot_0x24.y = 0x7A51;
                self->rot_0x24.z = 0x53E9;
                setVector3(&offset, 5.5f, -28.0f, 12.0f);
                mhchar_joint_mtx_get(EFT_PL_CHR(source), 7, &mtx);
                start = 0;
                break;
            case 3:
                eft_light_main_part_show(self, 6);
                eft_light_sub_part_show(self, 0xFF);
                work->head.scale_0x04.x = 1.0f;
                work->head.scale_0x04.y = 1.0f;
                work->head.scale_0x04.z = 1.0f;
                {
                    nw4r::g3d::ScnMdl::CopiedMatAccess access(
                        (nw4r::g3d::ScnMdl*)work->slots_0x10.items.items_0x10[0]->field_0x118, 6);
                    if (copied_mat_access_valid_ck(&access) != 0) {
                        nw4r::g3d::ResTexSrt srt;
                        res_tex_srt_copy_ctor(&srt, access.GetResTexSrt(false));
                        srt.GetEffectMtx(0, &pose);
                        pose.m[0][3] = 0.123f;
                        srt.SetEffectMtx(0, &pose);
                    }
                }
                self->rot_0x24.x = 0xD99B;
                self->rot_0x24.y = 0x7A51;
                self->rot_0x24.z = 0x53E9;
                setVector3(&offset, 8.9f, -25.0f, 4.6f);
                mhchar_joint_mtx_get(EFT_PL_CHR(source), 7, &mtx);
                start = 0;
                break;
            case 4:
                eft_light_main_part_show(self, 14);
                eft_light_sub_part_show(self, 0xFF);
                work->head.scale_0x04.x = 2.5f;
                work->head.scale_0x04.y = 3.0f;
                work->head.scale_0x04.z = 2.0f;
                self->rot_0x24.x = 0xF60C;
                self->rot_0x24.y = 0x749F;
                self->rot_0x24.z = 0x3333;
                setVector3(&offset, 3.3f, -24.0f, 4.0f);
                mhchar_joint_mtx_get(EFT_PL_CHR(source), 7, &mtx);
                color_a.r = eft_light_tint_color[0];
                color_a.g = eft_light_tint_color[1];
                color_a.b = eft_light_tint_color[2];
                color_a.a = 0xFF;
                work->slots_0x10.items.items_0x10[0]->setMatColor(9, GX_COLOR0A0, color_a, false);
                start = 0;
                break;
            default:
                continue;
            }
        } else {
            switch (motion) {
            case 315:
            case 352:
                if (EFT_PL_MENU(source)->item_menu_0x5BB != 0) {
                    eft_light_blade_place(self);
                }
                eft_light_main_part_show(self, 1);
                eft_light_sub_part_show(self, 0xFF);
                mhchar_joint_mtx_get(EFT_PL_CHR(source), 7, &mtx);
                setVector3(&offset, 17.0f, 0.9f, 22.0f);
                self->rot_0x24.x = 0x0EEF;
                self->rot_0x24.y = 0x071C;
                self->rot_0x24.z = 0x3777;
                work->head.scale_0x04.x = 1.0f;
                work->head.scale_0x04.y = 1.0f;
                work->head.scale_0x04.z = 1.0f;
                start = 0;
                break;
            case 316:
                if ((EFT_PL_CHR(source)->field_0x44 > 0.0f && source->field_0x007 == 1)
                    || (Pl_frame_check(source, 1, 16.0f, 0.0f) != 0 && Pl_frame_check(source, 1, 164.0f, 0.0f) == 0)) {
                    switch (source->field_0x306) {
                    case 208:
                    case 214:
                        eft_light_main_part_show(self, 0xFF);
                        eft_light_sub_part_show(self, 3);
                        setVector3(&offset, -8.0f, -3.0f, 0.0f);
                        self->rot_0x24.x = 0xCE3A;
                        self->rot_0x24.y = 0x038E;
                        self->rot_0x24.z = 0xF05C;
                        break;
                    case 26:
                        if (Pl_frame_check(source, 0, 26.0f, 0.0f) != 0) {
                            copyVec3(&pos, &self->pos_0x18);
                            pos.y += 15.0f;
                            eft019_set_core(&pos, self->area_0x44, 27, 0.7f, 0.7f);
                            if (self->area_0x44 == get_now_areano()) {
                                se_req_pos_id63(&self->pos_0x18);
                            }
                        }
                    case 27:
                    case 28:
                    case 166:
                    case 167:
                        eft_light_main_part_show(self, 0xFF);
                        eft_light_sub_part_show(self, 1);
                        setVector3(&offset, -17.0f, -30.0f, 1.0f);
                        self->rot_0x24.x = 0xF05C;
                        self->rot_0x24.y = 0x0C17;
                        self->rot_0x24.z = 0xAFA6;
                        break;
                    }
                    {
                        u8 index;

                        switch (source->field_0x306) {
                        case 27:
                            index = 1;
                            break;
                        case 26:
                        case 166:
                        case 167:
                            index = 2;
                            break;
                        case 28:
                            index = 4;
                            break;
                        default:
                            index = 0;
                            break;
                        }
                        color_a.r = eft_light_colors_a[index * 3];
                        color_a.g = eft_light_colors_a[index * 3 + 1];
                        color_a.b = eft_light_colors_a[index * 3 + 2];
                        color_b.r = eft_light_colors_b[index * 3];
                        color_b.g = eft_light_colors_b[index * 3 + 1];
                        color_b.b = eft_light_colors_b[index * 3 + 2];
                        color_a.a = 0xFF;
                        color_b.a = 0xFF;
                    }
                    work->slots_0x10.items.items_0x10[1]->setMatColor(3, GX_COLOR0A0, color_a, false);
                    work->slots_0x10.items.items_0x10[1]->setMatColor(4, GX_COLOR0A0, color_b, false);
                    mhchar_joint_mtx_get(EFT_PL_CHR(source), 11, &mtx);
                    work->head.scale_0x04.x = 1.0f;
                    work->head.scale_0x04.y = 1.0f;
                    work->head.scale_0x04.z = 1.0f;
                    start = 0;
                    break;
                }
                continue;
            case 318:
                if (EFT_PL_MENU(source)->item_menu_0x5BB != 0) {
                    eft_light_blade_place(self);
                }
                if (Pl_frame_check(source, 1, 118.0f, 0.0f) != 0) {
                    continue;
                }
                eft_light_main_part_show(self, 2);
                eft_light_sub_part_show(self, 0xFF);
                mhchar_joint_mtx_get(EFT_PL_CHR(source), 11, &mtx);
                setVector3(&offset, -9.0f, -1.0f, 65.0f);
                self->rot_0x24.x = 0;
                self->rot_0x24.y = 0x8000;
                self->rot_0x24.z = 0x8000;
                work->head.scale_0x04.x = 1.0f;
                work->head.scale_0x04.y = 1.0f;
                work->head.scale_0x04.z = 1.0f;
                start = 0;
                break;
            case 319:
            case 322:
            case 358:
                if (EFT_PL_MENU(source)->item_menu_0x5BB != 0) {
                    eft_light_blade_place(self);
                }
                switch (motion) {
                case 319:
                    if (Pl_frame_check(source, 3, 26.0f, 248.0f) == 0) {
                        continue;
                    }
                    break;
                case 322:
                    if (Pl_frame_check(source, 2, 26.0f, 0.0f) != 0) {
                        continue;
                    }
                    break;
                case 358:
                    if (Pl_frame_check(source, 3, 16.0f, 250.0f) == 0) {
                        continue;
                    }
                    break;
                }
                eft_light_main_part_show(self, 3);
                eft_light_sub_part_show(self, 0xFF);
                mhchar_joint_mtx_get(EFT_PL_CHR(source), 7, &mtx);
                setVector3(&offset, 8.0f, 0.0f, -6.0f);
                self->rot_0x24.x = 0;
                self->rot_0x24.y = 0;
                self->rot_0x24.z = 0;
                work->head.scale_0x04.x = 1.0f;
                work->head.scale_0x04.y = 1.0f;
                work->head.scale_0x04.z = 1.0f;
                start = 0;
                break;
            case 320:
            case 321:
                switch (motion) {
                case 320:
                    if (Pl_frame_check(source, 2, 6.0f, 0.0f) != 0) {
                        continue;
                    }
                    break;
                case 321:
                    if (Pl_frame_check(source, 1, 34.0f, 0.0f) != 0) {
                        continue;
                    }
                    break;
                }
                eft_light_main_part_show(self, 10);
                eft_light_sub_part_show(self, 0xFF);
                mhchar_joint_mtx_get(EFT_PL_CHR(source), 11, &mtx);
                setVector3(&offset, -8.4f, -3.8f, -5.2f);
                self->rot_0x24.x = 0xD3EA;
                self->rot_0x24.y = 0x871D;
                self->rot_0x24.z = 0x1C72;
                work->head.scale_0x04.x = 1.0f;
                work->head.scale_0x04.y = 1.0f;
                work->head.scale_0x04.z = 1.0f;
                start = 0;
                break;
            case 324:
            case 325:
                switch (motion) {
                case 324:
                    if (EFT_PL_CHR(source)->field_0x44 > 0.0f) {
                        continue;
                    }
                    break;
                case 325:
                    if (Pl_frame_check(source, 1, 58.0f, 0.0f) != 0) {
                        continue;
                    }
                    break;
                }
                eft_light_main_part_show(self, 11);
                eft_light_sub_part_show(self, 0xFF);
                mhchar_joint_mtx_get(EFT_PL_CHR(source), 11, &mtx);
                setVector3(&offset, -12.0f, -7.0f, -3.0f);
                self->rot_0x24.x = 0x749F;
                self->rot_0x24.y = 0xEEF0;
                self->rot_0x24.z = 0xD4A0;
                work->head.scale_0x04.x = 1.0f;
                work->head.scale_0x04.y = 1.0f;
                work->head.scale_0x04.z = 1.0f;
                start = 0;
                break;
            case 333:
            case 334:
            case 335:
            case 336:
            case 346:
            case 347:
            case 348:
            case 349:
            case 350:
                work->head.scale_0x04.x = 1.0f;
                work->head.scale_0x04.y = 1.0f;
                work->head.scale_0x04.z = 1.0f;
                switch (source->field_0x306) {
                case 265:
                    trail = 1;
                    beam = 1;
                    self->timer_0x0C++;
                    {
                        u8 kind = source->field_0x5BA;

                        color_a.r = eft_light_colors_a[kind * 3];
                        color_a.g = eft_light_colors_a[kind * 3 + 1];
                        color_a.b = eft_light_colors_a[kind * 3 + 2];
                        color_b.r = eft_light_colors_b[kind * 3];
                        color_b.g = eft_light_colors_b[kind * 3 + 1];
                        color_b.b = eft_light_colors_b[kind * 3 + 2];
                    }
                    color_a.a = 0xFF;
                    color_b.a = 0xFF;
                    switch (motion) {
                    case 333:
                        if (Pl_frame_check(source, 1, 26.0f, 0.0f) == 0) {
                            continue;
                        }
                        self->demo_flag_0x08 |= 1;
                        self->field_0x07 = 1;
                        self->field_0x06 = 0;
                        show = 0;
                        start = 0;
                        break;
                    case 334:
                        if (Pl_frame_check(source, 1, 106.0f, 0.0f) != 0) {
                            continue;
                        }
                        if (Pl_frame_check(source, 1, 96.0f, 0.0f) != 0) {
                            beam = 2;
                        }
                        if (Pl_frame_check(source, 0, 28.0f, 0.0f) != 0) {
                            copyVec3(&pos, &self->pos_0x18);
                            pos.y += 20.0f;
                            eft019_set_core(&pos, self->area_0x44, 27, 0.7f, 0.7f);
                            if (self->area_0x44 == get_now_areano()) {
                                se_req_pos_id63(&pos);
                            }
                        }
                        if (Pl_frame_check(source, 0, 2.0f, 0.0f) != 0) {
                            self->demo_flag_0x08 = 0;
                            self->field_0x07 = 0;
                            work->color_index_0x66 = source->field_0x5BA;
                        }
                        show = 1;
                        start = 1;
                        break;
                    case 335:
                        if (Pl_frame_check(source, 1, 100.0f, 0.0f) != 0) {
                            continue;
                        }
                        if (Pl_frame_check(source, 0, 78.0f, 0.0f) != 0) {
                            self->field_0x06 = 1;
                            self->timer_0x0C = 0;
                        }
                        if (Pl_frame_check(source, 0, 2.0f, 0.0f) != 0) {
                            work->field_0x65 = 0;
                            if (self->field_0x07 == 1) {
                                self->demo_flag_0x08 = 0;
                                self->field_0x07 = 0;
                            } else {
                                self->field_0x07--;
                                self->demo_flag_0x08 &= ~1;
                            }
                            work->color_index_0x66 = source->field_0x5BA;
                        }
                        if (self->field_0x07 == 0 && Pl_frame_check(source, 1, 70.0f, 0.0f) != 0) {
                            beam = 2;
                        }
                        show = 1;
                        start = 1;
                        break;
                    case 346:
                        if (Pl_frame_check(source, 0, 2.0f, 0.0f) != 0) {
                            self->field_0x07--;
                            self->demo_flag_0x08 &= ~2;
                            work->field_0x65 = 0;
                            work->color_index_0x66 = source->field_0x5BA;
                        }
                        if (Pl_frame_check(source, 0, 18.0f, 0.0f) != 0) {
                            copyVec3(&pos, &self->pos_0x18);
                            pos.y += 20.0f;
                            eft019_set_core(&pos, self->area_0x44, 27, 0.7f, 0.7f);
                            if (self->area_0x44 == get_now_areano()) {
                                se_req_pos_id63(&pos);
                            }
                        }
                        if (Pl_frame_check(source, 1, 64.0f, 0.0f) != 0) {
                            self->field_0x06 = 0;
                            self->timer_0x0C = 0;
                        }
                        show = 2;
                        start = 1;
                        break;
                    case 347:
                        if (Pl_frame_check(source, 0, 2.0f, 0.0f) != 0) {
                            self->field_0x07--;
                            self->demo_flag_0x08 &= ~1;
                            work->field_0x65 = 0;
                            work->color_index_0x66 = source->field_0x5BA;
                        }
                        if (Pl_frame_check(source, 0, 18.0f, 0.0f) != 0) {
                            copyVec3(&pos, &self->pos_0x18);
                            pos.y += 20.0f;
                            eft019_set_core(&pos, self->area_0x44, 27, 0.7f, 0.7f);
                            if (self->area_0x44 == get_now_areano()) {
                                se_req_pos_id63(&pos);
                            }
                        }
                        if (Pl_frame_check(source, 1, 64.0f, 0.0f) != 0) {
                            self->field_0x06 = 1;
                            self->timer_0x0C = 0;
                        }
                        show = 1;
                        start = 1;
                        break;
                    case 348:
                        if (Pl_frame_check(source, 0, 2.0f, 0.0f) != 0) {
                            work->field_0x65 = 0;
                            if (self->field_0x07 == 1) {
                                self->demo_flag_0x08 = 0;
                                self->field_0x07 = 0;
                            } else {
                                self->field_0x07--;
                                self->demo_flag_0x08 &= ~2;
                            }
                            work->color_index_0x66 = source->field_0x5BA;
                        }
                        if (Pl_frame_check(source, 1, 60.0f, 0.0f) != 0) {
                            self->field_0x06 = 0;
                            self->timer_0x0C = 0;
                        }
                        show = 2;
                        start = 1;
                        break;
                    case 349:
                        if (Pl_frame_check(source, 0, 42.0f, 0.0f) != 0) {
                            self->demo_flag_0x08 |= 2;
                            self->field_0x07++;
                        }
                        if (Pl_frame_check(source, 3, 10.0f, 40.0f) != 0) {
                            show = 2;
                            work->color_index_0x66 = 0;
                        }
                        start = 1;
                        break;
                    case 350:
                        if (Pl_frame_check(source, 0, 46.0f, 0.0f) != 0) {
                            self->demo_flag_0x08 |= 1;
                            self->field_0x07++;
                        }
                        if (Pl_frame_check(source, 3, 20.0f, 44.0f) != 0) {
                            show = 1;
                            work->color_index_0x66 = 0;
                        }
                        start = 1;
                        break;
                    default:
                        show = 0;
                        start = 0;
                        break;
                    }
                    if (show != 0) {
                        color_a.r = eft_light_colors_a[work->color_index_0x66 * 3];
                        color_a.g = eft_light_colors_a[work->color_index_0x66 * 3 + 1];
                        color_a.b = eft_light_colors_a[work->color_index_0x66 * 3 + 2];
                        color_b.r = eft_light_colors_b[work->color_index_0x66 * 3];
                        color_b.g = eft_light_colors_b[work->color_index_0x66 * 3 + 1];
                        color_b.b = eft_light_colors_b[work->color_index_0x66 * 3 + 2];
                        work->slots_0x10.items.items_0x10[1]->setMatColor(3, GX_COLOR0A0, color_a, false);
                        work->slots_0x10.items.items_0x10[1]->setMatColor(4, GX_COLOR0A0, color_b, false);
                    }
                    ring_a.a = 0xFF;
                    ring_b.a = 0xFF;
                    core_a.a = 0xFF;
                    core_b.a = 0xFF;
                    if (source->field_0x00A == 0 && (u16)(source->act_no + 0xFF5E) <= 3) {
                        eft_light_ring_colors_get(0, 0, &ring_a, &ring_b, &core_a, &core_b);
                    } else if (self->timer_0x0C >= 4) {
                        if (self->field_0x06 == 0) {
                            eft_light_ring_colors_get(0, source->field_0x5BA, &ring_a, &ring_b, &core_a, &core_b);
                        } else {
                            eft_light_ring_colors_get(1, source->field_0x5BA, &ring_a, &ring_b, &core_a, &core_b);
                        }
                    } else if (self->field_0x06 == 0) {
                        eft_light_ring_colors_get(1, source->field_0x5BA, &ring_a, &ring_b, &core_a, &core_b);
                    } else {
                        eft_light_ring_colors_get(0, source->field_0x5BA, &ring_a, &ring_b, &core_a, &core_b);
                    }
                    if (beam != 0) {
                        eft_light_main_part_show(self, 12);
                        work->slots_0x10.items.items_0x10[0]->setVisibility(17, true);
                        eft_light_sub_part_show(self, 0xFF);
                        if ((self->demo_flag_0x08 & 1) != 0) {
                            work->slots_0x10.items.items_0x10[0]->setVisibility(19, true);
                            work->slots_0x10.items.items_0x10[0]->setMatColor(12, GX_COLOR0A0, core_a, false);
                            work->slots_0x10.items.items_0x10[0]->setMatColor(13, GX_COLOR0A0, core_b, false);
                        }
                        if ((self->demo_flag_0x08 & 2) != 0) {
                            work->slots_0x10.items.items_0x10[0]->setVisibility(18, true);
                            work->slots_0x10.items.items_0x10[0]->setMatColor(16, GX_COLOR0A0, ring_a, false);
                            work->slots_0x10.items.items_0x10[0]->setMatColor(17, GX_COLOR0A0, ring_b, false);
                        }
                        mtx34_identity(&pose);
                        pose.m[0][3] = source->motion_pos_0x3C;
                        pose.m[1][3] = source->ground_y_0x060;
                        pose.m[2][3] = source->motion_pos_0x44;
                        rotMatrixY(source->field_0x058, &pose);
                        work->slots_0x10.items.items_0x10[0]->move2(&pose, 0);
                        setVector3(&offset, 0.0f, 30.0f, 145.0f);
                        rotVecY(&offset, source->field_0x058);
                        mtx34_trans_add(&pose, &offset);
                        eft_light_beam_bind(self, work->slots_0x10.items.items_0x10[0], motion);
                        if (beam == 2) {
                            continue;
                        }
                    }
                    if (show != 0) {
                        eft_light_sub_part_show(self, 1);
                        switch ((s32)show) {
                        case 1:
                            mhchar_joint_mtx_get(EFT_PL_CHR(source), 7, &mtx);
                            self->rot_0x24.x = 0x2AAB;
                            self->rot_0x24.y = 0xE223;
                            self->rot_0x24.z = 0x1111;
                            offset.x = 41.0f;
                            offset.y = 14.0f;
                            offset.z = 32.0f;
                            break;
                        case 2:
                            mhchar_joint_mtx_get(EFT_PL_CHR(source), 11, &mtx);
                            self->rot_0x24.x = 0xC001;
                            self->rot_0x24.y = 0xAE3A;
                            self->rot_0x24.z = 0xF1C8;
                            offset.x = -30.0f;
                            offset.y = 6.0f;
                            offset.z = 45.0f;
                            break;
                        }
                    } else {
                        placed = 1;
                    }
                    if (self->field_0x07 != 0) {
                        work->field_0x67--;
                        if ((s8)work->field_0x67 < 0) {
                            work->field_0x67 = 12;
                            mtx34_trans_get(&pose, &sound_pos);
                            if (self->area_0x44 == get_now_areano()) {
                                se_req_pos_id27(&sound_pos);
                            }
                        }
                    }
                    break;
                case 40:
                case 47:
                    trail = 1;
                    self->field_0x07 = 0;
                    self->demo_flag_0x08 = 0;
                    show = 1;
                    work->head.scale_0x04.x = 1.0f;
                    work->head.scale_0x04.y = 1.0f;
                    work->head.scale_0x04.z = 1.0f;
                    switch (motion) {
                    case 333:
                        if (Pl_frame_check(source, 1, 26.0f, 0.0f) == 0) {
                            continue;
                        }
                        beam = 0;
                        start = 0;
                        break;
                    case 334:
                        if (Pl_frame_check(source, 1, 106.0f, 0.0f) != 0) {
                            continue;
                        }
                        if (Pl_frame_check(source, 1, 96.0f, 0.0f) != 0) {
                            show = 2;
                        }
                        if (Pl_frame_check(source, 0, 28.0f, 0.0f) != 0) {
                            copyVec3(&pos, &self->pos_0x18);
                            pos.y += 20.0f;
                            eft019_set_core(&pos, self->area_0x44, 27, 0.7f, 0.7f);
                            if (self->area_0x44 == get_now_areano()) {
                                se_req_pos_id63(&pos);
                            }
                        }
                        beam = 1;
                        start = 1;
                        break;
                    case 335:
                        if (Pl_frame_check(source, 1, 100.0f, 0.0f) != 0) {
                            continue;
                        }
                        if (Pl_frame_check(source, 1, 70.0f, 0.0f) != 0) {
                            show = 2;
                        }
                        beam = 1;
                        start = 1;
                        break;
                    default:
                        beam = 0;
                        start = 0;
                        break;
                    }
                    if (show != 0) {
                        eft_light_main_part_show(self, 4);
                        eft_light_sub_part_show(self, 0xFF);
                        mtx34_identity(&pose);
                        pose.m[0][3] = source->motion_pos_0x3C;
                        pose.m[1][3] = source->ground_y_0x060;
                        pose.m[2][3] = source->motion_pos_0x44;
                        rotMatrixY(source->field_0x058, &pose);
                        work->slots_0x10.items.items_0x10[0]->move2(&pose, 0);
                        eft_res_models_spawn(self, (void**)&work->slots_0x10.items.items_0x10[0], 2, 1, 0);
                        if (show == 2) {
                            continue;
                        }
                    }
                    color_a.r = eft_light_colors_a[source->field_0x5BA * 3];
                    color_a.g = eft_light_colors_a[source->field_0x5BA * 3 + 1];
                    color_a.b = eft_light_colors_a[source->field_0x5BA * 3 + 2];
                    color_b.r = eft_light_colors_b[source->field_0x5BA * 3];
                    color_b.g = eft_light_colors_b[source->field_0x5BA * 3 + 1];
                    color_b.b = eft_light_colors_b[source->field_0x5BA * 3 + 2];
                    color_a.a = 0xFF;
                    color_b.a = 0xFF;
                    work->slots_0x10.items.items_0x10[1]->setMatColor(3, GX_COLOR0A0, color_a, false);
                    work->slots_0x10.items.items_0x10[1]->setMatColor(4, GX_COLOR0A0, color_b, false);
                    if (beam != 0) {
                        eft_light_main_part_show(self, 4);
                        eft_light_sub_part_show(self, 1);
                        self->rot_0x24.x = 0x2AAB;
                        self->rot_0x24.y = 0xE223;
                        self->rot_0x24.z = 0x1111;
                        offset.x = 41.0f;
                        offset.y = 14.0f;
                        offset.z = 32.0f;
                        mhchar_joint_mtx_get(EFT_PL_CHR(source), 7, &mtx);
                        break;
                    }
                    eft_light_main_part_show(self, 4);
                    eft_light_sub_part_show(self, 2);
                    offset.x = 0.0f;
                    offset.y = 0.0f;
                    offset.z = 128.0f;
                    rotVecY(&offset, source->field_0x058);
                    pos.x = source->motion_pos_0x3C + offset.x;
                    pos.y = 77.0f + source->ground_y_0x060;
                    pos.z = source->motion_pos_0x44 + offset.z;
                    offset.x = -52.0f;
                    offset.y = 0.0f;
                    offset.z = 0.0f;
                    angle = source->field_0x058 + 0xC000;
                    rotVecY(&offset, angle);
                    pivot.x = pos.x + offset.x;
                    pivot.y = pos.y + offset.y;
                    pivot.z = pos.z + offset.z;
                    if (motion == 336) {
                        offset.x = 12.0f;
                        offset.y = 0.0f;
                        offset.z = 9.0f;
                        mhchar_joint_mtx_get(EFT_PL_CHR(source), 7, &mtx);
                        mulVecMat(&offset, &mtx);
                        x = offset.x + mtx.m[0][3];
                        y = offset.y + mtx.m[1][3];
                        z = offset.z + mtx.m[2][3];
                        offset.x = x - pivot.x;
                        offset.y = y - pivot.y;
                        offset.z = z - pivot.z;
                        rotVecY(&offset, 0x10000 - angle);
                        beam = (u16)(0.5f + (65536.0f * atan2f(offset.z, offset.y)) / 6.2831855f);
                        setVector3(&offset, 0.0f, 0.0f, 130.0f);
                        rotVecY(&offset, source->field_0x058);
                        mtx34_trans_add(&pose, &offset);
                        self->field_0x07 = 3;
                        work->field_0x67--;
                        if ((s8)work->field_0x67 < 0) {
                            work->field_0x67 = 12;
                            mtx34_trans_get(&pose, &sound_pos);
                            if (self->area_0x44 == get_now_areano()) {
                                se_req_pos_id27(&sound_pos);
                            }
                        }
                    } else {
                        beam = 0;
                    }
                    placed = 1;
                    mtx34_identity(&mtx);
                    rotMatrixX(beam, &mtx);
                    rotMatrixY(angle, &mtx);
                    mtx.m[0][3] = pos.x;
                    mtx.m[1][3] = pos.y;
                    mtx.m[2][3] = pos.z;
                    work->slots_0x10.items.items_0x10[1]->move2(&mtx, 0);
                    eft_res_models_spawn(self, (void**)&work->slots_0x10.items.items_0x10[1], 2, 1, 0);
                    break;
                case 438:
                    self->timer_0x0C++;
                    {
                        u8 kind = source->field_0x5BA;

                        color_a.r = eft_light_colors_a[kind * 3];
                        color_a.g = eft_light_colors_a[kind * 3 + 1];
                        color_a.b = eft_light_colors_a[kind * 3 + 2];
                        color_b.r = eft_light_colors_b[kind * 3];
                        color_b.g = eft_light_colors_b[kind * 3 + 1];
                        color_b.b = eft_light_colors_b[kind * 3 + 2];
                    }
                    color_a.a = 0xFF;
                    color_b.a = 0xFF;
                    switch (motion) {
                    case 333:
                        if (Pl_frame_check(source, 1, 26.0f, 0.0f) == 0) {
                            continue;
                        }
                        self->demo_flag_0x08 |= 1;
                        self->field_0x07 = 1;
                        self->field_0x06 = 0;
                        show = 0;
                        start = 1;
                        break;
                    case 334:
                        if (Pl_frame_check(source, 1, 106.0f, 0.0f) != 0) {
                            continue;
                        }
                        Pl_frame_check(source, 1, 96.0f, 0.0f);
                        if (Pl_frame_check(source, 0, 28.0f, 0.0f) != 0) {
                            copyVec3(&pos, &self->pos_0x18);
                            pos.y += 20.0f;
                            eft019_set_core(&pos, self->area_0x44, 27, 0.7f, 0.7f);
                            if (self->area_0x44 == get_now_areano()) {
                                se_req_pos_id63(&pos);
                            }
                        }
                        if (Pl_frame_check(source, 0, 2.0f, 0.0f) != 0) {
                            self->demo_flag_0x08 = 0;
                            self->field_0x07 = 0;
                            work->color_index_0x66 = source->field_0x5BA;
                        }
                        show = 1;
                        start = 1;
                        break;
                    case 335:
                        if (Pl_frame_check(source, 1, 100.0f, 0.0f) != 0) {
                            continue;
                        }
                        if (Pl_frame_check(source, 0, 78.0f, 0.0f) != 0) {
                            self->field_0x06 = 1;
                            self->timer_0x0C = 0;
                        }
                        if (Pl_frame_check(source, 0, 2.0f, 0.0f) != 0) {
                            work->field_0x65 = 0;
                            if (self->field_0x07 == 1) {
                                self->demo_flag_0x08 = 0;
                                self->field_0x07 = 0;
                            } else {
                                self->field_0x07--;
                                self->demo_flag_0x08 &= ~1;
                            }
                            work->color_index_0x66 = source->field_0x5BA;
                        }
                        if (self->field_0x07 == 0) {
                            Pl_frame_check(source, 1, 70.0f, 0.0f);
                        }
                        show = 1;
                        start = 1;
                        break;
                    case 346:
                        if (Pl_frame_check(source, 0, 2.0f, 0.0f) != 0) {
                            self->field_0x07--;
                            self->demo_flag_0x08 &= ~2;
                            work->field_0x65 = 0;
                            work->color_index_0x66 = source->field_0x5BA;
                        }
                        if (Pl_frame_check(source, 0, 18.0f, 0.0f) != 0) {
                            copyVec3(&pos, &self->pos_0x18);
                            pos.y += 20.0f;
                            eft019_set_core(&pos, self->area_0x44, 27, 0.7f, 0.7f);
                            if (self->area_0x44 == get_now_areano()) {
                                se_req_pos_id63(&pos);
                            }
                        }
                        if (Pl_frame_check(source, 1, 64.0f, 0.0f) != 0) {
                            self->field_0x06 = 0;
                            self->timer_0x0C = 0;
                        }
                        show = 2;
                        start = 1;
                        break;
                    case 347:
                        if (Pl_frame_check(source, 0, 2.0f, 0.0f) != 0) {
                            self->field_0x07--;
                            self->demo_flag_0x08 &= ~1;
                            work->field_0x65 = 0;
                            work->color_index_0x66 = source->field_0x5BA;
                        }
                        if (Pl_frame_check(source, 0, 18.0f, 0.0f) != 0) {
                            copyVec3(&pos, &self->pos_0x18);
                            pos.y += 20.0f;
                            eft019_set_core(&pos, self->area_0x44, 27, 0.7f, 0.7f);
                            if (self->area_0x44 == get_now_areano()) {
                                se_req_pos_id63(&pos);
                            }
                        }
                        if (Pl_frame_check(source, 1, 64.0f, 0.0f) != 0) {
                            self->field_0x06 = 1;
                            self->timer_0x0C = 0;
                        }
                        show = 1;
                        start = 1;
                        break;
                    case 348:
                        if (Pl_frame_check(source, 0, 2.0f, 0.0f) != 0) {
                            work->field_0x65 = 0;
                            if (self->field_0x07 == 1) {
                                self->demo_flag_0x08 = 0;
                                self->field_0x07 = 0;
                            } else {
                                self->field_0x07--;
                                self->demo_flag_0x08 &= ~2;
                            }
                            work->color_index_0x66 = source->field_0x5BA;
                        }
                        if (Pl_frame_check(source, 1, 60.0f, 0.0f) != 0) {
                            self->field_0x06 = 0;
                            self->timer_0x0C = 0;
                        }
                        show = 2;
                        start = 1;
                        break;
                    case 349:
                        if (Pl_frame_check(source, 0, 42.0f, 0.0f) != 0) {
                            self->demo_flag_0x08 |= 2;
                            self->field_0x07++;
                        }
                        if (Pl_frame_check(source, 3, 10.0f, 40.0f) != 0) {
                            show = 2;
                            work->color_index_0x66 = 0;
                        }
                        start = 1;
                        break;
                    case 350:
                        if (Pl_frame_check(source, 0, 46.0f, 0.0f) != 0) {
                            self->demo_flag_0x08 |= 1;
                            self->field_0x07++;
                        }
                        if (Pl_frame_check(source, 3, 20.0f, 44.0f) != 0) {
                            show = 1;
                            work->color_index_0x66 = 0;
                        }
                        start = 1;
                        break;
                    default:
                        show = 0;
                        start = 1;
                        break;
                    }
                    if (show != 0) {
                        color_a.r = eft_light_colors_a[work->color_index_0x66 * 3];
                        color_a.g = eft_light_colors_a[work->color_index_0x66 * 3 + 1];
                        color_a.b = eft_light_colors_a[work->color_index_0x66 * 3 + 2];
                        color_b.r = eft_light_colors_b[work->color_index_0x66 * 3];
                        color_b.g = eft_light_colors_b[work->color_index_0x66 * 3 + 1];
                        color_b.b = eft_light_colors_b[work->color_index_0x66 * 3 + 2];
                        work->slots_0x10.items.items_0x10[1]->setMatColor(3, GX_COLOR0A0, color_a, false);
                        work->slots_0x10.items.items_0x10[1]->setMatColor(4, GX_COLOR0A0, color_b, false);
                    }
                    if (show != 0) {
                        eft_light_sub_part_show(self, 1);
                        switch ((s32)show) {
                        case 1:
                            mhchar_joint_mtx_get(EFT_PL_CHR(source), 7, &mtx);
                            self->rot_0x24.x = 0x2AAB;
                            self->rot_0x24.y = 0xE223;
                            self->rot_0x24.z = 0x1111;
                            offset.x = 41.0f;
                            offset.y = 14.0f;
                            offset.z = 32.0f;
                            break;
                        case 2:
                            mhchar_joint_mtx_get(EFT_PL_CHR(source), 11, &mtx);
                            self->rot_0x24.x = 0xC001;
                            self->rot_0x24.y = 0xAE3A;
                            self->rot_0x24.z = 0xF1C8;
                            offset.x = -30.0f;
                            offset.y = 6.0f;
                            offset.z = 45.0f;
                            break;
                        }
                    } else {
                        placed = 1;
                    }
                    break;
                default:
                    continue;
                }
                break;
            case 338:
            case 339:
            case 340:
            case 341:
            case 342:
            case 343:
                eft_light_main_part_show(self, eft_light_motion_part_get(source));
                eft_light_sub_part_show(self, 0xFF);
                mhchar_joint_mtx_get(EFT_PL_CHR(source), 11, &mtx);
                setVector3(&offset, -9.2f, -1.7f, 12.0f);
                self->rot_0x24.x = 0;
                self->rot_0x24.y = 0;
                self->rot_0x24.z = 0xC001;
                work->head.scale_0x04.x = 1.0f;
                work->head.scale_0x04.y = 1.0f;
                work->head.scale_0x04.z = 1.0f;
                start = 0;
                break;
            case 359:
                if (Pl_frame_check(source, 3, 6.0f, 68.0f) == 0) {
                    continue;
                }
                eft_light_main_part_show(self, 13);
                eft_light_sub_part_show(self, 0xFF);
                mhchar_joint_mtx_get(EFT_PL_CHR(source), 11, &mtx);
                setVector3(&offset, -7.6f, -0.75f, 1.07f);
                self->rot_0x24.x = 0xF778;
                self->rot_0x24.y = 0xDA51;
                self->rot_0x24.z = 0x0333;
                work->head.scale_0x04.x = 1.0f;
                work->head.scale_0x04.y = 1.0f;
                work->head.scale_0x04.z = 1.0f;
                start = 0;
                break;
            case 420:
            case 421:
            case 422:
            case 423:
                switch (motion) {
                case 420:
                    if (Pl_frame_check(source, 2, 20.0f, 0.0f) != 0) {
                        continue;
                    }
                    break;
                case 423:
                    if (Pl_frame_check(source, 1, 90.0f, 0.0f) != 0) {
                        continue;
                    }
                    break;
                }
                eft_light_main_part_show(self, 15);
                eft_light_sub_part_show(self, 0xFF);
                switch (source->chunk_ofs) {
                case 1:
                case 3:
                    setVector3(&pos, 137.0f, 80.0f, 1983.0f);
                    break;
                default:
                    setVector3(&pos, -137.0f, 80.0f, 1983.0f);
                    break;
                }
                mhchar_joint_mtx_get(EFT_PL_CHR(source), 1, &mtx);
                mtx34_trans_get(&mtx, &self->pos_0x18);
                subVec3(&diff, &pos, &self->pos_0x18);
                copyVec3(&offset, &diff);
                x = vec3_len(&offset.x);
                work->head.scale_0x04.x = 1.0f;
                work->head.scale_0x04.y = 1.0f;
                work->head.scale_0x04.z = x / 1000.0f;
                calcVecAngXY(&offset, &self->rot_0x24.x, &self->rot_0x24.y);
                self->rot_0x24.y += 0x10000 - (u16)source->field_0x058;
                self->rot_0x24.z = 0;
                setVector3(&offset, 0.0f, 0.0f, 0.0f);
                start = 0;
                break;
            case 403:
            case 404:
            case 408:
            case 409:
                if (work->motion_0x30 != motion) {
                    eft025_pl_set(source, 2);
                }
                beam = 1;
            default:
                if (EFT_PL_MENU(source)->item_menu_0x5BB != 0) {
                    eft_light_main_part_show(self, 5);
                    eft_light_sub_part_show(self, 0xFF);
                    mhchar_joint_mtx_get(EFT_PL_CHR(source), 11, &mtx);
                    setVector3(&offset, -10.0f, 0.0f, 5.0f);
                    self->rot_0x24.x = 0xFC73;
                    self->rot_0x24.y = 0xFB07;
                    self->rot_0x24.z = 0;
                    work->head.scale_0x04.x = 1.0f;
                    work->head.scale_0x04.y = 1.0f;
                    work->head.scale_0x04.z = 1.0f;
                    mtx34_trans_get(&mtx, &self->pos_0x18);
                    setVector3(&pos, 0.0f, 0.0f, 55.0f);
                    addVec3To(&pos, &offset);
                    mulVecMat(&pos, &mtx);
                    addVec3To(&pos, &self->pos_0x18);
                    self->field_0x10--;
                    if (motion != 401 || Pl_frame_check(source, 2, 26.0f, 0.0f) == 0) {
                        if (self->field_0x10 < 0) {
                            eft025_pl_set(source, beam);
                            if (beam == 1) {
                                self->field_0x10 = 1;
                            } else {
                                self->field_0x10 = 4;
                            }
                        }
                        if (self->area_0x44 == get_now_areano()) {
                            setVector3(&pivot, 0.0f, 0.0f, -20.0f);
                            mhchar_joint_mtx_get(EFT_PL_CHR(source), 13, &local);
                            mulVecMat(&pivot, &local);
                            pivot.x += local.m[0][3];
                            pivot.y += local.m[1][3];
                            pivot.z += local.m[2][3];
                            copyVec3(&light_pos, &pos);
                            coll_wall_slide_c1(&pivot, &pos, 40.0f, &light_pos, 0xFFFF, 0, 1, self->area_0x44);
                            light_point_scale_set(work->light_id_0x2C, &light_pos, 1.0f, 0);
                        }
                    }
                    start = 0;
                    break;
                }
                if (motion != 402 || Pl_frame_check(source, 2, 50.0f, 0.0f) == 0) {
                    continue;
                }
                eft_light_main_part_show(self, 5);
                eft_light_sub_part_show(self, 0xFF);
                mhchar_joint_mtx_get(EFT_PL_CHR(source), 11, &mtx);
                setVector3(&offset, -10.0f, 0.0f, 5.0f);
                self->rot_0x24.x = 0;
                self->rot_0x24.y = 0;
                self->rot_0x24.z = 0;
                work->head.scale_0x04.x = 1.0f;
                work->head.scale_0x04.y = 1.0f;
                work->head.scale_0x04.z = 1.0f;
                start = 0;
                if (Pl_frame_check(source, 2, 20.0f, 0.0f) != 0) {
                    if (--self->field_0x10 < 0) {
                        eft025_pl_set(source, beam);
                        self->field_0x10 = 4;
                    }
                }
                break;
            }
        }
        work->motion_0x30 = motion;
        if (placed == 0) {
            mulVecMat(&offset, &mtx);
            mtx34_trans_get(&mtx, &self->pos_0x18);
            self->pos_0x18.x += offset.x;
            self->pos_0x18.y += offset.y;
            self->pos_0x18.z += offset.z;
            mtx34_identity(&local);
            scaleMat34W(&local, &work->head.scale_0x04);
            rotMatrixXYZ(&self->rot_0x24, &local);
            mtx34_concat_assign(&mtx, &local);
            mtx.m[0][3] = self->pos_0x18.x;
            mtx.m[1][3] = self->pos_0x18.y;
            mtx.m[2][3] = self->pos_0x18.z;
            mode = 2;
            if (Pl_master_ck(source) == 1) {
                mode = eft_stage_spawn_flag(source) | 2;
            }
            for (i = start; i < work->count; i++) {
                work->slots_0x10.items.items_0x10[i]->move2(&mtx, 0);
                eft_res_models_spawn(self, (void**)&work->slots_0x10.items.items_0x10[i], mode, 1, 0);
            }
        }
    } while (0);
    eft_light_trail_update(self, trail);
}
#pragma optimization_level reset
#pragma peephole reset

/* Drops the thrown light: it falls with the record's speed until it lands (a lantern lights up, a bomb-type item
 * bursts), then fades out by the key table and places the model at the record's position. */
#pragma peephole off
#pragma fp_contract off
#pragma optimization_level 4
void eft_light_drop_update(_EFT* self)
{
    _PLW* source;
    EftWork* work;
    _GXColor color;

    source = (_PLW*)self->source_0x30;
    work = EFT_WORK(self);
    self->flag_0x01 = source->field_0x001;
    self->area_0x44 = source->area_0x16;
    self->timer_0x0C++;
    self->field_0x10++;
    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    switch (self->field_0x06) {
    case 0:
        if (eft_water_state_ck(self) == 0) {
            work->field_0x6C += -0.72727275f * (f32)self->field_0x10;
        } else {
            work->field_0x6C += (f32)self->field_0x10;
            if (work->field_0x6C >= -5.0f) {
                work->field_0x6C = -5.0f;
            }
        }
        self->pos_0x18.y += work->field_0x6C;
        if (self->pos_0x18.y < 5.0f + work->field_0x68) {
            switch ((s32)(source->field_0x5C4 & 0xF0) >> 4) {
            case 1:
            case 2:
                self->pos_0x18.y = 5.0f + source->ground_y_0x060;
                self->rot_0x24.x = 0;
                self->rot_0x24.y = 0;
                self->rot_0x24.z = 0;
                self->timer_0x0C = 0;
                if (self->area_0x44 == get_now_areano()) {
                    se_req_pos_id59(&self->pos_0x18);
                }
                self->field_0x06++;
                if (GameMode_ck() == 2) {
                    eft_light_lobby_part_show(self, 2);
                } else {
                    eft_light_main_part_show(self, 16);
                }
                setVector3(&work->head.scale_0x04, 1.0f, 1.0f, 1.0f);
                copyVec3(&work->slots_0x10.items.items_0x10[0]->scale_0x1C, &work->head.scale_0x04);
                work->slots_0x10.items.items_0x10[0]->ready = 1;
                break;
            default:
                self->state_0x05++;
                self->flag_0x01 = 0;
                return;
            case 4:
                self->state_0x05++;
                self->flag_0x01 = 0;
                {
                    u8 area = self->area_0x44;
                    if (area == get_now_areano()) {
                        eft019_set(&self->pos_0x18, area, 0);
                    }
                }
                shell_set_func_ptr->method_0x08(&self->pos_0x18, 0, self->area_0x44, 0);
                return;
            }
        }
        if (self->timer_0x0C > 60) {
            self->field_0x06 = 2;
            self->timer_0x0C = 0;
            work->slots_0x10.items.items_0x10[0]->ready = 1;
        }
        break;
    case 1:
        switch ((s32)(source->field_0x5C4 & 0xF0) >> 4) {
        case 1:
        case 2:
            if (self->timer_0x0C > 20) {
                self->state_0x05++;
                self->flag_0x01 = 0;
                return;
            }
            color.a = eftGetKeyAlpha(eft_light_fade_keys, self->timer_0x0C);
            color.b = 0xFF;
            color.g = 0xFF;
            color.r = 0xFF;
            if (GameMode_ck() == 2) {
                work->slots_0x10.items.items_0x10[0]->setMatColor(1, GX_COLOR0A0, color, false);
                work->slots_0x10.items.items_0x10[0]->setMatColor(3, GX_COLOR0A0, color, false);
            } else {
                work->slots_0x10.items.items_0x10[0]->setMatColor(10, GX_COLOR0A0, color, false);
                work->slots_0x10.items.items_0x10[0]->setMatColor(20, GX_COLOR0A0, color, false);
            }
            break;
        default:
            if (self->timer_0x0C > 10) {
                self->state_0x05++;
                self->flag_0x01 = 0;
                return;
            }
            break;
        }
        break;
    default:
        if (eft_water_state_ck(self) == 0) {
            work->field_0x6C += -0.72727275f * (f32)self->field_0x10;
        } else {
            work->field_0x6C += (f32)self->field_0x10;
            if (work->field_0x6C >= -5.0f) {
                work->field_0x6C = -5.0f;
            }
        }
        self->pos_0x18.y += work->field_0x6C;
        switch ((s32)(source->field_0x5C4 & 0xF0) >> 4) {
        case 1:
        case 2:
            if (self->timer_0x0C > 20) {
                self->state_0x05++;
                self->flag_0x01 = 0;
                return;
            }
            color.a = eftGetKeyAlpha(eft_light_fade_keys, self->timer_0x0C);
            color.b = 0xFF;
            color.g = 0xFF;
            color.r = 0xFF;
            if (GameMode_ck() == 2) {
                work->slots_0x10.items.items_0x10[0]->setMatColor(0, GX_COLOR0A0, color, false);
            } else {
                work->slots_0x10.items.items_0x10[0]->setMatColor(6, GX_COLOR0A0, color, false);
            }
            break;
        default:
            if (self->timer_0x0C > 10) {
                self->state_0x05++;
                self->flag_0x01 = 0;
                return;
            }
            break;
        }
        break;
    }
    switch (self->field_0x06) {
    case 0:
    case 2:
        work->body_0x34.matrix_0x34.m[0][3] = self->pos_0x18.x;
        work->body_0x34.matrix_0x34.m[1][3] = self->pos_0x18.y;
        work->body_0x34.matrix_0x34.m[2][3] = self->pos_0x18.z;
        work->slots_0x10.items.items_0x10[0]->move2(&work->body_0x34.matrix_0x34, 0);
        break;
    case 1:
        copyVec3(&work->slots_0x10.items.items_0x10[0]->pos_0x04, &self->pos_0x18);
        eft_rot_vec_copy(&work->slots_0x10.items.items_0x10[0]->rot_0x28, &self->rot_0x24);
        work->slots_0x10.items.items_0x10[0]->move(0);
        break;
    }
    eft_res_models_spawn(self, (void**)&work->slots_0x10.items.items_0x10[0], 2, 1, 0);
}
#pragma optimization_level reset
#pragma fp_contract reset
#pragma peephole reset

/* Places the light models on the NPC's hand joint while it holds its lantern motion, coloured by the NPC's item, and
 * hands them to the model spawner. */
#pragma peephole off
void eft_light_npc_update(_EFT* self)
{
    VEC3 offset;
    VEC3 pos;
    MTX34 mtx;
    MTX34 local;
    _AINPC_W* source;
    EftWork* work;
    u16 motion;
    u8 color_index;
    s32 i;
    _GXColor color_a;
    _GXColor color_b;

    VEC3_ctor(&offset);
    VEC3_ctor(&pos);
    MTX34_ctor(&mtx);
    MTX34_ctor(&local);
    work = EFT_WORK(self);
    if (eft_res_spawn_gate_ck(self, 2) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    source = (_AINPC_W*)self->source_0x30;
    self->flag_0x01 = source->in_area;
    self->area_0x44 = source->area;
    motion = ai_get_motion_no(source);
    switch (motion) {
    case 7:
        if (ai_npc_frame_ck(source, 1, 90.0f, 0.0f) != 0) {
            return;
        }
        eft_light_npc_part_show(self, 1);
        setVector3(&offset, -3.5f, 0.0f, 42.0f);
        self->rot_0x24.x = 0x2000;
        self->rot_0x24.y = 0xC001;
        self->rot_0x24.z = 0x2000;
        switch (source->field_0x3C2) {
        case 27:
            color_index = 1;
            break;
        case 26:
            color_index = 2;
            if (ai_npc_frame_ck(source, 0, 26.0f, 0.0f) != 0) {
                copyVec3(&pos, &self->pos_0x18);
                pos.y += 15.0f;
                eft019_set_core(&pos, self->area_0x44, 27, 0.7f, 0.7f);
            }
            break;
        case 28:
            color_index = 4;
            break;
        case 30:
            color_index = 5;
            break;
        case 31:
            color_index = 6;
            break;
        case 32:
            color_index = 7;
            break;
        default:
            color_index = 0;
            break;
        }
        color_a.r = eft_light_colors_a[color_index * 3];
        color_a.g = eft_light_colors_a[color_index * 3 + 1];
        color_a.b = eft_light_colors_a[color_index * 3 + 2];
        color_b.r = eft_light_colors_b[color_index * 3];
        color_b.g = eft_light_colors_b[color_index * 3 + 1];
        color_b.b = eft_light_colors_b[color_index * 3 + 2];
        color_a.a = 0xFF;
        color_b.a = 0xFF;
        work->slots_0x10.items.items_0x10[0]->setMatColor(3, GX_COLOR0A0, color_a, false);
        work->slots_0x10.items.items_0x10[0]->setMatColor(4, GX_COLOR0A0, color_b, false);
        mhchar_joint_mtx_get(&source->model, 12, &mtx);
        work->head.scale_0x04.x = 0.9f;
        work->head.scale_0x04.y = 0.9f;
        work->head.scale_0x04.z = 0.9f;
        break;
    default:
        return;
    }
    work->motion_0x30 = motion;
    mulVecMat(&offset, &mtx);
    mtx34_trans_get(&mtx, &self->pos_0x18);
    self->pos_0x18.x += offset.x;
    self->pos_0x18.y += offset.y;
    self->pos_0x18.z += offset.z;
    mtx34_identity(&local);
    scaleMat34W(&local, &work->head.scale_0x04);
    rotMatrixXYZ(&self->rot_0x24, &local);
    mtx34_concat_assign(&mtx, &local);
    mtx.m[0][3] = self->pos_0x18.x;
    mtx.m[1][3] = self->pos_0x18.y;
    mtx.m[2][3] = self->pos_0x18.z;
    for (i = 0; i < work->count; i++) {
        work->slots_0x10.items.items_0x10[i]->move2(&mtx, 0);
        eft_res_models_spawn(self, (void**)&work->slots_0x10.items.items_0x10[i], 2, 1, 0);
    }
}
#pragma peephole reset

/* Places the single light model on the player's hand joint for the lantern pose or the map-21 carry motions, and
 * hands it to the model spawner; any other pose skips the frame. */
#pragma peephole off
void eft_light_lobby_update(_EFT* self)
{
    VEC3 offset;
    VEC3 unused_a;
    VEC3 unused_b;
    VEC3 unused_c;
    MTX34 mtx;
    MTX34 local;
    MTX34 unused_mtx;
    _PLW* source;
    EftWork* work;
    u16 motion;
    s32 mode;

    VEC3_ctor(&offset);
    VEC3_ctor(&unused_a);
    VEC3_ctor(&unused_b);
    VEC3_ctor(&unused_c);
    MTX34_ctor(&mtx);
    MTX34_ctor(&local);
    MTX34_ctor(&unused_mtx);
    work = EFT_WORK(self);
    if (eft_res_spawn_gate_ck(self, 0) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    source = (_PLW*)self->source_0x30;
    self->flag_0x01 = source->field_0x001;
    self->area_0x44 = source->area_0x16;
    motion = Get_motion_no(source);
    if ((source->field_0x5C4 & 0xF) != 0) {
        if (lb_pl_act_off_ck(source) != 0 && (u32)(((s32)(source->field_0x5C4 & 0xF0) >> 4) - 1) <= 1) {
            eft_light_lobby_part_show(self, 1);
            work->head.scale_0x04.x = 1.0f;
            work->head.scale_0x04.y = 1.3f;
            work->head.scale_0x04.z = 1.0f;
            self->rot_0x24.x = 0xD99B;
            self->rot_0x24.y = 0x7A51;
            self->rot_0x24.z = 0x53E9;
            setVector3(&offset, 5.5f, -28.0f, 12.0f);
            mhchar_joint_mtx_get(EFT_PL_CHR(source), 7, &mtx);
        } else {
            return;
        }
    } else if (get_now_mapno() == 21) {
        switch (motion) {
        case 0x261:
        case 0x263:
        case 0x264:
        case 0x265:
        case 0x266:
        case 0x267:
        case 0x268:
        case 0x269:
        case 0x26A:
        case 0x271:
        case 0x272:
            eft_light_lobby_part_show(self, 3);
            self->rot_0x24.x = 0x4000;
            self->rot_0x24.y = 0;
            self->rot_0x24.z = 0;
            work->head.scale_0x04.x = 1.0f;
            work->head.scale_0x04.y = 1.0f;
            work->head.scale_0x04.z = 1.0f;
            setVector3(&offset, -23.5f, 0.0f, -13.5f);
            mhchar_joint_mtx_get(EFT_PL_CHR(source), 11, &mtx);
            break;
        default:
            return;
        }
    } else {
        return;
    }
    work->motion_0x30 = motion;
    mulVecMat(&offset, &mtx);
    mtx34_trans_get(&mtx, &self->pos_0x18);
    self->pos_0x18.x += offset.x;
    self->pos_0x18.y += offset.y;
    self->pos_0x18.z += offset.z;
    mtx34_identity(&local);
    scaleMat34W(&local, &work->head.scale_0x04);
    rotMatrixXYZ(&self->rot_0x24, &local);
    mtx34_concat_assign(&mtx, &local);
    mtx.m[0][3] = self->pos_0x18.x;
    mtx.m[1][3] = self->pos_0x18.y;
    mtx.m[2][3] = self->pos_0x18.z;
    mode = 2;
    if (Pl_master_ck(source) == 1) {
        mode = eft_stage_spawn_flag(source) | 2;
    }
    work->slots_0x10.items.items_0x10[0]->move2(&mtx, 0);
    eft_res_models_spawn(self, (void**)&work->slots_0x10.items.items_0x10[0], mode, 1, 0);
}
#pragma peephole reset

/* Advances the state. */
void eft_light_state_next(_EFT* self)
{
    self->state_0x05++;
}

/* Drops the sub-effect record. */
void eft_light_retire(_EFT* self)
{
    eft_res_slot_release(self);
}

/* Shows only the numbered light of the first item. */
void eft_light_main_part_show(_EFT* self, u32 arg1)
{
    s32 i;
    EftWork* work;

    work = EFT_WORK(self);
    for (i = 1; i <= 0x13; i++) {
        if (i == arg1) {
            work->slots_0x10.items.items_0x10[0]->setVisibility(i, true);
        } else {
            work->slots_0x10.items.items_0x10[0]->setVisibility(i, false);
        }
    }
}

/* Shows one or all of the second item's lights. */
void eft_light_sub_part_show(_EFT* self, u32 arg1)
{
    EftWork* work;
    s32 i;

    work = EFT_WORK(self);
    if (arg1 == 2) {
        for (i = 1; i < 3; i++) {
            work->slots_0x10.items.items_0x10[1]->setVisibility(i, true);
        }
        return;
    }
    for (i = 1; i < 5; i++) {
        if (i == arg1) {
            work->slots_0x10.items.items_0x10[1]->setVisibility(i, true);
        } else {
            work->slots_0x10.items.items_0x10[1]->setVisibility(i, false);
        }
    }
}

/* Shows one or all of the first item's lights. */
void eft_light_npc_part_show(_EFT* self, u32 arg1)
{
    EftWork* work;
    s32 i;

    work = EFT_WORK(self);
    if (arg1 == 2) {
        for (i = 1; i < 3; i++) {
            work->slots_0x10.items.items_0x10[0]->setVisibility(i, true);
        }
        return;
    }
    for (i = 1; i < 5; i++) {
        if (i == arg1) {
            work->slots_0x10.items.items_0x10[0]->setVisibility(i, true);
        } else {
            work->slots_0x10.items.items_0x10[0]->setVisibility(i, false);
        }
    }
}

/* Shows only the numbered light of the first item. */
void eft_light_lobby_part_show(_EFT* self, u32 arg1)
{
    s32 i;
    EftWork* work;

    work = EFT_WORK(self);
    for (i = 1; i <= 3; i++) {
        if (i == arg1) {
            work->slots_0x10.items.items_0x10[0]->setVisibility(i, true);
        } else {
            work->slots_0x10.items.items_0x10[0]->setVisibility(i, false);
        }
    }
}

/* Aims the light model's three beam joints from the player's side at the held joint: the job callback
 * `eft_light_beam_bind` installs. */
void eft_light_beam_aim_job(struct PrimRec* job)
{
    EftWork* work;
    _EFT* self;
    _PLW* source;
    MTX34 beam_mtx;
    MTX34 back_mtx;
    VEC3 dir;
    VEC3 unused;
    VEC3 base;
    VEC3 pivot;
    u32 angle;
    f32 y;
    f32 x;
    f32 z;
    MTX34* world;

    self = ((EftLightJob*)job)->owner_0x2C;
    work = EFT_WORK(self);
    MTX34_ctor(&beam_mtx);
    MTX34_ctor(&back_mtx);
    VEC3_ctor(&dir);
    VEC3_ctor(&unused);
    VEC3_ctor(&base);
    VEC3_ctor(&pivot);
    source = (_PLW*)self->source_0x30;
    dir.x = 0.0f;
    dir.y = 0.0f;
    dir.z = 128.0f;
    rotVecY(&dir, source->field_0x058);
    base.x = source->motion_pos_0x3C + dir.x;
    base.y = 77.0f + source->ground_y_0x060;
    base.z = source->motion_pos_0x44 + dir.z;
    dir.x = -52.0f;
    dir.y = 0.0f;
    dir.z = 0.0f;
    angle = (u16)(source->field_0x058 + 0xC000);
    rotVecY(&dir, angle);
    pivot.x = base.x + dir.x;
    pivot.y = base.y + dir.y;
    pivot.z = base.z + dir.z;
    switch (Get_motion_no(source)) {
    case 0x15E:
        if (Pl_frame_check(source, 3, 6.0f, 54.0f) != 0) {
            dir.x = -12.0f;
            dir.y = 0.0f;
            dir.z = 9.0f;
            mhchar_joint_mtx_get(EFT_PL_CHR(source), 11, &beam_mtx);
        } else {
            dir.x = 12.0f;
            dir.y = 0.0f;
            dir.z = 9.0f;
            mhchar_joint_mtx_get(EFT_PL_CHR(source), 7, &beam_mtx);
        }
        break;
    default:
        dir.x = 12.0f;
        dir.y = 0.0f;
        dir.z = 9.0f;
        mhchar_joint_mtx_get(EFT_PL_CHR(source), 7, &beam_mtx);
        break;
    }
    mulVecMat(&dir, &beam_mtx);
    x = dir.x + beam_mtx.m[0][3];
    y = dir.y + beam_mtx.m[1][3];
    z = dir.z + beam_mtx.m[2][3];
    dir.x = x - pivot.x;
    dir.y = y - pivot.y;
    dir.z = z - pivot.z;
    rotVecY(&dir, 0x10000 - angle);
    angle = (u16)(0.5f + (65536.0f * atan2f(dir.z, dir.y)) / 6.2831855f);
    mtx34_identity(&beam_mtx);
    mtx34_identity(&back_mtx);
    rotMatrixY(source->field_0x058, &beam_mtx);
    rotLocalMatZ(angle, &beam_mtx);
    rotMatrixY(source->field_0x058, &back_mtx);
    rotLocalMatZ(0x10000 - angle, &back_mtx);
    world = mhchar_wld_mtx_array_get((nw4r::g3d::ScnMdlSimple*)work->slots_0x10.items.items_0x10[0]->field_0x118);
    {
        nw4r::g3d::ResNode node = ((nw4r::g3d::ResMdl*)&work->slots_0x10.items.items_0x10[0]->field_0x114)->GetResNode(21);
        copyMat33(&world[res_node_mtx_id_get(&node)], &beam_mtx);
    }
    {
        nw4r::g3d::ResNode node = ((nw4r::g3d::ResMdl*)&work->slots_0x10.items.items_0x10[0]->field_0x114)->GetResNode(22);
        copyMat33(&world[res_node_mtx_id_get(&node)], &back_mtx);
    }
    {
        nw4r::g3d::ResNode node = ((nw4r::g3d::ResMdl*)&work->slots_0x10.items.items_0x10[0]->field_0x114)->GetResNode(23);
        copyMat33(&world[res_node_mtx_id_get(&node)], &back_mtx);
    }
}

/* Attaches a light effect to the model for the named motion. */
void eft_light_beam_bind(_EFT* self, MHchar* model, s32 arg2)
{
    EftWork* work;
    EftLightJob* job;
    _PLW* source;

    work = EFT_WORK(self);
    source = (_PLW*)self->source_0x30;
    if ((u32)((u16)arg2 - 0x15D) <= 1
        || ((s32)(u16)arg2 == 0x150 && Pl_frame_check_alt(source, 1, 0.0f, 0.0f) != 0)) {
        if (self->area_0x44 != get_now_areano() || self->flag_0x01 == 0) {
            g3d_root_model_bind(pRoot, model->field_0x118);
            return;
        }
        if (model != NULL && model->field_0x118 == ((EftG3dWork*)model->g3d_0x10C)->model_0x04) {
            job = (EftLightJob*)sound_job_request(1, model->ready, model->field_0x118, 0, 0x40, NULL, eft_light_beam_aim_job);
            if (job != NULL) {
                job->owner_0x2C = self;
            }
            g3d_root_model_bind(pRoot, model->field_0x118);
        }
    } else {
        eft_res_models_spawn(self, (void**)&work->slots_0x10.items.items_0x10[0], 2, 1, 0);
    }
}

#pragma peephole off
void eft_light_ring_colors_get(u8 arg0, s32 arg1, _GXColor* arg2, _GXColor* arg3, _GXColor* arg4, _GXColor* arg5)
{
    switch (arg0) {
    case 0:
        arg2->r = eft_light_colors_a[0];
        arg2->g = eft_light_colors_a[1];
        arg2->b = eft_light_colors_a[2];
        arg3->r = eft_light_colors_b[0];
        arg3->g = eft_light_colors_b[1];
        arg3->b = eft_light_colors_b[2];
        arg4->r = eft_light_colors_a[(u8)arg1 * 3];
        arg4->g = eft_light_colors_a[(u8)arg1 * 3 + 1];
        arg4->b = eft_light_colors_a[(u8)arg1 * 3 + 2];
        arg5->r = eft_light_colors_b[(u8)arg1 * 3];
        arg5->g = eft_light_colors_b[(u8)arg1 * 3 + 1];
        arg5->b = eft_light_colors_b[(u8)arg1 * 3 + 2];
        return;
    case 1:
        arg4->r = eft_light_colors_a[0];
        arg4->g = eft_light_colors_a[1];
        arg4->b = eft_light_colors_a[2];
        arg5->r = eft_light_colors_b[0];
        arg5->g = eft_light_colors_b[1];
        arg5->b = eft_light_colors_b[2];
        arg2->r = eft_light_colors_a[(u8)arg1 * 3];
        arg2->g = eft_light_colors_a[(u8)arg1 * 3 + 1];
        arg2->b = eft_light_colors_a[(u8)arg1 * 3 + 2];
        arg3->r = eft_light_colors_b[(u8)arg1 * 3];
        arg3->g = eft_light_colors_b[(u8)arg1 * 3 + 1];
        arg3->b = eft_light_colors_b[(u8)arg1 * 3 + 2];
        return;
    }
}
#pragma peephole reset

/* Picks the frame-range index for the player's current motion. */
s32 eft_light_motion_part_get(_PLW* self)
{
    s32 part;

    switch (Get_motion_no(self)) {
    case 0x154:
        if (Pl_frame_check(self, 2, 20.0f, 0.0f) != 0) {
            part = 7;
        } else if (Pl_frame_check(self, 2, 30.0f, 0.0f) != 0) {
            part = 8;
        } else {
            part = 9;
        }
        break;
    case 0x155:
        if (Pl_frame_check(self, 2, 20.0f, 0.0f) != 0) {
            part = 8;
        } else if (Pl_frame_check(self, 2, 26.0f, 0.0f) != 0) {
            part = 9;
        } else if (Pl_frame_check(self, 2, 28.0f, 0.0f) != 0) {
            part = 8;
        } else {
            part = 7;
        }
        break;
    case 0x156:
        if (Pl_frame_check(self, 2, 36.0f, 0.0f) != 0) {
            part = 8;
        } else if (Pl_frame_check(self, 2, 40.0f, 0.0f) != 0) {
            part = 9;
        } else if (Pl_frame_check(self, 2, 42.0f, 0.0f) != 0) {
            part = 8;
        } else {
            part = 7;
        }
        break;
    case 0x157:
        if (Pl_frame_check(self, 2, 20.0f, 0.0f) != 0) {
            part = 8;
        } else if (Pl_frame_check(self, 2, 22.0f, 0.0f) != 0) {
            part = 9;
        } else if (Pl_frame_check(self, 2, 24.0f, 0.0f) != 0) {
            part = 8;
        } else {
            part = 7;
        }
        break;
    default:
        part = 7;
        break;
    }
    return part;
}

/* Places the effect at the weapon tip and the point light at the blade, slid along the walls and kept above the
 * ground. */
void eft_light_blade_place(_EFT* self)
{
    MTX34 grip_mtx;
    MTX34 tip_mtx;
    VEC3 offset;
    VEC3 blade;
    VEC3 tip;
    VEC3 light_pos;
    _PLW* source;
    EftWork* work;

    MTX34_ctor(&grip_mtx);
    MTX34_ctor(&tip_mtx);
    VEC3_ctor(&offset);
    VEC3_ctor(&blade);
    VEC3_ctor(&tip);
    VEC3_ctor(&light_pos);
    work = EFT_WORK(self);
    if (self->area_0x44 == get_now_areano()) {
        source = (_PLW*)self->source_0x30;
        mhchar_joint_mtx_get(EFT_PL_CHR(source), 11, &grip_mtx);
        mtx34_trans_get(&grip_mtx, &self->pos_0x18);
        setVector3(&blade, 0.0f, 0.0f, 55.0f);
        setVector3(&offset, -10.0f, 0.0f, 5.0f);
        addVec3To(&blade, &offset);
        mulVecMat(&blade, &grip_mtx);
        blade.x += grip_mtx.m[0][3];
        blade.y += grip_mtx.m[1][3];
        blade.z += grip_mtx.m[2][3];
        setVector3(&tip, 0.0f, 0.0f, -20.0f);
        mhchar_joint_mtx_get(EFT_PL_CHR(source), 13, &tip_mtx);
        mulVecMat(&tip, &tip_mtx);
        tip.x += tip_mtx.m[0][3];
        tip.y += tip_mtx.m[1][3];
        tip.z += tip_mtx.m[2][3];
        copyVec3(&light_pos, &blade);
        coll_wall_slide_c1(&tip, &blade, 40.0f, &light_pos, 0xFFFF, 0, 1, self->area_0x44);
        if (light_pos.y < 10.0f + source->ground_y_0x060) {
            light_pos.y = 10.0f + source->ground_y_0x060;
        }
        light_point_scale_set(work->light_id_0x2C, &light_pos, 1.0f, 0);
    }
}

}
