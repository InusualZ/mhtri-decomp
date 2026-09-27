/* auto/80103D28_fn_80103D28.cpp - the `eft009` enemy-effect cluster, 0x80103D28..0x80104BD0 (10 functions).
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_ name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * The unit is the enemy-side effect family: `fn_80104A68` is the allocator every setter funnels through
 * (it pools a 72-byte effect object, installs `fn_80104B94` as the `state_0x05` dispatcher and
 * `fn_80104B54` as the pool release), `fn_80103D28` / `fn_801041BC` are the two per-frame handlers that
 * place the pooled effects on an enemy joint (`get_joint_wmat_em` / `get_joint_wpos_em`) and recolour
 * them, and `eft009_set_pos` is the one function the map already names. The neighbour unit
 * `auto/800FCED4_fn_800FCED4.cpp` is the `eft002` sibling and shares this shape (allocator + `byte_5`
 * dispatcher + pool release), which is where the `_EFT` layout below was cross-checked.
 *
 * `splits.txt` `.text` 0x80103D28..0x80104BD0, 10 symbols in address order:
 *   fn_80103D28 (0x494)  per-frame handler, shared joint matrix, type switch on `type_0x02`
 *   fn_801041BC (0x6E4)  per-frame handler, joint matrix/pos, colour switch, `effect_move` liveness
 *   fn_801048A0 (0x10)   `state_0x05++`
 *   fn_801048B0 (0x04)   destroys the effect object (`fn_800F886C`)
 *   fn_801048B4 (0x98)   setter: single effect, re-scaled by `get_em_chg_scale`
 *   eft009_set_pos (0x84) setter: position + rotation vector, no scale
 *   fn_801049D0 (0x98)   setter: as fn_801048B4 plus `copyVec3`
 *   fn_80104A68 (0xEC)   the allocator every setter funnels through
 *   fn_80104B54 (0x40)   pool release handler (`release_0x40`)
 *   fn_80104B94 (0x3C)   `state_0x05` dispatcher (`dispatch_0x34`)
 *
 * Language: C++. The object's own definition `eft009_set_pos__FUcPQ34nw4r4math4VEC3P10_CP_VECTORfUl` is
 * mangled, so the file is `.cpp` and every definition whose map name is plain (`fn_80103D28`, ...) is
 * `extern "C"` - MWCC would otherwise mangle it and objdiff, which pairs by name, would report 0 %
 * (playbook 42). The mangled callees are declared with the signatures their map names encode.
 *
 * Two pool-block views share `_EFT::work_0x38` (the `eft002` unit's pattern): the move family
 * (`fn_80103D28` / `fn_801041BC`) drives a shared joint matrix at +0x0C and a placement offset at +0x3C,
 * the create/set family drives a single pooled effect at +0x08 and an `f32` paramscale at +0x14. The two
 * overlap (the matrix covers +0x0C..0x3C), so the field is a union of the two typed pointers.
 *
 * Flag lever: the whole unit needs the peephole pass off (the `#pragma peephole off` / `reset` pair
 * around the bodies). With it on, `eft009_set_pos` measures 92.12, `fn_801048B4` 95.26, `fn_801049D0`
 * 95.00 and `fn_80104A68` 91.61; with it off all four are 100. The pass was fusing the u8 argument
 * truncations and the byte masks retail keeps unfused (playbook 39). `fn_80103D28` 96.06 -> 99.32 and
 * `fn_801041BC` 94.76 -> 99.55.
 *
 * Result: 8 of 10 symbols byte-identical (100 %); `fn_80103D28` 99.32 and `fn_801041BC` 99.55. All ten
 * sizes match, and `.text` (0xEA8), `extab` (0x38) and `extabindex` (0x54) are byte-identical to the
 * target.
 *
 * Residuals (both in the two big handlers, both scheduling rather than source-shaped):
 *   * the loop-invariant `lbl_80796750` (`f32 neg`) load is one slot early - retail emits
 *     `li r26,0; mr r27,r31; lfs f31` where we emit `lfs f31; li r26,0; mr r27,r31`. Declaring `neg`
 *     inside the loop body, in the `for`-init and via a `while` preheader all schedule it to the same
 *     place, so the source form is not the lever.
 *   * our object emits its own three switch tables in `.data` (0x2D0) where retail references
 *     `jumptable_8059E160` / `E268` / `E328` from the shared pool. That is a relocation *name*
 *     difference only (the report metric does not count it), but a `Matching` flip would add the 0x2D0
 *     bytes unless the tables are claimed in the measured data pass (playbook 23/29).
 *
 * Data: the `.data` run 0x8059DBF0..0x8059E430 (the two `u16` id tables and the `u32` joint table) and
 * the `.sdata2` run 0x80796750..0x80796778 (the scale constants) are the shared pool, `extern`-declared
 * here and never defined (playbook 23/29).
 *
 * `MTX34` comes from `include/nw4r/math.h` (landed with the `auto/800FF8D4` batch).
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "ef/fn_80104BD0.h"
#include "sound/fn_800D7F54.h"   /* se_req_pos_ps - owner sound/fn_800D7F54.cpp */
#include "ef/fn_80105314.h"
#include "unsplit/ef.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

