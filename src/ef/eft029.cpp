/* ef/eft029.cpp - the eft028 effect records' handlers: the timed start `eft028_start_by_area` (picks the
 *   (id, param) pair from the map, stage area and item-list state, builds the pool and places it by type), the plain
 *   start `eft028_start`, the per-type move dispatcher `eft028_move` and its four handlers, the state step and the
 *   release that `ef/fn_80119C44.c`'s dispatcher installs, and the static initialiser of the zone capsule's ends.
 * RANGE. .text 0x80119DEC-0x8011AD58 (10 functions); extab 0x8000C56C-0x8000C5A4, extabindex 0x8002670C-0x80026760,
 *   .ctors 0x8056F314-0x8056F318, .data 0x805A06F0-0x805A07F0, .bss 0x806A4548-0x806A4560, .sdata
 *   0x80791970-0x80791978, .sdata2 0x80796B30-0x80796B50.  The family's setters `eft029_set_scale`/`eft029_set_kaihou`
 *   (the runtime dump's names) and the rest of it are `ef/eft029_fx.cpp`.
 * FLAGS. `cflags_main`; `#pragma peephole off` over every body.
 * NAMES. GUESS (from the bodies and `ef/fn_80119C44.c`'s dispatcher, which builds the eft028 records
 *   `ef/eft026_fx.cpp`'s eft028 setters ask for; the unit's own name is a misnomer):
 *   `eft028_start_by_area`, `eft028_start`, `eft028_move`, `eft028_move_view_fade`, `eft028_move_scaled`,
 *   `eft028_move_zone_pulse`, `eft028_move_view`, `eft028_state_next`, `eft028_free`; the data `eft028_effect_ids`,
 *   `eft028_param_ids`, `eft028_alpha_by_id`, `eft028_view_alpha_keys`, `eft028_capsule_ends`; the static initialiser is
 *   MWCC's `__sinit_\eft029_cpp`.
 * RESIDUALS. none in code.  `.data` is 0xFC of the claim's 0x100 (the claim's last
 *   word is the link's alignment fill).
 * SHAPES. The vector helpers are called through their by-value spellings (`EFT_FOCUS_POS`, `VEC3_SUB`, `VEC3_ADD`,
 *   `VEC3_SCALE`): retail builds every result in a compiler temporary in call order, which explicit out-locals do not
 *   reproduce.  The zone test is read as a `u8` return (raw `stb`); the colour bytes are `(c >> n) & 0xFF`; the type
 *   compares in `eft028_move_scaled` are `(s32)`; `eft028_move_view_fade` hoists its scale into a local and its player
 *   scan steps `n++, pl++` in that order.  `eft028_capsule_ends` is a global array of a `setVec3`-constructed vector.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/eft001.h"
#include "unsplit/unknown.h"
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "fn_8004CAD8.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "gx.h"
#include "camera/camera.h" /* get_camera_pos (rule 2) */
#include "camera/camera_main.h" /* the camera target accessors (rule 2) */
#include "ef/effect.h" /* effect_retire / effect_move / change_paramscale_eff (rule 2) */
#include "ef/eft_res.h" /* eft_res_models_spawn / eft_res_slot_release (rule 2) */
#include "ef/get_move_work_adrs.h" /* get_move_work_adrs / get_move_work_max (rule 2) */
#include "ef/fn_800CDB2C.h" /* my_player_no (rule 2) */
#include "Pl/Pl_master_ck.h" /* Pl_master_ck (rule 2) */
#include "Pl/fn_8028F66C.h"
#include "Pl/pl_hit_sphere.h" /* PlCapsuleBox (rule 1) */
#include "lobby/lb_server_sel_trans.h" /* event_demo_ck (rule 2: the owner's header) */
#include "sound/fn_800D7F54.h" /* fn_800DD1F0 (rule 2: the owner's header) */

extern "C" {
u32 getItemListSelection();
void eft028_free(_EFT* self);
void eft028_move(_EFT* self);
}

/* `map_se_req(u8, nw4r::math::VEC3*)` - `sound/se_req.cpp`'s (0x800DDCB8), declared locally with the map's mangled
 * spelling (a rule-2 residual). */
