/*
 * ef/ef_particle.cpp - the nw4r effect particle record.
 *
 * The range this unit owns is `.text` 0x800AA18C..0x800AB658 (19 functions, 0x14CC B), reconstructed
 * from the split object `build/RMHE08/obj/auto/800AA18C_fn_800AA18C.o`.  The name is evidence class 1:
 * every `nw4r::db::Panic` call in the object passes the bare `__FILE__` string "ef_particle.cpp"
 * (`lbl_80592D50`, read from the DOL), so the module is `ef` and the extension `.cpp`.  The inline
 * asserts that come from `particle.h` pass that header's own name (`lbl_80592E78`/`lbl_80592EB4`/...),
 * which is what an nw4r assert macro defined in a header does.
 *
 * The class is the engine's particle record (`include/ef.h`): a dispatch-table pointer at +0x1C, the
 * emitter-parameter sub-record at +0x20, the position at +0xCC and the phase index at +0xDC.
 *
 * State (measured with `recompile.py`'s report path, target object
 * `build/RMHE08/obj/auto/800AA18C_fn_800AA18C.o`), 19 functions:
 *   100.00 %  fn_800AA18C, fn_800AA27C, fn_800AA2C0, fn_800AA2CC, fn_800AA700, fn_800AA75C,
 *             fn_800AA78C, fn_800AB2DC, fn_800AB37C, fn_800AB388, fn_800AB3D0, fn_800AB3D8
 *    94.54 %  fn_800AA1D8   - loop rotation: retail's two 3-element loops are `body; test; branch
 *             back`, this build emits a pre-test and a `b`.
 *    93.62 %  fn_800AB058   - address-computation schedule and one register in the assert chain.
 *    89.53 %  fn_800AB220   - same, plus the layer/index register swap.
 *    82.45 %  fn_800AB3FC   - the ramp is complete; the remaining diff is which register the
 *             parameter record and `mode` live in (`r31`/`r6` in retail).
 *    77.78 %  fn_800AB3AC   - RESIDUAL, below the bar: retail is 9 instructions, this is 7.  The
 *             retail object contains `mr r4,r3` (an argument setup whose call was inlined) and an
 *             unrelocated `b +4` (the vestigial call branch) that C source cannot reproduce; the
 *             arithmetic itself (`scale_0x10.x * scale_0x18.x * manager->scale_a`) is byte-for-byte.
 *     0.37 %  fn_800AA2D0   - STUB, not reconstructed (0x430 B).
 *     0.18 %  fn_800AA7A0   - STUB, not reconstructed (0x8B8 B).
 *
 * Pragma state: `#pragma peephole off` from the top through fn_800AA27C (fn_800AA18C's vtable store
 * and fn_800AA27C's `extsh` + `cmpwi` pair need it), `#pragma peephole on` from the colour getters
 * down (their assert materialisation needs it), `#pragma peephole off` again for fn_800AB3FC (retail
 * has no fused `clrlslwi` there), and file-scoped `#pragma fp_contract off`.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>` for all 21 symbols of the proposal - every one
 * resolves to `map=fn_XXXXXXXX`, and the shared dump has no name for the helpers either).
 */

#include "types.h"
#include "ef.h"
#include "sys_mem.h"
#include "unsplit/ef.h"
#include "unsplit/g3d.h"

/* The retail object keeps `0.01f * v * x` as a separate fmuls/fsubs (fn_800AA700); the command line's
 * -fp_contract on fuses them into fmsubs.  Nothing else in the unit emits an fma, so the pragma is
 * file-scoped and costs nothing (same lever as the neighbouring ef units). */
#pragma fp_contract off
#pragma peephole off

/* This unit's own pooled data (still another unit's range in splits.txt - declared, never defined). */
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

/* nw4r helpers, all still `fn_*` in the symbol map and unsplit (no owner file to move the declaration
 * to - the rule-2 gap the campaign records for an unsplit address). */
extern "C" void fn_800A4080(void* self); /* the particle's base constructor */
extern "C" f32 fn_800A8A04(f32 v);       /* the angle -> byte rounding helper */
extern "C" void* fn_800A4864(void* p);   /* walks to an object's chain head */
extern "C" void* fn_800A8C24(void* p);
extern f32 lbl_807960A0; /* 2pi  .sdata2 */

/* nw4r::db::Panic.  The map already carries its real C++ mangling, and declaring the C++ spelling is
 * what reproduces the map's symbol exactly (tools/units/mangle.py). */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }

/* Forward declarations: the unit's own functions, in address order, since a later one is called by an
 * earlier one (the object lays them out the same way). */
