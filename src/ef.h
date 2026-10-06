#ifndef EF_H
#define EF_H

#include "types.h"
#include "nw4r/math.h"
#include "nw4r/db_assert.h"   /* nw4r::db::Panic, owner nw4r/db_assert.cpp (rule 2) */

#ifdef __cplusplus
extern "C" {
#endif

/* nw4r effect-library (`nw4r::ef`) shared declarations for the emitter-shape units.  The shape files
 * (`ef_disc.cpp`, `ef_cylinder.cpp`, `ef_line.cpp`, ...) each define one `EmitterForm` subclass whose
 * single virtual method is its `CreateEmitter`; they all take the same `em`/`pm`/`params` triple and
 * build a particle transform from the same helper set, so the types and prototypes live here once.
 *
 * The game-side effect records live here too: `_EFT` is the 0x48-byte effect instance, `_EFT_WORK` the
 * counter block it carries at +0x38, `_CP_VECTOR` the three-word rotation `cpSetRotMatrix` takes, and
 * `_SHELL_W` the shell actor the player effect holds at +0x2C; each layout is the union of the fields the ef
 * units read (see each type's note).
 */

/* The engine's 3-float vector.  Same LAYOUT as `nw4r::math::VEC3`, but a different type: the
 * engine's vector library works on this one and `vec_to_mh_vec3` converts it to nw4r's
 * (`vec_to_mh_vec3(nw4r::math::VEC3* dst, Vec* src)`), so a `Vec*` and a `Vec3*` are NOT
 * interchangeable. size: 0x0C */
typedef struct Vec {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
    /* +0x08 */ f32 z;
} Vec; /* size: 0x0C */

/* The emission parameter block.  Its interpretation is per-shape; the disc reads the two radii, the
 * sweep rate and the start/end angle. */
typedef struct EfParams {
    f32 scale_x;     /* +0x00  x radius */
    f32 rate;        /* +0x04  sweep/scale rate, in percent */
    f32 angle_base;  /* +0x08  start angle of the sweep */
    f32 angle_end;   /* +0x0C  end angle of the sweep */
    f32 scale_z;     /* +0x10  second radius (the disc's z, the torus's y) */
    f32 scale_c;     /* +0x14  third radius (read by the torus shape; the disc ignores it) */
} EfParams; /* size: 0x18 */

typedef struct EfParticle EfParticle;

/* The emitter-parameter sub-record the particle carries at +0x20 (`ef/ef_particle.cpp`).  Offsets
 * 0x44..0x4F and 0x7A..0x7F are present in the object but untouched by every function that unit
 * owns, so they stay padding.  `colors` is the 2x2 {r, g, b, a} table the two colour getters read;
 * the four floats are the scale factors their product reads; `manager` is the record's owner. */
typedef struct EfParticleParams EfParticleParams;

/* One 8-byte sub-object the constructor builds with VEC2_ctor; three of them form each array. */
typedef struct EfParticleNode {
    /* +0x00 */ u8 data[8];
} EfParticleNode; /* size: 0x08 */

/* A pair of scale factors the constructor builds and the scale helpers multiply. */
typedef struct EfParticleScale {
    /* +0x00 */ f32 x;
    /* +0x04 */ f32 y;
} EfParticleScale; /* size: 0x08 */

struct EfParticleParams {
    /* +0x00 */ u8 colors[2][2][4];
    /* +0x10 */ EfParticleScale scale_0x10; /* the phases select which pair multiplies */
    /* +0x18 */ EfParticleScale scale_0x18;
    /* +0x20 */ VEC3 field_0x20;
    /* +0x2C */ EfParticleNode field_0x2C[3];
    /* +0x44 */ u8 pad_0x44[0x0C];
    /* +0x50 */ EfParticleNode field_0x50[3];
    /* +0x68 */ u8 field_0x68[0x11]; /* fn_800AB3D0 returns this address */
    /* +0x79 */ s8 field_0x79;      /* read as a signed flag by ef_particle_flick_alpha */
    /* +0x7A */ u8 pad_0x7A[0x06];
    /* +0x80 */ VEC3 field_0x80;
    /* +0x8C */ VEC3 field_0x8C;
    /* +0x98 */ VEC3 field_0x98;
    /* +0xA4 */ u8 pad_0xA4[0x04];
    /* +0xA8 */ struct EfParticleMgr* manager;
}; /* size: 0xAC (bounded by the particle's field_0xCC) */

/* The parameter record `ef_res_emitter_desc` walks to (reached through the owner's +0x24 pointer).  Only the
 * offsets this unit's functions read are named. */
typedef struct EfParticleChain EfParticleChain;
struct EfParticleChain {
    /* +0x000 */ u8 pad_0x000[0x105];
    /* +0x105 */ u8 mode;       /* 0 = no colour; 1..5 select a ramp, anything else asserts */
    /* +0x106 */ u16 count;
    /* +0x108 */ u8 count_step; /* per-frame step, in 1/12700ths of the count */
    /* +0x109 */ u8 amplitude;  /* the ramp's half-range */
}; /* size: 0x10A (lower bound: the highest offset any function of this unit reads) */

/* The record's owner: the effect manager the spawn path reaches through.  Only the offsets this
 * unit's functions read are named. */
typedef struct EfParticleMgr EfParticleMgr;
struct EfParticleMgr {
    /* +0x00 */ u8 pad_0x00[0x24];
    /* +0x24 */ void* context;   /* the object ef_resource_draw_setting walks into */
    /* +0x28 */ u8 pad_0x28[0x30];
    /* +0x58 */ f32 scale_a;     /* read by ef_particle_get_scale (inlined) */
    /* +0x5C */ f32 scale_b;     /* read by fn_800AB37C */

#ifdef __cplusplus
    /* The factor ef_particle_get_scale multiplies its scale product by.  It is an inline member in the original:
     * retail keeps the argument setup (`mr r4,r3`) and the call's branch (`b +4`) with the body inlined
     * right after it - so it stays a MEMBER and cannot move out of the struct.  Guarded because this
     * header is included by C units (ef/fn_8011722C.c), which must see the data fields only; a member
     * function adds no storage, so the layout is identical either way. */
    f32 GetScaleA(EfParticle* self, f32 v) { return v * scale_a; }
#endif
}; /* size: 0x60 (lower bound: the highest offset any function of this unit reads) */

/* The particle manager's dispatch table; +0x14 is the spawn entry. */
typedef struct EfParticleSlots {
    u8 pad_0x00[0x14]; /* +0x00 */
    void (*spawn)(EfParticle* self, u16 id, Vec* pos, Vec* dir, s32 param, u8* extra,
                  u32 extra2, u16 extra3, f32 scale); /* +0x14 */
} EfParticleSlots; /* size: at least 0x18 */

struct EfParticle {
    /* +0x00 */ u8 pad_0x00[0x1C];
    /* +0x1C */ EfParticleSlots* slots;
    /* +0x20 */ EfParticleParams params;
    /* +0xCC */ VEC3 field_0xCC;
    /* +0xD8 */ u8 pad_0xD8[0x04];
    /* +0xDC */ u16 field_0xDC;
    /* +0xDE */ u8 pad_0xDE[0x02];
}; /* size: 0xE0 (lower bound: +0xDC is the highest offset any function of this unit reads) */

/* The per-emitter work record `em`.  Only the fields the shape files read are named. */
typedef struct EfWork {
    u8 pad_0x00[0x32];  /* +0x00 */
    u16 split_count;    /* +0x32  number of steps the sweep is divided into */
    u8 pad_0x34[0x32];  /* +0x34 */
    s8 size_jitter;     /* +0x66  per-frame size jitter, in hundredths (read as signed) */
    s8 scale_rate;      /* +0x67  per-frame scale step, in hundredths */
    /* The five transform stages of the emitter form, each "off" at 0.0f.  The names come from what
     * the stage does in `ef/ef_emitterform.cpp` (fn_800A99B4), the only reader. */
    f32 dir_weight;       /* +0x68  blend the emitter direction into the result */
    f32 rot_weight;       /* +0x6C  blend the emitter's rotated axis into the result */
    f32 spread_scale;     /* +0x70  scale of the random Euler spread */
    f32 offset_scale;     /* +0x74  scale of the emitter position offset */
    f32 spread;           /* +0x78  orientation spread; 0 means axis-aligned (ef_disc family) */
    f32 axis_angle_scale; /* +0x7C  scale of the axis-angle spread */
    f32 axis_angle_y;     /* +0x80  its second angle; 0 selects the plain Euler path */
    f32 euler_x;          /* +0x84  Euler angles of the axis-angle stage */
    f32 euler_y;          /* +0x88 */
    f32 euler_z;          /* +0x8C */
    u8 pad_0x90[0x58];    /* +0x90 */
    u16 spawn_flag;     /* +0xE8  passed through to the particle's spawn slot */
    u8 pad_0xEA[0x02];  /* +0xEA */
    u32 progress;       /* +0xEC  fixed-point progress read by ef_random_float */
    u8 pad_0xF0[0x08];  /* +0xF0 */
    u32 spawn_extra;    /* +0xF8  passed through to the particle's spawn slot */
    u8 spawn_data;      /* +0xFC  address passed through to the particle's spawn slot */
} EfWork; /* size: at least 0xFD (the record continues past what this unit reads) */

/* --------------------------------------------------------------------------------------------------
 * the game-side effect records
 * -------------------------------------------------------------------------------------------------- */

#include "ef/cp_vector.h" /* `_CP_VECTOR`, the rotation triple `cpSetRotMatrix` takes */

/* The effect's per-family work block.  `count/scale/param_id` agree everywhere; the `effect` pointer is
 * `nw4r::ef::Effect*` in `ef/eft002.cpp` and `void*` in the two C units -- a type-only disagreement
 * (both are 4-byte pointers), kept as `void*` so the type needs no C++ name here.
 * size: 0x10 */
typedef struct _EFT_WORK {
    /* +0x000 */ s32 count;
    /* +0x004 */ void* effect;
    /* +0x008 */ f32 scale;
    /* +0x00C */ u32 param_id;
} _EFT_WORK;

/* The 0x48-byte effect instance.  Union of the copies in `ef/eft002.cpp`, `ef/eft009.cpp`,
 * `ef/fn_800FD520.c`, `ef/fn_800FD718.c`, `ef/fn_80114E34.cpp` and `ef/fn_80119C44.c`.
 *
 * Disagreements found (all reported): `type_0x02` is `u8` in five sites but `s8` in `ef/fn_800FD718.c`;
 * `source_0x30` is `void*` / `_ENEMY_WORK*`; `work_0x38` is `_EFT_WORK*` in five sites, `void*` in
 * `fn_80114E34.cpp`, `_EFT_HEAP_WORK*` in `fn_80119C44.c`, and stored BY VALUE as `_EFTWork` in
 * `ef/eft009.cpp` (the one that matters: a by-value store cannot be a pointer).  The pointer spelling
 * is canonical here. size: 0x48 */
typedef struct _EFT _EFT;
struct _EFT {
    /* +0x000 */ u8 pad_0x0[0x1];
    /* +0x001 */ u8 flag_0x01;
    /* +0x002 */ u8 type_0x02;
    /* +0x003 */ u8 field_0x03;
    /* +0x004 */ u8 field_0x04;
    /* +0x005 */ u8 state_0x05;
    /* +0x006 */ u8 field_0x06;
    /* +0x007 */ u8 field_0x07;
    /* +0x008 */ u8 demo_flag_0x08;
    /* +0x009 */ u8 pad_0x9[0x3];
    /* +0x00C */ s32 timer_0x0C;
    /* +0x010 */ s32 field_0x10;
    /* +0x014 */ u8 pad_0x14[0x4];
    /* +0x018 */ VEC3 pos_0x18;
    /* +0x024 */ _CP_VECTOR rot_0x24;
    /* +0x030 */ void* source_0x30;
    /* +0x034 */ void (*dispatch_0x34)(_EFT*);
    /* +0x038 */ _EFT_WORK* work_0x38;
    /* +0x03C */ u8 pad_0x3C[0x4];
    /* +0x040 */ void (*release_0x40)(_EFT*);
    /* +0x044 */ u8 area_0x44;
    /* +0x045 */ u8 pad_0x45[0x3];
};

/* The shell actor the player effect and the shell family read: `_PLW::equip_0x2C` points at one.  The
 * full record (size 0x10C) is owned by the shell pool unit; its definition is `stage/shell.h`'s. */
typedef struct _SHELL_W _SHELL_W;

/* The effect-model entry the ef units carry (`ef/fn_8010D1A8.c` and `ef/fn_803066F0.c`).  The two
 * views are compatible: `model`/`created` sit inside the first view's +0x00..0x35 block, the +0x35
 * byte and the +0x10C/+0x118 fields inside the second's.  Union of both. size: 0x11C */
struct EftModelSlot; /* ef/fn_8010D1A8.c; only pointed at here */
typedef struct EftModel {
    /* +0x000 */ struct MHchar* model;
    /* +0x004 */ void* volatile created;
    /* +0x008 */ u8 pad_0x8[0x14];
    /* +0x01C */ VEC3 pos_0x1C;        /* the placement position `eft052_place` clears (added by
                                        * `ef/eft052.cpp`) */
    /* +0x028 */ u32 field_0x28;       /* the two handles `eft052_place` clears beside it (added by
                                        * the same unit) */
    /* +0x02C */ u32 field_0x2C;
    /* +0x030 */ u8 pad_0x30[0x5];
    /* +0x035 */ u8 field_0x35;
    /* +0x036 */ u8 pad_0x36[0xD6];
    /* +0x10C */ struct EftModelSlot* field_0x10C;
    /* +0x110 */ u8 pad_0x110[0x8];
    /* +0x118 */ u32 field_0x118;
} EftModel;

/* The sound-request handle `se_req_pos_ps` takes (defined in `sound/fn_800D7F54.cpp`); only ever
 * reached through a pointer, so the incomplete type is enough.  Guarded so that a unit including both
 * `pl.h` and `enemy.h` sees the typedef once. */
#ifndef MHTRI_SE_W_DEFINED
#define MHTRI_SE_W_DEFINED
struct _se_w;
typedef struct _se_w _se_w;
#endif

/* nw4r::math and effect-library helpers, all still `fn_*` in the symbol map.
 * The two `src/mh3_pad.cpp` helpers declared here (`VEC3_ctor`, `setVec3`) are spelled EXACTLY as
 * `mh3_pad/vec3.h` spells them (the owner's header), with the record's real type `nw4r::math::VEC3`; two
 * spellings in one TU are MWCC `(10197) illegal function overloading`.  They stay here rather than in an
 * `#include "mh3_pad.h"`: that header carries other declarations its consumers spell differently, and this
 * header's declaration set is a codegen input (adding one declaration moved `ef/ef_disc.cpp`'s `fn_800CC5B0`). */
extern void VEC3_ctor(VEC3* out);                                   /* 0x80043EA8 - owner mh3_pad.cpp */
extern VEC3* setVec3(VEC3* out, f32 x, f32 y, f32 z);               /* 0x80041E8C - owner mh3_pad.cpp */
extern void assignVec3(Vec* out, Vec* in);                         /* out = in */
extern void fn_8009C6F0(Vec* out, f32 angle);                       /* sin/cos of angle */
extern void ef_vec_sin_cos(Vec* out, f32 angle);                    /* the same, under the owner's name */
extern void ef_sin_cos(f32* out_a, f32* out_b, f32 angle);         /* sin/cos of angle */
extern void fn_800A99B4(s32 ctx, Vec* out, EfWork* em, Vec* pos, Vec* a, Vec* b, Vec* c);
extern u16 fn_800A9FB0(s32 ctx, u16 id, f32 scale, EfWork* em);
extern f32 ef_random_float(u32* progress);                              /* pseudo-random 0..1 */
/* 0x80050BC0 is `src/fn_8004CAD8.cpp`'s, declared in `fn_8004CAD8.h`: it takes one float (its body reads only f1
 * and returns `x * FrSqrt(x)`); the second float the callers materialise is the hoisted `1.0f - t` their `else`
 * branch reuses (`ef_disc.cpp` 0x800CCA7C `fsubs f2, f30, f1`, 0x800CCA98 `fsubs f0, f30, f1`). */
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
        nw4r::db::Panic(file, __LINE__, msg, (ptr))

#ifdef __cplusplus
}
#endif