/* ---------------------------------------------------------------------------------------------------
 * engine types the mangled callees encode
 * ------------------------------------------------------------------------------------------------- */

/* The rotation triple `cpSetRotMatrix` takes (Pl/pl_act.cpp carries the same type). size: 0x0C */
/* The rotation triple `cpSetRotMatrix` takes.  The SDK type is a float triple, but THIS unit stores
 * the enemy's integer joint angles (`rot_x_0x1BC` / `rot_y_0x1C0`, u32) into it and passes it by
 * pointer; the `f32` spelling emits an int->float conversion the target does not have (99.546 ->
 * 99.184, object 1764 -> 1768 B), so the integer spelling is the one that reproduces this unit.  The
 * canonical `f32` is reported as a disagreement (docs/plan.md 6.5). size: 0x0C */
struct _CP_VECTOR {
    /* +0x00 */ u32 x;
    /* +0x04 */ u32 y;
    /* +0x08 */ u32 z;
};

/* The 4-byte colour `change_color_eff` takes by value comes from `gx.h` - one definition, in the
 * owner's header (rule 1). */

/* The sound-request handle `se_req_pos_ps` takes; opaque here. */
struct _se_w;

namespace nw4r {
namespace ef {

/* The pooled effect object `fn_800F8788` hands out. Only ever reached through a pointer here. */
/* size: 0x04 - lower bound, an approximation (opaque here; only reached through a pointer) */
struct Effect {
    void SetRootMtx(const nw4r::math::MTX34& mtx);
};

}  // namespace ef
}  // namespace nw4r

/* ---------------------------------------------------------------------------------------------------
 * the game work types
 * ------------------------------------------------------------------------------------------------- */

/* The enemy the effect group belongs to. Only the offsets this unit reads are named; the rest of the
 * object belongs to the enemy units. size: 0xB18 - lower bound, an approximation (the object continues
 * past the sound handle this unit reads). */
struct _ENEMY_WORK {
    /* +0x000 */ u8 unused_0x000[0x1BC];
    /* +0x1BC */ u32 rot_x_0x1BC;      /* copied into the effect's rotation vector */
    /* +0x1C0 */ u32 rot_y_0x1C0;      /* copied into the effect's rotation vector, plus a joint delta */
    /* +0x1C4 */ u8 unused_0x1C4[0x1E1 - 0x1C4];
    /* +0x1E1 */ u8 effect_type_0x1E1; /* the per-enemy effect id passed to fn_80104A68 */
    /* +0x1E2 */ u8 unused_0x1E2[0xB14 - 0x1E2];
    /* +0xB14 */ _se_w* se_handle_0xB14; /* passed to se_req_pos_ps */
    /* +0xB18 */ u8 unused_0xB18[0x04];
};

/* ---------------------------------------------------------------------------------------------------
 * the pooled effect object and its two +0x38 pool-block views
 * ------------------------------------------------------------------------------------------------- */

/* Pool block of the move family: `count` pooled effects (1 in this unit - the matrix at +0x0C bounds the
 * array to two entries) sharing one joint transform and one placement offset. size: 0x48 */
struct _EFT_GROUP_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ nw4r::ef::Effect* items_0x04[2];
    /* +0x0C */ nw4r::math::MTX34 mtx_0x0C; /* the shared joint world matrix */
    /* +0x3C */ nw4r::math::VEC3 offset_0x3C;
};

