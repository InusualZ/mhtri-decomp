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


#endif /* MHTRI_G3D_G3D_ANMCHR_H */

