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
/* 0x80064C14 - releases the object the second argument points at; added with `light/light.cpp`
 * (rule 2: this range owns the address), whose `fn_802C1AD8` tail-calls it. */
void fn_80064C14(void *self, const f32 *v);
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

/* 0x800649B4 - the animation-object flag setter the ScnMdl unit's fn_8007EA08 calls (rule 2).  The
 * target object references the plain name, so C linkage: it sits inside this `extern "C"` block.
 * The object is spelled `void*` here - the same opaque-pointer convention the rest of this header
 * uses - and the owner casts it to its local `G3dFlagWord` view of the +0xC flag word. */
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

/*
 * The resolved-resource records `fn_8006584C`/`fn_80066E80` return (rule 1: the second consumer
 * `g3d/g3d_resanmlight.cpp` needs them, so they move here from `fn_80063888.cpp`).  The layouts are
 * the union of the fields both consumers evidence; the record continues past the last field read
 * here, so sizes are lower bounds.
 */
typedef struct {
    /* +0x00 */ u8 pad_0x00[0x10];
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u8 pad_0x14[0x20];
    /* +0x34 */ u16 field_0x34;
    /* +0x36 */ u16 field_0x36;
    /* +0x38 */ u32 field_0x38;
    /* +0x3C */ u16 field_0x3C;
    /* +0x3E */ u16 field_0x3E;
    /* +0x40 */ u16 field_0x40;
    /* +0x42 */ u16 field_0x42;
    /* +0x44 */ u16 field_0x44;
} ResAnmScnConfig; /* size: 0x46 (approximate - the record continues past +0x44) */

typedef struct {
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ u32 field_0x04;      /* sub-resource offset resolved against the object base */
    /* +0x08 */ u8 pad_0x08[0x08];
    /* +0x10 */ u32 field_0x10;
    /* +0x14 */ u32 field_0x14;      /* the type word copied verbatim into the result */
    /* +0x18 */ u16 field_0x18;
    /* +0x1A */ u8 field_0x1A;
    /* +0x1B */ u8 pad_0x1B;
    /* +0x1C */ u32 flags;           /* per-channel "inline value" bits */
    /* +0x20 */ u32 dataOffset;      /* base of the packed channel array */
    /* +0x24 */ u32 channel_0x24;
    /* +0x28 */ u32 channel_0x28;
    /* +0x2C */ u32 channel_0x2C;
    /* +0x30 */ u32 channel_0x30;
    /* +0x34 */ u32 channel_0x34;
    /* +0x38 */ u32 channel_0x38;
    /* +0x3C */ u32 channel_0x3C;
    /* +0x40 */ u32 field_0x40;
    /* +0x44 */ u32 channel_0x44;
    /* +0x48 */ u32 channel_0x48;
    /* +0x4C */ u32 field_0x4C;
    /* +0x50 */ u32 channel_0x50;
    /* +0x54 */ u32 channel_0x54;
    /* +0x58 */ u32 channel_0x58;
} ResAnmLightConfig; /* size: 0x5C (approximate - the record continues past +0x58) */

#ifdef __cplusplus
extern "C" {
#endif

ResAnmScnConfig *fn_8006584C(void *p);   /* 0x8006584C - the checked `ResAnmScn` config getter */
ResAnmLightConfig *fn_80066DB4(void *p); /* 0x80066DB4 - the checked light-config getter */
ResAnmLightConfig *fn_80066E80(void *p); /* 0x80066E80 - the checked light-config getter */

/* Added when `g3d/g3d_resfile.cpp` registered (rule 2): the checked resource resolver its revision
 * checks read.  `fn_8006584C` (above) already returns the `ResAnmScnConfig` this file defines. */
u32 fn_80063FD0(void *p);                /* 0x80063FD0 - the checked `ResAnmScn` resource resolver */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_FN_80063888_H */

