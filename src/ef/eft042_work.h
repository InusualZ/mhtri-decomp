/* ef/eft042_work.h - the effect work area `ef/eft041.cpp` and `ef/fn_8030681C.cpp` share (rule 1). */
#ifndef MHTRI_EF_EFT042_WORK_H
#define MHTRI_EF_EFT042_WORK_H

#include "types.h"
#include "nw4r/math.h"

struct MHchar;

/* One model record of the work area: the pooled model handle the spawner stores and the create
 * result the state-0 bodies file.  Two families disagree about the second word - the kind-2 bodies
 * (eft042_set_placed stores, eft042_model_create_kind2 reads) use it as the model's scale factor - so it is a union
 * (rule 5: each member names the site that uses it, and both keep the +0x04 offset).
 * size: 0x08 */
typedef struct Eft042Model {
    struct MHchar* model;      /* +0x00  `eft_res_model_get`'s pooled model handle */
    union {
        void* created;  /* +0x04  the `res_eft_UV_model_create*` result (effect 41, kind 0) */
        f32   scale;    /* +0x04  the model's uniform scale (kind-2 bodies) */
    } v_0x04;
} Eft042Model;

/* The work area.  `count`/`models` are the head every family shares; the 0x10 bytes at +0x0C are the
 * four `_g3d_work` handles for effect 41 and the kind-0 bodies (which use the last two), a placement
 * offset plus an angle for effect 42's kind-1 body.  size: 0x20 */
typedef struct Eft042Work {
    u32         count;       /* +0x00  the model count `fn_800F8B44`'s record was taken for */
    Eft042Model models[1];   /* +0x04 */
    union {
        /* effect 41 (`ef/fn_803066F0.c`'s `EftWorkData::works`) and the kind-0 bodies, which use
         * only the last two slots (`eft042_set`/`eft042_slot_get_typed` clear them, `eft042_release_kind0` releases
         * them, `eft042_model_create_kind0` fills them). */
        void* works[4];      /* +0x0C */
        struct {             /* size: 0x10 */
            nw4r::math::VEC3 pos;  /* +0x0C  the model's placement offset (kind 1) */
            f32              angle;/* +0x18  its yaw (kind 1) */
        } kind1;
    } v_0x0C;
    void*       resource;    /* +0x1C  the area resource `lb_area_fx_set` stores */
} Eft042Work;

#endif /* MHTRI_EF_EFT042_WORK_H */
