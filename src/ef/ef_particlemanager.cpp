/*
 * ef/ef_particlemanager.cpp - the nw4r::ef::ParticleManager bookkeeping object: its constructor `fn_800AB664`
 *   (installs the table `lbl_805934E0`, slots 2-7 = fn_800AB730, fn_800AB73C, fn_800ABA6C, fn_800AC1BC,
 *   fn_800AC2F8, fn_800ADED8), the object's lifecycle, and the static initializer `fn_800AEE14`, which constructs
 *   the file's two static matrices.
 * RANGE. .text 0x800AB658-0x800AEE48 (44 functions); extab 0x80009E6C-0x80009F74, extabindex 0x80023148-0x800232D4,
 *   .ctors 0x8056F2DC-0x8056F2E0, .data 0x80592F78-0x80593580 (the `__FILE__` string "ef_particlemanager.cpp"
 *   first), .bss 0x80694538-0x80694598, .sdata2 0x807960A0-0x807960E8.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps `extsh` + `cmpwi`, `clrlwi` + `slwi`, the
 *   flag booleanisations and the `li r0,N; psq_lx` epilogues); `#pragma fp_contract off` around one body.
 * NAMES. The map has only `fn_` stems for the range.
 *   GUESS (from the body and its callers): `ef_pm_handle`, `ef_pm_get_mtx`, `ef_pm_modulate_color`.
 *   GUESS (the out-of-line `ut::List_GetLast` and the emitter resource's particle tracks, from `ef/ef_resource.cpp`):
 *   GUESS: `ef_list_get_last`, `ef_emres_num_ptcl_track`, `ef_emres_get_ptcl_track_tbl`, `ef_emres_get_ptcl_track_at`.
 *   GUESS: `ef_field_vortex` (0x800AD254): turns about an axis with a power blended across a distance.
 *   GUESS: `ef_field_random` (0x800AD520): a random push every interval, free or in a cone about the velocity.
 *   GUESS: `ef_field_newton` (0x800AD384): pulls toward a point, fading as 1/d^2 beyond a distance.
 *   GUESS: `ef_field_magnet` (0x800AD420): pulls toward a point with a constant power.
 *   GUESS: `ef_field_spin` (0x800AD47C): turns the position about an axis given as rotation angles.
 *   GUESS: `ef_field_gravity` (0x800AD9CC): a constant direction given as rotation angles, scaled by the power.
 *   GUESS: `ef_draw_info_copy` (0x800AE2A4): the draw info's (`EfDrawInfo`, 0xA0 bytes) field-by-field copy.
 *   GUESS: `ef_draw_info_set_depth_offset` (0x800AE298): sets the draw info's depth offset and origin.
 *   GUESS: `ef_pm_draw` (0x800ADED8): table slot 7, draws the particles through the manager's draw strategy.
 *   GUESS: `ef_pm_calc_emitter_pos` (0x800ADA5C): a position relative to the emitter, from the three matrices.
 * RESIDUALS. 4 rows unwritten (empty bodies): 0x800ABA6C-0x800AC0E4, 0x800AC1BC-0x800AC2F8,
 *   0x800AC2F8-0x800AD0C4, 0x800AE6A8-0x800AEE0C.  The source order differs from retail's, so `.text`, extab and
 *   extabindex run in another order.
 *  - `ef_field_random`: three products keep their operands in the other order (`fmuls f1, f1, f0`).
 *  - `ef_pm_draw`: retail tests the emitter's hidden bit as `beq` + `b` where ours branches once.
 *   relocdiff: our `.ctors` word carries the symbol `lbl_8056F2DC`, retail's none.
 *   flipcheck: `.bss`, `.data` and `.sdata2` claimed, not emitted (`.sdata2` is a partial pool: flipcheck names a
 *   fold with `ef/ef_particle.cpp`, one shared literal; request #28 moves the seam).
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `fn_805013FC`,
 *     `MTX34RotAxisFIdx__Q24nw4r4mathFPQ34nw4r4math5MTX34PCQ34nw4r4math4VEC3f`.
 *   flipcheck: referenced but defined by nothing a flip can use: `fn_805013FC`, `fn_80501C80`.
 *   `ef_pm_modulate_color` is unwritten (an empty body).
 * SHAPES. The pointer asserts are the `NW4R_POINTER_ASSERT` six-BOOL chain taking the file string
 *   ("ef_particlemanager.cpp", "particle.h" or "res_emitter.h"); `#line` reproduces each assert's line (61, 72, 73,
 *   0x29E, 0x2D5, ...).
 * SHAPES. A particle's link sits at `manager->list.linkOffset` (`next` at `node + linkOffset + 4`, retail's `lhz
 *   r0,0x42(self)` + `lwz rX,4(r3)`); the offset is a runtime value, so the walk keeps the byte offset.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef/ef_particlemanager.h"
#include "ef/ef_emitter.h" /* ef_store_word (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8.h" /* the vector and matrix helpers (rule 2) */
#include "ef/ef_vec3_normalize_to.h" /* ef_vec3_normalize_to (rule 2) */
#include "ef/ef_emitter_tex_flags.h" /* ef_emitter_get_mtx (rule 2) */
#include "ef/ef_util.h" /* ef_vec3_from_rotation (rule 2) */
#include "ef/ef_effect.h" /* ef_res_emitter_desc (rule 2) */
#include "ef/ef_particle.h" /* ef_resource_draw_setting (rule 2) */
#include "ef/ef_animcurve.h" /* ef_anim_rand_next / ef_anim_name_hash (rule 2) */
#include "g3d/mtx34_inverse.h" /* mtx34_inverse (rule 2) */
#include "g3d/g3d_calcview.h" /* mtx34_concat (rule 2) */
#include "g3d/fn_80063888.h" /* fn_80067E54 (rule 2) */
#include "draw_shape/mtx34_copy.h" /* mtx34_copy (rule 2) */
#include "vec3_scale.h" /* vec3_scale (rule 2) */

namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }
namespace nw4r { namespace math { f32 SinFIdx(f32); } }

/* The assert strings, the class table and the pool constants of this unit's claimed data, declared, never defined. */
extern "C" {
extern const f32 lbl_807960A4;
extern const f32 lbl_807960A8;
extern const f32 lbl_807960C0;
extern const f32 ef_pm_f32_zero; /* 0.0f */
extern const f32 ef_pm_f32_two; /* 2.0f */
extern const f32 ef_pm_f32_32768; /* 32768.0f */
extern const f32 ef_pm_f32_65535; /* 65535.0f */
extern const f32 ef_pm_f32_pi; /* pi */
extern const char lbl_80592F78[]; /* "ef_particlemanager.cpp" */
extern const char lbl_80592F90[]; /* "NW4R:Pointer Error\ntarget(=%p) is not valid pointer." */
extern const char lbl_80592FC8[]; /* "NW4R:Failed assertion target->mParticleManager == this" */
extern const char ef_pm_result_pointer_error[];
extern const char ef_pm_err_resource[]; /* "NW4R:Pointer Error\nmResource(=%p) is not valid pointer." */
extern const char ef_pm_err_emitter_desc[]; /* "NW4R:Pointer Error\ned(=%p) is not valid pointer." */
extern const char ef_pm_err_draw_strategy[]; /* "NW4R:Pointer Error\nmDrawStrategy(=%p) is not valid pointer." */
extern const char ef_pm_err_mtx_local_to_emitter[]; /* "NW4R:Pointer Error\nmtxLocalToEmitter(=%p) is not valid pointer." */
extern const char ef_pm_err_mtx_local_to_global[]; /* "NW4R:Pointer Error\nmtxLocalToGlobal(=%p) is not valid pointer." */
extern const char ef_pm_err_mtx_emitter_to_global[]; /* "NW4R:Pointer Error\nmtxEmitterToGlobal(=%p) is not valid pointer." */ /* "NW4R:Pointer Error\nresult(=%p) is not valid pointer." */
extern const char lbl_80593500[]; /* "NW4R:Pointer Error\nadd(=%p) is not valid pointer." */
extern const char lbl_80593534[]; /* "particle.h" */
extern const char lbl_80593540[]; /* "NW4R:Failed assertion num < NumPtclTrack()" */
extern const char lbl_8059356C[]; /* "res_emitter.h" */
extern void* lbl_805934E0[];      /* ParticleManager vtable */
extern nw4r::math::MTX34 lbl_80694538;
extern nw4r::math::MTX34 lbl_80694568;
}

/* Callees outside this unit. */
extern "C" {
void fn_800A4080(void* self);
void fn_800A3FFC(void* dst, s32 n);
void fn_800A3800(void* p);
void fn_800A45DC(void* list, void* node);
void fn_800A4A1C(void* list, void* node);
void fn_800A49B8(void* node);
void fn_800A6554(void* em, void* self);
void VEC2_ctor(void* self);
void color_rgba_copy(void* dst, const void* src);
void mtx34_mult_vec3(void* dst, const void* a, const void* b);
s32 ef_get_life_status(void* self);
u8* ef_emres_get_ptcl_track();
void fn_80501C80(void* self, s32 v);
void fn_805013FC(void* a, void* b, f32 f);
}

void operator delete(void* p) throw();

#pragma peephole off

/* --------------------------------------------------------------------------------------------- */
/* Types                                                                                          */
/* --------------------------------------------------------------------------------------------- */

struct EfPmManager;

/* A particle on a manager's active list. Only the fields this unit touches are named; the link is
 * addressed through the list's own `linkOffset`, so no fixed link field is declared. */
struct EfPmParticle {
    /* +0x00 */ u8 pad_0x00[0x0C];
    /* +0x0C */ s32 state;            /* 1 = created (ad/retire), 3 = retired */
    /* +0x10 */ u8 pad_0x10[0x9C];
    /* +0xAC */ nw4r::math::VEC3 accel; /* the velocity the fields add into */
    /* +0xB8 */ u8 pad_0xB8[0x0C];
    /* +0xC4 */ f32 accelScale;       /* the factor every added acceleration is scaled by */
    /* +0xC8 */ EfPmManager* manager; /* the ParticleManager that owns this particle */
    /* +0xCC */ u8 pad_0xCC[0x0C];
    /* +0xD8 */ s32 retireFlag;       /* distinguished by value 1 and 3 in the frame walk */
    /* +0xDC */ u16 age;              /* frames lived; 0 on the creation frame */
    /* +0xDE */ u8 pad_0xDE[0x04];
    /* +0xE2 */ u16 life;             /* decremented each frame while non-zero */
    /* +0xE4 */ u8 field_0xE4;
    /* +0xE5 */ u8 field_0xE5;
}; /* size: 0xE6 */