/* Pool block of the create/set family: one pooled effect, the caller's id and the parameter scale that
 * `get_em_chg_scale` re-scales. size: 0x18 - lower bound, an approximation (nothing here reads past
 * +0x14). */
struct _EFT_SCALE_WORK {
    /* +0x00 */ s32 count;
    /* +0x04 */ u32 id_0x04;                  /* caller-supplied id; 0xFF for the set_pos family */
    /* +0x08 */ nw4r::ef::Effect* effect_0x08; /* the pooled effect the release path hands back */
    /* +0x0C */ u8 unused_0x0C[0x14 - 0x0C];
    /* +0x14 */ f32 scale_0x14;                /* paramscale, re-scaled by get_em_chg_scale */
};

/* The +0x38 pool block, viewed per family. */
union _EFTWork {
    /* +0x00 */ _EFT_GROUP_WORK* group;
    /* +0x00 */ _EFT_SCALE_WORK* scale;
};

/* The pooled effect object `fn_800F8788` hands out and `fn_800F886C` destroys. size: 0x48 */
struct _EFT {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 flag_0x01;    /* alive flag; the handlers clear it on death */
    /* +0x02 */ u8 type_0x02;    /* effect type, 0..0x41 */
    /* +0x03 */ u8 field_0x03;   /* seeded to 9 by the allocator */
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 state_0x05;   /* state-machine index, dispatched by fn_80104B94 */
    /* +0x06 */ u8 unused_0x06[0x08 - 0x06];
    /* +0x08 */ u8 demo_flag_0x08; /* set when a demo is playing */
    /* +0x09 */ u8 unused_0x09[0x0C - 0x09];
    /* +0x0C */ s32 timer_0x0C;  /* frames until the next re-place */
    /* +0x10 */ u8 unused_0x10[0x18 - 0x10];
    /* +0x18 */ nw4r::math::VEC3 pos_0x18;
    /* +0x24 */ _CP_VECTOR rot_0x24;
    /* +0x30 */ _ENEMY_WORK* source_0x30; /* the owning enemy */
    /* +0x34 */ void (*dispatch_0x34)(_EFT*);
    /* +0x38 */ _EFTWork work_0x38;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ void (*release_0x40)(_EFT*);
    /* +0x44 */ u8 area_0x44;    /* current-area id, the colour switches gate on it */
    /* +0x45 */ u8 unused_0x45[0x48 - 0x45];
};

/* ---------------------------------------------------------------------------------------------------
 * externs
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_80073F68(nw4r::math::VEC3* out, nw4r::math::VEC3* in);
extern "C" void fn_800F886C(void* self);
extern "C" void fn_800F93D8(void* self, void* list, u32 mode, s32 count, u32 arg);
extern "C" void fn_800F9DF4(_EFT* self, u8 a, u8 b);
extern "C" _EFT* fn_800F8788(u32 pool_id);
extern "C" void fn_800FBB90(nw4r::math::MTX34* mtx, nw4r::math::VEC3* pos);
extern "C" void fn_8010140C(nw4r::math::MTX34* mtx, void* vec);
/* fn_80104BD0 comes from its owner's header (rule 2). */

f32 get_em_scale(_ENEMY_WORK* enemy);
f32 get_em_chg_scale(_ENEMY_WORK* enemy);
s32 em_work_die_ck(_ENEMY_WORK* enemy);
u8 em_parts_damage_level_get(_ENEMY_WORK* enemy, u8 part);
u8 get_now_areano();
u32 event_demo_ck();
u32 get_stg_eft_col(u8 area, u8 kind);
void get_joint_wmat_em(_ENEMY_WORK* enemy, u32 joint, nw4r::math::MTX34* out);
void get_joint_wpos_em(_ENEMY_WORK* enemy, u32 joint, nw4r::math::VEC3* out);
/* `se_req_pos_ps` comes from the owner's header `sound/fn_800D7F54.h` (rule 2); this unit's local
 * `void` copy collided with the owner's `SeSlot*` once the header declared it. */
