/* g3d/g3d_anmchr.h - the cross-unit declarations of `g3d/g3d_anmchr.cpp` (C linkage, plain map stems).
 *   0x8005DC60/0x8005DCD0 store through their first argument and return it; 0x800628A4 and 0x8005DC24 load through
 *   theirs.  The owner's own forward prototypes are ABI-identical. */
#ifndef MHTRI_G3D_G3D_ANMCHR_H
#define MHTRI_G3D_G3D_ANMCHR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void **fn_8005DC60(void **out, void *v);   /* 0x8005DC60 - stores `v` through `out`, returns `out` */
void **fn_8005DCD0(void **out, void *v);   /* 0x8005DCD0 - stores `v` through `out`, returns `out` */
void dtor_8005D384(void *self, s32 flag);  /* 0x8005D384 - the teardown destructor */
void fn_8005D3E0(void *self);              /* 0x8005D3E0 - empty advance (`blr`) */
u32 fn_800628B4(void *self);               /* 0x800628B4 - `*(u32*)self != 0` */
void *fn_800628A4(void *self);             /* 0x800628A4 - loads the word at +0x0 of `self` */

u32 fn_8005DC24(u32 *p);                   /* 0x8005DC24 - loads the word at +0x0 of `p`, +4 */

/* The name-record store helper (0x800638B8) `g3d/fn_800680CC.cpp` and `g3d/g3d_scnmdl.cpp` also call. */
const u8 **type_obj_set_name(const u8 **out, const u8 *v); /* stores `v` through `out` and returns `out` */

/* The list-insert chain steps (0x800638F8/0x800639D0/0x80063964) the other g3d units' insert steps call. */
u32 fn_800638F8(void *self, u32 *other);
u32 fn_800639D0(u32 **a, u32 **b);
u32 fn_80063964(void *self, u32 *other);

/* The frame/rate helpers `g3d/g3d_resanmchr.cpp`'s channel evaluators call (types the target bodies imply). */
f32 math_reciprocal(f32 value);                /* 0x800610AC - the reciprocal helper */
void *fn_800618BC(void *self);             /* 0x800618BC - the resource-table base */
s32 fn_800628C8(void *self, s32 key);      /* 0x800628C8 - the table entry lookup */

/* 0x800600C0 - `GetParent()`, the `!GetParent()` assert's test the ScnMdl destructor calls; C linkage, because the
 * map name is plain. */
u32 fn_800600C0(u32* p);

#ifdef __cplusplus
}
#endif

/* The object/vtable pair the dispatch wrappers of this unit, `g3d/fn_80063888.cpp` and `g3d/fn_800680CC.cpp` share (rule 1). */
typedef u32 (*G3dVtMethod)(void *);
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ G3dVtMethod method_0x14;
} G3dVtbl; /* size: 0x18 */
typedef struct {
    /* +0x00 */ G3dVtbl *vt;
} G3dObj; /* size: 0x4 */

#endif /* MHTRI_G3D_G3D_ANMCHR_H */