/* The list header at manager +0x38. `linkOffset` is the byte offset from a node to the node's link. */
struct EfPmList {
    /* +0x00 */ EfPmParticle* head;
    /* +0x04 */ u8 pad_0x04[0x06];
    /* +0x0A */ u16 linkOffset;
    /* +0x0C */ u8 pad_0x0C[0x10];
    /* +0x1C */ s32 activeCount;      /* zeroed at the start of each frame walk */
}; /* size: 0x20 */

struct EfEmitterManager;
struct EfResource;

/* The setter block at manager +0x6C (the `stateA.b` sub-object). */
struct EfPmStateB {
    /* +0x00 */ u8 dirMode;
    /* +0x01 */ u8 colorPri[4];
    /* +0x05 */ u8 colorSec[4];
    /* +0x09 */ u8 pad_0x09[3];
    /* +0x0C */ f32 scale;
    /* +0x10 */ nw4r::math::VEC3 vec;
}; /* size: 0x1C */

struct EfPmStateAA {
    /* +0x00 */ f32 a;
    /* +0x04 */ f32 b;
    /* +0x08 */ nw4r::math::VEC3 vec;
}; /* size: 0x14 */

/* The scalar/vec block constructed at manager +0x58. */
struct EfPmStateA {
    /* +0x00 */ EfPmStateAA aa;
    /* +0x14 */ EfPmStateB b;
}; /* size: 0x30 */

/* The manager. Only the fields this unit reads are named; the size is a lower bound. */
struct EfPmManager {
    /* +0x00 */ u8 pad_0x00[0x0C];
    /* +0x0C */ s32 state;
    /* +0x10 */ u8 pad_0x10[0x0C];
    /* +0x1C */ void** vtable;
    /* +0x20 */ EfEmitterManager* managerEM;
    /* +0x24 */ EfResource* resource;
    /* +0x28 */ u32 flags;
    /* +0x2C */ nw4r::ef::DrawStrategy* drawStrategy;
    /* +0x30 */ u8 pad_0x30[0x08];
    /* +0x38 */ EfPmList list;
    /* +0x58 */ EfPmStateA stateA;
    /* +0x88 */ s8 inheritTranslate;  /* the share of the emitter's translation the matrix inherits, in % */
    /* +0x89 */ u8 pad_0x89;
    /* +0x8A */ u8 mtxDirty;          /* ef_pm_get_mtx rebuilds `mtx` while it is set */
    /* +0x8B */ u8 pad_0x8B;
    /* +0x8C */ nw4r::math::MTX34 mtx; /* the manager's matrix, inherited from its emitter's */
}; /* size: 0xBC */

/* A direction/rate parameter block (its rate at +0x04, its vector at +0x08). */
struct EfPmDirParam {
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ f32 rate;
    /* +0x08 */ nw4r::math::VEC3 vec;
}; /* size: 0x14 */

/* The parameter block fn_800AC0E4 forwards. */
struct EfPmSetterParams {
    /* +0x00 */ u8 pad_0x00[0x77];
    /* +0x77 */ u8 mode;
    /* +0x78 */ u32 colorPri;
    /* +0x7C */ u32 colorSec;
    /* +0x80 */ f32 scale;
    /* +0x84 */ nw4r::math::VEC3 vec;
}; /* size: 0x90 (lower bound) */

/* The field parameter records the per-particle field functions read: the field's power at +0x04, then
 * the kind's own payload. */
struct EfPmFieldRotation {
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ f32 power;
    /* +0x08 */ EfRotation rotation; /* the direction (gravity) or the axis (spin), as rotation angles */
}; /* size: 0x14 */

struct EfPmFieldPoint {
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ f32 power;
    /* +0x08 */ nw4r::math::VEC3 pos; /* the point the particles are pulled to */
}; /* size: 0x14 */

struct EfPmFieldNewtonParam {
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ f32 power;
    /* +0x08 */ f32 distance; /* the pull fades as 1/d^2 beyond it */
    /* +0x0C */ nw4r::math::VEC3 pos;
}; /* size: 0x18 */

struct EfPmFieldVortexParam {
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ f32 innerPower; /* the power at the axis */
    /* +0x08 */ f32 outerPower; /* the power at and beyond the distance */
    /* +0x0C */ f32 distance;   /* squared in place by the first call */
    /* +0x10 */ EfRotation rotation; /* the vortex axis, as rotation angles */
}; /* size: 0x1C */

/* The emitter descriptor ef_res_emitter_desc resolves a manager's resource to: +0x94 holds its flags (bit 10: not drawn). */
struct EfPmEmitterDesc {
    /* +0x00 */ u8 pad_0x00[0x94];
    /* +0x94 */ u16 flags;
}; /* size: 0x96 */

/* The emitter's draw setting as the particle manager reads it: the depth offset at +0xB4. */
struct EfPmDrawSetting {
    /* +0x00 */ u8 pad_0x00[0xB4];
    /* +0xB4 */ f32 depthOffset;
}; /* size: 0xB8 */

/* The emitter flags the draw pass reads (+0x20, bit 1: hidden). */
struct EfPmEmitterView {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ u32 flags;
}; /* size: 0x24 */