void map_se_req(u8 id, nw4r::math::VEC3* pos);

/* The per-family work block `_EFT::work_0x38` points at (`ef/fn_80119C44.c` allocates 0x18 bytes).  size: 0x18 */
struct _EFT029_WORK {
    /* +0x00 */ s32 count;                        /* the number of pooled effects at +0x04 */
    /* +0x04 */ nw4r::ef::Effect* effects[3];     /* the pool `res_eft_create` fills */
    /* +0x10 */ f32 scale;                        /* the parameter scale the area handler applies */
    /* +0x14 */ _GXColor color;                   /* the first effect's colour in area 0 */
};

/* The zone-pulse view of the same block (type 2): one effect, its colour and the capsule the local player is
 * tested against.  size: 0x48 */
struct _EFT029_ZONE_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* effects[1];
    /* +0x08 */ _GXColor color;                   /* the alpha pulses with the zone state */
    /* +0x0C */ PlCapsuleBox zone;                /* the capsule `pl_capsule_box_set` built */
    /* +0x44 */ u8 inside;                        /* the player is inside the zone this frame */
    /* +0x45 */ u8 pad_0x45[3];
};

/* The player fields the view handler reads (a view of `_PLW`, stride 0xB20).  size: 0xB20 */
struct _EFT029_PLAYER {
    /* +0x000 */ u8 pad_0x000[0x3C];
    /* +0x03C */ nw4r::math::VEC3 pos;            /* the position the zone test takes */
    /* +0x048 */ u8 pad_0x048[0x1C];
    /* +0x064 */ f32 ground_y;                    /* the height the view effect is clamped to */
    /* +0x068 */ u8 pad_0x068[0x542];
    /* +0x5AA */ u8 state_flags;                  /* bit 7: the player is under cover */
    /* +0x5AB */ u8 pad_0x5AB[0xB20 - 0x5AB];
};

/* The by-value spellings of the out-pointer vector helpers (the result pointer is the hidden first argument either
 * way): retail builds each result in a compiler temporary, in call order. */
#define EFT_FOCUS_POS ((nw4r::math::VEC3 (*)(void))get_camera_target_pos)
#define VEC3_SUB ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))subVec3)
#define VEC3_ADD ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, const nw4r::math::VEC3*))addVec3)
#define VEC3_SCALE ((nw4r::math::VEC3 (*)(const nw4r::math::VEC3*, f32))vec3_scale)

/* The zone test returns a byte (`stb r3` with no mask). */
#define EFT_ZONE_TEST ((u8 (*)(nw4r::math::VEC3*, PlCapsuleBox*))pl_capsule_point_ck)

