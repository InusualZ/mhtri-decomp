/* auto/800CCFB0_fn_800CCFB0.c - placeholder attribution, 1 function(s), 0x800CCFB0..0x800CD584.
 *
<<<<<<< HEAD
 * No source has been recovered for this unit yet: the range was claimed in bulk from the DOL's own
 * layout (docs/plan.md 12 item 5, the attribution pass), and the bodies are still the original bytes
 * (`Object(NonMatching, …)`, so the link keeps them and `ninja build/RMHE08/ok` cannot move).
=======
 * The unit is the NW4R effect library's `nw4r::ef` module.  The assert strings it references name the
 * file `ef_line.cpp` (0x80594EE0), the panic formatter is `nw4r::db::Panic`, and the sibling units pin
 * the seams `ef_line.cpp -> ef_point.cpp -> ef_sphere.cpp` - so the TU is `ef_line.cpp`, and the
 * function is `EmitterFormLine::Emission` (the NintendoWare 2009-04-03 source, whose three
 * `NW4R_POINTER_ASSERT` calls sit on lines 42/43/44 and whose `em`/`pm`/`params` names are exactly the
 * ones the panic format strings stringify).  It is the line emitter's per-sample generator: validate the
 * three pointers, walk `count` samples, build a scaled direction vector from three `SinCos` Euler angles,
 * project it through `CalcVelocity` and submit it through `ParticleManager::CreateParticle`.
>>>>>>> 59a4ae6b (auto: use nw4r ef_line's original names for fn_800CCFB0)
 *
 * What the seam rests on (see the `tu-boundary-discovery` skill for the method):
 *   pinned seam (source): source file change ef_line.cpp -> ef_point.cpp
 *   pinned seam (source): source file change ef_point.cpp -> ef_sphere.cpp
 *   pinned seam (pool): .sdata2 run jump lbl_807962F8 -> lbl_80796300
 *   pinned seam (pool): .sdata2 run jump lbl_80796320 -> lbl_80796328
 *
<<<<<<< HEAD
 * Data runs in this range. They are recorded in `splits.txt` as comments and **not** claimed: a stub
 * object emits nothing, and a range our object does not emit must not be claimed (playbook 23,
 * docs/plan.md 8.4). The claim belongs to the measured data pass - `tools/units/dataclaim.py`,
 * docs/plan.md 7.8 / 12 item 7 - once the source emits the bytes:
 *   extabindex   0x80023C58..0x80023C64   1 labels  proposed    (dataclaim: no queue run)
 *   .data        0x80594EE0..0x80594F89   4 labels  proposed    (dataclaim: unowned)
 *   .sdata2      0x807962E8..0x80796300   5 labels  proposed    (dataclaim: unowned)
 *
 *   The `extab`/`extabindex` fragment sections are the exception: they travel with the code unit
 *   (`dataqueue.py`'s `FRAGMENT_SECTIONS`) and are claimed in `splits.txt` with its `.text`;
 *   `.ctors`/`.dtors` are added after the split. Everything else above waits for the measured pass.
 *
 * Evidence for this batch: `python tools/units/attribute.py plan 0x80070000 0x801e0000
 * --max-total-bytes 0x80000` (this unit's plan line), `.pi/attribution-batch-2.patch.md` (the exact
 * splits/configure edits) and `.pi/notes/attribution-batch-2.md` (the chosen/dropped table).
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit auto/800CCFB0_fn_800CCFB0.c`.
 * The name is provisional - `auto/` plus the first symbol's address - because nothing in the object
 * names the original source file. Rename it the moment there is evidence.
 */
=======
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

#pragma fp_contract off
#pragma peephole off

typedef struct {
    f32 x; /* +0x00 */
    f32 y; /* +0x04 */
    f32 z; /* +0x08 */
} VEC3; /* size: 0x0C */

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

extern void Panic__Q24nw4r2dbFPCciPCce(const char* file, int line, const char* fmt, ...);
extern void fn_8009C760(f32* sin, f32* cos, f32 rad);      /* PSSinCosRad */
extern void fn_80043EA8(VEC3* v);                          /* VEC3::VEC3() */
extern void fn_80041E8C(VEC3* v, f32 x, f32 y, f32 z);     /* VEC3::VEC3(f32, f32, f32) */
extern f32 fn_800A8A08(Random* r);                         /* Random::RandFloat */
extern void fn_800A99B4(void* self, VEC3* result, Emitter* em, VEC3* position, VEC3* normalDir,
                        VEC3* fromOrigin, VEC3* fromYAxis); /* EmitterForm::CalcVelocity */
extern u16 fn_800A9FB0(void* self, u16 aPtclLife, f32 aPtclLifeRnd, Emitter* em); /* CalcLife */

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
    (NW4R_VALID_PTR(p) ? (void)0 : Panic__Q24nw4r2dbFPCciPCce(lbl_80594EE0, line, msg, (p)))

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
>>>>>>> 59a4ae6b (auto: use nw4r ef_line's original names for fn_800CCFB0)