/* The random field's parameter record. */
struct EfPmFieldRandomParam {
    /* +0x00 */ u8 pad_0x00;
    /* +0x01 */ u8 baseMode;   /* 1: a particle past its first frame bases the power on its previous velocity */
    /* +0x02 */ u8 pad_0x02;
    /* +0x03 */ u8 flags;      /* bit 0: power from the base velocity's length; bit 1: a free random direction;
                                * bits 2-4: keep the x/y/z component */
    /* +0x04 */ f32 power;
    /* +0x08 */ f32 diffusion; /* the cone angle about the base direction; 0 keeps the direction */
    /* +0x0C */ u16 interval;  /* the field acts every interval + 1 frames */
}; /* size: 0x10 */

/* The record the random field's hash is keyed by: its id at +0x06. */
struct EfPmFieldHeader {
    /* +0x00 */ u8 pad_0x00[0x06];
    /* +0x06 */ u16 id;
}; /* size: 0x08 */

/* The emitter flags the random field reads: bit 4 makes it act on the creation frame too. */
struct EfPmFieldOwner {
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ u8 flags;
}; /* size: 0x05 */

/* The three-vec block used by the particle transform paths. */
struct EfPmVecBlock {
    /* +0x00 */ nw4r::math::VEC3 a;
    /* +0x0C */ nw4r::math::VEC3 b;
    /* +0x18 */ nw4r::math::VEC3 c;
}; /* size: 0x24 */

/* --------------------------------------------------------------------------------------------- */
/* Assert (shared shape, ef shape units)                                                          */
/* --------------------------------------------------------------------------------------------- */

#define NW4R_POINTER_ASSERT(file, ptr, msg)                                                  \
    {                                                                                        \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;   \
        u32 top_ = (u32)(ptr) & 0xFF000000u;                                                 \
        if (!(top_ == 0x80000000u) && !(((u32)(ptr) & 0xFF800000u) == 0x81000000u))          \
            ok6_ = FALSE;                                                                    \
        if (!ok6_ && !(((u32)(ptr) & 0xF8000000u) == 0x90000000u))                           \
            ok5_ = FALSE;                                                                    \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                 \
            ok4_ = FALSE;                                                                    \
        if (!ok4_ && !(((u32)(ptr) & 0xFF800000u) == 0xC1000000u))                           \
            ok3_ = FALSE;                                                                    \
        if (!ok3_ && !(((u32)(ptr) & 0xF8000000u) == 0xD0000000u))                           \
            ok2_ = FALSE;                                                                    \
        if (!ok2_ && !(((u32)(ptr) & 0xFFFFC000u) == 0xE0000000u))                           \
            ok1_ = FALSE;                                                                    \
        if (!ok1_)                                                                           \
            nw4r::db::Panic(file, __LINE__, msg, (ptr));                                     \
    }

/* --------------------------------------------------------------------------------------------- */
/* Functions, in address order                                                                    */
/* --------------------------------------------------------------------------------------------- */

/* Forward declarations of this unit's own constructors and accessors. */
extern "C" EfPmStateA* fn_800AB6BC(EfPmStateA* self);
extern "C" EfPmStateB* fn_800AB6FC(EfPmStateB* self);
extern "C" u8* ef_emres_get_ptcl_track_tbl(void* self);
extern "C" u16 ef_emres_num_ptcl_track(void* self);
extern "C" void fn_800AC100(EfPmManager* self, u8 a, void* b, void* c, f32 f, const nw4r::math::VEC3* d);
extern "C" void fn_800AEE14();
extern "C" EfDrawInfo* ef_draw_info_copy(EfDrawInfo* dst, const EfDrawInfo* src);
extern "C" void ef_draw_info_set_depth_offset(EfDrawInfo* self, f32 offset, const nw4r::math::VEC3* origin);

/* Scale a normalised rate onto the sine table. */
extern "C" f32 fn_800AB658(f32 x) {
    return nw4r::math::SinFIdx(lbl_807960A4 * x);
}

/* ParticleManager constructor: base ctor, vtable, then the three sub-objects at +0x38/+0x58/+0x8C. */
extern "C" EfPmManager* fn_800AB664(EfPmManager* self) {
    fn_800A4080(self);
    self->vtable = lbl_805934E0;
    fn_800A3FFC(&self->list, 0x14);
    fn_800AB6BC(&self->stateA);
    MTX34_ctor(&self->mtx);
    return self;
}

/* Construct the scalar/vec block at manager +0x58. */
extern "C" EfPmStateA* fn_800AB6BC(EfPmStateA* self) {
    VEC2_ctor(self);
    VEC3_ctor(&self->aa.vec);
    fn_800AB6FC(&self->b);
    return self;
}

extern "C" EfPmStateB* fn_800AB6FC(EfPmStateB* self) {
    VEC3_ctor(&self->vec);
    return self;
}

/* Virtual slot 2: forward the manager to the emitter manager. */
extern "C" void fn_800AB730(EfPmManager* self) {
    fn_800A6554(self->managerEM, self);
}

/* Virtual slot 3: the base's empty implementation (the destroy path lives elsewhere). */
extern "C" void fn_800AB73C(EfPmManager* self) {
    (void)self;
}

/* Put `target` on the manager's list and mark it retired. */
extern "C" s32 fn_800AB740(EfPmManager* self, EfPmParticle* target) {
#line 61
    NW4R_POINTER_ASSERT(lbl_80592F78, target, lbl_80592F90);
    fn_800A3800(target->manager);
    fn_800A45DC(&self->list, target);
    target->state = 3;
    return 1;
}

