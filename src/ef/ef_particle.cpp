/*
 * ef/ef_particle.cpp - the nw4r effect particle record (`EfParticle`, `ef.h`: a table pointer at +0x1C, the
 *   emitter-parameter sub-record at +0x20, the position at +0xCC, the phase index at +0xDC): its constructors and
 *   deleting destructor, the phase scale getters, the colour getters and the colour ramp.
 * RANGE. .text 0x800AA18C-0x800AB658 (19 functions); extab 0x80009E04-0x80009E6C, extabindex 0x800230AC-0x80023148,
 *   .data 0x80592D50-0x80592F78 (the `__FILE__` string "ef_particle.cpp" first; the inline asserts of
 *   `particle.h` pass that header's name), .sdata2 0x80796060-0x807960A0.
 * FLAGS. `cflags_main`; file-wide `#pragma fp_contract off` (retail keeps `0.01f * v * x` as `fmuls`/`fsubs` in
 *   `fn_800AA700`); `#pragma peephole off` through `fn_800AA27C` (the table store and the `extsh` + `cmpwi` pair),
 *   on for the colour getters (their assert materialisation), off again for `ef_particle_flick_alpha` (no fused `clrlslwi`).
 * NAMES. The map has only `fn_` stems for the range.
 *   GUESS (from the body and its callers): `ef_particle_get_color`, `ef_particle_get_alpha`,
 *   GUESS: `ef_particle_get_scale_y`, `ef_resource_draw_setting`, `ef_particle_get_scale`,
 *   GUESS: `ef_particle_flick_alpha`.
 *   GUESS (the particle parameters `ef/ef_resource.cpp` binds textures into): `ef_emres_get_ptcl_param`.
 * RESIDUALS. 2 rows unwritten (empty bodies): 0x800AA2D0-0x800AA700, 0x800AA7A0-0x800AB058.  They are defined
 *   last, so our `.text` (and the extab and extabindex records) run in a different order from retail's.
 *   5 partial rows:
 *  - `fn_800AA1D8`: retail's two 3-element loops are `body; test; branch back`, ours add a pre-test `b`, and the
 *    two element pointers swap r30/r31;
 *  - `ef_particle_get_color`, `ef_particle_get_alpha`: the two phase tests compile branchless (`xori`/`cntlzw`/`slw`) where retail
 *    compares `cmplwi ...,1; bgt`, and the address computation is scheduled differently; `ef_particle_get_alpha` also passes
 *    `ef_particle_get_color`'s copies of the `particle.h` assert strings (`lbl_80592E84`/`EB4`/`EC0`/`EF0`) where retail
 *    passes its own (`lbl_80592EFC`/`F2C`/`F38`/`F68`);
 *  - `ef_particle_get_scale` (ours 0x1C of 0x24): retail keeps an inlined call's `mr r4,r3` and its vestigial `b` to the
 *    next instruction; the arithmetic is retail's;
 *  - `ef_particle_flick_alpha`: the `+0x108` step is loaded after the `+0x99` byte where retail loads it first, the
 *    clamped count is narrowed into its own register, and `3 * amplitude` is computed after the division.
 *   flipcheck: `.data` claimed, not emitted; `.sdata2` 0x10 of 0x40 (a partial pool: flipcheck names a fold with
 *   `ef/ef_particlemanager.cpp`, one shared literal); `.text` 0x7D0 of 0x14CC; extab 0x58 of 0x68; extabindex
 *   0x84 of 0x9C.
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `lbl_80592EF0`,
 *     `lbl_80592F68`, `lbl_80592EC0`, `lbl_80592F38`, `lbl_80592EB4`, `lbl_80592F2C`, `lbl_80592E84`,
 *     `lbl_80592EFC`.
 */

#include "types.h"
#include "ef.h"
#include "sys_mem.h"
#include "unsplit/ef.h"
#include "ef/ef_emitter.h" /* ef_truncate_float/ef_res_block_body (rule 2) */
#include "ef/ef_particlemanager.h" /* fn_800AB740 / fn_800AB658 (rule 2) */
#include "g3d/g3d_scnroot.h" /* VEC2_ctor (rule 2) */

#pragma fp_contract off
#pragma peephole off

/* This unit's own strings (its claimed `.data`), declared, never defined. */
extern char lbl_80592D50[]; /* "ef_particle.cpp"                                      .data */
extern char lbl_80592D60[]; /* "NW4R:Pointer Error\nppd(=%p) is not valid pointer."    .data */
extern char lbl_80592E44[]; /* "NW4R:Pointer Error\ncolor(=%p) is not valid pointer."  .data */
extern char lbl_80592E78[]; /* "particle.h"                                            .data */
extern char lbl_80592E84[]; /* "NW4R:Failed assertion index >= 0 && index < 2"         .data */
extern char lbl_80592EB4[]; /* "particle.h"                                            .data */
extern char lbl_80592EC0[]; /* "NW4R:Failed assertion layer >= 0 && layer < 2"         .data */
extern char lbl_80592EF0[]; /* "particle.h"                                            .data */
extern char lbl_80592E14[]; /* "NW4R:Failed assertion false"                           .data */

