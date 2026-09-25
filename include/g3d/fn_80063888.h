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

/* Added when `g3d/g3d_resanmchr.cpp` registered (rule 2): the frame walkers clamp a frame through
 * this cluster's 3-float helper. */
f32 fn_8006497C(f32 a, f32 b, f32 c);       /* 0x8006497C - the 3-float clamp helper */

/* The name-record store helper (0x800638B8), needed by the right-hand `g3d/fn_800680CC.cpp` cluster
 * (rule 2: the second consumer moves the declaration here from `fn_80063888.cpp`). */
void **fn_800638B8(void **out, void *v);    /* stores `v` through `out` and returns `out` */

/* The list-insert chain steps (0x800638F8/0x800639D0/0x80063964), also needed by the right-hand
 * cluster's own insert steps (rule 2). */
u32 fn_800638F8(void *self, u32 *other);
u32 fn_800639D0(u32 **a, u32 **b);
u32 fn_80063964(void *self, u32 *other);

/* 0x800649B4 - the animation-object flag setter `g3d/g3d_scnmdl.cpp`'s fn_8007EA08 calls (rule 2).
 * The target references the plain name, so it sits inside this extern "C" block. */
s32 fn_800649B4(void* pSelf, u32 bits);

#ifdef __cplusplus
}
#endif

/* The one real C++ free function of the cluster: the front-end mangles this to `fn_80066C8C__FPv`. */
void *fn_80066C8C(void *obj);

/* The object/vtable pair the cluster's dispatch wrappers share (rule 1: moved here from
 * `fn_80063888.cpp` when the right-hand `g3d/fn_800680CC.cpp` cluster needed the same layout). */
typedef u32 (*G3dVtMethod)(void *);
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ G3dVtMethod method_0x14;
} G3dVtbl; /* size: 0x18 */
typedef struct {
    /* +0x00 */ G3dVtbl *vt;
} G3dObj; /* size: 0x4 */

#endif /* MHTRI_G3D_FN_80063888_H */

