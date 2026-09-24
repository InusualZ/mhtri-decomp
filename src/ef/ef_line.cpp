/* auto/800CCFB0_fn_800CCFB0.c - one function, .text 0x800CCFB0..0x800CD584.
 *
 * The unit is the NW4R effect library's `nw4r::ef` module.  The assert strings it references name the
 * file `ef_line.cpp` (0x80594EE0), the panic formatter is `nw4r::db::Panic`, and the sibling units pin
 * the seams `ef_line.cpp -> ef_point.cpp -> ef_sphere.cpp` - so the TU is `ef_line.cpp`, and the
 * function is `EmitterFormLine::Emission` (the NintendoWare 2009-04-03 source, whose three
 * `NW4R_POINTER_ASSERT` calls sit on lines 42/43/44 and whose `em`/`pm`/`params` names are exactly the
 * ones the panic format strings stringify).  It is the line emitter's per-sample generator: validate the
 * three pointers, walk `count` samples, build a scaled direction vector from three `SinCos` Euler angles,
 * project it through `CalcVelocity` and submit it through `ParticleManager::CreateParticle`.
 *
 * Flags: the auto lib (cflags_main, Wii/1.3, `-lang=c`) is the registered home.  Two file-local pragmas
 * are load-bearing: `#pragma fp_contract off` (the target's FP is unfused - `fmuls`+`fadds`, never
 * `fmadds` - while cflags_base passes `-fp_contract on`) and `#pragma peephole off` (the target keeps the
 * paired-single epilogue in its indexed `li r0,off; psq_lx` form, which the peephole pass folds into
 * `psq_l off(r1)`; the same stand-in the other auto unit uses, docs/plan.md 6.5).
 *
 * Result: 99.96 % (`.text` 0x5D4/0x5D4, `extab` and `extabindex` equal).  The residual is two
 * instructions, both C-vs-C++ front-end rather than source shape:
 *   - the `(f32)(s32)` conversions load MWCC's 2^52+2^31 magic double; the target references the shared
 *     pool label `lbl_807962F8`, ours emits the same constant under a private pool label (`@153`).
 *   - the `CreateParticle` dispatch: the target loads the slot through the first-argument register
 *     (`lwz r12, 0x1C(r3); lwz r12, 0x14(r12)`), ours through the saved parameter
 *     (`lwz r11, 0x1C(r23); lwz r12, 0x14(r11)`); the original's C++ front end ties the object and
 *     `this`, which a C indirect call does not.  Tried and rejected: a by-value `VEC3` parameter
 *     (93.5 %), a `static inline` forwarding helper, a local copy of `pm`, a duplicate-expression macro,
 *     and compiling the file as C++ (`-lang=c++`, 97.8 %).
 *
 * The callees are still `fn_*` in the map; the field names are the original source's.  The two explicit
 * `VEC3` copies (`velArg`/`posArg`) stand in for the by-value `p`/`v` arguments of `CreateParticle`; the
 * target copies `v` first, and the declaration order below puts their stack slots where the target has them.
 */

#include "types.h"
#include "nw4r/math.h"

#pragma fp_contract off
#pragma peephole off

/* `VEC3` comes from `nw4r/math.h` - one definition, in the owner's header (rule 1). */

/* nw4r::ef::Random - `RandFloat()` stays out of line under `-inline noauto`. */
typedef struct {
    /* +0x00 */ u32 mSeed;
} Random; /* size: 0x04 */

typedef struct Emitter Emitter;
typedef struct ParticleManager ParticleManager;

/* ParticleManager's vtable; slot +0x14 is `CreateParticle`. */
typedef struct {
    u8 pad_0x00[0x14]; /* +0x00 */
    /* +0x14 */ void (*createParticle)(ParticleManager* self, u16 life, VEC3* position, VEC3* velocity,
                                       u32 space, f32 momentum, void* setting, void* reference,
                                       u16 calcRemain);
} ParticleManagerVtbl; /* size: 0x18 */

struct ParticleManager {
    u8 pad_0x00[0x1C]; /* +0x00 */
    /* +0x1C */ ParticleManagerVtbl* vtable;
}; /* size: 0x20 */

/* The emitter fields this unit reads; the full Emitter is much larger. */
struct Emitter {
    u8 pad_0x00[0x67]; /* +0x00 */
    /* +0x67 */ s8 mVelMomentumRandom; /* EmitterParameter::mVelMomentumRandom */
    u8 pad_0x68[0xE8 - 0x68]; /* +0x68 */
    /* +0xE8 */ u16 mCalcRemain;
    u8 pad_0xEA[0xEC - 0xEA]; /* +0xEA */
    /* +0xEC */ Random mRandom;
    /* +0xF0 */ u32 unused_0xF0;
    u8 pad_0xF4[0xF8 - 0xF4]; /* +0xF4 */
    /* +0xF8 */ void* mpReferenceParticle;
    /* +0xFC */ u32 mInheritSetting;
}; /* size: 0x100 */

