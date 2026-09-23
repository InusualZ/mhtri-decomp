#ifndef EF_H
#define EF_H

#include "types.h"

/* nw4r effect-library (`nw4r::ef`) shared declarations for the emitter-shape units.  The shape files
 * (`ef_disc.cpp`, `ef_cylinder.cpp`, `ef_line.cpp`, ...) each define one `EmitterForm` subclass whose
 * single virtual method is its `CreateEmitter`; they all take the same `em`/`pm`/`params` triple and
 * build a particle transform from the same helper set, so the types and prototypes live here once. */

/* A 3-float vector (nw4r::math::VEC3). */
typedef struct Vec {
    f32 x; /* +0x00 */
    f32 y; /* +0x04 */
    f32 z; /* +0x08 */
} Vec; /* size: 0x0C */

/* The emission parameter block.  Its interpretation is per-shape; the disc reads the two radii, the
 * sweep rate and the start/end angle. */
typedef struct EfParams {
    f32 scale_x;     /* +0x00  x radius */
    f32 rate;        /* +0x04  sweep/scale rate, in percent */
    f32 angle_base;  /* +0x08  start angle of the sweep */
    f32 angle_end;   /* +0x0C  end angle of the sweep */
    f32 scale_z;     /* +0x10  z radius */
    f32 unused_0x14; /* +0x14  not read by the disc shape */
} EfParams; /* size: 0x18 */

typedef struct EfParticle EfParticle;

/* The particle manager's dispatch table; +0x14 is the spawn entry. */
typedef struct EfParticleSlots {
    u8 pad_0x00[0x14]; /* +0x00 */
    void (*spawn)(EfParticle* self, u16 id, Vec* pos, Vec* dir, s32 param, u8* extra,
                  u32 extra2, u16 extra3, f32 scale); /* +0x14 */
} EfParticleSlots; /* size: at least 0x18 */

struct EfParticle {
    u8 pad_0x00[0x1C];      /* +0x00 */
    EfParticleSlots* slots; /* +0x1C */
}; /* size: at least 0x20 */

/* The per-emitter work record `em`.  Only the fields the shape files read are named. */
typedef struct EfWork {
    u8 pad_0x00[0x32];  /* +0x00 */
    u16 split_count;    /* +0x32  number of steps the sweep is divided into */
    u8 pad_0x34[0x33];  /* +0x34 */
    s8 scale_rate;      /* +0x67  per-frame scale step, in hundredths */
    u8 pad_0x68[0x10];  /* +0x68 */
    f32 spread;         /* +0x78  orientation spread; 0 means axis-aligned */
    u8 pad_0x7C[0x6C];  /* +0x7C */
    u16 spawn_flag;     /* +0xE8  passed through to the particle's spawn slot */
    u8 pad_0xEA[0x02];  /* +0xEA */
    u32 progress;       /* +0xEC  fixed-point progress read by fn_800A8A08 */
    u8 pad_0xF0[0x08];  /* +0xF0 */
    u32 spawn_extra;    /* +0xF8  passed through to the particle's spawn slot */
    u8 spawn_data;      /* +0xFC  address passed through to the particle's spawn slot */
} EfWork; /* size: at least 0xFD (the record continues past what this unit reads) */

/* nw4r::db::Panic - the assert failure handler (variadic). */
extern void Panic__Q24nw4r2dbFPCciPCce(const char* file, int line, const char* fmt, ...);

/* nw4r::math and effect-library helpers, all still `fn_*` in the symbol map. */
extern void fn_80043EA8(Vec* out);                                  /* out = (0, 0, 0) */
extern void fn_80041E8C(Vec* out, f32 x, f32 y, f32 z);             /* out = (x, y, z) */
extern void fn_80051490(Vec* out, Vec* in);                         /* out = in */
extern void fn_8009C6F0(Vec* out, f32 angle);                       /* sin/cos of angle */
extern void fn_8009C760(f32* out_a, f32* out_b, f32 angle);         /* sin/cos of angle */
extern void fn_800A99B4(s32 ctx, Vec* out, EfWork* em, Vec* pos, Vec* a, Vec* b, Vec* c);
extern u16 fn_800A9FB0(s32 ctx, u16 id, f32 scale, EfWork* em);
extern f32 fn_800A8A08(u32* progress);                              /* pseudo-random 0..1 */
extern f32 fn_80050BC0(f32 a, f32 b);                               /* eased interpolation */
extern f32 fn_800C9DCC(f32 a);                                      /* fabsf */
extern f32 fn_80463F10(f32 a, f32 b);                               /* fmodf */

/* This file's own pooled data (`ef_disc.cpp`), declared but never defined here. */
extern char lbl_80594DE0[]; /* "ef_disc.cpp"                                  .data 0x80594DE0 */
extern char lbl_80594DEC[]; /* "NW4R:Pointer Error\nem(=%p) is not valid..." .data 0x80594DEC */
extern char lbl_80594E20[]; /* "NW4R:Pointer Error\npm(=%p) is not valid..." .data 0x80594E20 */
extern char lbl_80594E54[]; /* "NW4R:Pointer Error\nparams(=%p) is not valid." .data 0x80594E54 */

extern f32 lbl_807962B0; /* FLT_EPSILON     .sdata2 0x807962B0 */
extern f32 lbl_807962B4; /* 0.0f            .sdata2 0x807962B4 */
extern f32 lbl_807962B8; /* 2.0f            .sdata2 0x807962B8 */
extern f32 lbl_807962BC; /* pi              .sdata2 0x807962BC */
extern f32 lbl_807962C0; /* 2pi             .sdata2 0x807962C0 */
extern f32 lbl_807962C4; /* 2pi * 2^-15     .sdata2 0x807962C4 */
extern f32 lbl_807962C8; /* 2pi - 2^-14     .sdata2 0x807962C8 */
extern f32 lbl_807962CC; /* 100.0f          .sdata2 0x807962CC */
extern f32 lbl_807962D0; /* 1.0f            .sdata2 0x807962D0 */
extern f32 lbl_807962D4; /* 0.01f           .sdata2 0x807962D4 */

/* The console's cached/uncached MEM1 and MEM2 windows plus the 0xE0000000 register page.  Inlined
 * into every caller (`-inline noauto` still inlines an `inline` function). */
inline int IsValidPointer(u32 ptr) {
    return ((ptr & 0xFF000000) == 0x80000000)
        || ((ptr & 0xFF800000) == 0x81000000)
        || ((ptr & 0xF8000000) == 0x90000000)
        || ((ptr & 0xFF000000) == 0xC0000000)
        || ((ptr & 0xFF800000) == 0xC1000000)
        || ((ptr & 0xF8000000) == 0xD0000000)
        || ((ptr & 0xFFFFC000) == 0xE0000000);
}

/* The file's pointer guard: the message comes from the call site, the line from `__LINE__`. */
#define EF_ASSERT_PTR(file, msg, ptr) \
    if (!IsValidPointer((u32)(ptr))) \
        Panic__Q24nw4r2dbFPCciPCce(file, __LINE__, msg, (ptr))

#ifdef __cplusplus
}
#endif

#endif /* EF_H */