/* The class dispatch table (data another unit owns; the constructor stores its address at +0x1C). */
extern char lbl_80592E30[];

/* This unit's own .sdata2 pool (unclaimed; the split owns no data section). */
extern f32 lbl_8079607C; /* pi      .sdata2 */
extern f32 lbl_80796078; /* 128.0f  .sdata2 */
extern f32 lbl_80796080; /* 0.5f    .sdata2 */

/* nw4r helpers, declared locally with C linkage. */
extern "C" void fn_800A4080(void* self); /* the particle's base constructor */
extern "C" void* ef_res_emitter_desc(void* p);   /* walks to an object's chain head */
extern f32 lbl_807960A0; /* 2pi  .sdata2 */

/* nw4r::db::Panic.  The map already carries its real C++ mangling, and declaring the C++ spelling is
 * what reproduces the map's symbol exactly (tools/units/mangle.py). */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }

/* Forward declarations: the unit's own functions, in address order, since a later one is called by an
 * earlier one (the object lays them out the same way). */
extern "C" EfParticleParams* fn_800AA1D8(EfParticleParams* self);
extern "C" f32 fn_800AB37C(EfParticleMgr* mgr, EfParticle* self, f32 v);
extern "C" u8* ef_resource_draw_setting(void* p);

/* Constructor.  Installs the dispatch table, builds the parameter sub-record and clears the position. */
extern "C" EfParticle* fn_800AA18C(EfParticle* self)
{
    fn_800A4080(self);
    self->slots = (EfParticleSlots*)lbl_80592E30;
    fn_800AA1D8(&self->params);
    VEC3_ctor(&self->field_0xCC);
    return self;
}

/* Builds the emitter-parameter sub-record: two single sub-objects, one vector, two arrays of three
 * sub-objects and three vectors. */
extern "C" EfParticleParams* fn_800AA1D8(EfParticleParams* self)
{
    EfParticleNode* p;
    EfParticleNode* end;

    VEC2_ctor(&self->scale_0x10);
    VEC2_ctor(&self->scale_0x18);
    VEC3_ctor(&self->field_0x20);

    end = &self->field_0x2C[3];
    for (p = &self->field_0x2C[0]; p < end; p++) {
        VEC2_ctor(p);
    }
    end = &self->field_0x50[3];
    for (p = &self->field_0x50[0]; p < end; p++) {
        VEC2_ctor(p);
    }

    VEC3_ctor(&self->field_0x80);
    VEC3_ctor(&self->field_0x8C);
    VEC3_ctor(&self->field_0x98);
    return self;
}

/* Frees the record when the caller asks for it (positive flag); always returns the pointer so a
 * deleting expression can keep using it. */
extern "C" void* fn_800AA27C(void* p, s32 flag)
{
    if (p != 0) {
        if ((s16)flag > 0) {
            operator delete(p);
        }
    }
    return p;
}

/* The destructor slot: hands the record to its owner for the actual teardown. */
extern "C" void* fn_800AA2C0(EfParticle* self)
{
    return (void*)fn_800AB740((struct EfPmManager*)self->params.manager, (struct EfPmParticle*)self);
}

/* The empty slot. */
extern "C" void fn_800AA2CC(EfParticle* self)
{
}

/* Copies one colour entry out of the 2x2 table.  `layer` and `index` are both asserted into [0, 2). */
#pragma peephole on
extern "C" void ef_particle_get_color(EfParticle* self, u32 layer, u32 index, u8* color)
{
    u8* entry;
    int ok;

    ok = (layer < 2);
    if (!ok) {
        nw4r::db::Panic(lbl_80592EF0, 329, lbl_80592EC0);
    }
    ok = (index < 2);
    if (!ok) {
        nw4r::db::Panic(lbl_80592EB4, 330, lbl_80592E84);
    }
    ok = IsValidPointer((u32)color);
    if (!ok) {
        nw4r::db::Panic(lbl_80592E78, 331, lbl_80592E44, color);
    }

    entry = &self->params.colors[layer][index][0];
    color[0] = entry[0];
    color[1] = entry[1];
    color[2] = entry[2];
}

/* Returns one colour entry's alpha byte. */
extern "C" u8 ef_particle_get_alpha(EfParticle* self, u32 layer, u32 index)
{
    int ok;

    ok = (layer < 2);
    if (!ok) {
        nw4r::db::Panic(lbl_80592EF0, 340, lbl_80592EC0);
    }
    ok = (index < 2);
    if (!ok) {
        nw4r::db::Panic(lbl_80592EB4, 341, lbl_80592E84);
    }
    return self->params.colors[layer][index][3];
}