/* nw4r::db::Panic. The map already carries its real C++ mangling
 * (Panic__Q24nw4r2dbFPCciPCce), and declaring that spelling as a C++ identifier re-mangles it
 * (Panic__Q24nw4r2dbFPCciPCce__FPCciPCce) - which only shows up at LINK time, so a NonMatching
 * unit hides it until it is flipped. Declare the real thing and the front-end reproduces the
 * map's spelling exactly: tools/units/mangle.py confirms it. */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }
/* The nw4r helper callees are C functions: the target object's relocations carry their plain names
 * (`fn_80043EA8`, not `fn_80043EA8__FP...`), so they are declared `extern "C"`. */
extern "C" void fn_8009C760(f32* sin, f32* cos, f32 rad); /* PSSinCosRad */
extern "C" void fn_80043EA8(VEC3* v);                     /* VEC3::VEC3() */
extern "C" void fn_80041E8C(VEC3* v, f32 x, f32 y, f32 z); /* VEC3::VEC3(f32, f32, f32) */
extern "C" f32 fn_800A8A08(Random* r);                    /* Random::RandFloat */
extern "C" void fn_800A99B4(void* self, VEC3* result, Emitter* em, VEC3* position, VEC3* normalDir,
                        VEC3* fromOrigin, VEC3* fromYAxis); /* EmitterForm::CalcVelocity */
extern "C" u16 fn_800A9FB0(void* self, u16 aPtclLife, f32 aPtclLifeRnd, Emitter* em); /* CalcLife */

extern char lbl_80594EE0[];
extern char lbl_80594EEC[];
extern char lbl_80594F20[];
extern char lbl_80594F54[];
extern const f32 lbl_807962E8;
extern const f32 lbl_807962EC;
extern const f32 lbl_807962F0;
extern const f32 lbl_807962F4;

/* NW4R_POINTER_ASSERT's RVL address-range check (MEM1/MEM2, cached and uncached, plus locked cache). */
#define NW4R_VALID_PTR(p)                                                                          \
    (((u32)(p) & 0xFF000000) == 0x80000000 || ((u32)(p) & 0xFF800000) == 0x81000000 ||             \
     ((u32)(p) & 0xF8000000) == 0x90000000 || ((u32)(p) & 0xFF000000) == 0xC0000000 ||             \
     ((u32)(p) & 0xFF800000) == 0xC1000000 || ((u32)(p) & 0xF8000000) == 0xD0000000 ||             \
     ((u32)(p) & 0xFFFFC000) == 0xE0000000)

#define NW4R_POINTER_ASSERT(p, line, msg)                                                          \
    (NW4R_VALID_PTR(p) ? (void)0 : nw4r::db::Panic(lbl_80594EE0, line, msg, (p)))

/* EmitterFormLine::Emission */
void fn_800CCFB0(void* self, Emitter* em, ParticleManager* pm, int count, u32 optionFlag, f32* params,
                 u16 aPtclLife, f32 aPtclLifeRnd, u32 space) {
    NW4R_POINTER_ASSERT(em, 42, lbl_80594EEC);
    NW4R_POINTER_ASSERT(pm, 43, lbl_80594F20);
    NW4R_POINTER_ASSERT(params, 44, lbl_80594F54);

    if (count >= 1) {
        int i;
        for (i = 0; i < count; i++) {
            f32 pos;
            f32 sx, cx, sy, cy, sz, cz;
            VEC3 p, normal, fromYAxis, v, posArg, velArg;
            f32 momentum;
            u16 life;

            if ((optionFlag & 0x00020000) == 0) {
                pos = fn_800A8A08(&em->mRandom);
            } else if (count > 1) {
                pos = (f32)i / (f32)(count - 1);
            } else {
                pos = lbl_807962E8;
            }
            if ((optionFlag & 0x04000000) != 0)
                pos -= lbl_807962EC;
            pos *= params[0];

            fn_8009C760(&sx, &cx, params[1]);
            fn_8009C760(&sy, &cy, params[2]);
            fn_8009C760(&sz, &cz, params[3]);

            fn_80043EA8(&p);
            p.x = (cx * cz * sy + sx * sz) * pos;
            p.y = (-cz * sx + cx * sy * sz) * pos;
            p.z = (cx * cy) * pos;

            fn_80043EA8(&normal);
            normal.x = lbl_807962E8;
            normal.y = lbl_807962F0;
            normal.z = lbl_807962E8;

            fn_80041E8C(&fromYAxis, p.x, lbl_807962E8, p.z);

            fn_80043EA8(&v);
            fn_800A99B4(self, &v, em, &p, &normal, &p, &fromYAxis);

            velArg = v;
            posArg = p;

            momentum = lbl_807962F0 + lbl_807962F4 * (f32)(s32)em->mVelMomentumRandom *
                                           fn_800A8A08(&em->mRandom);
            life = fn_800A9FB0(self, aPtclLife, aPtclLifeRnd, em);

            pm->vtable->createParticle(pm, life, &posArg, &velArg, space, momentum, &em->mInheritSetting,
                                       em->mpReferenceParticle, em->mCalcRemain);
        }
    }
}