/* nw4r::ef::Effect - the pooled effect object.  It is a C++ class (the `change_color_eff` /
 * `setTevKColor` / `setMatColor` mangled names encode `Q34nw4r2ef6Effect`), so it can only be declared
 * from C++.  A C unit reaches it as `void*` through `_EFT_WORK::effect`.  The methods are the ones the ef units
 * call (`RetireEmitterAll`, `SetRootMtx`, `ForeachParticleManager`, ...); the size is stated at the closing brace. */
#ifdef __cplusplus
namespace nw4r {
namespace ef {
struct Effect {
    /* +0x00 */ u8 pad_0x00[0x44];
    /* +0x44 */ void* owner_0x44;   /* back-pointer to the spawning `_EFT` (ef/fn_800FD864_fx.cpp) */
    void SetRootMtx(const nw4r::math::MTX34& mtx); /* eft007/eft009 */
    u32 RetireEmitterAll();                         /* eft004/eft007; ef/ef_effectsystem.cpp adds it */
    /* Walks the effect's particle-manager pool, calling `cb` with each entry and its index; the
     * third argument is the per-walk flag (eft001's `fn_800FCEC8` is the only callback observed).
     * The owner `ef/ef_effect.cpp` returns the walked count (the target accumulates it and the
     * sweep units add it); the consumers call it as a statement, whose `.text` is unaffected. */
    u32 ForeachParticleManager(void (*cb)(void*, u32), u32 arg, bool flag); /* eft001/effect.cpp */
}; /* size: 0x48 (lower bound: +0x44 is the highest offset the spawn handler reads) */

/* The effect system the manager keeps at `eft_control` +0x04 (`fn_800D3C0C` builds it); the per-frame handler
 * and the resource manager both need it.  `RetireEffect` is a direct call returning `u32`, as its owner
 * `ef/ef_effectsystem.cpp` defines it; the consumers call it as a statement.  The two `virtual_0xN` model the
 * *memory manager* `fn_800A4420` returns (that object's table is [0x08, 0x0C]), which the eft004 per-frame handler
 * calls through, not this class' own layout: the system's first word is a data member, and its real layout is
 * `ef/ef_effectsystem.cpp`'s `EfSys` (size 0xC068, the map's size for lbl_806884D0) - two definitions to fold. */
class EffectSystem {
public:
    virtual void virtual_0x08();
    virtual void virtual_0x0C();
    u32 RetireEffect(Effect* effect);
};
/* size: 0x04 - lower bound, an approximation (an opaque handle here) */
}  // namespace ef
}  // namespace nw4r
#endif

#endif /* EF_H */