/* Retire `target` from the manager's list if it is in the created state. */
extern "C" s32 fn_800AB880(EfPmManager* self, EfPmParticle* target) {
#line 72
    NW4R_POINTER_ASSERT(lbl_80592F78, target, lbl_80592F90);
#line 73
    if (target->manager != self) nw4r::db::Panic(lbl_80592F78, __LINE__, lbl_80592FC8);
    if (target->state != 1)
        return 0;
    fn_800A4A1C(&self->list, target);
    fn_800A49B8(target);
    return 1;
}

/* Walk the list, retiring every created particle. */
extern "C" s32 fn_800AB9F4(EfPmManager* self) {
    s32 total = 0;
    EfPmParticle* node = self->list.head;
    while (node != NULL) {
        EfPmParticle* next = *(EfPmParticle**)((u8*)node + self->list.linkOffset + 4);
        if (node->state == 1)
            total += fn_800AB880(self, node);
        node = next;
    }
    return total;
}

/* Forward a parameter block to the manager's own setter. */
extern "C" void fn_800AC0E4(EfPmManager* self, EfPmSetterParams* p) {
    fn_800AC100(self, p->mode, &p->colorPri, &p->colorSec, p->scale, &p->vec);
}

extern "C" void fn_800AC100(EfPmManager* self, u8 a, void* b, void* c, f32 f, const nw4r::math::VEC3* d);

/* The setter fn_800AC0E4 forwards to. */
extern "C" void fn_800AC100(EfPmManager* self, u8 a, void* b, void* c, f32 f, const nw4r::math::VEC3* d) {
    self->stateA.b.dirMode = a;
    color_rgba_copy(self->stateA.b.colorPri, b);
    color_rgba_copy(self->stateA.b.colorSec, c);
    self->stateA.b.scale = f;
    copyVec3(&self->stateA.b.vec, (const nw4r::math::VEC3*)d);
}

/* Delete-like helper: free only a live object. */
extern "C" void* fn_800AC178(void* p, s32 n) {
    if (p != NULL && (s16)n > 0)
        operator delete(p);
    return p;
}

/* Slot-6 helper: forward to the SDK's matrix function. */
extern "C" void ef_list_get_last(void* self) {
    fn_80501C80(self, 0);
}

/* The resource-table accessors: the table's count, its first array base and its third u16. */
extern "C" u16 ef_emres_num_ptcl_track(void* self) {
    (void)self;
    return *(u16*)ef_emres_get_ptcl_track();
}

extern "C" u16 fn_800ADE74(void* self) {
    (void)self;
    return *(u16*)(ef_emres_get_ptcl_track() + 2);
}

extern "C" u8* ef_emres_get_ptcl_track_tbl(void* self) {
    (void)self;
    return ef_emres_get_ptcl_track() + 4;
}

/* 0x800ADA5C (0x370): the position `pos` relative to the emitter: through the local-to-emitter matrix, or through
 * the local-to-global one minus the emitter's global origin. */
extern "C" void ef_pm_calc_emitter_pos(nw4r::math::VEC3* result, BOOL emitterSpace, const nw4r::math::MTX34* mtxLocalToEmitter,
                            const nw4r::math::MTX34* mtxLocalToGlobal, const nw4r::math::MTX34* mtxEmitterToGlobal,
                            const nw4r::math::VEC3* pos) {
#line 194
    NW4R_POINTER_ASSERT(lbl_80592F78, mtxLocalToEmitter, ef_pm_err_mtx_local_to_emitter);
    NW4R_POINTER_ASSERT(lbl_80592F78, mtxLocalToGlobal, ef_pm_err_mtx_local_to_global);
    NW4R_POINTER_ASSERT(lbl_80592F78, mtxEmitterToGlobal, ef_pm_err_mtx_emitter_to_global);
    VEC3_ctor(result);
    if (emitterSpace) {
        mtx34_mult_vec3(result, mtxLocalToEmitter, pos);
        return;
    }
    nw4r::math::VEC3 origin;
    mtx34_mult_vec3(result, mtxLocalToGlobal, pos);
    setVec3(&origin, ef_pm_f32_zero, ef_pm_f32_zero, ef_pm_f32_zero);
    mtx34_mult_vec3(&origin, mtxEmitterToGlobal, &origin);
    PSVECSubtract(&result->x, &result->x, &origin.x);
}

/* 0x800ADED8 (0x3C0): draws the manager's particles through its draw strategy, with the draw info's depth
 * offset measured from the emitter's origin when the draw setting asks for one. */
extern "C" void ef_pm_draw(EfPmManager* self, const EfDrawInfo* info) {
#line 698
    NW4R_POINTER_ASSERT(lbl_80592F78, self->resource, ef_pm_err_resource);
    EfPmEmitterDesc* ed = (EfPmEmitterDesc*)ef_res_emitter_desc(self->resource);
#line 701
    NW4R_POINTER_ASSERT(lbl_80592F78, ed, ef_pm_err_emitter_desc);
    if ((ed->flags & 0x400) == 0) {
        if ((((EfPmEmitterView*)self->managerEM)->flags & 2) == 0) {
            EfDrawInfo local;
            ef_draw_info_copy(&local, info);
            f32 offset = ((EfPmDrawSetting*)ef_resource_draw_setting(self->resource))->depthOffset;
            if (ef_pm_f32_zero != offset) {
                nw4r::math::MTX34 m;
                nw4r::math::VEC3 origin;
                MTX34_ctor(&m);
                ef_emitter_get_mtx((EfDrawEmitter*)self->managerEM, &m);
                setVec3(&origin, m.m[0][3], m.m[1][3], m.m[2][3]);
                ef_draw_info_set_depth_offset(&local, offset, &origin);
            }
#line 719
            NW4R_POINTER_ASSERT(lbl_80592F78, self->drawStrategy, ef_pm_err_draw_strategy);
            self->drawStrategy->Draw(local, (EfDrawParticleManager*)self);
        }
    }
}

