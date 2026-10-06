/* ef/fn_803066F0.c - the state-0 handler of the effect-41 machine: it builds the stage's effect-resource name
 *   (`fn_802B050C(0)` plus ".brres"), creates each model through `res_eft_UV_model_create_name`, and on success arms
 *   the step timers, resets the models and runs the state-1 handler `fn_8030681C` directly.
 * RANGE. .text 0x803066F0-0x8030681C (1 function); extab 0x80015AAC-0x80015AB4, extabindex 0x80034668-0x80034674,
 *   .sdata 0x80792B70-0x80792B78 (the ".brres" string).  The dispatcher `fn_803066B4` and the creator
 *   `fn_80306524` (`eft_res_slot_get(32)`, `+0x03 = 41`) are `lobby/fn_8030121C.cpp`'s.
 * NAMES. The map has only the `fn_` stem (the runtime dump has a placeholder for 0x803066F0).  The file is C; the
 *   two mangled callees are called by their map spelling.
 * RESIDUALS. none in code: the row matches.
 *   flipcheck: our `.sdata` is 0x7 of the claimed 0x8 (the claim carries the alignment pad), and the 1.0f literal is
 *   an unclaimed 4-byte `.sdata2` (retail files it at 0x8079ADE0 with `ef/fn_8030681C.cpp`): a fold candidate with
 *   `ef/fn_8030681C.cpp` (one TU, one pool).
 * SHAPES. `created` is `void* volatile`: retail stores the call result and re-reads it (`stw r3,8(r28);
 *   lwz r0,8(r28)`), which no flag set reproduces.
 *   The declarations run `data; i; name;` (with `name` first the `name`/`i` registers swap; playbook 18).
 *   `EftWork` is this unit's view of the 0x48-byte `_EFT` record, and declares `fn_8030681C`/`fn_80306A94` through it.
 */

#include "types.h"

/* The touched tail of the model object `res_eft_UV_model_create_name` returns.  `scale` is set to 1,1,1
 * by this unit; the three accumulators are the ones `fn_8030627C` adds a masked `ran_suu()` delta to every
 * frame; `ready` is set to 1 by the model creator (`res_eft_UV_model_create_name`, `stb r0,53(r25)`) and
 * again here. */
typedef struct UvModelTail {
    u8   pad_0x00[0x1C];  /* +0x00 */
    f32  scale[3];        /* +0x1C */
    u32  rand_accum_a;    /* +0x28 */
    u32  rand_accum_b;    /* +0x2C */
    u32  rand_accum_c;    /* +0x30 */
    u8   pad_0x34;        /* +0x34 */
    u8   ready;           /* +0x35 */
} UvModelTail;                 /* size: 0x36 (the model object continues past +0x36) */

/* One model record of the effect's work area: the resource handle and the created model. */
typedef struct EftModel {
    UvModelTail* model;          /* +0x00 */
    void* volatile created; /* +0x04 */
} EftModel;                 /* size: 0x08 */

/* The per-effect work area at `self->+0x38` (`fn_800F8B44`'s return): the model count, the model records
 * and their four `_g3d_work` slots, plus the area resource. */
typedef struct EftWorkData {
    u32      count;       /* +0x00 */
    EftModel models[1];   /* +0x04 */
    void*    works[1][4]; /* +0x0C */
    void*    resource;    /* +0x1C */
} EftWorkData;            /* size: 0x20 */

typedef struct EftWork EftWork;

/* The 0x48-byte effect record `eft_res_slot_get` allocates and the engine drives.  Only the fields this unit
 * and the neighbours that share the type touch are named. */
struct EftWork {
    u8   in_use;               /* +0x00 */
    u8   active;               /* +0x01 */
    u8   subtype;              /* +0x02 */
    u8   eft_id;               /* +0x03 */
    u8   pad_0x04;             /* +0x04 */
    u8   state;                /* +0x05 */
    u8   substate;             /* +0x06 */
    u8   pad_0x07;             /* +0x07 */
    u8   pad_0x08[0x04];       /* +0x08 */
    s32  timer_a;              /* +0x0C */
    s32  timer_b;              /* +0x10 */
    u8   pad_0x14[0x20];       /* +0x14 */
    void (*update)(EftWork*);  /* +0x34 */
    EftWorkData* data;         /* +0x38 */
    void* resource;            /* +0x3C */
    void (*destroy)(EftWork*); /* +0x40 */
    u8   areano;               /* +0x44 */
    u8   pad_0x45[0x03];       /* +0x45 */
};                             /* size: 0x48 */

extern void* fn_802B050C(u8 index); /* a `stage_w` name buffer for the stage's effect resource */
extern void  fn_8030681C(EftWork* self);
extern void  fn_80306A94(EftWork* self);
extern void* res_eft_UV_model_create_name__FP6MHcharPcUllPP9_g3d_worklUc(UvModelTail* model, char* name,
                                                                        u32 a, u32 b, void** works,
                                                                        s32 n, u8 c);
extern void  setVector3__FPQ34nw4r4math4VEC3fff(f32* v, f32 x, f32 y, f32 z);
extern char* strcat(char* dst, const char* src);

/* Creates the effect's models from the stage's ".brres" resource name, resets the model transforms and
 * the step timers, then advances the effect to its next state and runs that state's handler. */
void fn_803066F0(EftWork* self)
{
    EftWorkData* data;
    u32 i;
    char* name;

    data = self->data;
    name = fn_802B050C(0);
    strcat(name, ".brres");

    for (i = 0; i < data->count; i++) {
        data->models[i].created = res_eft_UV_model_create_name__FP6MHcharPcUllPP9_g3d_worklUc(
            data->models[i].model, name, 0x108, 0, &data->works[i][0], 3, 2);
        if (data->models[i].created == NULL) {
            fn_80306A94(self);
            return;
        }
    }

    self->state++;
    self->active = 1;
    self->timer_a = 0;
    self->timer_b = 0;

    for (i = 0; i < data->count; i++) {
        setVector3__FPQ34nw4r4math4VEC3fff(data->models[i].model->scale, 1.0f, 1.0f, 1.0f);
        data->models[i].model->rand_accum_a = 0;
        data->models[i].model->rand_accum_b = 0;
        data->models[i].model->rand_accum_a = 0;
        data->models[i].model->ready = 1;
    }

    fn_8030681C(self);
}
