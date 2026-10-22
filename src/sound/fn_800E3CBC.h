/*
 * sound/fn_800E3CBC.h - the declarations `src/sound/fn_800E3CBC.cpp` owns (docs/plan.md 6.5 rule 2).
 * `prim_init_all` (0x800E3D40, the map's `prim_init_all__Fv`) resets the primitive draw state every
 * frame; added with `quest/arenatask.cpp`, the first consumer.
 */
#ifndef MHTRI_SOUND_FN_800E3CBC_H
#define MHTRI_SOUND_FN_800E3CBC_H

#include "types.h"
#include "nw4r/math.h"

/* The 0x34-byte primitive record both record lists hold (`sound/sound_job.cpp` allocates, initialises and
 * fills it).  Layout from the field accesses of both units (`+0x04` sort key, `+0x18` handle, `+0x1C` kind,
 * `+0x1D` layer, `+0x1E` flags, `+0x20` callback).
 * size: 0x34 */
typedef struct PrimRec PrimRec;
struct PrimRec {
    /* 0x00 */ u32 next;                       /* free-list / sorted-list link */
    /* 0x04 */ f32 key;                        /* depth metric (fn_800E4284 sorts on it) */
    /* 0x08 */ u8 state;                       /* 0 idle, 1/2 the two drawn states */
    /* 0x09 */ u8 started;                     /* the "callback already fired" latch */
    /* 0x0A */ u8 pad_0x0A[2];
    /* 0x0C */ VEC3 pos;                       /* +0x0C..+0x17; pos.y is read at +0x10 */
    /* 0x18 */ void* handle;                   /* engine object the record wraps */
    /* 0x1C */ u8 kind;                        /* 0/1/2: the three draw kinds */
    /* 0x1D */ u8 layer;                       /* 0..5: which free-list / sort direction */
    /* 0x1E */ u16 flags;                      /* bit field; see the masks in fn_800E3CBC.cpp */
    /* 0x20 */ void (*callback)(PrimRec*);
    /* 0x24 */ f32 limit_lo;                   /* lower cull bound (fnegated) */
    /* 0x28 */ f32 limit_hi;                   /* upper cull bound */
    /* 0x2C */ f32 f_0x2C;
    /* 0x30 */ u8 pad_0x30[4];
};

#ifdef __cplusplus
extern "C" {
#endif
/* .sbss 0x807949A0 / 0x807949A8 - the two record lists' bases and live counts.  GUESS names. */
extern u32 prim_rec_pool[2];
extern u32 prim_rec_count[2];
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
void prim_init_all(void);
#endif

#ifdef __cplusplus
/* 0x800E4574 - sets the 2D blend mode (C++ scope: `set_blendmode__FUcUcUc`). */
void set_blendmode(u8 src, u8 dst, u8 op);
#endif

#ifdef __cplusplus
extern "C" {
#endif
/* 0x800E3DE0 - the sound resets the system core's full reset runs after prim_init_all. */
void sound_free_lists_reset(void);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_FN_800E3CBC_H */