void cpSetRotMatrix(_CP_VECTOR* rot, nw4r::math::MTX34* mtx);
void mulVecMat(nw4r::math::VEC3* out, nw4r::math::MTX34* mtx);
void rotLocalMatX(u32 angle, nw4r::math::MTX34* mtx);
void change_paramscale_eff(nw4r::ef::Effect* effect, f32 scale);
void change_color_eff(nw4r::ef::Effect* effect, nw4r::math::VEC3* pos, _GXColor color);
s32 effect_move(nw4r::ef::Effect* effect);
void push_eft_effect_heap_num(nw4r::ef::Effect** effects, long count);
nw4r::ef::Effect* res_eft_create(u16 id, u16 res_id, u32 arg);

/* Forward declarations for the unit's own functions, defined below in address order. */
extern "C" void fn_801048B0(_EFT* self);
extern "C" void fn_80104B54(_EFT* self);
extern "C" void fn_80104B94(_EFT* self);
extern "C" void fn_801048B4(_ENEMY_WORK* self, u32 id, u32 type, s32 joint_delta, f32 scale);
extern "C" void fn_801049D0(_ENEMY_WORK* self, u32 id, u32 type, s32 joint_delta, nw4r::math::VEC3* pos,
                             f32 scale);
extern "C" _EFT* fn_80104A68(u32 id, u8 type, f32 scale, u8 area);

/* The unit's shared pool, referenced but not emitted (see the header). */
extern "C" u16 lbl_8059DBF0[]; /* effect id per type */
extern "C" u16 lbl_8059DC74[]; /* resource id per type */
extern "C" u32 lbl_8059DCF8[]; /* joint id per type */
extern "C" f32 lbl_80796750;
extern "C" f32 lbl_80796754;
extern "C" f32 lbl_80796758;
extern "C" f32 lbl_8079675C;
extern "C" f32 lbl_80796760;
extern "C" f32 lbl_80796764;
extern "C" f32 lbl_80796768;
extern "C" f32 lbl_8079676C;
extern "C" f32 lbl_80796770;

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

#pragma peephole off

/* Per-frame body of the group handler: pools the group's effects on the first frame, re-places them on
 * the enemy joint every frame and recolours them by type. */
