/*
 * ef/ef_drawlinestrategy.cpp - nw4r::ef DrawLineStrategy: `Draw` (walks the draw-order particle list and emits one GX
 *   line per eligible particle, width `(u8)(6.0f * min(t, 42.5f))`), its per-draw setup, the GX helpers, the class's
 *   destructor and the DrawPointStrategy constructor.
 * RANGE. .text 0x800BEF98-0x800BF818 (10 functions); extab 0x8000A344-0x8000A36C, extabindex 0x8002388C-0x800238C8,
 *   .data 0x80594330-0x80594408 (the `__FILE__` string "ef_drawlinestrategy.cpp" first), .sdata
 *   0x80791348-0x80791350, .sdata2 0x807961A8-0x807961B8.
 *   Seam (playbook 80, measured): every strategy TU runs [constructor, ..., inline destructor], and each constructor
 *   installs the vtable at the end of its own TU's `.data`; so this class's constructor is the tail of `ef/ef_drawfreestrategy.cpp`'s range
 *   (0x800BEF5C) and the DrawPointStrategy constructor at this range's tail (0x800BF7DC) is the next TU's; both are
 *   defined where their range is until the seam is re-drawn.
 * FLAGS. `cflags_main`; `#pragma peephole off` before the includes, `#pragma dont_inline on` at the end (the inline
 *   destructor calls its base out of line, see there).
 * NAMES. GUESS: `ef_min_float` (0x800BF5B0) from its body; the assert names (`pm`, `pm->mResource`, `&ed`) are
 *   NintendoWare's.
 * RESIDUALS. 1 partial row: `fn_800BF58C` (ours 0x20 of 0x24): retail has a `b` to the next instruction between the
 *   three `lfs` and the three FIFO stores and loads into f1-f3 (ours f0-f2); no source shape tried (a block,
 *   `if (1)`, `do {} while (0)`, `for (;;)`, a `switch`, a local array, a forwarder) emits the branch.
 *   flipcheck: `.data` claimed, not emitted; `.sdata` (the width clamp) and `.sdata2` (the literals' pool) are emitted
 *   with their bytes equal, 0x4 short of the claim each (the trailing pad the next object's alignment adds).
 *   `DrawPointStrategy` (constructor): the empty body is the whole source - the compiler
 *     emits the base call and the vtable store.
 */

#pragma peephole off

#include "types.h"
#include "nw4r/math.h"
#include "gx.h"
#include "ef/fn_800AEE48.h"
#include "ef/ef_drawlinestrategy.h"
#include "ef/ef_drawpointstrategy.h" /* DrawPointStrategy, whose constructor closes this range */
#include "ef/ef_particle.h"
#include "ef/ef_particlemanager.h"
#include "ef/ef_drawstripestrategy.h" /* ef_draw_info_view_mtx (rule 2) */
#include "g3d/g3d_calcview.h" /* fn_800710BC (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r

