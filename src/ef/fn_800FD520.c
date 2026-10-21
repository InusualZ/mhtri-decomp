/* ef/fn_800FD520.c - the state-0 handler of the eft002 effect machine: it creates the nw4r effect for the actor's
 *   weapon class and places it at the weapon joint (the joint's world matrix through `_PLW::physics_0x13C + 4`, a
 *   per-class muzzle offset, then `SetRootMtxTrans`), the shape of `Pl/pl_act.cpp`'s `fn_8027C064`.
 * RANGE. .text 0x800FD520-0x800FD718 (1 function); extab 0x8000BDFC-0x8000BE04, extabindex 0x80025BE4-0x80025BF0,
 *   .sdata 0x807916E0-0x807916E8 (the per-class effect and parameter ids), .sdata2 0x80796658-0x80796688 (the 12
 *   per-class offsets).
 * FLAGS. `cflags_main`; `#pragma peephole off` around the function: with the pass on, MWCC forwards
 *   `work->effect = res_eft_create(...)` into the test and drops retail's `lwz r0,4(work)` reload.
 * NAMES. The map has only the `fn_` stem.  GUESS: `eft002_effect_ids`/`eft002_param_ids` (map `lbl_807916E0`/
 *   `lbl_807916E4`, the `res_eft_create` arguments indexed by `type_0x02`).
 *   The file is C: its own symbol is plain, the mangled callees are declared by their map spelling.
 * RESIDUALS. none: the row and the claimed sections match (`.sdata` the two id tables, `.sdata2` the body's literals).
 * SHAPES. The pool-block store is its own statement (`if ((work->effect = ...) == 0)` does not reload even with the
 *   pragma).  `source` is declared before `work` (the other order swaps their callee-saved registers).
 *   `VEC3_ctor` and `MTX34_ctor` are one-instruction `blr` stubs in the DOL, so their calls stay written out.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "ef/fn_800FD718.h"
#include "unsplit/sound.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers */
#include "Pl/plw.h"

/* ---------------------------------------------------------------------------------------------------
 * the nw4r math types the mangled callees take
 * ------------------------------------------------------------------------------------------------- */

/* `VEC3` / `MTX34` come from `nw4r/math.h` - one definition, in the owner's header (rule 1). */

/* ---------------------------------------------------------------------------------------------------
 * the 0x48-byte effect record and its pool block
 * ------------------------------------------------------------------------------------------------- */

/* `_EFT` (the effect object `eft_res_slot_get(16)` hands out and `fn_800FD4E4` dispatches) and `_EFT_WORK` (the pool block it attaches) come from `ef.h` (rule 1: one definition). */

/* `_PLW`, the player work record, comes from `Pl/plw.h` - one definition, in the owner's header (rule 1). */

/* What `_PLW::physics_0x13C` points at: a 4-byte word, then the actor's `MHchar` block. size: 0x144 - lower bound, an
 * approximation (only the block's address is taken). */
struct _EfPlBody {
    /* +0x000 */ u32 unused_0x000;
    /* +0x004 */ u8 chr_0x04[0x140];
};

/* ---------------------------------------------------------------------------------------------------
 * externs - the callees and the shared pool
 * ------------------------------------------------------------------------------------------------- */

      /* a `blr` stub in the DOL: a no-op, but the call is in the bytes */
     /* likewise */
extern u32 eft_res_spawn_gate_ck(struct _EFT* self, u32 mode);
/* fn_800FD718 / fn_800FD860 come from their owner's header (rule 2). */

extern void* res_eft_create__FUsUsUl(u16 id, u16 param, u32 idx);
extern void mulVecMat__FPQ34nw4r4math4VEC3PQ34nw4r4math5MTX34(VEC3* out, MTX34* mtx);
extern void SetRootMtxTrans__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3(void* effect, VEC3* pos);

/* The unit's `.sdata`: the effect and parameter id per effect type (`self->type_0x02`); the twelve per-class
 * muzzle offsets are the body's float literals (`.sdata2`, first-use order). */
u16 eft002_effect_ids[2] = { 0x0297, 0x0398 };
u16 eft002_param_ids[2] = { 0x003E, 0x0014 };

/* ---------------------------------------------------------------------------------------------------
 * body
 * ------------------------------------------------------------------------------------------------- */

#pragma peephole off /* the pool-block store keeps the target's reload - see the unit header */

/* Creates the actor's effect and places it at the weapon joint the pool block names. */
void fn_800FD520(struct _EFT* self)
{
    struct _PLW* source;
    VEC3 v;
    MTX34 mtx;
    struct _EFT_WORK* work;

    work = self->work_0x38;
    VEC3_ctor(&v);
    MTX34_ctor(&mtx);

    self->state_0x05++;
    work->effect = res_eft_create__FUsUsUl(eft002_effect_ids[self->type_0x02],
                                           eft002_param_ids[self->type_0x02], 0);
    if (work->effect == 0) {
        fn_800FD860(self);
        return;
    }
    if (self->type_0x02 == 0) {
        source = self->source_0x30;
        if (eft_res_spawn_gate_ck(self, 0) == 0) {
            self->flag_0x01 = 0;
            self->state_0x05 = 3;
            return;
        }
        mhchar_joint_mtx_get(((struct _EfPlBody*)source->physics_0x13C)->chr_0x04, work->param_id, &mtx);
        switch (source->field_0x002) {
        default:
            break;
        case 0:
        case 7:
            v.x = 10.0f;
            v.y = -2.0f;
            v.z = 190.0f;
            break;
        case 1:
            v.x = -4.9f;
            v.y = -0.7f;
            v.z = 90.0f;
            break;
        case 3:
            v.x = -7.8f;
            v.y = -10.0f;
            v.z = 195.0f;
            break;
        case 2:
            v.x = -5.0f;
            v.y = -20.0f;
            v.z = 155.0f;
            break;
        case 8:
            v.x = 10.0f;
            v.y = -2.0f;
            v.z = 190.0f;
            break;
        }
        mulVecMat__FPQ34nw4r4math4VEC3PQ34nw4r4math5MTX34(&v, &mtx);
        self->pos_0x18.x = mtx.m[0][3] + v.x;
        self->pos_0x18.y = mtx.m[1][3] + v.y;
        self->pos_0x18.z = mtx.m[2][3] + v.z;
        SetRootMtxTrans__FPQ34nw4r2ef6EffectPQ34nw4r4math4VEC3(work->effect, &self->pos_0x18);
    }
    self->flag_0x01 = 1;
    fn_800FD718(self);
}

#pragma peephole reset
