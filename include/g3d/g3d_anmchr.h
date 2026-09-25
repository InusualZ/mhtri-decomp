/*
 * The `g3d/g3d_anmchr.cpp` cluster's cross-unit declarations (docs/plan.md 6.5 rule 2).  A symbol a
 * registered unit owns is declared once, in that owner's header, and every consumer includes it; this
 * is that header for the g3d character-animation cluster registered from proposal `8005ABD8`
 * (`.text` 0x8005ABD8-0x80063888).
 *
 * `g3d/fn_800680CC.cpp` - the right-hand animation-object cluster (0x800680CC-0x8006EAC0) - calls
 * these nine `fn_XXXXXXXX`/`dtor_XXXXXXXX` stems and used to declare them locally, because the range
 * was unclaimed and its address band named no single module.  Registering `g3d_anmchr.cpp` makes them
 * owned, so the declarations move here and the consumer includes this header instead.
 *
 * All nine keep C linkage (the map carries plain `fn_XXXXXXXX`/`dtor_XXXXXXXX` stems).  The return
 * types are the ones the target bodies imply and the consumers use: 0x8005DC60/0x8005DCD0 store
 * through their first argument and return it; 0x800628A4 and 0x8005DC24 load through their argument.
 * The owner's own forward prototypes in `g3d_anmchr.cpp` predate this header and are ABI-identical.
 */
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
void **fn_80062914(void **out, u32 v);     /* 0x80062914 - stores `v` through `out`, returns `out` */
void *fn_8006268C(void *self, u32 value);   /* 0x8006268C - stores `value` at +0x0 of `self`, returns `self`.
                                             * ONE declaration only: this branch's consumer
                                             * g3d/g3d_resanmtexsrt.cpp spells the parameters `(out, v)` and
                                             * main's g3d/g3d_resanmlight.cpp spells them `(self, value)`,
                                             * but the type is identical, so the merge keeps main's landed
                                             * line and drops the duplicate (a second, disagreeing-looking
                                             * declaration is the "illegal function overloading" trap). */

u32 fn_80062750(void *out, void *key);     /* 0x80062750 - the resource-table lookup */
u32 fn_8005DC24(u32 *p);                   /* 0x8005DC24 - loads the word at +0x0 of `p`, +4 */
void *fn_8005B1E4(void *self, u32 value);  /* 0x8005B1E4 - stores `value` at +0x0 of `self`, returns `self` */

/* Added when `g3d/g3d_resanmchr.cpp` registered (rule 2): the `ResAnmChr` channel evaluators call
 * back into this cluster's frame/rate helpers.  Types are the ones the target bodies imply. */
f32 fn_800610AC(f32 value);                /* 0x800610AC - the reciprocal helper */
void *fn_800618BC(void *self);             /* 0x800618BC - the resource-table base */
s32 fn_800628C8(void *self, s32 key);      /* 0x800628C8 - the table entry lookup */

/* 0x800600C0 - `GetParent()`, the `!GetParent()` assert's test; the ScnMdl destructor calls it.
 * Inside the `extern "C"` block: the map name is plain, so a C++ consumer must not mangle it
 * (`g3d_scnmdl.cpp` referenced `fn_800600C0__FPUl`).  The owner defines it `extern "C" u32
 * fn_800600C0(u32*)` in `g3d_anmchr.cpp`; `fn_80075DCC.cpp`'s zero-argument declaration is a
 * separate TU and its object already references the plain name. */
u32 fn_800600C0(u32* p);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_ANMCHR_H */