#ifdef __cplusplus
extern "C" {
#endif


/* This unit's `__FILE__`/assert strings (its claimed `.data`), declared,
 * never defined. */
extern char lbl_80594330[];  /* "ef_drawlinestrategy.cpp"                                 .data */
extern char lbl_80594348[];  /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."        .data */
extern char lbl_8059437C[];  /* "NW4R:Pointer Error\npm->mResource(=%p) is not valid..."  .data */
extern char lbl_805943B8[];  /* "NW4R:Pointer Error\n&ed(=%p) is not valid pointer."      .data */
/* The widest line the width clamp allows (`.sdata`). */
static f32 ef_line_max_width = 42.5f;

/* The SDK GX entry points this unit calls; the target object's relocations carry the plain C names. */
extern void GXBegin(u32 primitive, u32 vtxfmt, u16 nverts);
extern void GXEnableTexOffsets(u8 coord, u8 lineOffset, u8 pointOffset);
extern void GXClearVtxDesc(void);
extern void GXSetVtxDesc(u32 attr, u32 type);
extern void GXSetVtxAttrFmt(u32 vtxfmt, u32 attr, u32 cnt, u32 type, u8 frac);
extern void GXSetCurrentMtx(u32 id);
extern void GXSetLineWidth(u8 width, u32 texOffsets);
extern void GXLoadPosMtxImm(void* mtx, u32 id);

/* The nw4r math/effect helpers retail references by their plain `fn_` names; the ef callees come from
 * the headers included above. */
extern MTX34* mtx34_get_ptr(MTX34* mtx);                      /* MTX34::Get() */
extern void fn_800513F0(VEC3* v, f32 scale);                /* VEC3::Scale / rotate helper */
extern void ef_particle_get_move_dir(void* particle, VEC3* out);         /* particle velocity/axis accessor */

/* `nw4r::math::PSVECSubtract` (the map's `PSVECSubtract`, a C symbol). */
extern void PSVECSubtract(const VEC3* a, const VEC3* b, VEC3* out);

/* forward declarations of this unit's own functions (the draw path calls the helpers below it) */
void fn_800BF4C8(const VEC3* a, const VEC3* b, u32 flag);
void fn_800BF564(void);
void fn_800BF568(f32 x, f32 y);
u32 fn_800BF578(u32 value);
void fn_800BF58C(const VEC3* v);
void fn_800BF5C8(nw4r::ef::DrawLineStrategy* self, const EfDrawInfo* info, EfDrawParticleManager* pm);

/* `NW4R_POINTER_ASSERT`'s RVL address-range check (MEM1/MEM2, cached and uncached, plus locked
 * cache), shared verbatim with ef/ef_line.cpp. */
#define NW4R_VALID_PTR(p)                                                                          \
    (((u32)(p) & 0xFF000000) == 0x80000000 || ((u32)(p) & 0xFF800000) == 0x81000000 ||             \
     ((u32)(p) & 0xF8000000) == 0x90000000 || ((u32)(p) & 0xFF000000) == 0xC0000000 ||             \
     ((u32)(p) & 0xFF800000) == 0xC1000000 || ((u32)(p) & 0xF8000000) == 0xD0000000 ||             \
     ((u32)(p) & 0xFFFFC000) == 0xE0000000)

#define NW4R_POINTER_ASSERT(p, line, msg)                                                          \
    (NW4R_VALID_PTR(p) ? (void)0 : nw4r::db::Panic(lbl_80594330, line, msg, (p)))

/* -------------------------------------------------------------------------------------------------
 * the functions, in address order
 * ------------------------------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

namespace nw4r {
namespace ef {

/* 0x800BEF98 (0x530): walks the draw-order particle list and emits one line per eligible particle. */
void DrawLineStrategy::Draw(const EfDrawInfo& info, EfDrawParticleManager* pm) {
    NW4R_POINTER_ASSERT(pm, 100, lbl_80594348);
    fn_800BF5C8(this, &info, pm);
    NW4R_POINTER_ASSERT(pm->resource, 104, lbl_8059437C);
    EfEmitterDrawSetting* ed = (EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource);
    NW4R_POINTER_ASSERT(ed, 106, lbl_805943B8);

    MTX34 mtxPm;
    MTX34 mtxEm;
    u32 screenSpace = (mNumTexmap != 0);

    MTX34_ctor(&mtxPm);
    ef_pm_get_mtx(pm, &mtxPm);
    MTX34_ctor(&mtxEm);
    ef_draw_info_view_mtx(&info, &mtxEm);
    mtx34_concat(&mtxEm, &mtxEm, &mtxPm);
    GXLoadPosMtxImm(mtx34_get_ptr(&mtxEm), 0);

    GetFirstDrawParticleFunc getFirst = GetGetFirstDrawParticleFunc(ed->flags & 0x800);
    GetNextDrawParticleFunc getNext = GetGetNextDrawParticleFunc(ed->flags & 0x800);

    u32 prevWidth = 0;
    bool first = true;
    EfDrawParticle* particle = getFirst(pm);
    f32 widthScale = 6.0f;

    for (; particle != NULL; particle = getNext(pm, particle)) {
        f32 t;
        f32 g;
        VEC3 pos;
        VEC3 dir;

        t = ef_particle_get_scale((struct EfParticle*)particle);
        if (t < ef_float_epsilon()) {
            continue;
        }
        g = ef_particle_get_scale_y((struct EfParticle*)particle);
        if (g < ef_float_epsilon()) {
            continue;
        }
        assignVec3((Vec*)&pos, (Vec*)&particle->world_pos);
        VEC3_ctor(&dir);
        ef_particle_get_move_dir(particle, &dir);
        if (ef_vec3_normalize(&dir) == 0) {
            continue;
        }
        fn_800513F0(&dir, g);
        PSVECSubtract(&dir, &pos, &dir);
        {
            u32 width = (u32)(s32)(widthScale * *ef_min_float(&ef_line_max_width, &t));
            if ((u8)width == 0) {
                continue;
            }
            if ((u8)prevWidth != (u8)width) {
                prevWidth = width;
                GXSetLineWidth((u8)width, 5);
                SetupGP(particle, *ed, info, first, true);
            } else {
                SetupGP(particle, *ed, info, first, false);
            }
        }
        first = false;
        fn_800BF4C8(&pos, &dir, screenSpace);
    }
}

}  // namespace ef
}  // namespace nw4r