/* The record's scale for the current emitter phase: the phase bits select which pair of the four
 * scale factors is multiplied, and the owner's own factor scales the result again. */
extern "C" f32 ef_particle_get_scale_y(EfParticle* self)
{
    f32 v;
    s32 bits;

    bits = *(u16*)ef_resource_draw_setting(self->params.manager->context);
    bits &= 0x6000;

    switch (bits) {
    case 0x4000:
        v = self->params.scale_0x10.y * self->params.scale_0x18.x;
        break;
    case 0x2000:
        v = self->params.scale_0x10.x * self->params.scale_0x18.y;
        break;
    case 0x6000:
        v = self->params.scale_0x10.x * self->params.scale_0x18.x;
        break;
    default:
        v = self->params.scale_0x10.y * self->params.scale_0x18.y;
        break;
    }
    return fn_800AB37C(self->params.manager, self, v);
}

/* The owner's factor the scale is finally multiplied by. */
extern "C" f32 fn_800AB37C(EfParticleMgr* mgr, EfParticle* self, f32 v)
{
    return v * mgr->scale_b;
}

/* The owner's colour block: the chain head plus its colour-table offset. */
extern "C" u8* ef_resource_draw_setting(void* p)
{
    return (u8*)ef_res_emitter_desc(p) + 148;
}

/* The record's scale product for the current phase, with the owner's factor folded in. */
extern "C" f32 ef_particle_get_scale(EfParticle* self)
{
    f32 v;

    v = self->params.scale_0x10.x * self->params.scale_0x18.x;
    return self->params.manager->GetScaleA(self, v);
}

/* The parameter sub-record's colour block base. */
extern "C" u8* fn_800AB3D0(EfParticle* self)
{
    return &self->params.field_0x68[0];
}

/* The owner's colour block, relocated to its table. */
extern "C" u8* ef_emres_get_ptcl_param(void* p)
{
    return (u8*)ef_res_block_body(p) + 4;
}

/* Maps a radian angle into the particle's byte angle: radians -> half-turns -> [-0.5, 127.5] -> u8. */
extern "C" u8 fn_800AA700(f32 angle)
{
    return (u8)((s32)ef_truncate_float(angle / lbl_8079607C * lbl_80796078 - lbl_80796080) % 256);
}

/* Two-float copy used by the vector helpers below. */
extern "C" void fn_800AA78C(f32* dst, f32* src)
{
    dst[0] = src[0];
    dst[1] = src[1];
}

extern "C" f32* fn_800AA75C(f32* dst, f32* src)
{
    fn_800AA78C(dst, src);
    return dst;
}

/* -------------------------------------------------------------------------------------------------
 * The two unwritten bodies (empty).
 * ----------------------------------------------------------------------------------------------- */

extern "C" void fn_800AA2D0(EfParticle* self, void* ppd, void* p)
{
}

extern "C" void fn_800AA7A0(EfParticle* self, u32 a, u32 b, void* c)
{
}

#pragma peephole off
/* Maps the record's phase into one of five byte colour ramps (mode 0: white), period +0xDC, amplitude and
 * step from the parameter chain, clamped to a byte. */
extern "C" u8 ef_particle_flick_alpha(EfParticle* self)
{
    EfParticleChain* p;
    u32 n;
    u16 count;
    u16 rem;
    s16 out;
    u8 mode;

    p = (EfParticleChain*)ef_res_emitter_desc(self->params.manager->context);
    mode = p->mode;
    if (mode == 0) {
        return 255;
    }

    count = p->count;
    n = count + (p->count_step * count * self->params.field_0x79) / 12700;
    count = (u16)(n > 65535 ? 65535 : n);
    rem = (self->field_0xDC - 1) % count;

    out = 0;
    switch (mode) {
    case 1:
        if (rem * 2 <= count) {
            out = (s16)((128 - p->amplitude) + (p->amplitude * (rem * 4)) / count);
        } else {
            out = (s16)((p->amplitude * 4 - p->amplitude) + -(p->amplitude * rem * 4) / count + 128);
        }
        break;
    case 2:
        out = (s16)(p->amplitude + -(p->amplitude * rem * 2) / count + 128);
        break;
    case 3:
        out = (s16)((128 - p->amplitude) + (p->amplitude * (rem * 2)) / count);
        break;
    case 4:
        if (rem * 2 <= count) {
            out = (s16)(p->amplitude + 128);
        } else {
            out = (s16)(128 - p->amplitude);
        }
        break;
    case 5:
        out = (s16)(128.0f + (f32)p->amplitude * fn_800AB658((lbl_807960A0 * (f32)rem) / (f32)count));
        break;
    default:
        nw4r::db::Panic(lbl_80592D50, 353, lbl_80592E14);
        break;
    }

    if (out < 0) {
        out = 0;
    }
    if (out > 255) {
        out = 255;
    }
    return (u8)out;
}
