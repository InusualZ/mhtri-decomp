/*
 * ef/ef_particlemanager.cpp - the nw4r::ef::ParticleManager bookkeeping object: its constructor `fn_800AB664`
 *   (installs the table `lbl_805934E0`, slots 2-7 = fn_800AB730, fn_800AB73C, ef_pm_initialize, ef_pm_create_particle,
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
 *   GUESS: `ef_pm_initialize` (0x800ABA6C): table slot 4, binds the manager to its emitter and resource.
 *   GUESS: `ef_pm_create_particle` (0x800AC1BC): table slot 5, allocates, initialises and lists a particle.
 *   GUESS: `ef_pm_calc` (0x800AC2F8): table slot 6, one frame of the particles' curve tracks, fields and steps.
 *   GUESS: `ef_emres_num_ptcl_track_init` (0x800ADE74): the track table's second count, where tracks past the
 *   creation frame start.
 *   GUESS: `ef_pm_draw` (0x800ADED8): table slot 7, draws the particles through the manager's draw strategy.
 *   GUESS: `ef_pm_calc_emitter_pos` (0x800ADA5C): a position relative to the emitter, from the three matrices.
 *   GUESS: `ef_pm_particle_mtx` (0x800AE698): the static matrix the colour fade takes a particle's position with.
 *   GUESS: `ef_pm_err_pp`, `ef_pm_err_color_pri`, `ef_pm_err_color_sec`, `ef_pm_f32_epsilon`, `ef_pm_f32_256`
 *   (the colour fade's assert messages and constants, named from their text and value).
 * RESIDUALS. No row is unwritten.  The source order differs from retail's, so `.text`, extab and
 *   extabindex run in another order.
 *  - `ef_field_random`: three products keep their operands in the other order (`fmuls f1, f1, f0`).
 *  - `ef_pm_calc`: the frame is 0x420 where retail's is 0x430 and the locals sit at other offsets; the field and
 *    post-field record addresses add their block sizes in another order (`lwz`/`add` scheduling).
 *  - `ef_pm_draw`: retail tests the emitter's hidden bit as `beq` + `b` where ours branches once (an early return
 *    and a `bool` local give the same single branch).
 *   relocdiff: our `.ctors` word carries the symbol `lbl_8056F2DC`, retail's none.
 *   flipcheck: `.bss`, `.data` and `.sdata2` claimed, not emitted (`.sdata2` is a partial pool: flipcheck names a
 *   fold with `ef/ef_particle.cpp`, one shared literal; request #28 moves the seam).
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `fn_805013FC`,
 *     `MTX34RotAxisFIdx__Q24nw4r4mathFPQ34nw4r4math5MTX34PCQ34nw4r4math4VEC3f`.
 *   flipcheck: referenced but defined by nothing a flip can use: `fn_805013FC`, `fn_80501C80`.
 * SHAPES. The pointer asserts are the `NW4R_POINTER_ASSERT` six-BOOL chain taking the file string
 *   ("ef_particlemanager.cpp", "particle.h" or "res_emitter.h"); `#line` reproduces each assert's line (61, 72, 73,
 *   0x29E, 0x2D5, ...).
 * SHAPES. The memory manager and the particle are classes with declared virtual tables (the particle's after a
 *   0x1C-byte head): `ef_pm_create_particle` calls `AllocParticle` and `Initialize` as virtual members.
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
#include "ef/ef_creationqueue.h" /* ef_ref_object_add_ref (rule 2) */
#include "ef/ef_particle.h" /* ef_resource_draw_setting (rule 2) */
#include "ef/ef_animcurve.h" /* ef_anim_rand_next / ef_anim_name_hash (rule 2) */
#include "g3d/mtx34_inverse.h" /* mtx34_inverse (rule 2) */
#include "g3d/g3d_calcview.h" /* mtx34_concat (rule 2) */
#include "g3d/fn_80063888.h" /* fn_80067E54 (rule 2) */
#include "draw_shape/mtx34_copy.h" /* mtx34_copy (rule 2) */
#include "vec3_scale.h" /* vec3_scale (rule 2) */
#include "nw4r/fn_805012C4.h" /* nw4r::ut::List_GetNext (rule 2) */
#include "g3d/g3d_calcworld.h" /* addVec3To (rule 2) */
#include "g3d/fn_8005AA28.h" /* mtx34_trans_apply (rule 2) */
#include "ef/ef_particle_get_move_dir.h" /* ef_particle_get_move_dir (rule 2) */
#include "ef/ef_pf_calc_particle.h" /* ef_pf_calc_particle (rule 2) */
#include "Runtime.PPCEABI.H/memcpy.h" /* memcpy (rule 2) */
#include "ef/ef_mtx34_scale_columns.h" /* ef_mtx34_scale_columns / ef_mtx34_rotate_xyz (rule 2) */

namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }
namespace nw4r { namespace math { f32 SinFIdx(f32); } }