/* Bounds-checked resource lookup (index into the emitter resource table). */
extern "C" u8* ef_emres_get_ptcl_track_at(void* self, u32 index) {
    u8* base = ef_emres_get_ptcl_track_tbl(self);
    if (!((u16)index < ef_emres_num_ptcl_track(self)))
        nw4r::db::Panic(lbl_8059356C, 0x29E, lbl_80593540);
    return ((u8**)base)[(u16)index];
}

#pragma fp_contract off
/* Adds a scaled direction into the particle's accumulated acceleration. */
extern "C" nw4r::math::VEC3* fn_800AD0CC(EfPmParticle* self, nw4r::math::VEC3* add) {
#line 281
    NW4R_POINTER_ASSERT(lbl_80593534, add, lbl_80593500);
    self->accel.x = self->accel.x + add->x * self->accelScale;
    self->accel.y = self->accel.y + add->y * self->accelScale;
    self->accel.z = self->accel.z + add->z * self->accelScale;
    return &self->accel;
}
#pragma fp_contract reset

/* Zero the three-vec block used by the particle transform paths. */
extern "C" EfPmVecBlock* fn_800ADE98(EfPmVecBlock* self) {
    VEC3_ctor(&self->a);
    VEC3_ctor(&self->b);
    VEC3_ctor(&self->c);
    return self;
}

/* Scale a rate by the pool constant before handing it to the SDK vector routine. */
extern "C" void fn_800AD514(void* a, void* b, f32 f) {
    fn_805013FC(a, b, lbl_807960C0 * f);
}

/* The reciprocal-rate wrapper: `out = in * (1 / f)`. */
extern "C" void fn_800AD9C0(nw4r::math::VEC3* out, nw4r::math::VEC3* in, f32 f) {
    vec3_scale(out, in, lbl_807960A8 / f);
}

/* Scale the parameter's rate by `1.0f - rate` onto the matrix. */
extern "C" s32 fn_800ADA24(nw4r::math::VEC3* out, EfPmDirParam* p, const nw4r::math::VEC3* v) {
    vec3_scale_by(&out->x, &v->x, p->rate - lbl_807960A8);
    return 1;
}

/* Sets the draw info's depth offset and the point it is measured from. */
extern "C" void ef_draw_info_set_depth_offset(EfDrawInfo* self, f32 offset, const nw4r::math::VEC3* origin) {
    self->depth_offset = offset;
    copyVec3(&self->depth_origin, origin);
}

/* Walk the list, aging every particle. */
extern "C" void fn_800AE500(EfPmManager* self, s32 arg1) {
    self->list.activeCount = 0;
    EfPmParticle* node = self->list.head;
    while (node != NULL) {
        if (arg1 == 0 || node->life != 0) {
            if (node->life != 0)
                node->life--;
            if (ef_get_life_status(node) == 1 && node->retireFlag == 1)
                node->retireFlag = 0;
        }
        node = *(EfPmParticle**)((u8*)node + self->list.linkOffset + 4);
    }
}

/* Walk the list, promoting every retired particle. */
extern "C" void fn_800AE5B0(EfPmManager* self) {
    EfPmParticle* node = self->list.head;
    while (node != NULL) {
        if (ef_get_life_status(node) == 1 && node->retireFlag == 3)
            node->retireFlag = 1;
        node = *(EfPmParticle**)((u8*)node + self->list.linkOffset + 4);
    }
}

/* The file's static block accessors. */
extern "C" nw4r::math::MTX34* fn_800AE698() {
    return &lbl_80694568;
}

extern "C" void fn_800AE6A4() {
}

extern "C" void ef_pm_handle(void* a, EfPmManager* m) {
    ef_store_word(a, (s32)m->resource);
}

/* The .ctors entry: construct the file's two static matrices. */
extern "C" void fn_800AEE14() {
    MTX34_ctor(&lbl_80694538);
    MTX34_ctor(&lbl_80694568);
}

__declspec(section ".ctors") void* const lbl_8056F2DC = (void*)fn_800AEE14;

/* 0x800AD254 (0x130): the vortex field: `out` turns `pos` about the field's axis, its power blended from the inner
 * to the outer one across the distance; 0 when `pos` sits on the axis. */
#pragma fp_contract off
extern "C" s32 ef_field_vortex(nw4r::math::VEC3* out, EfPmFieldVortexParam* p, const nw4r::math::VEC3* pos) {
    nw4r::math::VEC3 axis;
    nw4r::math::VEC3 radial;
    EfRotation rotation;
    f32 power;
    VEC3_ctor(&axis);
    ef_vec3_from_rotation((const EfRotation*)vec3_copy_construct(&rotation, &p->rotation), &axis);
    p->distance = p->distance * p->distance;
    VEC3_ctor(&radial);
    vec3_scale_by(&radial.x, &axis.x, vec3_dot(&axis.x, &pos->x));
    PSVECSubtract(&radial.x, &pos->x, &radial.x);
    f32 lenSq = vec3_length_sq(&radial.x);
    if (ef_pm_f32_zero == lenSq) {
        return 0;
    }
    if (lenSq >= p->distance) {
        power = p->outerPower;
    } else {
        f32 t = lenSq / p->distance;
        f32 inner = lbl_807960A8 - t;
        power = inner * p->innerPower + t * p->outerPower;
    }
    ef_vec3_normalize_to(&radial, &radial);
    vec3_cross(&out->x, &radial.x, &axis.x);
    vec3_scale_by(&out->x, &out->x, power);
    return 1;
}
#pragma fp_contract reset

