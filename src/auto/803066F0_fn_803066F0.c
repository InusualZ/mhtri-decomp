/* auto/803066F0_fn_803066F0.c - the initialisation step of one effect's update machine, `.text`
 * 0x803066F0-0x8030681C (one function, `fn_803066F0`).
 *
 * What it is.  `fn_803066B4` (0x803066B4, the last function of the previous unit) is a four-state update
 * dispatcher: it tail-calls `fn_803066F0` (state 0), `fn_8030681C` (state 1), `fn_80306A84` (state 2) and
 * `fn_80306A94` (state 3) on `self->state` (`+0x05`).  `fn_80306524` installs it as the `+0x34` update
 * hook of the effect record it creates with `fn_800F8788(32)` and stamps `+0x03 = 41`; `fn_80306BFC`
 * (reached from the named `eft042_set2`) installs `fn_80306FF0` and stamps `+0x03 = 42`.  This function is
 * therefore the state-0 handler of the effect-41 machine.  It builds the stage's effect-resource name
 * (`fn_802B050C(0)` plus the ".brres" suffix), creates each model through
 * `res_eft_UV_model_create_name` with that name, and on the first failure destroys the effect
 * (`fn_80306A94` = `fn_800F886C`).  On success it advances the state to 1, arms the two step timers, sets
 * each model's scale to 1 and resets its random accumulators, then runs the state-1 handler `fn_8030681C`
 * directly.
 *
 * Types.  The effect record is the 0x48-byte pool slot `fn_800F8788` hands out (its record stride is
 * `addi r31,r31,72` in that function, and it pre-clears `+0x04..+0x08` and `+0x14..+0x17`); the fields
 * this unit and the neighbours that share the type touch are named in `EftWork`.  `self->+0x38` is the
 * per-effect work area `fn_800F8B44` returns: a count, one 8-byte model record at `+0x04`, four
 * `_g3d_work` slots at `+0x0C`, and the area resource at `+0x1C` (`fn_80306524` stores
 * `fn_802B04A0(areano)` there and `memset`s the four slots; `fn_80306BFC` memsets the last two).  `count`
 * is 1 in both creators, which is why the model record and the work slots can sit at those fixed offsets.
 *
 * Names.  The shared runtime dump (`docs/memory-dump.md`) has no name for 0x803066F0 - it carries the
 * `zz_03066f0_` placeholder while it does name its callee's sibling `eft042_set2` - so the map's
 * `fn_803066F0` stays and the C symbol matches it (a rename needs the map and the source in one edit).
 * `fn_802B050C` indexes `stage_w` for a name buffer, `fn_8030681C`/`fn_80306A94`/`fn_803066B4` are
 * unowned neighbours, and `lbl_80792B70` (".brres") / `lbl_8079ADE0` (1.0f) are the unit's two data runs,
 * referenced externally here because `splits.txt` does not claim them yet.  The two mangled callees are
 * called by their map symbol name so the reloc pairs (`Pl/pl_act.cpp` does the same for `hit_flag_set`).
 *
 * Result: the object is **byte-identical to the target's `.text`, `extab` and `extabindex`** (300 / 8 / 12
 * bytes) and the function measures 100.00 %.  Two load-bearing source shapes:
 *   - `created` is `void* volatile`.  Retail stores the call result and then re-reads the field
 *     (`stw r3,8(r28); lwz r0,8(r28); cmpwi r0,0`); without the qualifier MWCC forwards the result into
 *     the compare, the object is 4 bytes short and the function measures 99.6 %.  None of the flag
 *     variants tried (`-O2`/`-O3`/`-O4`/`-O4,p`, `-inline auto|noauto|off`, `-schedule off`, `-opt all`,
 *     and both `GC/3.0a3` and every `Wii/1.x`) reproduces the re-read.
 *   - the declaration order `data; i; name;` decides the `name`/`i` register pair (r29/r26 in the target,
 *     r26/r29 with `name` first): 100.00 % with `i` before `name`, 99.6 % otherwise (playbook 18).
 * The unit's `.comment` is `"CodeWarrior" 0x0f` (Wii/1.3) while the target's is `0x0e` - a different
 * compiler build, not codegen: `.comment` is not allocated, so it does not reach the link.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/803066F0_fn_803066F0.c`.
 * The file name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file.  Rename it the moment there is evidence.
 */

#pragma exceptions on

#include "types.h"

/* The touched tail of the model object `res_eft_UV_model_create_name` returns.  `scale` is set to 1,1,1
 * by this unit; the three accumulators are the ones `fn_8030627C` adds a masked `ran_suu()` delta to every
 * frame; `ready` is set to 1 by the model creator (`res_eft_UV_model_create_name`, `stb r0,53(r25)`) and
 * again here. */
typedef struct MHchar {
    u8   pad_0x00[0x1C];  /* +0x00 */
    f32  scale[3];        /* +0x1C */
    u32  rand_accum_a;    /* +0x28 */
    u32  rand_accum_b;    /* +0x2C */
    u32  rand_accum_c;    /* +0x30 */
    u8   pad_0x34;        /* +0x34 */
    u8   ready;           /* +0x35 */
} MHchar;                 /* size: 0x36 (the model object continues past +0x36) */

/* One model record of the effect's work area: the resource handle and the created model. */
typedef struct EftModel {
    MHchar* model;          /* +0x00 */
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

/* The 0x48-byte effect record `fn_800F8788` allocates and the engine drives.  Only the fields this unit
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
extern void* res_eft_UV_model_create_name__FP6MHcharPcUllPP9_g3d_worklUc(MHchar* model, char* name,
                                                                        u32 a, u32 b, void** works,
                                                                        s32 n, u8 c);
extern void  setVector3__FPQ34nw4r4math4VEC3fff(f32* v, f32 x, f32 y, f32 z);
extern char* strcat(char* dst, const char* src);

extern char lbl_80792B70[7]; /* ".brres", .sdata 0x80792B70 */
extern f32  lbl_8079ADE0;    /* 1.0f,    .sdata2 0x8079ADE0 */

/* Creates the effect's models from the stage's ".brres" resource name, resets the model transforms and
 * the step timers, then advances the effect to its next state and runs that state's handler. */
void fn_803066F0(EftWork* self)
{
    EftWorkData* data;
    u32 i;
    char* name;

    data = self->data;
    name = fn_802B050C(0);
    strcat(name, lbl_80792B70);

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
        setVector3__FPQ34nw4r4math4VEC3fff(data->models[i].model->scale, lbl_8079ADE0, lbl_8079ADE0,
                                           lbl_8079ADE0);
        data->models[i].model->rand_accum_a = 0;
        data->models[i].model->rand_accum_b = 0;
        data->models[i].model->rand_accum_a = 0;
        data->models[i].model->ready = 1;
    }

    fn_8030681C(self);
}
