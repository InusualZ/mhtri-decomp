/* ef/eft028_set.cpp - the eft028 break/crumble family's setters (`eft028_set`, `eft028_set_koware`,
 *   `eft028_set_capsule`, `eft028_set_scaled`), which build their records through `ef/fn_80119C44.c`.
 * RANGE. .text 0x80119970-0x80119C44 (4 functions); extab 0x8000C534-0x8000C554, extabindex 0x800266B8-0x800266E8,
 *   .sdata2 0x80796B24-0x80796B2C (an unreferenced 0.0f, then 410.0f).  The seam with `ef/eft026_fx.cpp` is the pool:
 *   0.0f recurs at 0x80796B24 after eft026's own 0.0f at 0x80796AC0, extabindex record 12 names `eft028_set`, and
 *   `fn_801198F8` (the last eft026 function) is called from `fn_80118214`.
 * FLAGS. `cflags_main`, as `ef/eft026_fx.cpp`; `#pragma peephole off` over the bodies, as there.
 * NAMES. `eft028_set_koware` is the runtime dump's own name (C++ linkage, the map row carries the mangling); the rest
 *   are plain `extern "C"`.  GUESS: the file name `eft028_set` (from the family's setters; no `__FILE__` string);
 *   GUESS (from its body and its caller): eft028_set_scaled.
 * RESIDUALS. The unreferenced 0.0f at 0x80796B24 is not in the linked object: a dead store `param.radius = 0.0f` pools it
 *   (object 0x8) and an unreferenced `return 0.0f` function under `exceptions off` too, but the link strips the
 *   unreferenced entry either way (flipcheck NOT READY; the stripped-function trial flip fails the DOL hash).
 *   Open: whether this TU also holds `ef/fn_80119C44.c` (its pool 0x80796B2C = 1.0f follows with no duplicate), and the eft028 data
 *   (`eft028_alpha_by_id` .data 0x805A0730, `eft028_capsule_ends` .bss 0x806A4548) sitting in `ef/eft029.cpp`'s
 *   ranges, whose pool restarts with 0.0f at 0x80796B30.
 * SHAPES. `eft028_set_capsule` returns nothing.
 */

#include "ef/eft028_alpha_by_id.h" /* eft028_alpha_by_id (rule 2: the owner's header) */
#include "ef/eft028_capsule_ends.h" /* eft028_capsule_ends (rule 2: the owner's header) */
#include "ef/eft028_set.h"
#include "types.h"
#include "nw4r/math.h"
#include "ef.h"
#include "pl.h"
#include "unsplit/unknown.h"
#include "ef/eft_res.h"
#include "ef/effect.h"
#include "ef/eft001.h"
#include "ef/eft028_set_scaled.h"
#include "Pl/pl_hit_sphere.h" /* PlCapsule, PlCapsuleBox, pl_capsule_box_set (rule 2) */
#include "mh3_pad.h" /* VEC3::VEC3() / VEC3::operator= / VEC3(x, y, z) (rule 2) */

/* The eft028 work block `eft_res_slot_get(0x48)` attaches (the next unit, `ef/fn_80119C44.c`, owns the
 * record itself). size: 0x18 (lower bound). */
typedef struct _EFT28_WORK {
    /* +0x00 */ s32 mode;
    /* +0x04 */ u8 pad_0x04[4];
    /* +0x08 */ u8 col_r;      /* the tev colour eft028_set_capsule arms */
    /* +0x09 */ u8 col_g;
    /* +0x0A */ u8 col_b;
    /* +0x0B */ u8 col_a;
    /* +0x0C */ u8 param_0x0C[0x10 - 0x0C]; /* pl_capsule_box_set's output */
    /* +0x10 */ f32 field_0x10;             /* eft028_set_scaled's scale */
    /* +0x14 */ u8 key_0x14;                /* eft028_set's colour keys */
    /* +0x15 */ u8 key_0x15;
    /* +0x16 */ u8 key_0x16;
    /* +0x17 */ u8 key_0x17;
} _EFT28_WORK;

/* The 0x1C-byte parameter record `CAPSULE::CAPSULE()` builds and `pl_capsule_box_set` reads in eft028_set_capsule. */

extern "C" {
/* the plain callees with no reconstructed owner header yet (declared here, never `extern`-spelled, so the
 * symbol is the target's plain name) */
_EFT* fn_80119C44(u8 kind, u8 variant, u32 arg);
void fn_80119D10(_EFT* self);
void fn_80119D9C(_EFT* self);

}

#pragma peephole off

/* Spawns an eft028 record of `kind` for `variant` 0 or 1 and seeds its colour keys from the map. */
extern "C" void eft028_set(u8 kind, u8 variant, u8 id)
{
    u32 mode;
    _EFT* rec;
    _EFT28_WORK* work;

    switch (variant) {
    case 0:
        mode = 3;
        break;
    case 1:
        mode = 1;
        break;
    default:
        return;
    }
    rec = fn_80119C44(kind, id, mode);
    if (rec == 0) {
        return;
    }
    work = (_EFT28_WORK*)rec->work_0x38;
    work->key_0x14 = 0xff;
    work->key_0x15 = 0xff;
    work->key_0x16 = 0xff;
    if (get_now_mapno() == 0) {
        work->key_0x17 = eft028_alpha_by_id[id];
    } else {
        work->key_0x17 = 0xff;
    }
    rec->timer_0x0C = 0;
}

/* Spawns the eft028 break effect `kind` at `pos` with `timer`. */
void eft028_set_koware(u8 kind, nw4r::math::VEC3* pos, u8 variant, long timer)
{
    _EFT* rec = fn_80119C44(kind, variant, 1);

    if (rec == 0) {
        return;
    }
    rec->pos_0x18 = *pos;
    rec->timer_0x0C = timer;
}

/* Spawns the area's eft028 capsule record between the two capsule ends. */
extern "C" void eft028_set_capsule(u8 area)
{
    _EFT* rec;
    _EFT28_WORK* work;

    PlCapsule param;
    if (area != get_now_areano()) {
        return;
    }
    rec = (_EFT*)(void*)eft_res_slot_get(0x48);
    if (rec == 0) {
        return;
    }
    rec->field_0x03 = 28;
    rec->type_0x02 = 2;
    rec->area_0x44 = area;
    eft_state_flags_set(rec, 0, 0);
    work = (_EFT28_WORK*)rec->work_0x38;
    work->mode = 1;
    param.start = eft028_capsule_ends[0];
    param.end = eft028_capsule_ends[1];
    param.radius = 410.0f;
    pl_capsule_box_set(&param, (PlCapsuleBox*)&work->param_0x0C[0]);
    work->col_r = 0xff;
    work->col_g = 0xff;
    work->col_b = 0xff;
    work->col_a = 0;
    rec->pos_0x18 = param.start;
    rec->timer_0x0C = 0;
    rec->release_0x40 = fn_80119D10;
    rec->dispatch_0x34 = fn_80119D9C;
}

/* Spawns an eft028 record at `pos` with rotation `rot`, `scale` and `timer`. */
extern "C" void eft028_set_scaled(u8 kind, nw4r::math::VEC3* pos, _CP_VECTOR* rot, f32 scale, u8 variant, long timer)
{
    _EFT* rec = fn_80119C44(kind, variant, 1);
    _EFT28_WORK* work;

    if (rec == 0) {
        return;
    }
    work = (_EFT28_WORK*)rec->work_0x38;
    work->field_0x10 = scale;
    rec->pos_0x18 = *pos;
    eft_rot_vec_copy(&rec->rot_0x24, rot);
    rec->timer_0x0C = timer;
}