/* 0x800AD384 (0x9C): the newton field: pulls `out` toward the field's point with its power, fading as the
 * inverse square beyond its distance. */
extern "C" s32 ef_field_newton(nw4r::math::VEC3* out, const EfPmFieldNewtonParam* p, const nw4r::math::VEC3* pos) {
    PSVECSubtract(&out->x, &p->pos.x, &pos->x);
    f32 lenSq = vec3_length_sq(&out->x);
    ef_vec3_normalize_to(out, out);
    vec3_scale_by(&out->x, &out->x, p->power);
    f32 dSq = p->distance * p->distance;
    if (lenSq > dSq) {
        vec3_scale_by(&out->x, &out->x, dSq / lenSq);
    }
    return 1;
}

/* 0x800AD420 (0x5C): the magnet field: `out` points from `pos` to the field's point, scaled by its power. */
extern "C" s32 ef_field_magnet(nw4r::math::VEC3* out, const EfPmFieldPoint* p, const nw4r::math::VEC3* pos) {
    PSVECSubtract(&out->x, &p->pos.x, &pos->x);
    ef_vec3_normalize_to(out, out);
    vec3_scale_by(&out->x, &out->x, p->power);
    return 1;
}

/* 0x800AD47C (0x98): the spin field: `out` is how far `pos` moves when it turns about the field's axis by
 * its power. */
extern "C" s32 ef_field_spin(nw4r::math::VEC3* out, const EfPmFieldRotation* p, const nw4r::math::VEC3* pos) {
    nw4r::math::VEC3 axis;
    EfRotation rotation;
    nw4r::math::MTX34 m;
    VEC3_ctor(&axis);
    ef_vec3_from_rotation((const EfRotation*)vec3_copy_construct(&rotation, (void*)&p->rotation), &axis);
    MTX34_ctor(&m);
    fn_800AD514(&m, &axis, p->power);
    mtx34_mult_vec3(out, &m, pos);
    PSVECSubtract(&out->x, &out->x, &pos->x);
    return 1;
}

/* 0x800AD520 (0x4A0): the random field: every `interval + 1` frames `out` gets a random push - a free direction
 * or a cone about the (transformed) base velocity - whose power may come from the base velocity's length. */
extern "C" s32 ef_field_random(nw4r::math::VEC3* out, EfPmFieldRandomParam* p, const EfPmFieldHeader* header,
                           const EfPmFieldOwner* owner, const EfPmParticle* ptcl, u32 tick, u16 seed,
                           const nw4r::math::VEC3* velocity, const nw4r::math::VEC3* prevVelocity,
                           const nw4r::math::MTX34* mtx) {
    nw4r::math::MTX34 frame;
    nw4r::math::VEC3 base;
    nw4r::math::VEC3 dir;
    nw4r::math::VEC3 prev;
    f32 s;
    f32 c;
    u32 hash = ef_anim_name_hash(seed, header->id, tick, 0);
    if ((owner->flags & 0x10) == 0) {
        if (tick == 0 || tick % (p->interval + 1) != 0) {
            return 0;
        }
    } else if (ptcl->age != 0 && tick % (p->interval + 1) != 0) {
        return 0;
    }

    if ((p->flags & 1) != 0) {
        VEC3_ctor(&base);
        switch (p->baseMode) {
        case 0:
            copyVec3(&base, velocity);
            break;
        case 1:
            if (ptcl->age == 0) {
                copyVec3(&base, velocity);
            } else {
                fn_800AD9C0(&prev, (nw4r::math::VEC3*)prevVelocity, ptcl->accelScale);
                copyVec3(&base, &prev);
            }
            break;
        }
        mtx34_mult_vec3(&base, mtx, &base);
        p->power = vec3_len(&base.x);
    }

    if ((p->flags & 2) != 0) {
        out->x = (f32)(s16)(hash >> 16) / ef_pm_f32_32768;
        hash = ef_anim_rand_next(hash);
        out->y = (f32)(s16)(hash >> 16) / ef_pm_f32_32768;
        hash = ef_anim_rand_next(hash);
        out->z = (f32)(s16)(hash >> 16) / ef_pm_f32_32768;
        hash = ef_anim_rand_next(hash);
        if ((p->flags & 1) != 0 && ef_vec3_normalize_to(out, out) == 0) {
            out->y = lbl_807960A8;
        }
        vec3_scale_by(&out->x, &out->x, p->power);
    } else {
        VEC3_ctor(&dir);
        if (ptcl->age == 0) {
            copyVec3(&dir, velocity);
        } else {
            copyVec3(&dir, prevVelocity);
        }
        mtx34_mult_vec3(&dir, mtx, &dir);
        if (ef_vec3_normalize_to(&dir, &dir) == 0) {
            dir.y = lbl_807960A8;
        }
        MTX34_ctor(&frame);
        ef_mtx34_from_y_axis(frame.m[0], &dir.x);
        if (ef_pm_f32_zero != p->diffusion) {
            f32 tilt = (f32)(hash >> 16) / ef_pm_f32_65535 * p->diffusion;
            hash = ef_anim_rand_next(hash);
            f32 turn = ef_pm_f32_two * (ef_pm_f32_pi * ((f32)(hash >> 16) / ef_pm_f32_65535));
            hash = ef_anim_rand_next(hash);
            ef_vec_sin_cos((Vec*)out, tilt);
            out->z = out->x;
            ef_sin_cos(&s, &c, turn);
            out->x = out->x * s;
            out->z = out->z * c;
            if ((p->flags & 1) != 0) {
                vec3_scale_by(&out->x, &out->x, p->power);
            } else {
                vec3_scale_by(&out->x, &out->x, (f32)(hash >> 16) / ef_pm_f32_65535 * p->power);
            }
        } else {
            out->x = ef_pm_f32_zero;
            if ((p->flags & 1) != 0) {
                out->y = p->power;
            } else {
                out->y = (f32)(s16)(hash >> 16) / ef_pm_f32_32768 * p->power;
            }
            out->z = ef_pm_f32_zero;
        }
        mtx34_mult_vec3(out, &frame, out);
    }

    if ((p->flags & 4) == 0) {
        out->x = ef_pm_f32_zero;
    }
    if ((p->flags & 8) == 0) {
        out->y = ef_pm_f32_zero;
    }
    if ((p->flags & 0x10) == 0) {
        out->z = ef_pm_f32_zero;
    }
    return 1;
}