extern "C" void fn_80103D28(_EFT* self)
{
    nw4r::math::VEC3 vec;
    _EFT_GROUP_WORK* work = self->work_0x38.group;
    _ENEMY_WORK* enemy = self->source_0x30;
    s32 i;

    VEC3_ctor(&vec);
    self->state_0x05++;
    if ((s32)self->type_0x02 == 0x23) {
        for (i = 0; i < work->count; i++) {
            work->items_0x04[i] = res_eft_create(lbl_8059DBF0[self->type_0x02],
                                                 lbl_8059DC74[self->type_0x02], 0);
            if (work->items_0x04[i] == NULL) {
                fn_801048B0(self);
                return;
            }
        }
    } else {
        for (i = 0; i < work->count; i++) {
            work->items_0x04[i] = res_eft_create((u16)(lbl_8059DBF0[self->type_0x02] + i),
                                                 lbl_8059DC74[self->type_0x02], 0);
            if (work->items_0x04[i] == NULL) {
                fn_801048B0(self);
                return;
            }
        }
    }
    if (em_work_die_ck(enemy) != 0) {
        self->flag_0x01 = 0;
        self->state_0x05 = 3;
        return;
    }
    fn_8010140C(&work->mtx_0x0C, &self->pos_0x18);
    switch (self->type_0x02) {
    case 0x0:
    case 0x4:
    case 0x5:
    case 0x7:
    case 0x8:
    case 0xB:
    case 0xC:
    case 0xE:
    case 0xF:
    case 0x10:
    case 0x11:
    case 0x13:
    case 0x14:
    case 0x15:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1A:
    case 0x1D:
    case 0x20:
    case 0x22:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x27:
    case 0x2B:
    case 0x2C:
    case 0x2D:
    case 0x30:
    case 0x32:
    case 0x37:
    case 0x3A:
    case 0x3C:
    case 0x3F:
    case 0x40:
        cpSetRotMatrix(&self->rot_0x24, &work->mtx_0x0C);
        copyVec3(&vec, &work->offset_0x3C);
        mulVecMat(&vec, &work->mtx_0x0C);
        fn_80073F68(&self->pos_0x18, &vec);
        fn_800FBB90(&work->mtx_0x0C, &self->pos_0x18);
        work->items_0x04[0]->SetRootMtx(work->mtx_0x0C);
        change_paramscale_eff(work->items_0x04[0], get_em_scale(enemy));
        break;
    case 0x1:
    case 0x3:
    case 0x6:
    case 0x9:
    case 0xA:
    case 0x16:
    case 0x28:
    case 0x29:
    case 0x2A:
    case 0x2E:
    case 0x2F:
    case 0x31:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
    case 0x3B:
    case 0x3D:
    case 0x3E:
    case 0x41:
        copyVec3(&vec, &work->offset_0x3C);
        mulVecMat(&vec, &work->mtx_0x0C);
        fn_80073F68(&self->pos_0x18, &vec);
        fn_800FBB90(&work->mtx_0x0C, &self->pos_0x18);
        for (i = 0; i < work->count; i++) {
            work->items_0x04[i]->SetRootMtx(work->mtx_0x0C);
            change_paramscale_eff(work->items_0x04[i], get_em_scale(enemy));
        }
        break;
    case 0xD:
    case 0x12:
    case 0x21:
    case 0x23: {
        f32 neg = lbl_80796750;
        for (i = 0; i < work->count; i++) {
            copyVec3(&vec, &work->offset_0x3C);
            if (i == 1) {
                vec.x *= neg;
            }
            mulVecMat(&vec, &work->mtx_0x0C);
            fn_80073F68(&self->pos_0x18, &vec);
            fn_800FBB90(&work->mtx_0x0C, &self->pos_0x18);
            work->items_0x04[i]->SetRootMtx(work->mtx_0x0C);
            change_paramscale_eff(work->items_0x04[i], get_em_scale(enemy));
        }
        break;
    }
    case 0x1B:
    case 0x1C:
        copyVec3(&vec, &work->offset_0x3C);
        mulVecMat(&vec, &work->mtx_0x0C);
        fn_80073F68(&self->pos_0x18, &vec);
        fn_800FBB90(&work->mtx_0x0C, &self->pos_0x18);
        rotLocalMatX(self->rot_0x24.x, &work->mtx_0x0C);
        for (i = 0; i < work->count; i++) {
            work->items_0x04[i]->SetRootMtx(work->mtx_0x0C);
            change_paramscale_eff(work->items_0x04[i], get_em_scale(enemy));
        }
        break;
    case 0x1E:
    case 0x1F:
    case 0x38:
    case 0x39:
        get_joint_wmat_em(enemy, lbl_8059DCF8[self->type_0x02], &work->mtx_0x0C);
        copyVec3(&vec, &work->offset_0x3C);
        fn_8010140C(&work->mtx_0x0C, &self->pos_0x18);
        mulVecMat(&vec, &work->mtx_0x0C);
        fn_80073F68(&self->pos_0x18, &vec);
        cpSetRotMatrix(&self->rot_0x24, &work->mtx_0x0C);
        fn_800FBB90(&work->mtx_0x0C, &self->pos_0x18);
        for (i = 0; i < work->count; i++) {
            work->items_0x04[i]->SetRootMtx(work->mtx_0x0C);
            change_paramscale_eff(work->items_0x04[i], get_em_scale(enemy));
        }
        break;
    default:
        cpSetRotMatrix(&self->rot_0x24, &work->mtx_0x0C);
        change_paramscale_eff(work->items_0x04[0], get_em_scale(enemy));
        break;
    }
    switch ((s32)self->type_0x02) {
    case 0x9:
        se_req_pos_ps(enemy->se_handle_0xB14, 0x1A, 2, &self->pos_0x18);
        break;
    case 0x29:
    case 0x40:
        se_req_pos_ps(enemy->se_handle_0xB14, 0x79, 2, &self->pos_0x18);
        break;
    case 0x3E:
        se_req_pos_ps(enemy->se_handle_0xB14, 0xA0, 2, &self->pos_0x18);
        break;
    }
    self->flag_0x01 = 1;
}

/* Per-frame body of the group handler's timed variant: counts the frame down, then re-places and
 * recolours the group by type, and retires the effect once every pooled effect has finished. */
