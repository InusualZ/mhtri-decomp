/*
 * ef/ef_line.cpp - the line emitter form's per-sample generator: validates `em`/`pm`/`params`, walks `count`
 *   samples, builds a scaled direction from three `SinCos` Euler angles, projects it through `CalcVelocity` and
 *   submits it through the particle manager's `CreateParticle` slot.
 * RANGE. .text 0x800CCFB0-0x800CD584 (1 function); extab 0x8000A5CC-0x8000A5D4, extabindex 0x80023C58-0x80023C64,
 *   .data 0x80594EE0-0x80594F98 (the `__FILE__` string "ef_line.cpp" first), .sdata2 0x807962E8-0x80796300.
 * FLAGS. `cflags_main`; file-wide `#pragma fp_contract off` (retail's FP is unfused `fmuls` + `fadds`) and
 *   `#pragma peephole off` (retail keeps the paired-single epilogue as `li r0,off; psq_lx`).
 * NAMES. The function is a GUESS for NintendoWare's `EmitterFormLine::Emission` (its three pointer asserts on
 *   lines 42-44 and the `em`/`pm`/`params` names the panic strings stringify); the map row keeps its `fn_` stem.
 * RESIDUALS. `.text` is byte-identical.  The `.data` run is the three panic strings, the file name and, at
 *   0x80594F8C, the 12-byte table {0, 0, `fn_800CCFB0`} of the `EmitterFormLine` class whose one virtual this function
 *   is; it is emitted only once the emitter-form classes are reconstructed together with `ef/ef_emform.cpp`'s
 *   constructors (which install the table), so the strings stay declared.
 *  - the `(f32)(s32)` conversion constant is our pool's `@N`, retail's the claimed `lbl_807962F8`.
 *   flipcheck: `.data` claimed, not emitted; `.sdata2` 0x8 of the claimed 0x18.
 * SHAPES. The particle manager is a class whose vtable pointer follows a 0x1C-byte non-polymorphic head; the spawn
 *   is its virtual `CreateParticle`, taking the position and velocity by value: the call copies them (velocity
 *   first), evaluates the momentum before the life and dispatches through r3, as in retail.
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "ef/ef_emitter.h" /* ef_random_float (rule 2) */
#include "ef/ef_particlemanager.h" /* ParticleManager (rule 1) */

#pragma fp_contract off
#pragma peephole off

/* nw4r::ef::Random - `RandFloat()` stays out of line under `-inline noauto`. */
typedef struct {
    /* +0x00 */ u32 mSeed;
} Random; /* size: 0x04 */

typedef struct Emitter Emitter;


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

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }
/* The nw4r helper callees are C functions: the target object's relocations carry their plain names
 * (`VEC3_ctor`, not `fn_80043EA8__FP...`), so they are declared `extern "C"`. */
extern "C" void ef_sin_cos(f32* sin, f32* cos, f32 rad); /* PSSinCosRad */
extern "C" void ef_form_calc_velocity(void* self, VEC3* result, Emitter* em, VEC3* position, VEC3* normalDir,
                        VEC3* fromOrigin, VEC3* fromYAxis); /* EmitterForm::CalcVelocity */
extern "C" u16 ef_form_calc_life(void* self, u16 aPtclLife, f32 aPtclLifeRnd, Emitter* em); /* CalcLife */

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
            VEC3 p, normal, fromYAxis, v;

            if ((optionFlag & 0x00020000) == 0) {
                pos = ef_random_float(&em->mRandom.mSeed);
            } else if (count > 1) {
                pos = (f32)i / (f32)(count - 1);
            } else {
                pos = lbl_807962E8;
            }
            if ((optionFlag & 0x04000000) != 0)
                pos -= lbl_807962EC;
            pos *= params[0];

            ef_sin_cos(&sx, &cx, params[1]);
            ef_sin_cos(&sy, &cy, params[2]);
            ef_sin_cos(&sz, &cz, params[3]);

            VEC3_ctor(&p);
            p.x = (cx * cz * sy + sx * sz) * pos;
            p.y = (-cz * sx + cx * sy * sz) * pos;
            p.z = (cx * cy) * pos;

            VEC3_ctor(&normal);
            normal.x = lbl_807962E8;
            normal.y = lbl_807962F0;
            normal.z = lbl_807962E8;

            setVec3(&fromYAxis, p.x, lbl_807962E8, p.z);

            VEC3_ctor(&v);
            ef_form_calc_velocity(self, &v, em, &p, &normal, &p, &fromYAxis);

            pm->CreateParticle(ef_form_calc_life(self, aPtclLife, aPtclLifeRnd, em), p, v, space,
                               lbl_807962F0 + lbl_807962F4 * (f32)(s32)em->mVelMomentumRandom *
                                                  ef_random_float(&em->mRandom.mSeed),
                               (u8*)&em->mInheritSetting, (u32)em->mpReferenceParticle,
                               em->mCalcRemain);
        }
    }
}
