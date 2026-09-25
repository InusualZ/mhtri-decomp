/*
 * The `g3d/fn_80063888.cpp` cluster's cross-unit declarations (docs/plan.md 6.5 rule 2).  A symbol a
 * registered unit owns is declared once, in that owner's header, and every consumer includes it; this is
 * that header for the g3d animation-object cluster registered from proposal `80063888`
 * (`.text` 0x80063888-0x800680A8).
 *
 * Four of the cluster's `fn_XXXXXXXX` stems are called from other units (`ef/effect.cpp`,
 * `ef/fn_800AEE48.cpp`, `g3d/g3d_camera.cpp`, `g3d/g3d_resanm.c`, `g3d/g3d_anmscn.cpp`); those consumers
 * used to declare them locally because the address band was unsplit.  Registering the cluster makes them
 * owned, so the declarations move here (rule 2) and the consumers include this header instead.
 *
 * `fn_80066C8C` is the one symbol that is a real C++ free function - the map carries its mangling
 * (`fn_80066C8C__FPv`), so its declaration keeps C++ linkage and sits outside the `extern "C"` block.
 */
#ifndef MHTRI_G3D_FN_80063888_H
#define MHTRI_G3D_FN_80063888_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The cluster's owns with plain `fn_XXXXXXXX` map names, so C linkage. */
s32 fn_80064820(void *out);                 /* 0x80064820 - the caller keeps its own argument in r3 */
u32 fn_800651BC(void *self);                /* 0x800651BC - reads the word at +0x0 of `self` */
void *fn_80067E54(void *out, void *in);     /* 0x80067E54 - copies 0x10 B, returns `out` */
s32 fn_80067EE8(const void *p);             /* 0x80067EE8 - `*(u32*)p != 0` */

#ifdef __cplusplus
}
#endif

/* The one real C++ free function of the cluster: the front-end mangles this to `fn_80066C8C__FPv`. */
void *fn_80066C8C(void *obj);

#endif /* MHTRI_G3D_FN_80063888_H */