extern "C" void fn_801041BC(_EFT* self)
{
    nw4r::math::MTX34 mtx;
    nw4r::math::VEC3 vec;
    _GXColor color;
    s32 moved = 0;
    s32 i;

    MTX34_ctor(&mtx);
    _EFT_GROUP_WORK* work = self->work_0x38.group;
    _ENEMY_WORK* enemy = self->source_0x30;
    VEC3_ctor(&vec);
    self->timer_0x0C--;
    if (self->timer_0x0C >= 0) {
        return;
    }
    if (em_work_die_ck(enemy) != 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    /* The body order below is load-bearing: MWCC emits each shared case body at the position of its
     * first case label, and retail's bodies sit at 0x530, 0x5C4, 0x670, 0x704, 0x720, 0x73C, 0x758,
     * 0x7F8, 0x814, 0x830, 0x84C, 0x868, 0x878, 0x888. */
    switch (self->type_0x02) {
    case 0x1:
    case 0x3:
    case 0x6:
    case 0x8:
    case 0x9:
    case 0x16:
    case 0x21:
    case 0x22:
    case 0x28:
    case 0x2A:
    case 0x31:
    case 0x33:
    case 0x35:
    case 0x36:
    case 0x41:
        get_joint_wmat_em(enemy, lbl_8059DCF8[self->type_0x02], &work->mtx_0x0C);
        fn_8010140C(&work->mtx_0x0C, &self->pos_0x18);
        copyVec3(&vec, &work->offset_0x3C);
        mulVecMat(&vec, &work->mtx_0x0C);
        fn_80073F68(&self->pos_0x18, &vec);
        fn_800FBB90(&work->mtx_0x0C, &self->pos_0x18);
        for (i = 0; i < work->count; i++) {
            work->items_0x04[i]->SetRootMtx(work->mtx_0x0C);
            change_paramscale_eff(work->items_0x04[i], get_em_scale(enemy));
        }
        break;
    case 0xD:
    case 0x12:
    case 0x23: {
        get_joint_wmat_em(enemy, lbl_8059DCF8[self->type_0x02], &work->mtx_0x0C);
        fn_8010140C(&work->mtx_0x0C, &self->pos_0x18);
        f32 neg = lbl_80796750;
        for (i = 0; i < work->count; i++) {
            copyVec3(&vec, &work->offset_0x3C);
            if (i == 1) {
                vec.x *= neg;
            }
            mulVecMat(&vec, &work->mtx_0x0C);
            fn_80073F68(&self->pos_0x18, &vec);
            fn_800FBB90(&work->mtx_0x0C, &self->pos_0x18);
            work->items_0x04[i]->SetRootMtx(work->mtx_0x0C);
            change_paramscale_eff(work->items_0x04[i], get_em_scale(enemy));
        }
        break;
    }
    case 0x2:
    case 0x7:
    case 0x2C:
    case 0x2D:
        get_joint_wpos_em(enemy, lbl_8059DCF8[self->type_0x02], &self->pos_0x18);
        cpSetRotMatrix(&self->rot_0x24, &work->mtx_0x0C);
        copyVec3(&vec, &work->offset_0x3C);
        mulVecMat(&vec, &work->mtx_0x0C);
        fn_80073F68(&self->pos_0x18, &vec);
        fn_800FBB90(&work->mtx_0x0C, &self->pos_0x18);
        for (i = 0; i < work->count; i++) {
            work->items_0x04[i]->SetRootMtx(work->mtx_0x0C);
            change_paramscale_eff(work->items_0x04[i], get_em_scale(enemy));
        }
        break;
    case 0x18:
        change_paramscale_eff(work->items_0x04[0], lbl_80796754 * get_em_scale(enemy));
        break;
    case 0x19:
        change_paramscale_eff(work->items_0x04[0], lbl_80796758 * get_em_scale(enemy));
        break;
    case 0x1D:
        change_paramscale_eff(work->items_0x04[0], lbl_8079675C * get_em_scale(enemy));
        break;
    case 0x1B:
    case 0x1C:
        get_joint_wmat_em(enemy, lbl_8059DCF8[self->type_0x02], &work->mtx_0x0C);
        fn_8010140C(&work->mtx_0x0C, &self->pos_0x18);
        copyVec3(&vec, &work->offset_0x3C);
        mulVecMat(&vec, &work->mtx_0x0C);
        fn_80073F68(&self->pos_0x18, &vec);
        fn_800FBB90(&work->mtx_0x0C, &self->pos_0x18);
        rotLocalMatX(self->rot_0x24.x, &work->mtx_0x0C);
        for (i = 0; i < work->count; i++) {
            work->items_0x04[i]->SetRootMtx(work->mtx_0x0C);
            change_paramscale_eff(work->items_0x04[i], get_em_scale(enemy));
        }
        break;
    case 0x1E:
        change_paramscale_eff(work->items_0x04[0], lbl_80796760 * get_em_scale(enemy));
        break;
    case 0x1F:
        change_paramscale_eff(work->items_0x04[0], lbl_80796764 * get_em_scale(enemy));
        break;
    case 0x38:
        change_paramscale_eff(work->items_0x04[0], lbl_80796768 * get_em_scale(enemy));
        break;
    case 0x39:
        change_paramscale_eff(work->items_0x04[0], lbl_8079676C * get_em_scale(enemy));
        break;
    case 0x3C:
        change_paramscale_eff(work->items_0x04[0], lbl_80796770);
        break;
    case 0x40:
        change_paramscale_eff(work->items_0x04[0], get_em_scale(enemy));
        break;
    default:
        break;
    }
    for (i = 0; i < work->count; i++) {
        if (effect_move(work->items_0x04[i]) == 0) {
            moved++;
        }
        if (moved == work->count) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
    }
    switch (self->type_0x02) {
    case 0xA:
    case 0xB:
    case 0x10:
    case 0x1A:
        if (em_parts_damage_level_get(enemy, 0) >= 2) {
            color.r = 0x7F;
            color.g = 0x7F;
            color.b = 0x7F;
            color.a = 0x7F;
            change_color_eff(work->items_0x04[0], &self->pos_0x18, color);
        }
        break;
    case 0xC:
        if (em_parts_damage_level_get(enemy, 7) >= 2) {
            color.r = 0x7F;
            color.g = 0x7F;
            color.b = 0x7F;
            color.a = 0x7F;
            change_color_eff(work->items_0x04[0], &self->pos_0x18, color);
        }
        break;
    case 0x18:
        color.r = 0xC8;
        color.g = 0xC8;
        color.b = 0xC8;
        color.a = 0xFF;
        change_color_eff(work->items_0x04[0], &self->pos_0x18, color);
        break;
    case 0x19:
        color.r = 0xA8;
        color.g = 0x73;
        color.b = 0xFC;
        color.a = 0xFF;
        change_color_eff(work->items_0x04[0], &self->pos_0x18, color);
        break;
    case 0x1E:
    case 0x1F:
        color.r = 0xA0;
        color.g = 0xA3;
        color.b = 0x4F;
        color.a = 0xFF;
        change_color_eff(work->items_0x04[0], &self->pos_0x18, color);
        break;
    case 0x1D:
    case 0x24:
    case 0x26:
    case 0x27: {
        u32 packed = get_stg_eft_col(self->area_0x44, 0);
        color.r = packed >> 24;
        color.g = (packed >> 16) & 0xFF;
        color.b = (packed >> 8) & 0xFF;
        color.a = packed & 0xFF;
        change_color_eff(work->items_0x04[0], &self->pos_0x18, color);
        break;
    }
    case 0x20:
    case 0x25: {
        u32 packed = get_stg_eft_col(self->area_0x44, 1);
        color.r = packed >> 24;
        color.g = (packed >> 16) & 0xFF;
        color.b = (packed >> 8) & 0xFF;
        color.a = packed & 0xFF;
        change_color_eff(work->items_0x04[0], &self->pos_0x18, color);
        break;
    }
    case 0x38:
        color.r = 0xFF;
        color.g = 0x80;
        color.b = 0x40;
        color.a = 0xFF;
        change_color_eff(work->items_0x04[0], &self->pos_0x18, color);
        break;
    case 0x39:
        color.r = 0xD7;
        color.g = 0xFF;
        color.b = 0xFF;
        color.a = 0xFF;
        change_color_eff(work->items_0x04[0], &self->pos_0x18, color);
        break;
    }
    fn_800F93D8(self, &work->items_0x04[0], 1, work->count, 0);
}

/* Bumps the effect's state index. */
extern "C" void fn_801048A0(_EFT* self)
{
    self->state_0x05++;
}

/* Destroys an effect object. */
extern "C" void fn_801048B0(_EFT* self)
{
    fn_800F886C(self);
}

/* Spawns one effect of the given type on the enemy, seeded with the enemy's rotation and a joint delta. */
extern "C" void fn_801048B4(_ENEMY_WORK* self, u32 id, u32 type, s32 joint_delta, f32 scale)
{
    _EFT* effect = fn_80104A68(id, type, scale, self->effect_type_0x1E1);
    if (effect != NULL) {
        _EFT_SCALE_WORK* work = effect->work_0x38.scale;
        work->scale_0x14 *= get_em_chg_scale(self);
        effect->rot_0x24.x = self->rot_x_0x1BC;
        effect->rot_0x24.y = self->rot_y_0x1C0 + joint_delta;
        effect->rot_0x24.z = 0;
        effect->source_0x30 = self;
    }
}

/* Spawns one effect of the given type at an explicit position and rotation vector. */
void eft009_set_pos(u8 type, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale, u32 id)
{
    _EFT* effect = fn_80104A68(0xFF, type, scale, id);
    if (effect != NULL) {
        effect->work_0x38.scale->id_0x04 = 0xFF;
        effect->rot_0x24.x = rot->x;
        effect->rot_0x24.y = rot->y;
        effect->rot_0x24.z = 0;
        effect->source_0x30 = NULL;
        copyVec3(&effect->pos_0x18, pos);
    }
}

/* Spawns one effect of the given type on the enemy, seeded with the enemy's rotation and a position. */
extern "C" void fn_801049D0(_ENEMY_WORK* self, u32 id, u32 type, s32 joint_delta, nw4r::math::VEC3* pos,
                             f32 scale)
{
    _EFT* effect = fn_80104A68(id, type, scale, self->effect_type_0x1E1);
    if (effect != NULL) {
        _EFT_SCALE_WORK* work = effect->work_0x38.scale;
        work->scale_0x14 *= get_em_chg_scale(self);
        effect->rot_0x24.x = self->rot_x_0x1BC;
        effect->rot_0x24.y = self->rot_y_0x1C0 + joint_delta;
        effect->rot_0x24.z = 0;
        effect->source_0x30 = self;
        copyVec3(&effect->pos_0x18, pos);
    }
}

/* Allocates and seeds a pooled effect object, then installs its dispatch and release handlers. */
extern "C" _EFT* fn_80104A68(u32 id, u8 type, f32 scale, u8 area)
{
    if ((s32)type == 1) {
        return NULL;
    }
    if (area != get_now_areano()) {
        return NULL;
    }
    _EFT* effect = fn_800F8788(0x18);
    if (effect == NULL) {
        return NULL;
    }
    _EFT_SCALE_WORK* work = effect->work_0x38.scale;
    work->count = 1;
    work->id_0x04 = id;
    work->scale_0x14 = scale;
    effect->field_0x03 = 9;
    effect->type_0x02 = type;
    effect->area_0x44 = area;
    fn_800F9DF4(effect, 0, 0);
    effect->release_0x40 = fn_80104B54;
    effect->dispatch_0x34 = fn_80104B94;
    if (event_demo_ck() == 1) {
        effect->demo_flag_0x08 = 1;
    }
    return effect;
}

/* Hands the effect's pooled effects back and clears the pool block. */
extern "C" void fn_80104B54(_EFT* self)
{
    _EFT_SCALE_WORK* work = self->work_0x38.scale;
    push_eft_effect_heap_num(&work->effect_0x08, work->count);
    work->id_0x04 = 0;
    work->count = 0;
}

/* Runs the effect's state handler. */
extern "C" void fn_80104B94(_EFT* self)
{
    switch (self->state_0x05) {
    case 0:
        fn_80104BD0(self);
        break;
    case 1:
        fn_80105314(self);
        break;
    case 2:
        fn_80105550(self);
        break;
    case 3:
        fn_80105560(self);
        break;
    }
}

#pragma peephole reset
