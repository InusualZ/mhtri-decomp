/*
 * ef/ef_drawfreestrategy.cpp - nw4r::ef DrawFreeStrategy: the four-vertex quad emitter, the out-of-line GX FIFO
 *   writers, `Draw` (GX array state, then a camera-facing quad and an optional YZ quad per particle), the
 *   billboard and Euler/axis `MTX34` builders, the vertex-descriptor setup, a deleting destructor and the constructor
 *   that installs the DrawLineStrategy table `lbl_805943F0`.
 * RANGE. .text 0x800BE154-0x800BEF98 (11 functions); extab 0x8000A30C-0x8000A344, extabindex 0x80023838-0x8002388C,
 *   .rodata 0x8056F7D0-0x8056F830 (the XY and YZ quad corners), .data 0x805941F8-0x80594330 (the `__FILE__` string
 *   "ef_drawfreestrategy.cpp" first), .sdata 0x80791340-0x80791348, .sdata2 0x80796190-0x807961A8.
 *   Unproven seam (playbook 80: a TU's tables sit late in its `.data`): this unit's table `lbl_80594318` is installed
 *   by `fn_800BE118` at the tail of `ef/ef_drawbillboardstrategy.cpp`'s range, and this range ends with
 *   `fn_800BEF00`/`fn_800BEF5C`, the constructor installing `ef/ef_drawlinestrategy.cpp`'s table, so each `.text`
 *   seam may be off by that constructor.
 * FLAGS. `cflags_main`; `#pragma peephole off` (retail keeps the unfused `clrlwi`/`extsh`/`extsb` in front of every
 *   narrowing FIFO store and byte load).
 * NAMES. The map has only `fn_` stems for the range.
 * RESIDUALS. 4 partial rows:
 *  - `fn_800BE39C` (ours 0x20 of 0x24): retail has a `b` to the next instruction between the three loads and the
 *    `lis` of the FIFO base (`ef/ef_drawlinestrategy.cpp`'s `fn_800BF58C` is the same);
 *  - `fn_800BE3C0`: the three arguments are saved one register lower (r27-r29 against r28-r30) and one
 *    `lwz r6, 0x24(pm)` is scheduled earlier;
 *  - `fn_800BEA00`: the two products of the 0x24 component (`fmuls f6`, `fmuls f11`) are computed later and the
 *    float registers around them differ;
 *  - `fn_800BED2C`: the two pointer masks share one `clrrwi` (retail recomputes it), retail's dead `li r0,0;
 *    cmpwi r0,0` is missing, and `lbl_80791340` is reached with `lis`/`addi` where retail uses `li ...@sda21`.
 *   flipcheck: `.data`, `.rodata` and `.sdata` claimed, not emitted; `.text` 0xE34 of 0xE44.
 */

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "ef.h"
#include "ef/ef_drawstrategyimpl.h"
#include "ef/ef_particle.h"
#include "ef/ef_particlemanager.h"
#include "unsplit/ef.h"
#include "unsplit/g3d.h"
#include "g3d/fn_80075DCC.h" /* fn_80077DF0, owned by g3d/fn_80075DCC.cpp (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r

/* The runtime's scalar deleter.  `__dl__FPv` is its compiler mangling; the source calls the owner
 * (docs/plan.md 6.5 rule 9). */
void operator delete(void* ptr) throw();

