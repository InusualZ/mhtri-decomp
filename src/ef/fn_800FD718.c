/* ef/fn_800FD718.c - the state-1/2/3 handlers of the eft002 effect machine (`ef/eft002.cpp`'s `fn_800FD4E4`
 *   dispatches on `state_0x05` to `fn_800FD520` (0), `fn_800FD718` (1), `fn_800FD850` (2) and `fn_800FD860` (3)).
 * RANGE. .text 0x800FD718-0x800FD864 (3 functions); extab 0x8000BE04-0x8000BE0C, extabindex 0x80025BF0-0x80025BFC,
 *   .sdata2 0x80796688-0x80796690 (0.0f and 20.0f, the effect's offset above the shell).
 * NAMES. The map has only `fn_` stems for this range.  The file is C: the only C++ evidence is six mangled callees,
 *   which do not decide the caller's language (as in `ef/fn_800FD520.c`).
 * RESIDUALS. none in code: all three rows match.
 *   flipcheck: `.sdata2` claimed, not emitted.
 * SHAPES. `type_0x02` is `s8` (retail `cmpwi`; a `u8` emits `cmplwi`).
 *   The area test is an early return (`if (area != get_now_areano()) return;`); as a block MWCC branches to the
 *   `effect_move` block where retail branches to the epilogue.
 *   `work` is declared before `source` (the callee-saved registers colour the other way otherwise), and
 *   `work = self->work_0x38;` stays a statement before the `VEC3` construction.
 *   The state bump is written out in `fn_800FD718` rather than calling `fn_800FD850`.
 *   `VEC3` is a private C copy here (`nw4r/math.h` is C++), as in `ef/fn_80104BD0.c`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */

/* ---------------------------------------------------------------------------------------------------
 * the nw4r math type the mangled callees take
 * ------------------------------------------------------------------------------------------------- */

/* `VEC3` comes from `nw4r/math.h` - one definition, in the owner's header (rule 1). */

/* ---------------------------------------------------------------------------------------------------
 * the 0x48-byte effect record, its pool block, and the two actors it follows
 * ------------------------------------------------------------------------------------------------- */

struct _EFT_WORK;

/* The shell work the effect follows.  Size from `push_shell_work`'s `memset(self, 0, 0x10C)`. */
struct _SHELL_W {
    /* +0x000 */ u8 field_0x00;  /* 0 marks the shell as finished: the effect is dropped */
    /* +0x001 */ u8 unused_0x001[0x03 - 0x01];
    /* +0x003 */ u8 type_0x03;    /* the shell type, 0..29 (fn_800FD2B0's switch operand) */
    /* +0x004 */ u8 unused_0x004[0x08 - 0x04];
    /* +0x008 */ u8 area_0x08;    /* the area the shell is in, copied into _EFT_S8::area_0x44 */
    /* +0x009 */ u8 unused_0x009[0x18 - 0x09];
    /* +0x018 */ VEC3 pos_0x18;   /* world position; the effect is re-seated on it */
    /* +0x024 */ u32 rot_x_0x24;  /* X rotation, rotVecX's operand */
    /* +0x028 */ u8 unused_0x028[0x2C - 0x28];
    /* +0x02C */ u32 rot_z_0x2C;  /* Z rotation, rotVecZ's operand */
    /* +0x030 */ u8 unused_0x030[0x10C - 0x30];
}; /* size: 0x10C */

/* The effect record `eft_res_slot_get` hands out and `fn_800FD4E4` dispatches on. */
struct _EFT_S8 {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 flag_0x01;      /* 1 while the effect is live, 0 once it has been dropped */
    /* +0x02 */ s8 type_0x02;      /* the effect type; 0 is the player family, 1 the shell family; s8 is
                                    * this unit's codegen (u8: 100.0 -> 99.23), reported vs `_EFT_S8`'s u8 */
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 state_0x05;     /* the `fn_800FD4E4` state index the handlers advance */
    /* +0x06 */ u8 unused_0x06[0x0C - 0x06];
    /* +0x0C */ s32 timer_0x0C;    /* the frame budget the handlers count down */
    /* +0x10 */ u8 unused_0x10[0x18 - 0x10];
    /* +0x18 */ VEC3 pos_0x18;     /* the position handed to SetRootMtxTrans */
    /* +0x24 */ u8 unused_0x24[0x30 - 0x24];
    /* +0x30 */ void* source_0x30; /* the actor the effect was spawned for (_PLW or _SHELL_W) */
    /* +0x34 */ void (*dispatch_0x34)(struct _EFT_S8*); /* the state dispatcher (fn_800FD4E4) */
    /* +0x38 */ struct _EFT_WORK* work_0x38;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ void (*release_0x40)(struct _EFT_S8*); /* the pool release handler (fn_800FD4A8) */
    /* +0x44 */ u8 area_0x44;      /* the area the effect is legal in */
    /* +0x45 */ u8 unused_0x45[0x48 - 0x45];
}; /* size: 0x48 */