/* 0x800AD9CC (0x58): the gravity field: `out` is the field's direction scaled by its power. */
extern "C" s32 ef_field_gravity(nw4r::math::VEC3* out, const EfPmFieldRotation* p) {
    EfRotation rotation;
    ef_vec3_from_rotation((const EfRotation*)vec3_copy_construct(&rotation, (void*)&p->rotation), out);
    fn_800513F0(out, p->power);
    return 1;
}

/* 0x800AE2A4 (0xBC): copies the draw info field by field. */
extern "C" EfDrawInfo* ef_draw_info_copy(EfDrawInfo* dst, const EfDrawInfo* src) {
    ef_mtx34_copy(&dst->view_mtx, &src->view_mtx);
    ef_mtx34_copy(&dst->mtx_0x30, &src->mtx_0x30);
    dst->light_enable = src->light_enable;
    dst->light_mask = src->light_mask;
    dst->light_mask1 = src->light_mask1;
    dst->is_spot_light = src->is_spot_light;
    dst->fog_type = src->fog_type;
    dst->fog_start_z = src->fog_start_z;
    dst->fog_end_z = src->fog_end_z;
    dst->fog_near_z = src->fog_near_z;
    dst->fog_far_z = src->fog_far_z;
    *(u32*)&dst->fog_color = *(const u32*)&src->fog_color; /* the colour moves as one word (a ut::Color copy) */
    dst->depth_offset = src->depth_offset;
    assignVec3((Vec*)&dst->depth_origin, (Vec*)&src->depth_origin);
    *(u32*)&dst->mat_color = *(const u32*)&src->mat_color; /* the colour moves as one word (a ut::Color copy) */
    *(u32*)&dst->amb_color = *(const u32*)&src->amb_color; /* the colour moves as one word (a ut::Color copy) */
    return dst;
}

/* 0x800AE628 (0x70): stores the particle manager's matrix relative to its emitter's in the file's second
 * static matrix. */
extern "C" void fn_800AE628(EfPmManager* self) {
    nw4r::math::MTX34 emitterMtx;
    nw4r::math::MTX34 pmMtx;
    MTX34_ctor(&emitterMtx);
    MTX34_ctor(&pmMtx);
    ef_emitter_get_mtx((EfDrawEmitter*)self->managerEM, &emitterMtx);
    ef_pm_get_mtx(self, &pmMtx);
    mtx34_inverse(&emitterMtx, &emitterMtx);
    mtx34_concat(&lbl_80694568, &emitterMtx, &pmMtx);
}

/* 0x800AE360 (0x1A0): rebuilds the manager's matrix from its emitter's when it is dirty and copies it to
 * `out`. */
extern "C" MTX34* ef_pm_get_mtx(void* target, MTX34* out) {
    EfPmManager* self = (EfPmManager*)target;
#line 725
    NW4R_POINTER_ASSERT(lbl_80592F78, out, ef_pm_result_pointer_error);
    if (self->mtxDirty != 0) {
        nw4r::math::MTX34 emitterMtx;
        MTX34_ctor(&emitterMtx);
        ef_emitter_get_mtx((EfDrawEmitter*)self->managerEM, &emitterMtx);
        u32 flags = self->flags;
        ef_calc_inherit_mtx(&self->mtx, &emitterMtx, (flags & 1) != 0, (flags & 2) != 0, self->inheritTranslate,
                            (flags & 4) != 0);
        self->mtxDirty = 0;
    }
    mtx34_copy(out, &self->mtx);
    return out;
}

/* --------------------------------------------------------------------------------------------- */
/* Not yet reconstructed                                                                          */
/* --------------------------------------------------------------------------------------------- */

extern "C" void fn_800ABA6C() {}
extern "C" void fn_800AC1BC() {}
extern "C" void fn_800AC2F8() {}
extern "C" void ef_pm_modulate_color() {}