#ifdef __cplusplus
extern "C" {
#endif

#pragma peephole off
#pragma fp_contract off

/* This unit's `.data`/`.rodata`/`.sdata`/`.sdata2` symbols (its claimed ranges), declared, never
 * defined. */
extern char lbl_805941F8[]; /* "ef_drawfreestrategy.cpp"                                 .data 0x805941F8 */
extern char lbl_80594210[]; /* "NW4R:Pointer Error\np(=%p) is not valid pointer."        .data 0x80594210 */
extern char lbl_80594240[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."       .data 0x80594240 */
extern char lbl_80594274[]; /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."  .data 0x80594274 */
extern char lbl_805942B0[]; /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."      .data 0x805942B0 */
extern char lbl_805942E4[]; /* "NW4R:Pointer Error\npp(=%p) is not valid pointer."       .data 0x805942E4 */
extern char lbl_805943F0[]; /* the DrawLineStrategy table (end of ef_drawlinestrategy's .data) .data 0x805943F0 */
extern Vec3 lbl_8056F7D0[]; /* quad corners in the XY plane (-1,-1,0)..(1,-1,0)         .rodata 0x8056F7D0 */
extern Vec3 lbl_8056F800[]; /* quad corners in the YZ plane (0,-1,1)..(0,-1,-1)         .rodata 0x8056F800 */
extern u8 lbl_80791340[];   /* the GXSetArray coordinate table                          .sdata 0x80791340 */
extern f32 lbl_80796190;    /* 100.0f                                                   .sdata2 0x80796190 */
extern f32 lbl_80796194;    /* FLT_EPSILON                                              .sdata2 0x80796194 */
extern f64 lbl_80796198;    /* the int->double conversion magic (2^52 + 2^31)           .sdata2 0x80796198 */
extern f32 lbl_807961A0;    /* 1.0f                                                     .sdata2 0x807961A0 */
extern f32 lbl_807961A4;    /* 0.0f                                                     .sdata2 0x807961A4 */

/* The GX entry points this unit calls (the SDK spells them, so they are `extern "C"`). */
extern void GXBegin(u32 prim, u32 vtxfmt, u32 nverts);
extern void GXLoadPosMtxImm(const MTX34* mtx, u32 id);
extern void GXEnableTexOffsets(u32 coord, u32 line, u32 point);
extern void GXSetArray(u32 attr, const void* base, u32 stride);
extern void GXClearVtxDesc(void);
extern void GXSetVtxDesc(u32 attr, u32 type);
extern void GXSetVtxAttrFmt(u32 vtxfmt, u32 attr, u32 cnt, u32 type, u32 frac);
extern void GXSetCurrentMtx(u32 id);

/* nw4r::math / effect-library helpers; the ef callees come from the headers included above. */
extern void fn_800514FC(Vec3* out, const MTX34* mtx, const Vec3* in); /* out = mtx * in */
extern MTX34* fn_80050508(MTX34* mtx);                               /* returns its argument (size 0x4) */

/* The effect-particle record this unit walks.  Only the fields the walk reads are named; the record
 * continues past what is touched here.  `EfDrawState` (below) is a *different* view: its +0xAD byte lies
 * where this record stores the low byte of the +0xAC float, so the two are kept apart.
 * size: 0xCC (lower bound) */
typedef struct EfDrawParticle {
    /* +0x00 */ u16 flags;
    /* +0x02 */ u8 pad_0x02[0x2E];
    /* +0x30 */ f32 scale_x;
    /* +0x34 */ u8 pad_0x34[0x04];
    /* +0x38 */ f32 scale_z;
    /* +0x3C */ u8 pad_0x3C[0x04];
    /* +0x40 */ Vec3 pos;
    /* +0x4C */ u8 pad_0x4C[0x4E];
    /* +0x9A */ u8 jitter_x;
    /* +0x9B */ u8 jitter_y;
    /* +0x9C */ u8 jitter_z;
    /* +0x9D */ u8 pad_0x9D[0x0F];
    /* +0xAC */ f32 offset_x;
    /* +0xB0 */ f32 offset_y;
    /* +0xB4 */ f32 offset_z;
    /* +0xB8 */ u8 pad_0xB8[0x10];
    /* +0xC8 */ void* scale_owner;
} EfDrawParticle; /* size: 0xCC - a lower bound (the record continues past what this unit reads) */

/* The draw-state record `ed` returned by fn_800AB388. size: 0xB0 - a lower bound */
typedef struct EfDrawState {
    /* +0x00 */ u16 flags;
    /* +0x02 */ u8 pad_0x02[0xA7];
    /* +0xA9 */ s8 scale_a;         /* per-100 fade/scale factor (signed) */
    /* +0xAA */ s8 scale_b;
    /* +0xAB */ u8 pad_0xAB[0x02];
    /* +0xAD */ u8 draw_yz_quad;    /* == 1: also emit the YZ-plane quad */
    /* +0xAE */ u8 pad_0xAE;
    /* +0xAF */ u8 drawer_mode;     /* the rotation-axis selector fn_800BEA00 switches on */
} EfDrawState; /* size: 0xB0 - a lower bound (the record continues past what this unit reads) */

/* The particle manager `pm` this unit receives; only its +0x24 state pointer is read. */
typedef struct EfParticleManager {
    /* +0x00 */ u16 flags;
    /* +0x02 */ u8 pad_0x02[0x22];
    /* +0x24 */ void* state;        /* the object fn_800AB388 turns into the EfDrawState */
} EfParticleManager; /* size: 0x28 - a lower bound (the record continues past what this unit reads) */

/* The particle-list walkers the strategy's vtable hands out. */
typedef void* (*EfFirstParticleFn)(void* mgr);
typedef void* (*EfNextParticleFn)(void* mgr, void* particle);
typedef EfFirstParticleFn (*EfSelectFirstFn)(void* self, u32 light_bit);
typedef EfNextParticleFn (*EfSelectNextFn)(void* self, u32 light_bit);

/* The DrawStrategy vtable, up to the two slots this unit dispatches.  The offset-to-top / typeinfo words
 * sit ahead of slot 0x00. */
typedef struct EfDrawStrategyVtbl {
    /* +0x00 */ void* slot_0x00;
    /* +0x04 */ void* slot_0x04;
    /* +0x08 */ void* slot_0x08;
    /* +0x0C */ void* slot_0x0C;
    /* +0x10 */ EfSelectFirstFn select_first;
    /* +0x14 */ EfSelectNextFn select_next;
} EfDrawStrategyVtbl; /* size: 0x18 */

typedef struct EfDrawStrategy {
    /* +0x00 */ EfDrawStrategyVtbl* vtbl;
    /* +0x04 */ u8 pad_0x04[0xCC];
    /* +0xD0 */ u8 field_0xD0;
} EfDrawStrategy; /* size: 0xD1 - a lower bound (the record continues past what this unit reads) */

/* --------------------------------------------------------------------------------------------- *
 * Forward declarations, so the definitions can sit in address order.
 * --------------------------------------------------------------------------------------------- */
void fn_800BE154(const MTX34* mtx, const Vec3* verts, u32 flag);
void fn_800BE374(void);
void fn_800BE378(u8 value);
int fn_800BE388(u32 value);
void fn_800BE39C(const Vec3* v);
void fn_800BE3C0(EfDrawStrategy* self, void* a2, EfParticleManager* pm);
void fn_800BE938(MTX34* dst, const MTX34* m, f32 s, f32 t, f32 u, f32 v);
void fn_800BEA00(MTX34* dst, const Vec3* pp, u8 mode);
void fn_800BED2C(EfDrawStrategy* self, void* a2, EfParticleManager* pm);
void* fn_800BEF00(void* self, s16 flag);

/* --------------------------------------------------------------------------------------------- *
 * 0x800BE154 - the four-vertex emitter.
 * --------------------------------------------------------------------------------------------- */

/* Transforms four input vectors through `mtx` and writes them as a GX_QUADS primitive; when `flag` is
 * set each corner is preceded by its 1-byte index.  The guard is the file's `__FILE__` assert. */
void fn_800BE154(const MTX34* mtx, const Vec3* verts, u32 flag) {
    Vec3 v0, v1, v2, v3;

    if (!IsValidPointer((u32)verts))
        nw4r::db::Panic(lbl_805941F8, 88, lbl_80594210, verts);

    VEC3_ctor(&v0);
    VEC3_ctor(&v1);
    VEC3_ctor(&v2);
    VEC3_ctor(&v3);
    fn_800514FC(&v0, mtx, &verts[0]);
    fn_800514FC(&v1, mtx, &verts[1]);
    fn_800514FC(&v2, mtx, &verts[2]);
    fn_800514FC(&v3, mtx, &verts[3]);

    GXBegin(0x80, 0, 4);
    fn_800BE39C(&v0);
    if (fn_800BE388(flag)) {
        fn_800BE378(0);
    }
    fn_800BE39C(&v1);
    if (fn_800BE388(flag)) {
        fn_800BE378(1);
    }
    fn_800BE39C(&v2);
    if (fn_800BE388(flag)) {
        fn_800BE378(2);
    }
    fn_800BE39C(&v3);
    if (fn_800BE388(flag)) {
        fn_800BE378(3);
    }
    fn_800BE374();
}

/* --------------------------------------------------------------------------------------------- *
 * 0x800BE374..0x800BE39C - the out-of-line GX FIFO writers and predicates.
 * --------------------------------------------------------------------------------------------- */

/* Ends the current FIFO command (the SDK's `GXEnd`, which writes nothing). */
void fn_800BE374(void) {}

/* Writes one u8 to the pipe. */
void fn_800BE378(u8 value) {
    GXWGFifo.u8 = value;
}

/* Tests the low bit of a status word. */
int fn_800BE388(u32 value) {
    return (value & 1) != 0;
}

/* Writes one vector's three components to the pipe. */
void fn_800BE39C(const Vec3* v) {
    f32 x = v->x;
    f32 y = v->y;
    f32 z = v->z;
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* --------------------------------------------------------------------------------------------- *
 * 0x800BE3C0 - DrawFreeStrategy::Draw.
 * --------------------------------------------------------------------------------------------- */

void fn_800BE3C0(EfDrawStrategy* self, void* a2, EfParticleManager* pm) {
    EfDrawState* ed;
    EfDrawParticle* p;
    EfFirstParticleFn first;
    EfNextParticleFn next;
    MTX34 mtx_view, mtx_mgr;
    u32 flag;
    u32 first_pass;
    f32 scale_a, scale_b, scale_x, scale_z;

    if (!IsValidPointer((u32)pm))
        nw4r::db::Panic(lbl_805941F8, 200, lbl_80594240, pm);

    fn_800BED2C(self, a2, pm);

    if (!IsValidPointer((u32)pm->state))
        nw4r::db::Panic(lbl_805941F8, 204, lbl_80594274, pm->state);

    ed = (EfDrawState*)fn_800AB388(pm->state);

    if (!IsValidPointer((u32)ed))
        nw4r::db::Panic(lbl_805941F8, 206, lbl_805942B0, ed);

    flag = (self->field_0xD0 != 0);

    MTX34_ctor(&mtx_view);
    fn_800AE360(pm, &mtx_view);
    MTX34_ctor(&mtx_mgr);
    fn_800B7DB0(a2, &mtx_mgr);
    fn_800710BC(&mtx_mgr, &mtx_mgr, &mtx_view);
    GXLoadPosMtxImm(fn_80050508(&mtx_mgr), 0);

    scale_a = (f32)ed->scale_a / 100.0f;
    scale_b = (f32)ed->scale_b / 100.0f;

    first = self->vtbl->select_first(self, ed->flags & 0x800);
    next = self->vtbl->select_next(self, ed->flags & 0x800);

    first_pass = 1;
    for (p = (EfDrawParticle*)first(pm); p != NULL; p = (EfDrawParticle*)next(pm, p)) {
        MTX34 mtx_rot;
        MTX34 mtx_final;

        scale_x = fn_800AB3AC((EfParticle*)p);
        if (scale_x < 1.1920929e-7f) {
            continue;
        }
        scale_z = fn_800AB2DC((EfParticle*)p);
        if (scale_z < 1.1920929e-7f) {
            continue;
        }
        fn_800C68E8(self, p, ed, a2, first_pass, 0);
        first_pass = 0;

        fn_800BEA00(&mtx_rot, &p->pos, ed->drawer_mode);
        fn_800BE938(&mtx_final, &mtx_rot, scale_a, scale_b, scale_x, scale_z);

        mtx_final.m[0][3] += p->offset_x;
        mtx_final.m[1][3] += p->offset_y;
        mtx_final.m[2][3] += p->offset_z;

        fn_800BE154(&mtx_final, lbl_8056F7D0, flag);
        if (ed->draw_yz_quad == 1) {
            fn_800BE154(&mtx_final, lbl_8056F800, flag);
        }
    }
}

/* --------------------------------------------------------------------------------------------- *
 * 0x800BE938 - the rotation/billboard matrix builder.
 * --------------------------------------------------------------------------------------------- */

/* Builds an `MTX34` through `fn_80077DF0` from the source rows scaled by (u,v,u), with an `s`/`t`
 * correction in the trailing column of rows 0 and 1. */
void fn_800BE938(MTX34* dst, const MTX34* m, f32 s, f32 t, f32 u, f32 v) {
    f32 us = u * s;
    f32 vt = v * t;

    fn_80077DF0(dst,
                m->m[0][0] * u,
                m->m[0][1] * v,
                m->m[0][2] * u,
                s + (-m->m[0][0] * us - m->m[0][1] * vt),
                m->m[1][0] * u,
                m->m[1][1] * v,
                m->m[1][2] * u,
                t + (-m->m[1][0] * us - m->m[1][1] * vt),
                m->m[2][0] * u,
                m->m[2][1] * v,
                m->m[2][2] * u,
                -m->m[2][0] * us - m->m[2][1] * vt);
}

/* --------------------------------------------------------------------------------------------- *
 * 0x800BEA00 - the Euler/axis rotation builder.
 * --------------------------------------------------------------------------------------------- */

/* Builds an `MTX34` rotation from the particle's angle vector `pp`: mode 0 rotates about X, 1 about Y,
 * 2 about Z and any other value composes the full Euler rotation.  `fn_8009C760` is the sin/cos pair. */
void fn_800BEA00(MTX34* dst, const Vec3* pp, u8 mode) {
    Vec3 v;
    f32 sx, cx, sy, cy, sz, cz;

    if (!IsValidPointer((u32)pp))
        nw4r::db::Panic(lbl_805941F8, 136, lbl_805942E4, pp);

    VEC3_ctor(&v);
    fn_800B54B4((const void*)pp, &v);

    switch (mode) {
    case 0:
        fn_8009C760(&sx, &cx, v.x);
        fn_80077DF0(dst, 1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, cx, -sx, 0.0f,
                    0.0f, sx, cx, 0.0f);
        break;
    case 1:
        fn_8009C760(&sy, &cy, v.y);
        fn_80077DF0(dst, cy, 0.0f, sy, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    -sy, 0.0f, cy, 0.0f);
        break;
    case 2:
        fn_8009C760(&sz, &cz, v.z);
        fn_80077DF0(dst, cz, -sz, 0.0f, 0.0f,
                    sz, cz, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f);
        break;
    default:
        fn_8009C760(&sx, &cx, v.x);
        fn_8009C760(&sy, &cy, v.y);
        fn_8009C760(&sz, &cz, v.z);
        fn_80077DF0(dst,
                    cy * cz, sx * sy * cz - cx * sz, cx * sy * cz + sx * sz, 0.0f,
                    cy * sz, sx * sy * sz + cx * cz, cx * sy * sz - sx * cz, 0.0f,
                    -sy, cy * sx, cx * cy, 0.0f);
        break;
    }
}

/* --------------------------------------------------------------------------------------------- *
 * 0x800BED2C - the GX vertex-descriptor setup.
 * --------------------------------------------------------------------------------------------- */

/* Sets the free draw's vertex format: the position array, the indexed texcoord array when the state's
 * +0xD0 byte is set, the two attribute formats and the current matrix. */
void fn_800BED2C(EfDrawStrategy* self, void* a2, EfParticleManager* pm) {
    void* ed;

    if (!IsValidPointer((u32)pm))
        nw4r::db::Panic(lbl_805941F8, 278, lbl_80594240, pm);

    ed = fn_800AB388(pm->state);
    fn_800C6064((EfDrawStrategyImpl*)self, (u32)pm, (u16*)ed, (void*)a2);

    GXEnableTexOffsets(0, 1, 1);
    GXSetArray(0x0D, lbl_80791340, 2);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    if (self->field_0xD0) {
        GXSetVtxDesc(0x0D, 2);
    }
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 0x0D, 1, 0, 0);
    GXSetCurrentMtx(0);
}

/* --------------------------------------------------------------------------------------------- *
 * 0x800BEF00 - a scalar deleting destructor, and 0x800BEF5C - the DrawLineStrategy constructor.
 * --------------------------------------------------------------------------------------------- */

void* fn_800BEF00(void* self, s16 flag) {
    if (self != NULL) {
        fn_800B4B04(self, 0);
        if ((s16)flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

void* fn_800BEF5C(void* self) {
    fn_800C5F74((EfParticleLayers*)self);
    ((EfDrawStrategy*)self)->vtbl = (EfDrawStrategyVtbl*)lbl_805943F0;
    return self;
}

#ifdef __cplusplus
}
#endif