extern "C" EfParticleParams* fn_800AA1D8(EfParticleParams* self);
extern "C" f32 fn_800AB37C(EfParticleMgr* mgr, EfParticle* self, f32 v);
extern "C" u8* fn_800AB388(void* p);

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

    fn_800834F0(&self->scale_0x10);
    fn_800834F0(&self->scale_0x18);
    VEC3_ctor(&self->field_0x20);

    end = &self->field_0x2C[3];
    for (p = &self->field_0x2C[0]; p < end; p++) {
        fn_800834F0(p);
    }
    end = &self->field_0x50[3];
    for (p = &self->field_0x50[0]; p < end; p++) {
        fn_800834F0(p);
    }

    VEC3_ctor(&self->field_0x80);
    VEC3_ctor(&self->field_0x8C);
    VEC3_ctor(&self->field_0x98);
    return self;
}

/* Frees the record when the caller asks for it (positive flag); always returns the pointer so a
 * deleting expression can keep using it.
 *
 * This function and the two constructors above are built with `#pragma peephole off` (enabled just
 * below): with the peephole on, MWCC fuses the `extsh` + `cmpwi` pair into the record form `extsh.`,
 * which retail does not have, and rewrites fn_800AA18C's vtable store.  The two colour getters further
 * down need the peephole back ON - their assert materialisation depends on it. */
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
    return fn_800AB740(self->params.manager, self);
}

/* The empty slot. */
extern "C" void fn_800AA2CC(EfParticle* self)
{
}

/* Copies one colour entry out of the 2x2 table.  `layer` and `index` are both asserted into [0, 2). */
#pragma peephole on
extern "C" void fn_800AB058(EfParticle* self, u32 layer, u32 index, u8* color)
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
extern "C" u8 fn_800AB220(EfParticle* self, u32 layer, u32 index)
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
extern "C" f32 fn_800AB2DC(EfParticle* self)
{
    f32 v;
    s32 bits;

    bits = *(u16*)fn_800AB388(self->params.manager->context);
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
extern "C" u8* fn_800AB388(void* p)
{
    return (u8*)fn_800A4864(p) + 148;
}

/* The record's scale product for the current phase, with the owner's factor folded in. */
extern "C" f32 fn_800AB3AC(EfParticle* self)
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
extern "C" u8* fn_800AB3D8(void* p)
{
    return (u8*)fn_800A8C24(p) + 4;
}

/* Maps a radian angle into the particle's byte angle: radians -> half-turns -> [-0.5, 127.5] -> u8. */
extern "C" u8 fn_800AA700(f32 angle)
{
    return (u8)((s32)fn_800A8A04(angle / lbl_8079607C * lbl_80796078 - lbl_80796080) % 256);
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
 * Still open - the three large bodies.  Stubbed so the unit compiles and the rest can be measured.
 * ----------------------------------------------------------------------------------------------- */

extern "C" void fn_800AA2D0(EfParticle* self, void* ppd, void* p)
{
}

extern "C" void fn_800AA7A0(EfParticle* self, u32 a, u32 b, void* c)
{
}

#pragma peephole off
/* The particle's colour ramp: maps the record's phase into one of five byte ramps.  `mode` 0 means the
 * particle has no ramp (white); the ramp's period is the record's +0xDC, the amplitude and step come
 * from the parameter chain, and the ramp is clamped to a byte at the end. */
extern "C" u8 fn_800AB3FC(EfParticle* self)
{
    EfParticleChain* p;
    u32 n;
    u16 count;
    u16 period;
    u16 rem;
    s16 out;
    u8 f;
    s32 v;

    p = (EfParticleChain*)fn_800A4864(self->params.manager->context);
    if (p->mode == 0) {
        return 255;
    }

    f = p->amplitude;
    count = p->count;
    v = self->params.field_0x79;
    n = count + (p->count_step * count * v) / 12700;
    count = (u16)(n > 65535 ? 65535 : n);

    period = self->field_0xDC;
    rem = (period - 1) % count;

    out = 0;
    switch (p->mode) {
    case 1:
        if (rem * 2 > count) {
            out = (s16)((3 * f + (-4 * f * rem) / count) + 128);
        } else {
            out = (s16)((128 - f) + (f * (rem * 4)) / count);
        }
        break;
    case 2:
        out = (s16)((f + (-2 * f * rem) / count) + 128);
        break;
    case 3:
        out = (s16)((128 - f) + (f * (rem * 2)) / count);
        break;
    case 4:
        if (rem * 2 > count) {
            out = (s16)(128 - f);
        } else {
            out = (s16)(f + 128);
        }
        break;
    case 5:
        out = (s16)(128.0f + (f32)f * (f32)fn_800AB658(p, (lbl_807960A0 * (f32)rem) / (f32)count));
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