/* The assert strings, the class table and the pool constants of this unit's claimed data, declared, never defined. */
extern "C" {
extern const f32 lbl_807960A4;
extern const f32 lbl_807960A8;
extern const f32 lbl_807960C0;
extern const f32 ef_pm_f32_zero; /* 0.0f */
extern const f32 ef_pm_f32_two; /* 2.0f */
extern const f32 ef_pm_f32_inv_sqrt3; /* 1/sqrt(3) */
extern const f32 ef_pm_f32_nan; /* NaN: no tail origin */
extern const f32 ef_pm_f32_minus_one; /* -1.0f */
extern const f32 ef_pm_f32_32768; /* 32768.0f */
extern const f32 ef_pm_f32_65535; /* 65535.0f */
extern const f32 ef_pm_f32_pi; /* pi */
extern const f32 ef_pm_f32_epsilon; /* FLT_EPSILON: a fade range below it is no range */
extern const f32 ef_pm_f32_256; /* 256.0f: the fade factor's fixed-point one */
extern const char lbl_80592F78[]; /* "ef_particlemanager.cpp" */
extern const char lbl_80592F90[]; /* "NW4R:Pointer Error\ntarget(=%p) is not valid pointer." */
extern const char lbl_80592FC8[]; /* "NW4R:Failed assertion target->mParticleManager == this" */
extern const char ef_pm_result_pointer_error[];
extern const char ef_pm_err_track_keyed[]; /* "NW4R:Failed assertion *ptr == 0xac" */
extern const char ef_pm_err_track_rotate[]; /* "NW4R:Failed assertion kind == AC_TARGET_ROTATE" */
extern const char ef_pm_err_post_field_name[]; /* "...postfieldResource->mChildOption.mNameIdx < ..." */
extern const char ef_pm_err_parent[]; /* "NW4R:Pointer Error\nparent(=%p) is not valid pointer." */
extern const char ef_pm_err_resource_arg[]; /* "NW4R:Pointer Error\nresource(=%p) is not valid pointer." */
extern const char ef_pm_err_manager_em[]; /* "...\nmManagerEM(=%p) ..." */
extern const char ef_pm_err_manager_ef[]; /* "...\nmManagerEM->mManagerEF(=%p) ..." */
extern const char ef_pm_err_manager_es[]; /* "...\nmManagerEM->mManagerEF->mManagerES(=%p) ..." */
extern const char ef_pm_err_draw_strategy_builder[]; /* "...\n...->mDrawStrategyBuilder(=%p) ..." */
extern const char ef_pm_err_resource[]; /* "NW4R:Pointer Error\nmResource(=%p) is not valid pointer." */
extern const char ef_pm_err_emitter_desc[]; /* "NW4R:Pointer Error\ned(=%p) is not valid pointer." */
extern const char ef_pm_err_draw_strategy[]; /* "NW4R:Pointer Error\nmDrawStrategy(=%p) is not valid pointer." */
extern const char ef_pm_err_mtx_local_to_emitter[]; /* "NW4R:Pointer Error\nmtxLocalToEmitter(=%p) is not valid pointer." */
extern const char ef_pm_err_mtx_local_to_global[]; /* "NW4R:Pointer Error\nmtxLocalToGlobal(=%p) is not valid pointer." */
extern const char ef_pm_err_pp[]; /* "NW4R:Pointer Error\npp(=%p) is not valid pointer." */
extern const char ef_pm_err_color_pri[]; /* "NW4R:Pointer Error\ncolorPri(=%p) is not valid pointer." */
extern const char ef_pm_err_color_sec[]; /* "NW4R:Pointer Error\ncolorSec(=%p) is not valid pointer." */
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
struct EfPmParticleHead {
    /* +0x00 */ u8 pad_0x00[0x0C];
    /* +0x0C */ s32 state;            /* 1 = created (ad/retire), 3 = retired */
    /* +0x10 */ u8 pad_0x10[0x0C];
}; /* size: 0x1C */

struct EfPmParticle : EfPmParticleHead {
    /* +0x1C: the particle class's vtable pointer; slot +0x10 initialises a freshly allocated particle (by-value
     * position and velocity), nonzero on success.  Declared only. */
    virtual void slot_0x08();
    virtual void slot_0x0C();
    virtual s32 Initialize(u16 life, nw4r::math::VEC3 pos, nw4r::math::VEC3 vel, EfPmManager* manager, s32 param0,
                           f32 scale, s32 param1, s32 param2);
    /* +0x20 */ u8 animParams[0x20];  /* the block the curve tracks write at their target offset */
    /* +0x40 */ nw4r::math::VEC3 rotate; /* the signed-curve target (track target 0x20) */
    /* +0x4C */ u8 pad_0x4C[0x4A];
    /* +0x96 */ u8 texTypeBits;        /* the texture layers' key types, two bits each */
    /* +0x97 */ u8 pad_0x97[0x09];
    /* +0xA0 */ nw4r::math::VEC3 velocity;
    /* +0xAC */ nw4r::math::VEC3 accel; /* the position the velocity moves */
    /* +0xB8 */ nw4r::math::VEC3 prevPos; /* the position at the start of the frame */
    /* +0xC4 */ f32 accelScale;       /* the factor every added acceleration is scaled by */
    /* +0xC8 */ EfPmManager* manager; /* the ParticleManager that owns this particle */
    /* +0xCC */ u8 pad_0xCC[0x0C];
    /* +0xD8 */ s32 retireFlag;       /* distinguished by value 1 and 3 in the frame walk; 1 once calculated */
    /* +0xDC */ u16 age;              /* frames lived; 0 on the creation frame */
    /* +0xDE */ u16 seed;             /* the particle's random seed for its curve tracks */
    /* +0xE0 */ u16 lifeTime;         /* the age at which the particle retires */
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
    /* +0x1C */ s32 activeCount;      /* the last particle the frame walk calculated (cleared by the ager) */
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
    /* +0x00 */ u16 flags;     /* bit 10: the manager is not drawn */
    /* +0x02 */ u8 pad_0x02[0x75];
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

/* The emitter descriptor ef_res_emitter_desc resolves a manager's resource to: +0x94 holds its flags (bit 10: not
 * drawn) and the start of the setter block fn_800AC0E4 forwards, +0x140 the draw type. */
struct EfPmEmitterDesc {
    /* +0x000 */ u8 pad_0x000[0x94];
    /* +0x094 */ EfPmSetterParams setter;
    /* +0x124 */ u8 pad_0x124[0x1C];
    /* +0x140 */ u8 drawType;
}; /* size: 0x141 */

/* The emitter's draw setting as the particle manager reads it: the depth offset at +0xB4. */
struct EfPmDrawSetting {
    /* +0x00 */ u8 pad_0x00[0xB4];
    /* +0xB4 */ f32 depthOffset;
}; /* size: 0xB8 */

/* The effect system's memory manager: its table's slot +0x4C allocates a particle (the slots before it are declared
 * only to place it; the class's table is not this unit's). */
struct EfMemoryManager {
    /* +0x00: the vtable pointer */
    virtual void slot_0x08();
    virtual void slot_0x0C();
    virtual void slot_0x10();
    virtual void slot_0x14();
    virtual void slot_0x18();
    virtual void slot_0x1C();
    virtual void slot_0x20();
    virtual void slot_0x24();
    virtual void slot_0x28();
    virtual void slot_0x2C();
    virtual void slot_0x30();
    virtual void slot_0x34();
    virtual void slot_0x38();
    virtual void slot_0x3C();
    virtual void slot_0x40();
    virtual void slot_0x44();
    virtual void slot_0x48();
    virtual EfPmParticle* AllocParticle();
}; /* size: 0x04 */

/* The draw-strategy builder (`nw4r::ef::DrawStrategyBuilder`, defined elsewhere): its first virtual hands out the
 * strategy for a draw type. */
class EfPmDrawStrategyBuilder {
public:
    virtual nw4r::ef::DrawStrategy* GetDrawStrategy(u32 drawType) = 0;
}; /* size: 0x04 */

/* The effect system as the particle manager reads it: its draw-strategy builder at +0x08. */
struct EfPmEffectSystem {
    /* +0x00 */ u8 pad_0x00[0x08];
    /* +0x08 */ EfPmDrawStrategyBuilder* drawStrategyBuilder;
}; /* size: 0x0C */

/* The calc hooks an effect may carry: called with the manager, its list and the first particle of the walk. */
typedef void (*EfPmCalcHook)(EfPmManager* manager, EfPmList* list, EfPmParticle* first);

/* The effect an emitter belongs to: its effect system at +0x20, the two calc hooks at +0x4C/+0x50. */
struct EfPmEffect {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ EfPmEffectSystem* system;
    /* +0x24 */ u8 pad_0x24[0x28];
    /* +0x4C */ EfPmCalcHook preCalc;
    /* +0x50 */ EfPmCalcHook postCalc;
}; /* size: 0x54 */

/* The three-vec block used by the particle transform paths. */
struct EfPmVecBlock {
    /* +0x00 */ nw4r::math::VEC3 a;
    /* +0x0C */ nw4r::math::VEC3 b;
    /* +0x18 */ nw4r::math::VEC3 c;
}; /* size: 0x24 */

/* The emitter fields the particle manager reads: its flags (+0x20, bit 1: hidden), its effect and its
 * random block. */
struct EfPmEmitterView {
    /* +0x000 */ u8 pad_0x000[0x20];
    /* +0x020 */ u32 flags;          /* bit 1: hidden; bit 2: the emitter time never ends */
    /* +0x024 */ u8 pad_0x024[0x18];
    /* +0x03C */ u16 emitTime;       /* the emitter's end frame */
    /* +0x03E */ u8 pad_0x03E[0x7E];
    /* +0x0BC */ EfPmEffect* effect;
    /* +0x0C0 */ u8 pad_0x0C0[0x24];
    /* +0x0E4 */ u32 tick;           /* the emitter's frame counter */
    /* +0x0E8 */ u8 pad_0x0E8[0x02];
    /* +0x0EA */ u16 seed;
    /* +0x0EC */ u32 random;
    /* +0x0F0 */ u8 pad_0x0F0[0x24];
    /* +0x114 */ nw4r::math::VEC3 tailOrigin; /* the point the tail field pulls toward (NaN: none) */
}; /* size: 0x120 */

/* A particle curve track as the calc reads it (the animation curve record `ef/ef_animcurve.cpp` evaluates):
 * its kind byte (0xAB baked, 0xAC keyed), its target offset, its type, its flags and its block sizes. */
struct EfPmTrack {
    /* +0x00 */ u8 kind;
    /* +0x01 */ u8 target;
    /* +0x02 */ u8 type;            /* 0 u8, 2 post field, 3 f32, 4 pattern, 5 child, 6 rotation, 7 field */
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ u8 flags;           /* bit 3: skipped; bit 4: driven by the emitter's frame */
    /* +0x05 */ u8 pad_0x05[0x07];
    /* +0x0C */ u32 keyBytes;
    /* +0x10 */ u32 randomBytes;
    /* +0x14 */ u32 randomTableBytes;
    /* +0x18 */ u32 nameTableBytes;
    /* +0x1C */ u32 dataBytes;      /* the size of the record past the tables (a field or post-field block) */
}; /* size: 0x20 */

/* A field track's 0x1C-byte record: the space its result is taken in, where it is added, and the field's own
 * parameters (the power at +0x04). */
struct EfPmFieldRecord {
    /* +0x00 */ u8 space;          /* 0 manager space, 1 emitter space, 3 manager space scaled by the emitter */
    /* +0x01 */ u8 addTo;          /* 0 the velocity, 1 the position */
    /* +0x02 */ u8 pad_0x02[0x02];
    /* +0x04 */ f32 power;
    /* +0x08 */ u8 params[0x14];
}; /* size: 0x1C */

/* A post-field track's record: the field's transform (copied out), the collision info `ef/ef_postfield.cpp`
 * reads, the index of its spawned effect, and the optional wrap region. */
struct EfPmPostField {
    /* +0x00 */ EfPmVecBlock transform;
    /* +0x24 */ u8 info[0x22];
    /* +0x46 */ u16 effectIndex;   /* into the name table that follows the record's key tables */
    /* +0x48 */ u8 wrapFlags;      /* bit 0: wrap the particle into the region; bit 1: centre it on the emitter */
    /* +0x49 */ u8 pad_0x49[0x03];
    /* +0x4C */ nw4r::math::VEC3 wrapScale;
    /* +0x58 */ nw4r::math::VEC3 wrapRotate;
    /* +0x64 */ nw4r::math::VEC3 wrapTranslate;
}; /* size: 0x70 */

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
extern "C" u16 ef_emres_num_ptcl_track_init(void* self);
extern "C" void fn_800AC0E4(EfPmManager* self, EfPmSetterParams* p);
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

/* 0x800ABA6C (0x678): table slot 4: binds the manager to its emitter and resource, resets its state and picks
 * the draw strategy the resource's draw type asks for. */
extern "C" s32 ef_pm_initialize(EfPmManager* self, EfEmitterManager* parent, EfResource* resource) {
#line 109
    NW4R_POINTER_ASSERT(lbl_80592F78, parent, ef_pm_err_parent);
    NW4R_POINTER_ASSERT(lbl_80592F78, resource, ef_pm_err_resource_arg);
    ef_ref_object_init(self);
    ef_activity_list_clear(&self->list);
    self->stateA.aa.a = lbl_807960A8;
    self->stateA.aa.b = lbl_807960A8;
    self->stateA.aa.vec.x = ef_pm_f32_zero;
    self->stateA.aa.vec.y = ef_pm_f32_zero;
    self->stateA.aa.vec.z = ef_pm_f32_zero;
    self->managerEM = parent;
    ef_ref_object_add_ref((EffectManager*)parent);
    self->resource = resource;
    self->flags = 0;
    ef_pm_set_mtx_dirty(self);
    EfPmEmitterDesc* desc = (EfPmEmitterDesc*)ef_res_emitter_desc(self->resource);
#line 132
    NW4R_POINTER_ASSERT(lbl_80592F78, self->managerEM, ef_pm_err_manager_em);
    NW4R_POINTER_ASSERT(lbl_80592F78, ((EfPmEmitterView*)self->managerEM)->effect, ef_pm_err_manager_ef);
    NW4R_POINTER_ASSERT(lbl_80592F78, ((EfPmEmitterView*)self->managerEM)->effect->system, ef_pm_err_manager_es);
#line 135
    NW4R_POINTER_ASSERT(lbl_80592F78, ((EfPmEmitterView*)self->managerEM)->effect->system->drawStrategyBuilder, ef_pm_err_draw_strategy_builder);
    EfPmDrawStrategyBuilder* builder = ((EfPmEmitterView*)self->managerEM)->effect->system->drawStrategyBuilder;
    self->drawStrategy = builder->GetDrawStrategy(desc->drawType);
    self->list.activeCount = 0;
    fn_800AC0E4(self, &desc->setter);
    return 1;
}

/* Forward a parameter block to the manager's own setter. */
extern "C" void fn_800AC0E4(EfPmManager* self, EfPmSetterParams* p) {
    fn_800AC100(self, p->mode, &p->colorPri, &p->colorSec, p->scale, &p->vec);
}

extern "C" void fn_800AC100(EfPmManager* self, u8 a, void* b, void* c, f32 f, const nw4r::math::VEC3* d);

/* 0x800AC1BC (0x13C): allocates a particle from the effect system's memory manager, initialises it with
 * copies of `pos` and `vel`, extends its life and puts it on the manager's list. */
extern "C" EfPmParticle* ef_pm_create_particle(EfPmManager* self, u16 life, const nw4r::math::VEC3* pos,
                                     const nw4r::math::VEC3* vel, s32 param0, f32 scale, s32 param1, s32 param2,
                                     u16 lifeAdd) {
    EfMemoryManager* mm = ef_system_memory_manager(((EfPmEmitterView*)self->managerEM)->effect->system);
    EfPmParticle* p = mm->AllocParticle();
    if (p == NULL) {
        return NULL;
    }
    if (p->Initialize(life, *pos, *vel, self, param0, scale, param1, param2) == 0) {
        return NULL;
    }
    p->life += lifeAdd;
    ef_activity_list_add(&self->list, p);
    p->state = 1;
    p->field_0xE4 = 0;
    p->field_0xE5 = ef_random_u16(&((EfPmEmitterView*)self->managerEM)->random);
    return p;
}

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

/* Returns the list's last element (the previous of none), or null. */
/* untyped: caller-owned payload - the list holds elements of any type */
extern "C" void* ef_list_get_last(const nw4r::ut::List* list) {
    return nw4r::ut::List_GetPrev(list, NULL);
}

/* The resource-table accessors: the table's count, its first array base and its third u16. */
extern "C" u16 ef_emres_num_ptcl_track(void* self) {
    (void)self;
    return *(u16*)ef_emres_get_ptcl_track();
}

extern "C" u16 ef_emres_num_ptcl_track_init(void* self) {
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
    if ((ed->setter.flags & 0x400) == 0) {
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

/* 0x800AE698 (0xC): returns the matrix a particle's position is taken to world space with. */
extern "C" nw4r::math::MTX34* ef_pm_particle_mtx(EfPmManager* manager) {
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

/* 0x800AC2F8 (0xDCC): table slot 6, one frame of every particle not yet calculated: the effect's pre-calc hook,
 * the space matrices, then per particle its curve tracks (values, patterns, children, fields, post fields), the
 * velocity and position step, the post field and the wrap region; the post-calc hook last. */
extern "C" void ef_pm_calc(EfPmManager* self) {
    EfPmParticle* first = (EfPmParticle*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->list,
                                                                  (void*)self->list.activeCount);
    if (first == NULL) {
        return;
    }
    EfPmParticle* p = first;
#line 225
    NW4R_POINTER_ASSERT(lbl_80592F78, self->managerEM, ef_pm_err_manager_em);
    NW4R_POINTER_ASSERT(lbl_80592F78, ((EfPmEmitterView*)self->managerEM)->effect, ef_pm_err_manager_ef);
    if (((EfPmEmitterView*)self->managerEM)->effect->preCalc != NULL) {
        ((EfPmEmitterView*)self->managerEM)->effect->preCalc(self, &self->list, first);
    }

    nw4r::math::MTX34 pmMtx;
    MTX34_ctor(&pmMtx);
    ef_pm_get_mtx(self, &pmMtx);
    nw4r::math::MTX34 pmInv;
    MTX34_ctor(&pmInv);
    mtx34_inverse(&pmInv, &pmMtx);
    nw4r::math::MTX34 emMtx;
    MTX34_ctor(&emMtx);
    ef_emitter_get_mtx((EfDrawEmitter*)self->managerEM, &emMtx);
    nw4r::math::VEC3 axisSum;
    setVec3(&axisSum, emMtx.m[0][2] + (emMtx.m[0][0] + emMtx.m[0][1]),
            emMtx.m[1][2] + (emMtx.m[1][0] + emMtx.m[1][1]), emMtx.m[2][2] + (emMtx.m[2][0] + emMtx.m[2][1]));
    f32 emScale = ef_pm_f32_inv_sqrt3 * vec3_len(&axisSum.x);
    nw4r::math::MTX34 localToEm;
    MTX34_ctor(&localToEm);
    mtx34_concat(&localToEm, &pmInv, &emMtx);
    nw4r::math::MTX34 emToLocal;
    MTX34_ctor(&emToLocal);
    mtx34_inverse(&emToLocal, &localToEm);
    nw4r::math::MTX34 pmRot;
    ef_mtx34_copy(&pmRot, &pmInv);
    pmRot.m[0][3] = ef_pm_f32_zero;
    pmRot.m[1][3] = ef_pm_f32_zero;
    pmRot.m[2][3] = ef_pm_f32_zero;
    nw4r::math::MTX34 pmRotInv;
    MTX34_ctor(&pmRotInv);
    mtx34_inverse(&pmRotInv, &pmRot);
    nw4r::math::MTX34 emToLocalRot;
    ef_mtx34_copy(&emToLocalRot, &emToLocal);
    emToLocalRot.m[0][3] = ef_pm_f32_zero;
    emToLocalRot.m[1][3] = ef_pm_f32_zero;
    emToLocalRot.m[2][3] = ef_pm_f32_zero;
    nw4r::math::MTX34 localToEmRot;
    ef_mtx34_copy(&localToEmRot, &localToEm);
    localToEmRot.m[0][3] = ef_pm_f32_zero;
    localToEmRot.m[1][3] = ef_pm_f32_zero;
    localToEmRot.m[2][3] = ef_pm_f32_zero;

    while (p != NULL) {
        EfPmParticle* next = *(EfPmParticle**)((u8*)p + self->list.linkOffset + 4);
        if (p->state == 1 && p->retireFlag == 0) {
            p->retireFlag = 1;
            if (p->life != 0) {
                ef_effect_set_calc_flag(((EfPmEmitterView*)self->managerEM)->effect, 1);
            }
            nw4r::math::VEC3 pos;
            nw4r::math::VEC3 vel;
            nw4r::math::VEC3 moveDir;
            EfDrawParticleManager* handle;
            assignVec3((Vec*)&pos, (Vec*)&p->accel);
            assignVec3((Vec*)&vel, (Vec*)&p->velocity);
            VEC3_ctor(&moveDir);
            ef_particle_get_move_dir((EfDrawParticle*)p, &moveDir);
            copyVec3(&p->prevPos, &p->accel);
            ef_pm_handle(&handle, self);
            if ((ef_emitter_tex_flags(&handle)[3] & 1) == 0 && p->lifeTime <= p->age) {
                fn_800AB880(self, p);
            } else {
                nw4r::math::VEC3 addVel;
                nw4r::math::VEC3 addPos;
                EfPmVecBlock pfTransform;
                EfPmPostField* pf;
                EffectHandle* pfEffect;
                BOOL hasPostField;
                u16 i;
                setVec3(&addVel, ef_pm_f32_zero, ef_pm_f32_zero, ef_pm_f32_zero);
                setVec3(&addPos, ef_pm_f32_zero, ef_pm_f32_zero, ef_pm_f32_zero);
                hasPostField = FALSE;
                fn_800ADE98(&pfTransform);
                pfEffect = NULL;
                if (p->age == 0) {
                    i = 0;
                } else {
                    i = ef_emres_num_ptcl_track_init(self->resource);
                }
                for (; (u16)i < ef_emres_num_ptcl_track(self->resource); i++) {
                    EfPmTrack* track = (EfPmTrack*)ef_emres_get_ptcl_track_at(self->resource, (u16)i);
                    u32 tick;
                    u32 mode;
                    u32 seed;
                    if ((track->flags & 8) != 0) {
                        continue;
                    }
                    if ((track->flags & 0x10) != 0) {
                        EfPmEmitterView* em = (EfPmEmitterView*)p->manager->managerEM;
                        tick = em->tick;
                        if ((em->flags & 4) != 0) {
                            mode = 0xFFFFFFFF;
                        } else {
                            mode = em->emitTime;
                        }
                        seed = em->seed;
                    } else {
                        tick = p->age;
                        mode = p->lifeTime;
                        seed = p->seed;
                    }
                    if ((u8)(track->kind + 0x55) > 1) {
                        continue;
                    }
                    u8 target = track->target;
                    switch (track->type) {
                    case 4: {
                        const u8* nameTable;
                        u32* channel;
                        EfAnimDivider divider;
                        u8* flags;
                        if (track->kind != 0xAC) {
                            nw4r::db::Panic(lbl_80592F78, 0x181, ef_pm_err_track_keyed);
                        }
                        ef_pm_handle(&handle, self);
                        flags = ef_emitter_tex_flags(&handle);
                        ef_anim_curve_texture((const u8*)track, (EfAnimParticle*)p, tick, seed, mode, &nameTable, &channel, &divider);
                        if ((flags[2] & 1) != 0) {
                            ef_anim_latch_tex_type((EfAnimParticle*)p, &divider);
                        } else {
                            p->texTypeBits &= (u8)~(3 << (divider.mChannel * 2));
                            p->texTypeBits |= (u8)((divider.mType & 3) << (divider.mChannel * 2));
                        }
                        if ((flags[3] & 2) != 0) {
                            ef_anim_tex_ramp((EfAnimParticle*)p, &divider, (const EfAnimRamp*)flags, self, (const EfAnimNameTable*)nameTable, channel);
                        }
                        break;
                    }
                    case 5:
                        if (track->kind != 0xAC) {
                            nw4r::db::Panic(lbl_80592F78, 0x198, ef_pm_err_track_keyed);
                        }
                        ef_anim_curve_child((const u8*)track, (EfAnimParticle*)p, tick, seed, mode);
                        break;
                    case 6:
                        if (target != 0x20) {
                            nw4r::db::Panic(lbl_80592F78, 0x19D, ef_pm_err_track_rotate);
                        }
                        if (track->kind != 0xAC) {
                            nw4r::db::Panic(lbl_80592F78, 0x19E, ef_pm_err_track_keyed);
                        }
                        ef_anim_curve_rotate((const u8*)track, &p->rotate.x, tick, seed, mode);
                        break;
                    case 0:
                        ef_anim_curve_u8((const u8*)track, p->animParams + target, tick, seed, mode);
                        break;
                    case 3:
                        ef_anim_curve_f32((const u8*)track, (f32*)(p->animParams + target), tick, seed, mode);
                        break;
                    case 7: {
                        EfPmFieldRecord field;
                        nw4r::math::VEC3 rel;
                        nw4r::math::VEC3 out;
                        memcpy(&field,
                               (u8*)track + track->keyBytes + track->randomBytes + track->randomTableBytes +
                                   track->nameTableBytes + sizeof(EfPmTrack),
                               sizeof(EfPmFieldRecord));
                        if (track->keyBytes != 0) {
                            ef_anim_curve_f32((const u8*)track, &field.power, tick, seed, mode);
                        }
                        VEC3_ctor(&rel);
                        if (target == 6 || (u8)(target + 0xFE) <= 2) {
                            nw4r::math::VEC3 emPos;
                            ef_pm_calc_emitter_pos(&emPos, field.space == 1, &emToLocal, &pmMtx, &emMtx, &pos);
                            copyVec3(&rel, &emPos);
                        }
                        setVec3(&out, ef_pm_f32_zero, ef_pm_f32_zero, ef_pm_f32_zero);
                        switch (target) {
                        case 1:
                            fn_800ADA24(&out, (EfPmDirParam*)&field, &vel);
                            break;
                        case 0:
                            ef_field_gravity(&out, (EfPmFieldRotation*)&field);
                            break;
                        case 7:
                            ef_field_random(&out, (EfPmFieldRandomParam*)&field, (EfPmFieldHeader*)track,
                                            (EfPmFieldOwner*)track, p, tick, seed, &vel, &moveDir, &emToLocalRot);
                            break;
                        case 6:
                            ef_field_spin(&out, (EfPmFieldRotation*)&field, &rel);
                            break;
                        case 2:
                            ef_field_magnet(&out, (EfPmFieldPoint*)&field, &rel);
                            break;
                        case 3:
                            ef_field_newton(&out, (EfPmFieldNewtonParam*)&field, &rel);
                            break;
                        case 4:
                            ef_field_vortex(&out, (EfPmFieldVortexParam*)&field, &rel);
                            break;
                        case 8: {
                            EfPmEmitterView* em = (EfPmEmitterView*)self->managerEM;
                            if (ef_pm_f32_nan != em->tailOrigin.x) {
                                nw4r::math::MTX34 m;
                                nw4r::math::VEC3 origin;
                                MTX34_ctor(&m);
                                ef_emitter_get_mtx((EfDrawEmitter*)self->managerEM, &m);
                                setVec3(&origin, m.m[0][3], m.m[1][3], m.m[2][3]);
                                PSVECSubtract(&out.x, &origin.x, &((EfPmEmitterView*)self->managerEM)->tailOrigin.x);
                                fn_800513F0(&out, field.power);
                            }
                            break;
                        }
                        }
                        switch (field.space) {
                        case 1:
                            mtx34_mult_vec3(&out, &localToEmRot, &out);
                            break;
                        case 0:
                            mtx34_mult_vec3(&out, &pmRot, &out);
                            break;
                        case 3:
                            fn_800513F0(&out, emScale);
                            mtx34_mult_vec3(&out, &pmRot, &out);
                            break;
                        }
                        switch (field.addTo) {
                        case 0:
                            addVec3To(&addVel, &out);
                            break;
                        case 1:
                            addVec3To(&addPos, &out);
                            break;
                        }
                        break;
                    }
                    case 2:
                        if (track->dataBytes != 0) {
                            u8* names = (u8*)track + track->keyBytes + sizeof(EfPmTrack) + track->randomBytes +
                                        track->randomTableBytes;
                            pf = (EfPmPostField*)(names + track->nameTableBytes);
                            memcpy(&pfTransform, pf, sizeof(EfPmVecBlock));
                            if (track->nameTableBytes > 4) {
                                if (!(pf->effectIndex < *(u16*)names)) {
                                    nw4r::db::Panic(lbl_80592F78, 0x226, ef_pm_err_post_field_name);
                                }
                                pfEffect = ((EffectHandle**)(names + 4))[pf->effectIndex];
                            }
                        }
                        if (track->keyBytes != 0) {
                            ef_anim_curve_f32((const u8*)track, (f32*)((u8*)&pfTransform + target), tick, seed, mode);
                        }
                        hasPostField = TRUE;
                        break;
                    }
                }

                for (;;) {
                if (hasPostField) {
                    nw4r::math::VEC3 newVel;
                    u8 killed;
                    assignVec3((Vec*)&newVel, (Vec*)&p->velocity);
                    addVec3To(&newVel, &addVel);
                    killed = 0;
                    s32 moved = ef_pf_calc_particle((EfDrawParticle*)p, (EfPostFieldTransform*)&pfTransform,
                                                    (EfPostFieldInfo*)pf, pfEffect, &emMtx, &pmInv, &pos,
                                                    addPos, &newVel, &killed);
                    if (killed != 0) {
                        fn_800AB880(self, p);
                        break;
                    }
                    if (moved != 0) {
                        copyVec3(&p->velocity, &newVel);
                        fn_800AD0CC(p, &addPos);
                        fn_800AD0CC(p, &p->velocity);
                    }
                    if ((pf->wrapFlags & 1) != 0) {
                        nw4r::math::MTX34 region;
                        nw4r::math::MTX34 rot;
                        nw4r::math::MTX34 regionInv;
                        nw4r::math::VEC3 local;
                        BOOL wrapped;
                        MTX34_ctor(&region);
                        mtx34_identity(&region);
                        ef_mtx34_scale_columns(region.m[0], &pf->wrapScale.x, region.m[0]);
                        MTX34_ctor(&rot);
                        ef_mtx34_rotate_xyz(rot.m[0], pf->wrapRotate.x, pf->wrapRotate.y, pf->wrapRotate.z);
                        mtx34_concat(&region, &rot, &region);
                        mtx34_trans_apply(&region, &pf->wrapTranslate, &region);
                        if ((pf->wrapFlags & 2) != 0) {
                            nw4r::math::VEC3 emPos;
                            setVec3(&emPos, emMtx.m[0][3], emMtx.m[1][3], emMtx.m[2][3]);
                            mtx34_trans_apply(&region, &emPos, &region);
                        }
                        mtx34_concat(&region, &pmInv, &region);
                        MTX34_ctor(&regionInv);
                        mtx34_inverse(&regionInv, &region);
                        VEC3_ctor(&local);
                        mtx34_mult_vec3(&local, &regionInv, &p->accel);
                        wrapped = FALSE;
                        if (local.x > lbl_807960A8) {
                            local.x = fmodf(lbl_807960A8 + local.x, ef_pm_f32_two) - lbl_807960A8;
                            wrapped = TRUE;
                        } else if (local.x < ef_pm_f32_minus_one) {
                            local.x = lbl_807960A8 + fmodf(local.x - lbl_807960A8, ef_pm_f32_two);
                            wrapped = TRUE;
                        }
                        if (local.y > lbl_807960A8) {
                            local.y = fmodf(lbl_807960A8 + local.y, ef_pm_f32_two) - lbl_807960A8;
                            wrapped = TRUE;
                        } else if (local.y < ef_pm_f32_minus_one) {
                            local.y = lbl_807960A8 + fmodf(local.y - lbl_807960A8, ef_pm_f32_two);
                            wrapped = TRUE;
                        }
                        if (local.z > lbl_807960A8) {
                            local.z = fmodf(lbl_807960A8 + local.z, ef_pm_f32_two) - lbl_807960A8;
                            wrapped = TRUE;
                        } else if (local.z < ef_pm_f32_minus_one) {
                            local.z = lbl_807960A8 + fmodf(local.z - lbl_807960A8, ef_pm_f32_two);
                            wrapped = TRUE;
                        }
                        if (wrapped) {
                            mtx34_mult_vec3(&p->accel, &region, &local);
                        }
                    }
                } else {
                    addVec3To(&p->velocity, &addVel);
                    fn_800AD0CC(p, &addPos);
                    fn_800AD0CC(p, &p->velocity);
                }
                p->age++;
                break;
                }
            }
        }
        p = next;
    }

    self->list.activeCount = (s32)ef_list_get_last((nw4r::ut::List*)&self->list);
    if (((EfPmEmitterView*)self->managerEM)->effect->postCalc != NULL) {
        ((EfPmEmitterView*)self->managerEM)->effect->postCalc(self, &self->list, first);
    }
}

/* --------------------------------------------------------------------------------------------- */
/* Not yet reconstructed                                                                          */
/* --------------------------------------------------------------------------------------------- */

/* 0x800AE6A8 (0x764): multiplies a particle's two colours by the manager's colour, faded toward the
 * secondary colour with the distance to the manager's point in mode 2. */
extern "C" void ef_pm_modulate_color(EfPmManager* self, EfPmParticle* pp, u8* colorPri, u8* colorSec) {
#line 848
    NW4R_POINTER_ASSERT(lbl_80592F78, pp, ef_pm_err_pp);
    NW4R_POINTER_ASSERT(lbl_80592F78, colorPri, ef_pm_err_color_pri);
    NW4R_POINTER_ASSERT(lbl_80592F78, colorSec, ef_pm_err_color_sec);
    switch (self->stateA.b.dirMode) {
    case 1:
        colorPri[0] = (colorPri[0] * self->stateA.b.colorPri[0] + 0x80) >> 8;
        colorPri[1] = (colorPri[1] * self->stateA.b.colorPri[1] + 0x80) >> 8;
        colorPri[2] = (colorPri[2] * self->stateA.b.colorPri[2] + 0x80) >> 8;
        colorPri[3] = (colorPri[3] * self->stateA.b.colorPri[3] + 0x80) >> 8;
        colorSec[0] = (colorSec[0] * self->stateA.b.colorPri[0] + 0x80) >> 8;
        colorSec[1] = (colorSec[1] * self->stateA.b.colorPri[1] + 0x80) >> 8;
        colorSec[2] = (colorSec[2] * self->stateA.b.colorPri[2] + 0x80) >> 8;
        colorSec[3] = (colorSec[3] * self->stateA.b.colorPri[3] + 0x80) >> 8;
        break;
    case 2:
        if (self->stateA.b.scale < ef_pm_f32_epsilon) {
            colorPri[0] = (colorPri[0] * self->stateA.b.colorPri[0] + 0x80) >> 8;
            colorPri[1] = (colorPri[1] * self->stateA.b.colorPri[1] + 0x80) >> 8;
            colorPri[2] = (colorPri[2] * self->stateA.b.colorPri[2] + 0x80) >> 8;
            colorPri[3] = (colorPri[3] * self->stateA.b.colorPri[3] + 0x80) >> 8;
            colorSec[0] = (colorSec[0] * self->stateA.b.colorPri[0] + 0x80) >> 8;
            colorSec[1] = (colorSec[1] * self->stateA.b.colorPri[1] + 0x80) >> 8;
            colorSec[2] = (colorSec[2] * self->stateA.b.colorPri[2] + 0x80) >> 8;
            colorSec[3] = (colorSec[3] * self->stateA.b.colorPri[3] + 0x80) >> 8;
        } else {
            nw4r::math::VEC3 world;
            nw4r::math::MTX34* mtx = ef_pm_particle_mtx(pp->manager);
            f32 dist;
            VEC3_ctor(&world);
            mtx34_mult_vec3(&world, mtx, &pp->accel);
            PSVECSubtract(&world.x, &world.x, &self->stateA.b.vec.x);
            dist = vec3_len(&world.x);
            if (dist > self->stateA.b.scale) {
                colorPri[0] = (colorPri[0] * self->stateA.b.colorPri[0] + 0x80) >> 8;
                colorPri[1] = (colorPri[1] * self->stateA.b.colorPri[1] + 0x80) >> 8;
                colorPri[2] = (colorPri[2] * self->stateA.b.colorPri[2] + 0x80) >> 8;
                colorPri[3] = (colorPri[3] * self->stateA.b.colorPri[3] + 0x80) >> 8;
                colorSec[0] = (colorSec[0] * self->stateA.b.colorPri[0] + 0x80) >> 8;
                colorSec[1] = (colorSec[1] * self->stateA.b.colorPri[1] + 0x80) >> 8;
                colorSec[2] = (colorSec[2] * self->stateA.b.colorPri[2] + 0x80) >> 8;
                colorSec[3] = (colorSec[3] * self->stateA.b.colorPri[3] + 0x80) >> 8;
            } else {
                s32 t = (s32)(ef_pm_f32_256 * dist / self->stateA.b.scale);
                u16 r = (self->stateA.b.colorSec[0] << 8) + t * (self->stateA.b.colorPri[0] - self->stateA.b.colorSec[0]);
                u16 g = (self->stateA.b.colorSec[1] << 8) + t * (self->stateA.b.colorPri[1] - self->stateA.b.colorSec[1]);
                u16 b = (self->stateA.b.colorSec[2] << 8) + t * (self->stateA.b.colorPri[2] - self->stateA.b.colorSec[2]);
                u16 a = (self->stateA.b.colorSec[3] << 8) + t * (self->stateA.b.colorPri[3] - self->stateA.b.colorSec[3]);
                colorPri[0] = (colorPri[0] * r + 0x80) >> 16;
                colorPri[1] = (colorPri[1] * g + 0x80) >> 16;
                colorPri[2] = (colorPri[2] * b + 0x80) >> 16;
                colorPri[3] = (colorPri[3] * a + 0x80) >> 16;
                colorSec[0] = (colorSec[0] * r + 0x80) >> 16;
                colorSec[1] = (colorSec[1] * g + 0x80) >> 16;
                colorSec[2] = (colorSec[2] * b + 0x80) >> 16;
                colorSec[3] = (colorSec[3] * a + 0x80) >> 16;
            }
        }
        break;
    }
}
