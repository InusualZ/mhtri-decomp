/* ef/fn_8011722C.c - the kind-1 state-0 entry of the effect job machine (`ef/eft022_fx.cpp`'s `fn_80116FDC`
 *   dispatches on `state_0x05` and `kind_0x02`): it takes one effect character from the `eft_control` pool, creates
 *   its model, drops it at one of three random offsets (`lbl_805A0460`) and randomises its rotation; the kind-1
 *   state-1 handler `fn_801173AC` then integrates the spin.
 * RANGE. .text 0x8011722C-0x801173AC (1 function); extab 0x8000C494-0x8000C49C, extabindex 0x800265C8-0x800265D4,
 *   .data 0x805A0460-0x805A0488 (three `Vec` offsets and a 0.0f word), .sdata2 0x80796A80-0x80796A88 (0.4f, 0.0f).
 * FLAGS. `cflags_main`; `#pragma peephole off` around the function: with the pass on, MWCC forwards
 *   `work->chara[i] = eft_res_model_get()` into the loop test and drops retail's `lwz r0,4(r30)` reload.
 * NAMES. The map has only the `fn_` stem.  The file is C; the mangled nw4r callees are declared by their map spelling.
 * RESIDUALS. none in code: the row matches.
 *   flipcheck: `.data`/`.sdata2` claimed, not emitted.
 * SHAPES. The model id is the ternary `get_now_mapno() == 12 ? 24 : 23`: `(get_now_mapno() == 12) + 23` narrows to
 *   the `u16` parameter before `work->chara[0]` is loaded.
 *   `EftJob`/`EftCharaWork` are this unit's view of `job->work_0x38`; the kind-0 functions view the same block as 32
 *   `MHchar*` slots and two `s16` counters.
 */

#include "ef/fn_80116FCC.h" /* fn_80116FCC (rule 2: the owner's header) */
#include "ef/fn_80117074.h" /* fn_80117074 (rule 2: the owner's header) */
#include "types.h"
#include "ef.h"
#include "pl.h"

/* ---------------------------------------------------------------------------------------------------
 * the job record and its kind-1 work block
 * ------------------------------------------------------------------------------------------------- */

/* `MHchar`, the effect character `eft_res_model_get` hands out (one 0x168-byte slot of the shared `eft_control` pool, the pointer is slot+4), comes from `pl.h` (rule 1: one definition). */

/* The kind-1 view of the 0x84-byte work block `job->work_0x38` points at.  The kind-0 siblings
 * (fn_80116EEC/fn_80117120/fn_80117088) see the same block as 32 `MHchar*` slots at +0x00 followed
 * by two `s16` counters at +0x80, so this view is only valid while `job->kind_0x02 == 1`.
 * size: 0x20 - a lower bound; the whole allocation is 0x84, from fn_80117088's zero-fill. */
typedef struct EftCharaWork {
    /* +0x00 */ s32 count;          /* number of live character handles; always 1 here */
    /* +0x04 */ MHchar* chara[1];   /* the handles themselves */
    /* +0x08 */ Vec pos;            /* per-frame position delta, integrated by fn_801173AC */
    /* +0x14 */ u16 rot_x;          /* rotation angles, advanced by the spin deltas */
    /* +0x16 */ u16 rot_y;
    /* +0x18 */ u16 rot_z;
    /* +0x1A */ s16 spin_x;         /* per-frame rotation deltas, fn_801173AC integrates them */
    /* +0x1C */ s16 spin_y;
    /* +0x1E */ s16 spin_z;
} EftCharaWork;

/* The effect job record `fn_80116FDC` updates.  size: 0x44 - a lower bound, the callback pointer;
 * only the offsets the job machine touches are named. */
