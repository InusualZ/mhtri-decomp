/* auto/8011722C_fn_8011722C.c - the kind-1 start stage of the effect job machine,
 * `.text` 0x8011722C..0x801173AC (one function, `fn_8011722C`).
 *
 * What it is.  The previous unit (auto/80114E34_fn_80114E34.cpp, 0x80114E34..0x8011722C) holds the
 * job's update hook `fn_80116FDC`, a four-state switch on `job->state_0x05` that dispatches into
 * `fn_80117018`/`fn_80117050`/`fn_80117074`/`fn_80117084`; inside state 0 and state 1 a second byte,
 * `job->kind_0x02`, selects a variant (`fn_80117088`/`fn_8011722C` for kind 0/1 in state 0,
 * `fn_80117120`/`fn_801173AC` for kind 0/1 in state 1).  This function is the kind-1 state-0 entry: it
 * takes one effect character out of the shared `eft_control` pool (`fn_800F8914`), creates the model
 * for it, drops it at one of three fixed offsets picked at random (`lbl_805A0460`), zeroes its scale
 * and its position, randomises the three rotation angles and clears the three spin deltas.  The
 * kind-1 state-1 handler `fn_801173AC` is the one that then integrates those spin deltas into the
 * angles and pushes the result to the character, so the field names below are its names.
 *
 * Result: 100 %; `.text` (0x180), `extab` (0x8) and `extabindex` (0xC) byte-identical to the target.
 *
 * Data.  The unit owns no pool section (the target object carries none): the three spawn offsets in
 * `.data` and the two floats in `.sdata2` are declared `extern` by their map names and never defined
 * (playbook 29).  `lbl_80796A80` is 0.4f (the scale `setVector3` is given), `lbl_80796A84` is 0.0f
 * (position and spin), and `lbl_805A0460` is three `Vec`s of world offsets (36 B, followed by a
 * stray 0.0f word the range 0x805A0460..0x805A0488 covers).
 *
 * Load-bearing source shapes (each measured; the wrong form costs real points):
 *   * `#pragma peephole off` is required for the pool-handle loop.  With the pass on MWCC forwards
 *     `work->chara[i] = fn_800F8914()` into the test, dropping the target's `lwz r0, 4(r30)` - 4 B
 *     short, 96.41 %; the pragma keeps the reload (100 %).
 *   * the model id has to be the **ternary** `get_now_mapno() == 12 ? 24 : 23`, not the equivalent
 *     `(get_now_mapno() == 12) + 23`: both emit the same `cntlzw`/`srwi`/`addi`, but the addition
 *     narrows the value to the `u16` parameter *before* the first argument is loaded, where retail
 *     loads `work->chara[0]` first (97.92 % vs 100 %).
 *
 * Types.  `EftJob` and `EftCharaWork` are reconstructed minimally (only the offsets this function
 * reads) and are this unit's own copies: the sibling kind-0 functions (`fn_80116EEC`, `fn_80117120`,
 * `fn_80117088`) view the very same `job->work_0x38` block as 32 `MHchar*` slots plus two `s16`
 * counters, so the block is really one allocation with two views.  `MHchar` is `fn_800F8914`'s
 * 0x168-byte `eft_control` slot (the pointer it returns is slot+4), and only the three fields this
 * function touches are named.  The 3-float vector is `Vec` from the shared `include/ef.h`, not a
 * fourth local `Vec3`.
 *
 * Language.  The unit's own symbol is plain (`fn_8011722C`, `-lang=c`), so the file is C and the
 * mangled nw4r callees are declared by their map spelling, as auto/800FD520_fn_800FD520.c does.
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/8011722C_fn_8011722C.c`.
 */

#include "types.h"
#include "ef.h"

/* ---------------------------------------------------------------------------------------------------
 * the job record and its kind-1 work block
 * ------------------------------------------------------------------------------------------------- */

/* The effect character `fn_800F8914` hands out: one 0x168-byte slot of the shared `eft_control` pool
 * (the pointer it returns is slot+4, so this is the object res_eft_model_create* fills in).
 * size: 0x110 - a lower bound, from the two fields res_eft_model_create_light writes (+0x35 and
 * +0x10C); everything past +0x35 is untouched here. */
typedef struct MHchar {
    /* +0x000 */ u8 unused_0x000[0x04];
    /* +0x004 */ Vec pos_0x004;   /* the world position vec_to_mh_vec3 writes */
    /* +0x010 */ u8 unused_0x010[0x1C - 0x10];
    /* +0x01C */ Vec scale_0x01C; /* the scale setVector3 writes */
    /* +0x028 */ u8 unused_0x028[0x35 - 0x28];
    /* +0x035 */ u8 ready_0x035;   /* set by res_eft_model_create_light, cleared again here */
    /* +0x036 */ u8 unused_0x036[0x110 - 0x36];
} MHchar;

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

/* ---------------------------------------------------------------------------------------------------
 * externs - the callees and the shared pool
 * ------------------------------------------------------------------------------------------------- */

extern MHchar* fn_800F8914(void);                 /* takes a free eft_control slot, returns its chara */
extern void fn_80116FCC(EftJob* self);            /* the job's per-frame callback for the kind-1 work */
extern void fn_80117074(EftJob* self);            /* advances the job's dispatch state */
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
    self->callback_0x40 = fn_80116FCC;
    work->count = 1;
    for (i = 0; i < work->count; i++) {
        work->chara[i] = fn_800F8914();
        if (work->chara[i] == 0) {
            fn_80117074(self);
            return;
        }
    }
    if (!res_eft_model_create__FP6MHcharUsUl(work->chara[0],
                                             get_now_mapno__Fv() == 12 ? 24 : 23, 0)) {
        fn_80117074(self);
        return;
    }
    work->chara[0]->ready_0x035 = 0;
    vec_to_mh_vec3__FPQ34nw4r4math4VEC3P3Vec(&work->chara[0]->pos_0x004,
                                            (Vec*)&lbl_805A0460[ran_suu__Fl(1) % 3]);
    setVector3__FPQ34nw4r4math4VEC3fff(&work->chara[0]->scale_0x01C, lbl_80796A80, lbl_80796A80,
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
