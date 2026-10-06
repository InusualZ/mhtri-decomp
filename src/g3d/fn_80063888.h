/* g3d/fn_80063888.h - the cross-unit declarations of `g3d/fn_80063888.cpp` (C linkage, plain map stems), except
 *   `fn_80066C8C`, which keeps C++ linkage outside the `extern "C"` block (map row `fn_80066C8C__FPv`). */
#ifndef MHTRI_G3D_FN_80063888_H
#define MHTRI_G3D_FN_80063888_H

#include "types.h"
#include "g3d/g3d_camera_types.h" /* `nw4r::g3d::Camera::PostureInfo`, the object `camera_posture_info_ctor` initialises */

#ifdef __cplusplus
extern "C" {
#endif

/* The three type-name store copies the texture-SRT classes GetTypeObj members call (g3d/fn_800680CC.cpp): each stores v through out and returns out. */
const u8 **type_obj_set_name_texsrt_res(const u8 **out, const u8 *v);
const u8 **type_obj_set_name_texsrt_node(const u8 **out, const u8 *v);
const u8 **type_obj_set_name_texsrt_override(const u8 **out, const u8 *v);

/* The cluster's owns with plain `fn_XXXXXXXX` map names, so C linkage. */
/* 0x80064C14 - releases the object the second argument points at (`light/light.cpp`'s fn_802C1AD8 tail-calls it). */
void fn_80064C14(void *self, const f32 *v);
u32 fn_800651BC(void *self);                /* 0x800651BC - reads the word at +0x0 of `self` */
void *vec3_copy_construct(void *out, void *in);     /* 0x80067E54 - copies a VEC3 (three floats), returns `out` */
s32 fn_80067EE8(const void *p);             /* 0x80067EE8 - `*(u32*)p != 0` */
#ifdef __cplusplus
/* 0x80067E70 - the posture record's constructor: default-constructs its position, target and up vectors. */
nw4r::g3d::Camera::PostureInfo *camera_posture_info_ctor(nw4r::g3d::Camera::PostureInfo *self);
#endif

/* The animation type-name records (`.rodata`: a length word, then the NUL-terminated name) `g3d/g3d_anmchr.cpp`'s
 * type-info members read. */
extern u8 anm_typename_AnmObjChrNode[];   /* 0x8056F510 - "AnmObjChrNode" */
extern u8 anm_typename_AnmObjChrBlend[];  /* 0x8056F524 - "AnmObjChrBlend" */
extern u8 anm_typename_AnmObj[];          /* 0x8056F568 - "AnmObj" */

/* The 3-float clamp `g3d/g3d_resanmchr.cpp`'s frame walkers call. */
f32 fn_8006497C(f32 a, f32 b, f32 c);       /* 0x8006497C - the 3-float clamp helper */

/* 0x800649B4 - the animation-object flag setter `g3d/g3d_scnmdl.cpp`'s fn_8007EA08 calls; the object is opaque
 * here, and the owner reads it through its `G3dFlagWord` view of the +0xC flag word. */
s32 fn_800649B4(void* pSelf, u32 bits);

#ifdef __cplusplus
}
#endif

/* The one real C++ free function of the cluster: the front-end mangles this to `fn_80066C8C__FPv`. */
void *fn_80066C8C(void *obj);

/* The resolved-resource records `fn_8006584C`/`fn_80066E80` return, shared with `g3d/g3d_resanmlight.cpp`: the
 * fields both units read; each record continues past its last field, so the sizes are lower bounds. */
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

/* The checked resource resolver `g3d/g3d_resfile.cpp`'s revision checks read. */
u32 fn_80063FD0(void *p);                /* 0x80063FD0 - the checked `ResAnmScn` resource resolver */

#ifdef __cplusplus
}

/* 0x80065804 - the placement `operator delete` (a bare `blr`); the landing pad of a placement `new` whose
 * constructor throws calls it. */
/* untyped: opaque handle passed through - the block and the placement address */
void operator delete(void* block, void* place);
#endif

#endif /* MHTRI_G3D_FN_80063888_H */