/* `_EFT_WORK`, the pool block at `_EFT_S8::work_0x38`, comes from `ef.h` (rule 1: one definition). */

/* ---------------------------------------------------------------------------------------------------
 * externs - the plain-named callees and the shared pool
 * ------------------------------------------------------------------------------------------------- */

      /* a `blr` stub in the DOL: the VEC3 ctor */
extern void* addVec3To(VEC3* dst, const VEC3* src);
extern void eft_res_models_spawn(void* self, void* effects, u32 mode, s32 count, u32 arg);
extern void eft_res_slot_release(void* self);
extern void fn_800FD718(struct _EFT_S8* self);
extern void fn_800FD850(struct _EFT_S8* self);
extern void fn_800FD860(struct _EFT_S8* self);

/* The mangled callees, declared with the map's spelling (docs/plan.md, "The language comes from the
 * symbol"): `setVector3(nw4r::math::VEC3*, f32, f32, f32)`, `rotVecX/rotVecZ(..., u32)`,
 * `SetRootMtxTrans(nw4r::ef::Effect*, nw4r::math::VEC3*)`, `effect_move(nw4r::ef::Effect*)`,
 * `get_now_areano()`. */
extern u32 get_now_areano__Fv(void);
extern void setVector3__FPQ34nw4r4math4VEC3fff(VEC3* v, f32 x, f32 y, f32 z);
extern void rotVecX__FPQ34nw4r4math4VEC3Ul(VEC3* v, u32 angle);
extern void rotVecZ__FPQ34nw4r4math4VEC3Ul(VEC3* v, u32 angle);
extern void SetRootMtxTrans__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3(void* effect, VEC3* pos);
extern u32 effect_move__FPQ34nw4r2ef6Effect(void* effect);

/* The shared pool: the effect's offset from the shell.  Referenced but not emitted (see the header). */
extern f32 lbl_80796688; /* 0.0f  */
extern f32 lbl_8079668C; /* 20.0f */

/* ---------------------------------------------------------------------------------------------------
 * bodies
 * ------------------------------------------------------------------------------------------------- */

/* Per-frame body of the shell family's effect: counts the frame budget down, drops the effect once the
 * shell is gone, and re-seats it on the shell's position while the shell is in the effect's area. */
void fn_800FD718(struct _EFT_S8* self)
{
    struct _EFT_WORK* work;
    struct _SHELL_W* source;
    VEC3 v;

    work = self->work_0x38;
    VEC3_ctor(&v);

    if (self->type_0x02 == 1) {
        source = (struct _SHELL_W*)self->source_0x30;
        self->timer_0x0C--;
        if (source->field_0x00 == 0 && self->timer_0x0C < 0) {
            self->flag_0x01 = 0;
            self->state_0x05++;
            return;
        }
        self->area_0x44 = source->area_0x08;
        if (self->area_0x44 != (u8)get_now_areano__Fv()) {
            return;
        }
        copyVec3(&self->pos_0x18, &source->pos_0x18);
        setVector3__FPQ34nw4r4math4VEC3fff(&v, lbl_80796688, lbl_8079668C, lbl_80796688);
        rotVecX__FPQ34nw4r4math4VEC3Ul(&v, source->rot_x_0x24);
        rotVecZ__FPQ34nw4r4math4VEC3Ul(&v, source->rot_z_0x2C);
        addVec3To(&self->pos_0x18, &v);
        SetRootMtxTrans__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3(work->effect, &self->pos_0x18);
    }

    if (effect_move__FPQ34nw4r2ef6Effect(work->effect) == 0) {
        self->flag_0x01 = 0;
        self->state_0x05++;
        return;
    }
    eft_res_models_spawn(self, &work->effect, 1, work->count, 0);
}

/* Advances the effect one state: the state-2 "nothing left to do" step. */
void fn_800FD850(struct _EFT_S8* self)
{
    self->state_0x05++;
}

/* Releases the effect: the state-3 step, the release `eft_res_slot_release` performs. */
void fn_800FD860(struct _EFT_S8* self)
{
    eft_res_slot_release(self);
}