typedef struct EftJob {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 active_0x01;     /* 1 while the job is running */
    /* +0x02 */ u8 kind_0x02;       /* selects the work layout (0/1) */
    /* +0x03 */ u8 unused_0x03[0x05 - 0x03];
    /* +0x05 */ u8 state_0x05;      /* the fn_80116FDC dispatch state */
    /* +0x06 */ u8 unused_0x06[0x0C - 0x06];
    /* +0x0C */ s32 timer_0x0C;     /* frames since the last spawn, counted by fn_80117120 */
    /* +0x10 */ u8 unused_0x10[0x38 - 0x10];
    /* +0x38 */ EftCharaWork* work_0x38;
    /* +0x3C */ u8 unused_0x3C[0x40 - 0x3C];
    /* +0x40 */ void (*callback_0x40)(struct EftJob* self);
} EftJob;
#define fn_80116FCC_c1 ((void (*)(EftJob*))fn_80116FCC)
#define fn_80117074_c1 ((void (*)(EftJob*))fn_80117074)

/* The callees and the shared pool. */

extern MHchar* eft_res_model_get(void);                 /* takes a free eft_control slot, returns its chara */
/* fn_80116FCC / fn_80117074 come from their owner's leaf headers (`_EFT*` parameter); the consumer stores them in a typed
 * callback field (`void (*)(EftJob*)`), so it reaches them through the `_c1` cast macros above. */
extern BOOL res_eft_model_create__FP6MHcharUsUl(MHchar* chara, u16 model, u32 light);
extern u16 ran_suu__Fl(long index);               /* pseudo-random u16 out of system_w[index] */
extern u8 get_now_mapno__Fv(void);
extern void setVector3__FPQ34nw4r4math4VEC3fff(Vec* out, f32 x, f32 y, f32 z);
extern void vec_to_mh_vec3__FPQ34nw4r4math4VEC3P3Vec(Vec* out, Vec* in);

/* The shared pool.  `lbl_805A0460` is an unsized `Vec` array in `.data`, so MWCC keeps the target's
 * `lis`/`addi` addressing (a `.sdata2` scalar would fold into `@sda21`, and an unsized array is too
 * big for the small-data heuristic anyway).  The two floats stay scalars so their `@sda21` loads are
 * unchanged. */
extern const Vec lbl_805A0460[]; /* the three spawn offsets, indexed by the random pick */
extern const f32 lbl_80796A80;  /* 0.4f */
extern const f32 lbl_80796A84;  /* 0.0f */

/* ---------------------------------------------------------------------------------------------------
 * body
 * ------------------------------------------------------------------------------------------------- */

#pragma peephole off /* the loop test keeps the target's reload of work->chara[i] */

/* Takes one effect character out of the pool, creates its model and initialises its placement. */
void fn_8011722C(EftJob* self)
{
    EftCharaWork* work;
    s16 i;

    work = self->work_0x38;
    self->callback_0x40 = fn_80116FCC_c1;
    work->count = 1;
    for (i = 0; i < work->count; i++) {
        work->chara[i] = eft_res_model_get();
        if (work->chara[i] == 0) {
            fn_80117074_c1(self);
            return;
        }
    }
    if (!res_eft_model_create__FP6MHcharUsUl(work->chara[0],
                                             get_now_mapno__Fv() == 12 ? 24 : 23, 0)) {
        fn_80117074_c1(self);
        return;
    }
    work->chara[0]->ready = 0;
    vec_to_mh_vec3__FPQ34nw4r4math4VEC3P3Vec((Vec*)&work->chara[0]->pos_0x04,
                                            (Vec*)&lbl_805A0460[ran_suu__Fl(1) % 3]);
    setVector3__FPQ34nw4r4math4VEC3fff((Vec*)&work->chara[0]->scale_0x1C, lbl_80796A80, lbl_80796A80,
                                       lbl_80796A80);
    work->pos.x = lbl_80796A84;
    work->pos.y = lbl_80796A84;
    work->pos.z = lbl_80796A84;
    work->rot_x = ran_suu__Fl(1);
    work->rot_y = ran_suu__Fl(1);
    work->rot_z = ran_suu__Fl(1);
    work->spin_x = 0;
    work->spin_y = 0;
    work->spin_z = 0;
}

#pragma peephole reset