extern "C" {
/* The unit's `.data`: the `res_eft_create` effect id and parameter per `type_0x02`, and the eft028 alpha key per
 * variant (`ef/eft026_fx.cpp`'s setter reads it). */
u16 eft028_effect_ids[16] = {
    0x050C, 0x008B, 0x0011, 0x0509, 0x04BA, 0x0383, 0x055A, 0x085A,
    0x072D, 0x09E3, 0x09C0, 0x06E1, 0x00C6, 0x0526, 0x00C1, 0x00C2,
};
u16 eft028_param_ids[16] = {
    0x001A, 0x0011, 0x0004, 0x0047, 0x005F, 0x004E, 0x0049, 0x009C,
    0x0013, 0x006A, 0x003D, 0x0077, 0x0007, 0x0013, 0x0007, 0x0007,
};
u8 eft028_alpha_by_id[16] = {
    0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

/* The unit's `.sdata`: the alpha ramp the view handler fades the second effect along. */
u8 eft028_view_alpha_keys[8] = { 0x00, 0x50, 0x05, 0xFF, 0xFF, 0xFF, 0x00, 0x00 };
}

/* A vector built through `setVec3` (0x80041E8C), so a file-scope one is constructed by `__sinit`.  size: 0xC */
struct EftCapsuleEnd : public nw4r::math::VEC3 {
    /* Builds (x, y, z). */
    EftCapsuleEnd(f32 x, f32 y, f32 z)
    {
        setVec3(this, x, y, z);
    }
};

/* The two ends of the eft028 zone capsule (`ef/eft026_fx.cpp`'s setter copies them); MWCC's `__sinit` builds them. */
extern "C" EftCapsuleEnd eft028_capsule_ends[2] = {
    EftCapsuleEnd(1440.0f, 360.0f, -3660.0f),
    EftCapsuleEnd(1440.0f, 360.0f, -1440.0f),
};

#pragma peephole off

/* 0x80119DEC (0x4B4): Runs the timer down and, the frame it expires, picks the (id, param) pair, creates the pool
 * and places it by type; a failure destroys the record through `eft028_free`. */
extern "C" void eft028_start_by_area(_EFT* self)
{
    nw4r::math::MTX34 mtx;
    _EFT029_WORK* work;
    int id;
    u16 param;
    s32 i;

    MTX34_ctor(&mtx);
    work = (_EFT029_WORK*)self->work_0x38;
    if (--self->timer_0x0C >= 0) {
        return;
    }
    self->state_0x05++;
    id = eft028_effect_ids[self->type_0x02];
    param = eft028_param_ids[self->type_0x02];

    switch (self->type_0x02) {
    case 1:
        switch (get_now_mapno()) {
        case 1:
            switch (self->area_0x44) {
            case 7:
                id = 140;
                param = 53;
                break;
            case 12:
                id = 139;
                param = 17;
                break;
            default:
                eft028_free(self);
                return;
            }
            break;
        case 7:
            switch (self->area_0x44) {
            case 1:
                id = 2472;
                param = 181;
                break;
            case 2:
                id = 2476;
                param = 182;
                break;
            default:
                eft028_free(self);
                return;
            }
            break;
        case 12:
            switch (self->area_0x44) {
            case 7:
                id = 142;
                param = 128;
                break;
            case 12:
                id = 141;
                param = 132;
                break;
            default:
                eft028_free(self);
                return;
            }
            break;
        case 18:
            switch (self->area_0x44) {
            case 1:
                id = 2479;
                param = 183;
                break;
            case 2:
                id = 2483;
                param = 184;
                break;
            default:
                eft028_free(self);
                return;
            }
            break;
        default:
            eft028_free(self);
            return;
        }
        break;
    case 5:
        switch (self->area_0x44) {
        case 1:
            id = 899;
            param = 72;
            break;
        case 2:
            id = 900;
            param = 77;
            break;
        case 3:
            id = 901;
            param = 78;
            break;
        case 4:
            id = 902;
            param = 70;
            break;
        case 5:
            id = 903;
            param = 74;
            break;
        case 7:
            id = 931;
            param = 80;
            break;
        case 9:
            id = 963;
            param = 81;
            break;
        case 10:
            id = 918;
            param = 79;
            break;
        default:
            eft028_free(self);
            return;
        }
        break;
    case 6:
        se_req_splash(0, &self->pos_0x18);
        switch (get_now_mapno()) {
        case 5:
            if ((s32)self->area_0x44 == 9) {
                id = 1364;
                param = 101;
            }
            break;
        case 16:
            if ((s32)self->area_0x44 == 9) {
                id = 2432;
                param = 177;
            } else {
                id = 2440;
                param = 178;
            }
            break;
        }
        break;
    case 9:
        if ((s32)get_now_mapno() == 15) {
            id = 2532;
            param = 163;
        }
        break;
    case 10:
        switch (get_now_mapno()) {
        case 6:
            switch (self->area_0x44) {
            case 1:
                id = 2496;
                param = 61;
                break;
            case 2:
                id = 2497;
                param = 116;
                break;
            default:
                eft028_free(self);
                return;
            }
            break;
        case 17:
            switch (self->area_0x44) {
            case 1:
                id = 2529;
                param = 187;
                break;
            case 2:
                id = 2530;
                param = 188;
                break;
            default:
                eft028_free(self);
                return;
            }
            break;
        }
        break;
    case 11:
        map_se_req(10, &self->pos_0x18);
        break;
    case 12:
        if (getItemListSelection() == 0) {
            id = eft028_effect_ids[self->type_0x02];
            param = eft028_param_ids[self->type_0x02];
        } else {
            id = 1639;
            param = 22;
        }
        break;
    }

    for (i = 0; i < work->count; i++) {
        work->effects[i] = res_eft_create((u16)id, param, 0);
        if (work->effects[i] == NULL) {
            eft028_free(self);
            return;
        }
        id++;
    }

    switch (self->type_0x02) {
    case 1:
    case 6:
    case 9:
    case 11:
    case 14:
    case 15:
        for (i = 0; i < work->count; i++) {
            SetRootMtxTrans(work->effects[i], &self->pos_0x18);
        }
        break;
    case 8:
    case 10:
    case 12:
    case 13:
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        mtx34_set_trans(&mtx, &self->pos_0x18);
        for (i = 0; i < work->count; i++) {
            work->effects[i]->SetRootMtx(mtx);
        }
        break;
    }

    self->flag_0x01 = 1;
    eft028_move(self);
}

/* 0x8011A2A0 (0xAC): Fills the pool with one `res_eft_create(idTable[type], 4, 0)` per slot and runs the placement
 * dispatcher; the first failed create destroys the record. */
extern "C" void eft028_start(_EFT* self)
{
    s32 i;
    _EFT029_WORK* work = (_EFT029_WORK*)self->work_0x38;

    self->state_0x05++;
    for (i = 0; i < work->count; i++) {
        work->effects[i] = res_eft_create(eft028_effect_ids[self->type_0x02], 4, 0);
        if (work->effects[i] == NULL) {
            eft028_free(self);
            return;
        }
    }
    self->flag_0x01 = 1;
    eft028_move(self);
}

extern "C" void eft028_move_view_fade(_EFT* self);
extern "C" void eft028_move_scaled(_EFT* self);
extern "C" void eft028_move_zone_pulse(_EFT* self);
extern "C" void eft028_move_view(_EFT* self);

/* 0x8011A34C (0x38): Runs the per-frame handler of the record's type. */
extern "C" void eft028_move(_EFT* self)
{
    switch (self->type_0x02) {
    case 0:
    case 3:
    case 7:
        eft028_move_view_fade(self);
        break;
    case 1:
    case 6:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        eft028_move_scaled(self);
        break;
    case 2:
        eft028_move_zone_pulse(self);
        break;
    case 4:
    case 5:
        eft028_move_view(self);
        break;
    }
}

/* 0x8011A384 (0x3D8): Places the effects on the camera-to-focus ray, clamps the second above the local player and
 * fades it in while the player is under cover; the spare effects are retired and re-spawned. */
extern "C" void eft028_move_view_fade(_EFT* self)
{
    nw4r::math::VEC3 pos;
    nw4r::math::VEC3 spare_a;
    nw4r::math::VEC3 cam;
    nw4r::math::VEC3 focus;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 spare_b;
    s32 keep;
    _EFT029_WORK* work;
    u32 n;
    s32 i;
    f32 dist;
    _EFT029_PLAYER* pl;
    u16 max;
    _GXColor col;

    VEC3_ctor(&pos);
    VEC3_ctor(&spare_a);
    VEC3_ctor(&cam);
    VEC3_ctor(&focus);
    VEC3_ctor(&dir);
    VEC3_ctor(&spare_b);
    work = (_EFT029_WORK*)self->work_0x38;
    if (self->area_0x44 != get_now_areano()) {
        self->state_0x05++;
        return;
    }
    copyVec3(&focus, &EFT_FOCUS_POS());
    copyVec3(&cam, &get_camera_pos());
    if (cam.y > 0.0f || camera_mode1_ck() == 1) {
        keep = work->count;
    } else {
        keep = 0;
        max = (u16)get_move_work_max(2);
        pl = (_EFT029_PLAYER*)get_move_work_adrs(2);
        for (n = 0; n < max; n++, pl++) {
            if (Pl_master_ck((_PLW*)pl) == 1) {
                break;
            }
        }
        copyVec3(&dir, &VEC3_SUB(&focus, &cam));
        if (vec3_length_sq((f32*)&dir) > 0.0001f) {
            vec3_normalize_into(&dir, &dir);
        }
        copyVec3(&pos, &VEC3_ADD(&cam, &VEC3_SCALE(&dir, 150.0f)));
        for (i = 0; i < work->count; i++) {
            if (work->effects[i] != NULL) {
                SetRootMtxTrans(work->effects[i], &pos);
            }
        }
        focus.y = cam.y;
        copyVec3(&dir, &VEC3_SUB(&focus, &cam));
        dist = vec3_len((f32*)&dir);
        pos.x = focus.x;
        pos.z = focus.z;
        pos.y = focus.y + dist;
        if (pos.y >= pl->ground_y) {
            copyVec3(&dir, &VEC3_SUB(&pos, &cam));
            f32 s = (pl->ground_y - focus.y) / dist;
            vec3_scale_in_place(&dir, s);
            copyVec3(&pos, &VEC3_ADD(&cam, &dir));
        } else {
            copyVec3(&pos, &focus);
        }
        pos.y = pl->ground_y - 3.0f;
        copyVec3(&self->pos_0x18, &pos);
        SetRootMtxTrans(work->effects[0], &self->pos_0x18);
        col.r = 0xFF;
        col.g = 0xFF;
        col.b = 0xFF;
        if ((pl->state_flags & 0x80) != 0 && event_demo_ck() == 0) {
            if (++self->field_0x10 > 10) {
                self->field_0x10 = 10;
            }
            col.a = eftGetKeyAlpha(eft028_view_alpha_keys, self->field_0x10);
            change_color_eff(work->effects[1], &self->pos_0x18, col);
        } else {
            if (--self->field_0x10 < 0) {
                self->field_0x10 = 0;
            }
            col.a = eftGetKeyAlpha(eft028_view_alpha_keys, self->field_0x10);
            change_color_eff(work->effects[1], &self->pos_0x18, col);
        }
    }
    if (self->area_0x44 == 0) {
        change_color_eff(work->effects[0], &self->pos_0x18, work->color);
    }
    for (i = 0; i < work->count - keep; i++) {
        effect_retire(work->effects[i], 0);
        eft_res_models_spawn(self, (void**)&work->effects[i], 1, 1, 0);
    }
}

/* 0x8011A75C (0x140): Moves the effects at the record's scale while the area is current, tints types 8 and 13 with
 * the stage's effect colour and spawns the models; leaving the area or a finished effect ends the record. */
extern "C" void eft028_move_scaled(_EFT* self)
{
    _EFT029_WORK* work = (_EFT029_WORK*)self->work_0x38;
    s32 i;

    if (self->area_0x44 != get_now_areano()) {
        self->state_0x05++;
        self->flag_0x01 = 0;
        return;
    }
    for (i = 0; i < work->count; i++) {
        change_paramscale_eff(work->effects[i], work->scale);
        if (effect_move(work->effects[i]) == 0) {
            self->state_0x05++;
            self->flag_0x01 = 0;
            return;
        }
    }
    if ((s32)self->type_0x02 == 8 || (s32)self->type_0x02 == 13) {
        u32 c = get_stg_eft_col(self->area_0x44, 1);
        _GXColor col;
        col.r = c >> 24;
        col.g = (c >> 16) & 0xFF;
        col.b = (c >> 8) & 0xFF;
        col.a = c;
        for (i = 0; i < work->count; i++) {
            change_color_eff(work->effects[i], &self->pos_0x18, col);
        }
    }
    eft_res_models_spawn(self, (void**)work->effects, 1, work->count, 0);
}

/* 0x8011A89C (0x268): Tests the local player against the zone capsule and steps the alpha pulse: fade in, flash,
 * hold while inside, fade out; the effects move and take the colour unless the item list is open. */
extern "C" void eft028_move_zone_pulse(_EFT* self)
{
    _EFT029_ZONE_WORK* work = (_EFT029_ZONE_WORK*)self->work_0x38;
    nw4r::math::VEC3 unused;
    _EFT029_PLAYER* players;
    s32 i;

    VEC3_ctor(&unused);
    players = (_EFT029_PLAYER*)get_move_work_adrs(2);
    if (self->area_0x44 != get_now_areano()) {
        self->state_0x05++;
        self->flag_0x01 = 0;
        return;
    }
    work->inside = EFT_ZONE_TEST(&players[(s8)my_player_no()].pos, &work->zone);
    if (work->inside == 1 && self->field_0x06 == 0) {
        self->field_0x06 = 1;
    }
    switch (self->field_0x06) {
    case 1:
        work->color.a = 0;
        self->timer_0x0C = 20;
        self->field_0x06++;
    case 2:
        work->color.a += 13;
        if (--self->timer_0x0C <= 0) {
            work->color.a = 0xFF;
            self->timer_0x0C = 31;
            self->field_0x06++;
        }
        break;
    case 3:
        work->color.a -= 5;
        if (--self->timer_0x0C <= 0) {
            self->field_0x06++;
        }
        break;
    case 4:
        work->color.a = 100;
        if (work->inside == 0) {
            self->timer_0x0C = 34;
            self->field_0x06++;
        }
        break;
    case 5:
        work->color.a -= 3;
        if (--self->timer_0x0C <= 0) {
            self->field_0x06 = 0;
        }
        break;
    }
    if (self->field_0x06 == 0) {
        work->color.a = 0;
    }
    if (getItemListSelection() == 0) {
        for (i = 0; i < work->count; i++) {
            if (effect_move(work->effects[i]) == 0) {
                self->state_0x05++;
                self->flag_0x01 = 0;
                return;
            }
            change_color_eff(work->effects[i], &self->pos_0x18, work->color);
        }
        eft_res_models_spawn(self, (void**)work->effects, 1, work->count, 0);
    }
}

/* 0x8011AB04 (0x1EC): Places the effect 150 units along the camera-to-focus ray (type 4 only below the camera, type 5
 * only above it, type 5 also facing along the ray); the effects are retired and re-spawned. */
extern "C" void eft028_move_view(_EFT* self)
{
    nw4r::math::VEC3 cam;
    nw4r::math::VEC3 focus;
    nw4r::math::VEC3 dir;
    nw4r::math::MTX34 mtx;
    s32 i;
    _EFT029_WORK* work;

    VEC3_ctor(&cam);
    VEC3_ctor(&focus);
    VEC3_ctor(&dir);
    MTX34_ctor(&mtx);
    work = (_EFT029_WORK*)self->work_0x38;
    if (self->area_0x44 != get_now_areano()) {
        self->state_0x05++;
        return;
    }
    copyVec3(&focus, &EFT_FOCUS_POS());
    copyVec3(&cam, &get_camera_pos());
    switch (self->type_0x02) {
    case 4:
        if (cam.y > 0.0f) {
            return;
        }
        break;
    case 5:
        if (cam.y < 0.0f) {
            return;
        }
        break;
    }
    copyVec3(&dir, &VEC3_SUB(&focus, &cam));
    if (vec3_length_sq((f32*)&dir) > 0.0001f) {
        vec3_normalize_into(&dir, &dir);
    }
    copyVec3(&self->pos_0x18, &VEC3_ADD(&cam, &VEC3_SCALE(&dir, 150.0f)));
    switch (self->type_0x02) {
    case 4:
        SetRootMtxTrans(work->effects[0], &self->pos_0x18);
        break;
    case 5:
        calcVecAngXY(&dir, (u32*)&self->rot_0x24.x, (u32*)&self->rot_0x24.y);
        cpSetRotMatrix(&self->rot_0x24, &mtx);
        mtx34_set_trans(&mtx, &self->pos_0x18);
        work->effects[0]->SetRootMtx(mtx);
        break;
    }
    for (i = 0; i < work->count; i++) {
        effect_retire(work->effects[i], 0);
        eft_res_models_spawn(self, (void**)&work->effects[i], 1, 1, 0);
    }
}

/* 0x8011ACF0 (0x10): Advances the record's state. */
extern "C" void eft028_state_next(_EFT* self)
{
    self->state_0x05++;
}

/* 0x8011AD00 (0x4): Releases the record. */
extern "C" void eft028_free(_EFT* self)
{
    eft_res_slot_release(self);
}