#ifdef __cplusplus
extern "C" {
#endif

/* Draws one GX line between two positions, with the texture coords the target uses. */
void fn_800BF4C8(const VEC3* a, const VEC3* b, u32 flag) {
    GXBegin(0xA8, 0, 2);
    fn_800BF58C(a);
    if (fn_800BF578(flag)) {
        fn_800BF568(0.0f, 0.0f);
    }
    fn_800BF58C(b);
    if (fn_800BF578(flag)) {
        fn_800BF568(0.0f, 1.0f);
    }
    fn_800BF564();
}

/* The SDK's no-op `GXEnd`. */
void fn_800BF564(void) {}

/* Writes a pair of f32 to the GX FIFO (a texture coordinate). */
void fn_800BF568(f32 x, f32 y) {
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
}

/* Tests the low bit of a flag word. */
u32 fn_800BF578(u32 value) {
    return (value & 1) != 0;
}

/* Writes a position to the GX FIFO. */
void fn_800BF58C(const VEC3* v) {
    f32 x = v->x;
    f32 y = v->y;
    f32 z = v->z;
    GXWGFifo.f32 = x;
    GXWGFifo.f32 = y;
    GXWGFifo.f32 = z;
}

/* Returns a pointer to the smaller of the two floats. */
f32* ef_min_float(f32* a, f32* b) {
    return (*b < *a) ? b : a;
}

/* DrawLineStrategy per-draw setup: binds the resource and the line vertex format. */
void fn_800BF5C8(nw4r::ef::DrawLineStrategy* self, const EfDrawInfo* info, EfDrawParticleManager* pm) {
    NW4R_POINTER_ASSERT(pm, 174, lbl_80594348);
    self->InitGraphics(pm, *(EfEmitterDrawSetting*)ef_resource_draw_setting(pm->resource), *info);

    GXEnableTexOffsets(0, 1, 1);
    GXClearVtxDesc();
    GXSetVtxDesc(9, 1);
    if (self->mNumTexmap != 0) {
        GXSetVtxDesc(0xD, 1);
    }
    GXSetVtxAttrFmt(0, 9, 1, 4, 0);
    GXSetVtxAttrFmt(0, 0xD, 1, 4, 0);
    GXSetCurrentMtx(0);
}

#ifdef __cplusplus
}
#endif

namespace nw4r {
namespace ef {

/* 0x800BF7DC (0x3C): the DrawPointStrategy constructor (the next unit's class; see the unit header). */
DrawPointStrategy::DrawPointStrategy() {}

}  // namespace ef
}  // namespace nw4r

/* The destructors are the classes' inline ones, emitted at the end of this TU; retail calls each base destructor
 * out of line from them (`bl` + `extsh` of the delete flag), which a deferred inline body only gets with inlining
 * off at its point of emission. */
#pragma dont_inline on
