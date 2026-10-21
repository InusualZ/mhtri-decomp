/*
 * ef/ef_emitter.cpp - nw4r::ef::Emitter: its constructor, sub-object constructor and deleting destructor, the
 *   particle-manager sweeps and retire paths, the emitter manager's destroy and set-params hooks (delegate slots at
 *   manager+0xA0), the initialiser, the random generator (`state * 0x343FD + 0x269EC3`), the eight virtuals
 *   (`SendClosing`, `DestroyFunc`, `Initialize`, `CreateEmitter`, `CalcEmitter`, `CalcParticle`, `CalcEmission`,
 *   `CalcBillboard`), the frame's create pass, the distance fade and per-frame spawner, the resource's track
 *   walkers, the transform chain and the accessors.
 * RANGE. .text 0x800A6258-0x800A99B4 (52 functions); extab 0x80009CBC-0x80009DF4, extabindex 0x80022EC0-0x80023094,
 *   .data 0x80592850-0x80592CD0 (the strings in first-use order, `Emitter`'s table at 0x80592BA8, then the four
 *   header asserts' weak strings), .sbss 0x80794920-0x80794928 (`ef_pm_mtx_cache_id`), .sdata2
 *   0x80796000-0x80796030 (100, 0, the two conversion doubles, -1, 1, 2, 65536, pi/2).
 * FLAGS. `cflags_main`; `#pragma peephole off` from the first definition (retail keeps the flag booleanisations
 *   `rlwinm; neg; or; srwi`, `clrlwi` + `slwi` index scaling and the `li r0,N; psq_lx` epilogues), back on for the
 *   last definition `fn_800A98D4` (retail folds its byte store's `clrlwi`); file-wide `#pragma pool_data off` (each
 *   string its own `lis`/`addi`); `#pragma dont_inline` around the constructor (the base constructor stays a call).
 * NAMES. The virtuals are nw4r's `Emitter` slots (GUESS: the parameter types and which Calc is which, read from the
 *   table at 0x80592BA8 and the bodies).  The map has only `fn_` stems for the rest but GUESS names from the bodies:
 *   `ef_store_word`, `ef_truncate_float`, `ef_random_float` and `ef_res_block_body`.  The object's containers are
 *   named by its asserts: its own particle-manager list at +0xC0 (`NW4R_EF_MAX_PARTICLEMANAGER`, 0x400) and the
 *   emitter manager's list at manager+0x24 (`NW4R_EF_MAX_EMITTER`, 0x200); `mManagerEF` is at +0xBC, the work record
 *   at +0xB8, the parent at +0xF4 and the object's MTX34 at +0x124.
 *   GUESS (from the body and its callers): `ef_particle_get_move_dir`, `ef_emitter_tex_flags`,
 *   GUESS: `ef_emitter_get_mtx`.
 *   GUESS (the emitter resource's track blocks, from `ef/ef_resource.cpp`'s relocation): `ef_emres_num_emit_track`,
 *   GUESS: `ef_emres_get_ptcl_track`, `ef_emres_get_emit_track_tbl`.
 *   GUESS: `ef_vec3_dist_sq` (0x800A8300): the squared distance of two positions.
 *   GUESS: `ef_random_u16` (0x800A6E70): steps the random block and returns its high half.
 *   GUESS: `ef_pm_set_mtx_dirty` (0x800A96C0): marks a particle manager's matrix for rebuilding.
 *   GUESS: `ef_pm_mtx_cache_id` (0x80794920): the id of the manager whose matrix was built last.
 *   GUESS: `ef_mtx34_copy` (0x800A89A0): copies the twelve words of a 3x4 matrix and returns the destination.
 *   GUESS: `ef_calc_inherit_mtx` (0x800A90AC): builds the transform a child inherits (scale, rotation, a share of
 *   the translation) from its parent's matrix; the parent chain and the particle manager call it.
 *   GUESS: `EfManagerCallback::SetParams`/`Destroy` (the callback's first two virtuals, from the hooks that call them).
 * RESIDUALS. none: every row and the data sections are byte-identical (flipcheck READY).
 * SHAPES. `Emitter` derives from `nw4r::ef::ReferencedObject` (`ef/ef_effect.h`), so the constructor calls the base's
 *   implicit constructor (its weak copy is dropped for `ef/ef_effect.cpp`'s) and stores this unit's table; the
 *   memory manager's particle-manager pool (`GetParticleManager`, slot +0x38) and the particle manager's
 *   `Initialize` (slot +0x10) are virtual calls through r12.
 * SHAPES. The header accessors (the resource's track walkers, the effect hooks `fn_800A66AC`/`fn_800A723C`, the life
 *   setter `fn_800A66A4`, `ef_particle_get_move_dir`) are C-linkage `inline` functions marked
 *   `__attribute__((never_inline))`, defined at the top in their headers' order (res_emitter.h, effect.h,
 *   particle.h): their weak copies follow their first caller in reverse order of use, and their assert strings are
 *   the `@STRING@` tail in reverse definition order, as retail's.
 * SHAPES. The pointer guards are the `NW4R_POINTER_ASSERT` six-BOOL chain taking the file string
 *   ("ef_emitter.cpp", "particle.h" or "effect.h"); `#line` reproduces each assert's line.
 * SHAPES. The 0x1020/0x820 frames are `NW4R_EF_MAX_PARTICLEMANAGER`/`NW4R_EF_MAX_EMITTER`-wide stack arrays
 *   (`fn_800A6350`, `DestroyFunc`, `fn_800A6938`, `CalcParticle`); the list size is an `s32` local (retail narrows
 *   it straight into the argument register).
 * SHAPES. The emitter manager's hooks call its callback object (`EfManagerCallback`, a class declared here) and the
 *   form is `nw4r::ef::EmitterForm` (`Emission` virtual) built by `nw4r::ef::EmitterFormBuilder::Create`.
 * SHAPES. The track walkers index the resource bytes (`&p[n * 8 + 4]`): a subscript puts the offset first in the
 *   `add`, a pointer sum puts the base first.
 * SHAPES. The spawner's countdown adds a `(u16)` float conversion (no narrowing mask); `ef_vec3_dist_sq` passes
 *   both conversions as arguments (evaluated right to left); `fn_800A7750` takes the blended scale from an inline
 *   product helper (its arguments evaluate right to left too), with the emitter's step an `s8` and the mode a `u8`
 *   read through `(s8)` and the last two cases one conditional expression (the step's load then takes retail's r23).
 * SHAPES. The strings are literals and the `.sdata2` constants literal floats (the conversion doubles are the
 *   compiler's), in retail's creation order.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef/ef_particlemanager.h" /* fn_800AB9F4 / fn_800AE360 are that unit's (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */
#include "ef/ef_util.h" /* ef_mtx34_column_length, owned by ef_util.cpp's range (rule 2) */
#include "draw_shape/mtx34_copy.h" /* mtx34_copy, owned by draw_shape.cpp's range (rule 2) */
#include "ef/ef_random.h" /* nw4r::ef::Random, seeded through its Srand (rule 1) */
#include "nw4r/db_assert.h" /* nw4r::db::Panic / Warning and NW4R_ASSERT (rule 2) */
#include "ef/ef_emform.h" /* nw4r::ef::EmitterForm, the emitter's form (rule 1) */
#include "nw4r/math_types.h" /* nw4r::math::MTX34Trans (rule 2) */
#include "nw4r/ut_list.h" /* nw4r::ut::List_GetNext / List_GetNth (rule 2) */
#include "ef/ef_effectsystem.h" /* EfSysMemoryManager, the pool the particle managers come from (rule 1) */
#include "ef/ef_effect.h" /* nw4r::ef::ReferencedObject and the activity-list helpers (rule 1, rule 2) */

extern "C" {


/* Callees outside this unit, declared locally with C linkage. */
u16 fn_800A4AF0(void* list);                       /* UtlistSize */
u16 fn_8009B374(void* list, void** buf, u16 size); /* UtlistGetArray */
void* ef_list_get_first(void* list);                     /* GetFirst */
s32 ef_get_life_status(void* node);
void fn_800A45DC(void* list, void* node);
void fn_800A4A1C(void* list, void* node);
void fn_800A49B8(void* node);
void fn_800A4474(void* manager, void* self);
void fn_800A486C(void* manager, void* emitter);
u32 ef_emres_get_name(void* p);
void* fn_800A4654(void* manager, void* em, u8 flag, s32 mode);
void ef_ref_object_add_ref(void* p);
void fn_800A3390(void* dst, const void* src);
void fn_800A3800(void* p);
void* fn_800A5484(void* p);
void* fn_800A60C0(void* manager);
void fn_800A52E4(void* manager, void* cb, void* arg, s32 flag, void* self);
void ef_pm_handle(void* dst, const void* src);
void fn_8035B998(void* p);
void ef_anim_curve_f32(void* rec, void* target, u32 life, u16 seed, s32 range);
void* ef_resource_instance(void);

void fn_8009BF08(void* vec, void* out);
void fn_8009C040(void* vec, void* out);
void fn_8009B650(void* vec, void* mtx);
void fn_8009BCB4(void* mtx, void* vec);
void ef_mtx34_scale_columns(void* out, void* mtx, void* in);
void fn_8009CCAC(void* out, void* mtx, void* in);
f32 sqrt_f32(f32 x);
void subVec3(void* dst, void* a, void* b);
void* fn_800508AC(void* vec);
f32 PSVECSquareDistance(void* a, void* b);
void mtx34_identity(void* mtx);
void vec3_scale_by(void* dst, void* src, f32 scale);
void mtx34_mult_vec3(void* dst, void* mtx, void* vec);
void mtx34_copy_ps(void* dst, void* src);
void mtx34_concat(void* dst, void* a, void* b);
void mtx34_inverse(void* mtx, void* in);
void ef_mtx34_rotate_xyz(void* mtx, f32 x, f32 y, f32 z);
void fn_8009CBA0(void* dst, void* mtx, void* vec);
f32 ceilf(f32 x);
s32 ef_vec3_normalize_to(void* a, void* b);
}

/* -------------------------------------------------------------------------------------------------
 * Types.  Every record states its size; the offsets are the ones the bodies load or store.
 * ------------------------------------------------------------------------------------------------- */

/* The engine vector (the same layout as nw4r's, but the engine's own library type). */
typedef nw4r::math::VEC3 EfVec; /* size: 0x0C */

/* The `nw4r::ut::List` head: the walkers start at `head` and follow the runtime link offset. */
typedef struct EfList {
    /* +0x00 */ void* head;
    /* +0x04 */ void* tail;
    /* +0x08 */ u16 offset;
    /* +0x0A */ u16 size;
    /* +0x0C */ u16 linkOffset;
    /* +0x0E */ u16 field_0x0E;
    /* +0x10 */ void* field_0x10;
    /* +0x14 */ void* field_0x14;
} EfList; /* size: 0x18 */

/* The parameter record the object is initialised from (`em` in fn_800A6A04's body).  Only the
 * offsets this unit reads are named; the record continues past them. */
typedef struct EfEmitterWork {
    /* +0x000 */ u32 flags;      /* the object's +0x20 */
    /* +0x004 */ u32 field_0x04; /* the object's +0x24 */
    /* +0x008 */ u16 life;       /* the life cap */
    /* +0x00A */ u16 field_0x0A;
    /* +0x00C */ s8 field_0x0C;
    /* +0x00D */ s8 scale_step;  /* per-frame scale step, in hundredths */
    /* +0x00E */ s8 rate;        /* per-frame spawn rate, in hundredths */
    /* +0x00F */ s8 scale_range; /* the scale's random range, in hundredths */
    /* +0x010 */ f32 scale;
    /* +0x014 */ u16 field_0x14;
    /* +0x016 */ u16 field_0x16;
    /* +0x018 */ u16 interval;
    /* +0x01A */ u8 mode;
    /* +0x01B */ s8 alpha;
    /* +0x01C */ f32 vec_0x1C[3];
    /* +0x028 */ f32 vec_0x28[3];
    /* +0x034 */ u16 field_0x34;
    /* +0x036 */ u8 field_0x36;
    /* +0x037 */ u8 field_0x37;
    /* +0x038 */ f32 float_0x38[7];
    /* +0x054 */ EfVec vec_0x54;
    /* +0x060 */ EfVec vec_0x60;
    /* +0x06C */ EfVec vec_0x6C;
    /* +0x078 */ EfVec vec_0x78;
    /* +0x084 */ u8 color_r; /* the three channels, in hundredths */
    /* +0x085 */ u8 color_g;
    /* +0x086 */ u8 color_b;
    /* +0x087 */ u8 field_0x87;
    /* +0x088 */ u32 field_0x088; /* its low half seeds the object's random block */
} EfEmitterWork; /* size: 0x8C (lower bound: the record continues past it) */

/* The parameter block `fn_800A3390` copies into the object's +0xFC. */
typedef struct EfEmitterParam {
    /* +0x00 */ s16 field_0x00;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 field_0x04;
    /* +0x05 */ u8 field_0x05;
    /* +0x06 */ u8 field_0x06;
    /* +0x07 */ u8 field_0x07;
    /* +0x08 */ u8 pad_0x08[0x04];
    /* +0x0C */ nw4r::math::VEC3 vec_0x0C;
    /* +0x18 */ nw4r::math::VEC3 vec_0x18;
} EfEmitterParam; /* size: 0x24 (lower bound: the highest byte fn_800A3390 copies) */

/* The effect-side resource block the manager keeps at `managerEF + 0x20`.  Its random block is at
 * +0xC01C, the reference position at +0xC020, a matrix at +0xC02C and two range floats at
 * +0xC05C/+0xC060.  It continues past them. */
typedef struct EfEffectData {
    /* +0x0000 */ u8 pad_0x0000[0x0C];
    /* +0x000C */ nw4r::ef::EmitterFormBuilder* builder; /* maps a shape id to its emitter form */
    /* +0x0010 */ u8 pad_0x0010[0xC00C];
    /* +0x0C1C */ u32 random;
    /* +0x0C20 */ EfVec ref_pos;
    /* +0x0C2C */ nw4r::math::MTX34 ref_mtx;
    /* +0x0C5C */ f32 range_a;
    /* +0x0C60 */ f32 range_b;
    /* +0x0C64 */ u8 pad_0x0C64[0x04];
} EfEffectData; /* size: 0xC68 (lower bound) */

/* The callback object the emitter manager keeps at +0xA0 (its table pointer at +0x00): the manager's
 * set-params and destroy hooks call its first two virtuals.  Declared only, so this unit emits no table. */
class EfManagerCallback {
public:
    virtual void SetParams(struct EfEmitterManager* manager, struct EfPmView* pm);
    /* untyped: opaque handle - the emitter resource handle, passed through */
    virtual void Destroy(struct EfEmitterManager* manager, void* eh);
}; /* size: 0x04 (lower bound: the table pointer) */

/* The emitter manager: the emitters live on its activity list at +0x24 and the effect resource it
 * was made from at +0x20. */
typedef struct EfEmitterManager {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ EfEffectData* effect;
    /* +0x24 */ EfList emitters; /* mActivityList.mActiveList */
    /* +0x3C */ u8 pad_0x3C[0x08];
    /* +0x44 */ void* field_0x44;
    /* +0x48 */ void* field_0x48; /* the retire-all hook the frame path calls */
    /* +0x4C */ u8 pad_0x4C[0x54];
    /* +0xA0 */ EfManagerCallback* delegate;
} EfEmitterManager; /* size: 0xA4 (lower bound) */

struct EfParticleRec;

namespace nw4r {
namespace ef {
class Emitter;
} // namespace ef
} // namespace nw4r

/* The bodies' spelling of the emitter. */
typedef nw4r::ef::Emitter EfEmitterObj;

namespace nw4r {
namespace ef {

/* The emitter: the reference-counted base (its life status at +0x0C, its table at +0x1C), the parameters copied
 * from its resource, its particle-manager list at +0xC0, the parent chain and its matrix.  Its eight virtuals are
 * this unit's; `SendClosing` is the key function, so the table is emitted here. */
class Emitter : public ReferencedObject {
public:
    Emitter();

    virtual void SendClosing();
    virtual void DestroyFunc();
    /* untyped: opaque handle - the emitter resource, passed through */
    virtual s32 Initialize(EfEmitterManager* aParentEF, void* eh, u8 value);
    /* untyped: opaque handle - the emitter resource and the host position, passed through */
    virtual Emitter* CreateEmitter(void* em, const EfEmitterParam* params, EfParticleRec* pm, u16 life_bonus,
                                   void* host);
    virtual void CalcEmitter();
    virtual void CalcParticle();
    virtual void CalcEmission();
    virtual void CalcBillboard();

    /* +0x020 */ u32 flags;
    /* +0x024 */ u32 flags2; /* a pointer to the emitter's parameter record */
    /* +0x028 */ union {
        f32 scale;
        u32 scale_flags;
    } scale_0x028;
    /* +0x02C */ f32 scale_step;
    /* +0x030 */ u16 interval;    /* the frames between spawns */
    /* +0x032 */ u16 life_frames; /* the life a spawn starts with */
    /* +0x034 */ f32 rate;
    /* +0x038 */ f32 life;
    /* +0x03C */ u16 life_max;
    /* +0x040 */ f32 color_r;
    /* +0x044 */ f32 color_g;
    /* +0x048 */ f32 color_b;
    /* +0x04C */ f32 animate_0x4C[3];
    /* +0x058 */ f32 animate_0x58[3];
    /* +0x064 */ u8 flags3;
    /* +0x065 */ s8 alpha;
    /* +0x066 */ u8 field_0x066;
    /* +0x067 */ u8 field_0x067;
    /* +0x068 */ f32 float_0x068[7];
    /* +0x084 */ EfVec vec_0x84;
    /* +0x090 */ EfVec position;
    /* +0x09C */ EfVec vec_0x9C;
    /* +0x0A8 */ EfVec rotation;
    /* +0x0B4 */ u32 field_0x0B4;
    /* +0x0B8 */ EfEmitterWork* work;
    /* +0x0BC */ EfEmitterManager* managerEF;
    /* +0x0C0 */ EfList particles; /* mActivityList.mActiveList */
    /* +0x0D8 */ u8 pad_0x0D8[0x04];
    /* +0x0DC */ u8 field_0x0DC; /* set by the initialiser; one spawn pass keeps a scaled life at 1 or more */
    /* +0x0DD */ u8 pad_0x0DD;
    /* +0x0DE */ u16 field_0x0DE; /* the spawn countdown the frame tick uses */
    /* +0x0E0 */ u16 field_0x0E0; /* the spawn countdown the spawner uses */
    /* +0x0E2 */ u8 pad_0x0E2[0x02];
    /* +0x0E4 */ u32 field_0x0E4;
    /* +0x0E8 */ u16 field_0x0E8;
    /* +0x0EA */ u16 seed;
    /* +0x0EC */ nw4r::ef::Random random;
    /* +0x0F0 */ nw4r::ef::EmitterForm* form; /* mForm: the shape whose Emission spawns the particles */
    /* +0x0F4 */ EfEmitterObj* parent;
    /* +0x0F8 */ void* orig;
    /* +0x0FC */ EfEmitterParam param;
    /* +0x120 */ u8 transform_dirty;
    /* +0x124 */ nw4r::math::MTX34 matrix;
}; /* size: 0x154 (lower bound: +0x124 is the highest field any body touches) */

} // namespace ef
} // namespace nw4r

/* The particle-manager view of the same object: the extra fields the create/sweep paths use. */
typedef struct EfPmView {
    /* +0x00 */ u8 pad_0x00[0x04];
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u32 field_0x08;
    /* +0x0C */ u8 pad_0x0C[0x14];
    /* +0x20 */ void* owner; /* the record fn_800A3800 releases */
    /* +0x24 */ void* param; /* the parameter record the manager was made from */
    /* +0x28 */ u32 flags;  /* bit0..bit2 inherited from the emitter */
    /* +0x2C */ u8 pad_0x2C[0x5C];
    /* +0x88 */ s8 field_0x88;
    /* +0x89 */ u8 field_0x89;
    /* +0x8A */ u8 dirty;
} EfPmView; /* size: 0x8B (lower bound) */

/* The length-prefixed blocks the emitter resource is walked through: `ef_res_block_body` steps 8 bytes in
 * from its own `offset`, `ef_emres_get_ptcl_track` 4. */
typedef struct EfResHeader {
    /* +0x00 */ u32 field_0x00;
    /* +0x04 */ u32 offset;
} EfResHeader; /* size: 0x08 */

/* The particle-track block `ef_emres_get_ptcl_track` returns: a count of 8-byte particle tracks, the tracks,
 * then the emitter-track head. */
typedef struct EfPtclTrackBlock {
    /* +0x00 */ u16 num_ptcl;
    /* +0x02 */ u16 pad_0x02;
    /* +0x04 */ u8 tracks[8]; /* num_ptcl 8-byte tracks; the EfEmitTrackHead follows the last */
} EfPtclTrackBlock; /* size: 0x0C (lower bound: one track) */

/* The emitter-track head after the particle tracks: the track count, the first track a running emitter
 * resumes from, and the track pointer table. */
typedef struct EfEmitTrackHead {
    /* +0x00 */ u16 num;
    /* +0x02 */ u16 first;
    /* +0x04 */ void* table[1]; /* num entries */
} EfEmitTrackHead; /* size: 0x08 (lower bound: one entry) */

typedef struct EfChainHeader {
    /* +0x00 */ u32 offset;
} EfChainHeader; /* size: 0x04 */

/* The context block fn_800A9790 carries through fn_800A9784/fn_800A52E4. */
typedef struct EfWalkCtx {
    /* +0x00 */ s32 count;
    /* +0x04 */ void* cb;
    /* +0x08 */ void* arg;
    /* +0x0C */ u8 flag;
    /* +0x0D */ u8 pad_0x0D[0x03];
} EfWalkCtx; /* size: 0x10 */

/* One emitted track record of the emitter resource: its flag byte, the emitter-relative byte offset
 * the spawner adds, and the two flag bytes the resource's array header carries. */
typedef struct EfTrack {
    /* +0x00 */ u8 field_0x00;
    /* +0x01 */ u8 offset;
    /* +0x02 */ u8 field_0x02;
    /* +0x03 */ u8 field_0x03;
    /* +0x04 */ u8 flags;
    /* +0x05 */ u8 pad_0x05[0x03];
} EfTrack; /* size: 0x08 */

/* The particle the manager walkers read (`ef/ef_particlemanager.cpp`'s EfParticleRec). */
typedef struct EfParticleRec {
    /* +0x000 */ u8 pad_0x000[0x0C];
    /* +0x00C */ s32 field_0x00C;
    /* +0x010 */ u8 pad_0x010[0x9C];
    /* +0x0AC */ EfVec offset;
    /* +0x0B8 */ EfVec position;
    /* +0x0C4 */ u8 pad_0x0C4[0x04];
    /* +0x0C8 */ void* manager;
} EfParticleRec; /* size: 0xCC (lower bound) */

/* The work record a resource-backed object is initialised from (`ef_res_emitter_desc` is its chain head). */
static inline EfEmitterWork* EfGetWork(void* p) {
    return (EfEmitterWork*)ef_res_emitter_desc(p);
}

#define NW4R_EF_MAX_PARTICLEMANAGER 0x400
#define NW4R_EF_MAX_EMITTER 0x200

/* The global `operator delete` (its compiler mangling is `__dl__FPv`; declaring that spelling would be
 * rule 9's violation - the sibling ef units spell it the same way). */
void operator delete(void* ptr) throw();

/* The +0x20 sub-object of the emitter-side object `fn_800A62C0` initialises: four transform vectors,
 * cleared in this order.  The offsets are relative to the sub-object. size: 0x94 */
typedef struct EfSysResourceSub {
    /* +0x00 */ u8 pad_0x00[0x64];
    /* +0x64 */ nw4r::math::VEC3 vec_0x84;
    /* +0x70 */ nw4r::math::VEC3 vec_0x90;
    /* +0x7C */ nw4r::math::VEC3 vec_0x9C;
    /* +0x88 */ nw4r::math::VEC3 vec_0xA8;
} EfSysResourceSub; /* size: 0x94 */

extern "C" {
EfSysResourceSub* fn_800A62C0(EfSysResourceSub* self);
void* fn_800A630C(void* self, s16 flag);
void* fn_800A3FFC(void* list, u32 linkOffset);
}
/* This unit's own symbols, in address order (the file defines them in that order). */
extern "C" {
u32 fn_800A6350(EfEmitterObj* self);
s32 fn_800A6554(EfEmitterObj* self, EfEmitterObj* target);
inline __attribute__((never_inline)) void fn_800A66A4(EfEmitterObj* self, u32 state);
inline __attribute__((never_inline)) void fn_800A66AC(EfEmitterManager* self, void* eh);
s32 fn_800A67E8(EfEmitterObj* self, EfParticleRec* target);
u32 fn_800A6938(EfEmitterObj* self);
s32 fn_800A6A04(EfEmitterObj* self, void* eh, EfEmitterManager* ef);
u32 fn_800A6E64(EfEmitterWork* work);
s32 ef_random_u16(void* random);
void fn_800A6EA4(void* random);
inline __attribute__((never_inline)) void fn_800A723C(EfEmitterManager* self, void* pm);
s32 fn_800A7750(EfEmitterObj* self, void* eh, const EfEmitterParam* params, EfParticleRec* pm,
                u16 life_bonus, void* host);
inline __attribute__((never_inline)) EfVec* ef_particle_get_move_dir(EfParticleRec* self, EfVec* result);
void* fn_800A8040(EfEmitterObj* self, void* eh, u32 a, u32 b, s8 c, u32 d, u8 e);
f32 fn_800A8220(void* a, void* b, f32 p1, f32 p2, f32 p3, f32 p4);
f32 ef_vec3_dist_sq(void* a, void* b);
void fn_800A834C(EfEmitterObj* self, EfParticleRec* pm, const nw4r::math::MTX34* mtx);
void* ef_emitter_tex_flags(void* self);
void* fn_800A8968(EfEmitterObj* self);
void* fn_800A898C(void* dst, const void* src);
void* ef_store_word(void* dst, s32 v);
nw4r::math::MTX34* ef_mtx34_copy(nw4r::math::MTX34* dst, const nw4r::math::MTX34* src);
f32 ef_truncate_float(f32 x);
f32 ef_random_float(u32* random);
inline __attribute__((never_inline)) u16 ef_emres_num_emit_track(void* res);
inline __attribute__((never_inline)) void* ef_emres_get_ptcl_track(void* res);
inline __attribute__((never_inline)) void* ef_res_block_body(void* res);
inline __attribute__((never_inline)) void* fn_800A8C34(void* res, u16 index);
inline __attribute__((never_inline)) void* ef_emres_get_emit_track_tbl(void* res);
inline __attribute__((never_inline)) u16 fn_800A8CE8(void* res);
inline __attribute__((never_inline)) u32 fn_800A8D18(EfEmitterObj* self);
void* ef_calc_inherit_mtx(void* dst, void* orig, bool a, bool b, s8 c, bool d);
void* ef_emitter_get_mtx(EfEmitterObj* self, void* out);
void fn_800A95D8(EfEmitterObj* self);
void ef_pm_set_mtx_dirty(EfPmView* pm);
u16 fn_800A9700(EfPmView* pm);
u16 fn_800A970C(EfEmitterObj* self);
void* fn_800A9714(EfEmitterObj* self, u16 index);
void fn_800A9784(EfEmitterObj* self, void* cb, void* arg, s32 flag);
void fn_800A9790(EfEmitterObj* self, EfWalkCtx* arg);
s32 fn_800A98D4(EfEmitterObj* self, void* cb, void* arg, s32 flag, s32 recurse);
}

/* -------------------------------------------------------------------------------------------------
 * Assert shapes
 * ------------------------------------------------------------------------------------------------- */

/* `NW4R_POINTER_ASSERT`'s RVL address-range check (six materialised BOOLs), shared verbatim with
 * ef/ef_particlemanager.cpp and the ef shape units; the file differs by call site. */
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

/* -------------------------------------------------------------------------------------------------
 * 0x800A6350 - retire every particle of every manager on this object's list.
 * ------------------------------------------------------------------------------------------------- */

#pragma peephole off
#pragma pool_data off

extern "C" inline __attribute__((never_inline)) void* ef_res_block_body(void* res) {
    EfResHeader* h = (EfResHeader*)res;
    return &((u8*)h)[h->offset + sizeof(EfResHeader)];
}

extern "C" inline __attribute__((never_inline)) void* ef_emres_get_ptcl_track(void* res) {
    EfChainHeader* h = (EfChainHeader*)ef_res_block_body(res);
    return &((u8*)h)[h->offset + sizeof(EfChainHeader)];
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8BC8 / 0x800A8BF8 / 0x800A8C24 / 0x800A8C34 / 0x800A8CB8 / 0x800A8CE8 - the
 * emitter-resource track walkers.
 * ------------------------------------------------------------------------------------------------- */

extern "C" inline __attribute__((never_inline)) u16 ef_emres_num_emit_track(void* res) {
    EfPtclTrackBlock* blk = (EfPtclTrackBlock*)ef_emres_get_ptcl_track(res);
    return ((EfEmitTrackHead*)&((u8*)blk)[blk->num_ptcl * 8 + 4])->num;
}

extern "C" inline __attribute__((never_inline)) void* ef_emres_get_emit_track_tbl(void* res) {
    EfPtclTrackBlock* blk = (EfPtclTrackBlock*)ef_emres_get_ptcl_track(res);
    return ((EfEmitTrackHead*)&((u8*)blk)[blk->num_ptcl * 8 + 4])->table;
}

extern "C" inline __attribute__((never_inline)) void* fn_800A8C34(void* res, u16 index) {
    void* array = ef_emres_get_emit_track_tbl(res);
#line 736
    if ((u32)index >= (u32)ef_emres_num_emit_track(res)) {
        nw4r::db::Panic("res_emitter.h", __LINE__, "NW4R:Failed assertion num < NumEmitTrack()");
    }
    return *(void**)((u8*)array + (u32)index * 4);
}

extern "C" inline __attribute__((never_inline)) u16 fn_800A8CE8(void* res) {
    EfPtclTrackBlock* blk = (EfPtclTrackBlock*)ef_emres_get_ptcl_track(res);
    return ((EfEmitTrackHead*)&((u8*)blk)[blk->num_ptcl * 8 + 4])->first;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8D18 - this object's "freed" flag.
 * ------------------------------------------------------------------------------------------------- */

extern "C" inline __attribute__((never_inline)) u32 fn_800A8D18(EfEmitterObj* self) {
    return (self->flags & 0x200) != 0;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A66AC - the emitter manager's destroy hook (delegate slot 3).
 * ------------------------------------------------------------------------------------------------- */

extern "C" inline __attribute__((never_inline)) void fn_800A66AC(EfEmitterManager* self, void* eh) {
#line 244
    NW4R_POINTER_ASSERT("effect.h", eh, "NW4R:Pointer Error\npm(=%p) is not valid pointer.");
    self->delegate->Destroy(self, eh);
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A723C - the emitter manager's set-params hook (delegate slot 2).
 * ------------------------------------------------------------------------------------------------- */

extern "C" inline __attribute__((never_inline)) void fn_800A723C(EfEmitterManager* self, void* pm) {
#line 237
    NW4R_POINTER_ASSERT("effect.h", pm, "NW4R:Pointer Error\npm(=%p) is not valid pointer.");
    self->delegate->SetParams(self, (EfPmView*)pm);
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A66A4 - set an object's state word.
 * ------------------------------------------------------------------------------------------------- */

extern "C" inline __attribute__((never_inline)) void fn_800A66A4(EfEmitterObj* self, u32 state) {
    self->mLifeStatus = state;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A7F00 - a particle's global position.
 * ------------------------------------------------------------------------------------------------- */

extern "C" inline __attribute__((never_inline)) EfVec* ef_particle_get_move_dir(EfParticleRec* self, EfVec* result) {
#line 300
    NW4R_POINTER_ASSERT("particle.h", result, "NW4R:Pointer Error\nresult(=%p) is not valid pointer.");
    {
        EfVec v;
        subVec3(&v, &self->offset, &self->position);
        copyVec3(result, &v);
    }
    return result;
}

#pragma dont_inline on
/* 0x800A6258 (0x68): builds the emitter: the base, its transform vectors, its particle-manager list and matrix. */
nw4r::ef::Emitter::Emitter() {
    fn_800A62C0((EfSysResourceSub*)&flags);
    fn_800A3FFC(&particles, 0x14);
    VEC3_ctor(&param.vec_0x0C);
    VEC3_ctor(&param.vec_0x18);
    MTX34_ctor(&matrix);
}
#pragma dont_inline reset

/* 0x800A62C0 - the resource object's +0x20 sub-object: clear its four transform vectors. */
extern "C" EfSysResourceSub* fn_800A62C0(EfSysResourceSub* self) {
    VEC3_ctor(&self->vec_0x84);
    VEC3_ctor(&self->vec_0x90);
    VEC3_ctor(&self->vec_0x9C);
    VEC3_ctor(&self->vec_0xA8);
    return self;
}

/* 0x800A630C - the resource object's deleting destructor. */
extern "C" void* fn_800A630C(void* self, s16 flag) {
    if (self != NULL && flag > 0) {
        operator delete(self);
    }
    return self;
}


extern "C" u32 fn_800A6350(EfEmitterObj* self) {
    u32 total = 0;
#line 53
    NW4R_ASSERT(fn_800A4AF0(&self->particles) < NW4R_EF_MAX_PARTICLEMANAGER, "ef_emitter.cpp", __LINE__, "NW4R:Failed assertion UtlistSize(&mActivityList.mActiveList) < NW4R_EF_MAX_PARTICLEMANAGER");
    s32 size = fn_800A4AF0(&self->particles);
    void* list[NW4R_EF_MAX_PARTICLEMANAGER];
    u16 num = fn_8009B374(&self->particles, list, size);
    for (u16 i = 0; i < num; i++) {
        total += fn_800AB9F4((struct EfPmManager*)list[i]);
    }
    return total;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6414 - the table's destroy slot.
 * ------------------------------------------------------------------------------------------------- */

void nw4r::ef::Emitter::SendClosing() {
    EfEmitterObj* self = this;
    fn_800A4474(self->managerEF, self);
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6420 - the table's retire slot: drop the owned managers, then sweep the emitter manager's
 * list for this object's children.
 * ------------------------------------------------------------------------------------------------- */

void nw4r::ef::Emitter::DestroyFunc() {
    EfEmitterObj* self = this;
    if (self->form != NULL) {
        self->form = NULL;
    }
    if ((self->flags & 1) != 0) {
        fn_800A6350(self);
    }
    fn_800A6938(self);
    if ((self->flags & 1) != 0) {
#line 88
        NW4R_ASSERT(fn_800A4AF0(&self->managerEF->emitters) < NW4R_EF_MAX_EMITTER, "ef_emitter.cpp", __LINE__, "NW4R:Failed assertion UtlistSize(&mManagerEF->mActivityList.mActiveList) < NW4R_EF_MAX_EMITTER");
        s32 size = fn_800A4AF0(&self->managerEF->emitters);
        void* list[NW4R_EF_MAX_EMITTER];
        u16 num = fn_8009B374(&self->managerEF->emitters, list, size);
        for (u16 i = 0; i < num; i++) {
            EfEmitterObj* eh = (EfEmitterObj*)list[i];
            EfEmitterObj* p = eh->parent;
            while (p != NULL) {
                if (p == self) {
                    fn_800A6350(eh);
                    if (eh->mLifeStatus == 1) {
                        fn_800A486C(self->managerEF, eh);
                    }
                    break;
                }
                p = p->parent;
            }
        }
    }
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6554 - hand a manager to the emitter manager and retire it locally.
 * ------------------------------------------------------------------------------------------------- */

extern "C" s32 fn_800A6554(EfEmitterObj* self, EfEmitterObj* target) {
#line 118
    NW4R_POINTER_ASSERT("ef_emitter.cpp", target, "NW4R:Pointer Error\ntarget(=%p) is not valid pointer.");
    fn_800A66AC(self->managerEF, target);
    fn_800A3800(((EfPmView*)target)->owner);
    fn_800A45DC(&self->particles, target);
    fn_800A66A4((EfEmitterObj*)target, 3);
    return 1;
}



/* -------------------------------------------------------------------------------------------------
 * 0x800A67E8 - retire a manager if it is on the active list.
 * ------------------------------------------------------------------------------------------------- */

extern "C" s32 fn_800A67E8(EfEmitterObj* self, EfParticleRec* target) {
#line 130
    NW4R_POINTER_ASSERT("ef_emitter.cpp", target, "NW4R:Pointer Error\ntarget(=%p) is not valid pointer.");
    if (ef_get_life_status(target) != 1) {
        return 0;
    }
    fn_800A4A1C(&self->particles, target);
    fn_800A49B8(target);
    return 1;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6938 - retire this object's managers that are on the active list.
 * ------------------------------------------------------------------------------------------------- */

extern "C" u32 fn_800A6938(EfEmitterObj* self) {
    u32 total = 0;
#line 147
    NW4R_ASSERT(fn_800A4AF0(&self->particles) < NW4R_EF_MAX_PARTICLEMANAGER, "ef_emitter.cpp", __LINE__, "NW4R:Failed assertion UtlistSize(&mActivityList.mActiveList) < NW4R_EF_MAX_PARTICLEMANAGER");
    s32 size = fn_800A4AF0(&self->particles);
    void* list[NW4R_EF_MAX_PARTICLEMANAGER];
    u16 num = fn_8009B374(&self->particles, list, size);
    for (u16 i = 0; i < num; i++) {
        void* target = list[i];
        if (ef_get_life_status(target) == 1) {
            total += fn_800A67E8(self, (EfParticleRec*)target);
        }
    }
    return total;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6A04 - initialise an object from a parameter source (`eh`) and its emitter manager (`ef`).
 * ------------------------------------------------------------------------------------------------- */

extern "C" s32 fn_800A6A04(EfEmitterObj* self, void* eh, EfEmitterManager* ef) {
#line 166
    NW4R_POINTER_ASSERT("ef_emitter.cpp", eh, "NW4R:Pointer Error\neh(=%p) is not valid pointer.");
#line 167
    NW4R_POINTER_ASSERT("ef_emitter.cpp", ef, "NW4R:Pointer Error\nef(=%p) is not valid pointer.");
    EfEmitterWork* work;
    self->field_0x0E4 = 0;
    self->mLifeStatus = 1;
    self->work = (EfEmitterWork*)eh;
    work = EfGetWork(eh);
    self->field_0x0B4 = 0;
    self->field_0x0E8 = work->field_0x16;
    self->flags = work->flags;
    self->flags2 = work->field_0x04;
    self->life_max = work->life;
    self->field_0x0DE = work->field_0x14;
    self->field_0x0E0 = 0;
    self->color_g = (f32)work->color_g / 100.0f;
    self->color_r = (f32)work->color_r / 100.0f;
    self->color_b = (f32)work->color_b / 100.0f;
    self->scale_0x028.scale = work->scale;
    self->scale_step = (f32)work->scale_range / 100.0f;
    self->interval = work->interval;
    self->life_frames = work->field_0x34;
    self->rate = (f32)work->rate / 100.0f;
    self->life = 0.0f;
    self->field_0x0DC = 1;
    copyVec3(&self->position, &work->vec_0x78);
    copyVec3(&self->vec_0x9C, &work->vec_0x60);
    copyVec3(&self->rotation, &work->vec_0x6C);
    self->flags3 = 3;
    self->alpha = 100;
    self->field_0x066 = work->field_0x36;
    self->field_0x067 = work->field_0x37;
    for (s32 i = 0; i < 7; i++) {
        self->float_0x068[i] = work->float_0x38[i];
    }
    copyVec3(&self->vec_0x84, &work->vec_0x54);
    ef_resource_instance();
    self->seed = (u16)work->field_0x088;
    if (self->seed == 0) {
        self->seed = ef_random_u16(&ef->effect->random);
    }
    self->random.Srand(self->seed);
    self->animate_0x4C[0] = work->vec_0x1C[0];
    self->animate_0x4C[1] = work->vec_0x1C[1];
    self->animate_0x4C[2] = work->vec_0x1C[2];
    self->animate_0x58[0] = work->vec_0x28[0];
    self->animate_0x58[1] = work->vec_0x28[1];
    self->animate_0x58[2] = work->vec_0x28[2];
    self->transform_dirty = 1;
    self->orig = NULL;
    {
        u32 id = fn_800A6E64(work);
        self->form = ef->effect->builder->Create(id);
    }
    self->parent = NULL;
    self->managerEF = NULL;
    return 1;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6E64 - the parameter record's byte at +0x04.
 * ------------------------------------------------------------------------------------------------- */

extern "C" u32 fn_800A6E64(EfEmitterWork* work) {
    return work->field_0x04 & 0xFF;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6E70 - step a random block and return the high half of its state.
 * ------------------------------------------------------------------------------------------------- */

extern "C" s32 ef_random_u16(void* random) {
    fn_800A6EA4(random);
    return *(u32*)random >> 16;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6EA4 - the generator's step (state = state * 0x343FD + 0x269EC3).
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800A6EA4(void* random) {
    u32 state = *(u32*)random;
    *(u32*)random = state * 0x343FD + 0x269EC3;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A6EC4 - the table's create-child slot.
 * ------------------------------------------------------------------------------------------------- */

/* untyped: opaque handle - the emitter resource, passed through */
s32 nw4r::ef::Emitter::Initialize(EfEmitterManager* aParentEF, void* eh, u8 value) {
    EfEmitterObj* self = this;
#line 246
    NW4R_POINTER_ASSERT("ef_emitter.cpp", aParentEF, "NW4R:Pointer Error\naParentEF(=%p) is not valid pointer.");
#line 247
    NW4R_POINTER_ASSERT("ef_emitter.cpp", eh, "NW4R:Pointer Error\neh(=%p) is not valid pointer.");
    ef_res_emitter_desc(eh);
    ef_ref_object_init(self);
    ef_activity_list_clear(&self->particles);
    fn_800A6A04(self, eh, aParentEF);
    self->managerEF = aParentEF;
    {
        EfSysMemoryManager* mm = (EfSysMemoryManager*)ef_system_memory_manager(aParentEF->effect);
        EfPmView* created = (EfPmView*)mm->GetParticleManager();
        if (created == NULL) {
            return 0;
        }
        if (((ParticleManager*)created)->Initialize(self, eh) == 0) {
#line 265
            nw4r::db::Panic("ef_emitter.cpp", __LINE__, "NW4R:Failed assertion false");
            return 0;
        }
        ef_ref_object_add_ref(self->managerEF);
        ef_activity_list_add(&self->particles, created);
        fn_800A66A4((EfEmitterObj*)created, 1);
        created->flags = 0;
        if ((EfGetWork(eh)->flags & 0x20) != 0) {
            created->flags |= 1;
        }
        if ((EfGetWork(eh)->flags & 0x40) != 0) {
            created->flags |= 2;
        }
        created->field_0x88 = *(s8*)((u8*)EfGetWork(eh) + 0x1A);
        if ((EfGetWork(eh)->flags & 0x400) != 0) {
            created->flags |= 4;
        }
        created->field_0x89 = value;
        fn_800A723C(self->managerEF, created);
    }
    return 1;
}


/* -------------------------------------------------------------------------------------------------
 * 0x800A7378 - the table's create-emitter slot.
 * ------------------------------------------------------------------------------------------------- */

/* untyped: opaque handle - the emitter resource and the host position, passed through */
nw4r::ef::Emitter* nw4r::ef::Emitter::CreateEmitter(void* em, const EfEmitterParam* params, EfParticleRec* pm, u16 life_bonus,
                                                     void* host) {
    EfEmitterObj* self = this;
#line 307
    NW4R_POINTER_ASSERT("ef_emitter.cpp", em, "NW4R:Pointer Error\neh(=%p) is not valid pointer.");
    if ((EfGetWork(em)->flags & 0x40000000) == 0) {
        nw4r::db::Warning("ef_emitter.cpp", 0x137, "incomplete relocation (emitter:%s)", ef_emres_get_name(em));
        return NULL;
    }
    EfEmitterObj* e = (EfEmitterObj*)fn_800A4654(self->managerEF, em, params->field_0x05, 0);
    if (e == NULL) {
        return NULL;
    }
    e->field_0x0E8 += life_bonus;
    e->parent = self;
    ef_ref_object_add_ref(e->parent);
    e->flags3 = 0;
    if ((self->flags & 0x80) != 0) {
        e->flags3 |= 1;
    }
    if ((self->flags & 0x100) != 0) {
        e->flags3 |= 2;
    }
    e->alpha = EfGetWork(self->work)->alpha;
    if ((self->flags & 0x800) != 0) {
        e->flags3 |= 4;
    }
    {
        EfVec v;
        nw4r::math::MTX34 m1, m2;
        assignVec3((Vec*)&v, (Vec*)&e->position);
        e->position.x = 0.0f;
        e->position.y = 0.0f;
        e->position.z = 0.0f;
        fn_800A95D8(e);
        MTX34_ctor(&m1);
        MTX34_ctor(&m2);
        mtx34_identity(&m1);
        ef_mtx34_rotate_xyz(&m1, e->rotation.x, e->rotation.y, e->rotation.z);
        fn_8009CBA0(&m1, &m1, &e->vec_0x9C);
        ef_emitter_get_mtx(e, &m2);
        mtx34_inverse(&m2, &m2);
        mtx34_concat(&m1, &m1, &m2);
        if (pm != NULL) {
            ef_pm_get_mtx((void*)pm->manager, &m2);
            mtx34_concat(&m1, &m1, &m2);
        }
        if (host == NULL) {
            if (pm != NULL) {
                nw4r::math::MTX34Trans(&m1, &m1, &pm->offset);
            }
        } else {
            nw4r::math::MTX34Trans(&m1, &m1, (const nw4r::math::VEC3*)host);
        }
        ef_mtx34_rotate_xyz(&m2, e->rotation.x, e->rotation.y, e->rotation.z);
        fn_8009CBA0(&m2, &m2, &e->vec_0x9C);
        mtx34_inverse(&m2, &m2);
        mtx34_concat(&m1, &m1, &m2);
        e->position.x = v.x + m1.m[0][3];
        e->position.y = v.y + m1.m[1][3];
        e->position.z = v.z + m1.m[2][3];
        fn_800A95D8(e);
    }
    if (params->field_0x00 != 0 || params->field_0x02 != 0 || params->field_0x03 != 0 ||
        params->field_0x04 != 0 || (params->field_0x07 & 2) != 0) {
        e->orig = pm;
        ef_ref_object_add_ref(pm);
        fn_800A3390(&e->param, params);
    }
    return e;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A7750 - the frame's "create from the effect's emitter table" path.
 * ------------------------------------------------------------------------------------------------- */

static inline s8 ef_scale_product(s8 a, s8 b) {
    return a * b / 100;
}

extern "C" s32 fn_800A7750(EfEmitterObj* self, void* eh, const EfEmitterParam* params, EfParticleRec* pm,
                           u16 life_bonus, void* host) {
#line 432
    NW4R_POINTER_ASSERT("ef_emitter.cpp", eh, "NW4R:Pointer Error\neh(=%p) is not valid pointer.");
    if ((EfGetWork(eh)->flags & 0x40000000) == 0) {
        nw4r::db::Warning("ef_emitter.cpp", 0x1B4, "incomplete relocation (emitter:%s)", ef_emres_get_name(eh));
        return 0;
    }
    {
        EfEmitterObj e;
        EfEmitterObj* ep;
        EfVec v;

        ef_res_emitter_desc(eh);
        fn_800A6A04(&e, eh, self->managerEF);
        e.field_0x0E8 += life_bonus;
        e.managerEF = self->managerEF;
        e.parent = self;
        e.flags3 = 0;
        if ((self->flags & 0x1000) != 0) {
            e.flags3 |= 1;
        }
        if ((self->flags & 0x2000) != 0) {
            e.flags3 |= 2;
        }
        e.alpha = EfGetWork(self->work)->scale_step;
        if ((s8)e.alpha != 0) {
            e.alpha = 100;
        }
        if ((self->flags & 0x800) != 0) {
            e.flags3 |= 4;
        }
        ep = &e;
        if ((params->field_0x07 & 1) != 0 && params->field_0x00 != 0) {
            nw4r::math::MTX34 m1, m2;
            VEC3_ctor(&v);
            MTX34_ctor(&m1);
            MTX34_ctor(&m2);
            ef_pm_get_mtx((void*)pm->manager, &m2);
            m2.m[0][3] = m2.m[1][3] = m2.m[2][3] = 0.0f;
            ef_particle_get_move_dir(pm, &v);
            mtx34_mult_vec3(&v, &m2, &v);
            if (ef_vec3_normalize_to(&v, &v) != 0) {
                if (params->field_0x00 < 0) {
                    vec3_scale_by(&v, &v, -1.0f);
                }
                ef_mtx34_from_y_axis(m1.m[0], &v.x);
                ef_mtx34_rotate_xyz(&m2, ep->rotation.x, ep->rotation.y, ep->rotation.z);
                mtx34_concat(&m1, &m1, &m2);
                fn_8009BCB4(&m1, &ep->rotation);
            }
        }
        {
            EfVec pos;
            nw4r::math::MTX34 m1, m2;
            assignVec3((Vec*)&pos, (Vec*)&ep->position);
            ep->position.x = 0.0f;
            ep->position.y = 0.0f;
            ep->position.z = 0.0f;
            fn_800A95D8(ep);
            MTX34_ctor(&m1);
            MTX34_ctor(&m2);
            mtx34_identity(&m1);
            ef_mtx34_rotate_xyz(&m1, ep->rotation.x, ep->rotation.y, ep->rotation.z);
            fn_8009CBA0(&m1, &m1, &ep->vec_0x9C);
            ef_emitter_get_mtx(ep, &m2);
            mtx34_inverse(&m2, &m2);
            mtx34_concat(&m1, &m1, &m2);
            if (pm != NULL) {
                ef_pm_get_mtx((void*)pm->manager, &m2);
                mtx34_concat(&m1, &m1, &m2);
            }
            if (host == NULL) {
                if (pm != NULL) {
                    nw4r::math::MTX34Trans(&m1, &m1, &pm->offset);
                }
            } else {
                nw4r::math::MTX34Trans(&m1, &m1, (const nw4r::math::VEC3*)host);
            }
            ef_mtx34_rotate_xyz(&m2, ep->rotation.x, ep->rotation.y, ep->rotation.z);
            fn_8009CBA0(&m2, &m2, &ep->vec_0x9C);
            mtx34_inverse(&m2, &m2);
            mtx34_concat(&m1, &m1, &m2);
            ep->position.x = pos.x + m1.m[0][3];
            ep->position.y = pos.y + m1.m[1][3];
            ep->position.z = pos.z + m1.m[2][3];
            fn_800A95D8(ep);
        }
        e.CalcEmitter();
        e.CalcBillboard();
        {
            s32 use_a;
            s32 use_b;
            s32 use_c;
            s8 scale;
            s8 a;
            u8 b;
            EfPmView* created;
            use_a = 0;
            if ((self->flags & 0x1000) != 0 && (EfGetWork(eh)->flags & 0x20) != 0) {
                use_a = 1;
            }
            use_b = 0;
            if ((self->flags & 0x2000) != 0 && (EfGetWork(eh)->flags & 0x40) != 0) {
                use_b = 1;
            }
            use_c = 1;
            if ((self->flags & 0x800) == 0 && (EfGetWork(eh)->flags & 0x400) == 0) {
                use_c = 0;
            }
            a = EfGetWork(self->work)->scale_step;
            b = EfGetWork(eh)->mode;
            if ((s8)a == 0 || (s8)b == 0) {
                scale = 0;
            } else if ((s8)a == 100) {
                scale = (s8)b;
            } else {
                scale = ((s8)b == 100) ? (s8)a : ef_scale_product(EfGetWork(self->work)->scale_step, (s8)EfGetWork(eh)->mode);
            }
            created = (EfPmView*)fn_800A8040(self, eh, (u32)use_a, (u32)use_b, scale, (u32)use_c,
                                             params->field_0x05);
            if (created == NULL) {
                EfSysMemoryManager* mm = (EfSysMemoryManager*)ef_system_memory_manager(self->managerEF->effect);
                created = (EfPmView*)mm->GetParticleManager();
                if (created == NULL) {
                    return 0;
                }
                if (((ParticleManager*)created)->Initialize(self, eh) == 0) {
#line 623
                    nw4r::db::Panic("ef_emitter.cpp", __LINE__, "NW4R:Failed assertion false");
                    return 0;
                }
                ef_activity_list_add(&self->particles, created);
                fn_800A66A4((EfEmitterObj*)created, 1);
                created->flags = 0;
                if (use_a != 0) {
                    created->flags |= 1;
                }
                if (use_b != 0) {
                    created->flags |= 2;
                }
                created->field_0x88 = scale;
                if (use_c != 0) {
                    created->flags |= 4;
                }
                created->field_0x89 = params->field_0x05;
                fn_800A723C(self->managerEF, created);
            }
            if (params->field_0x00 != 0 || params->field_0x02 != 0 || params->field_0x03 != 0 ||
                params->field_0x04 != 0 || (params->field_0x07 & 2) != 0) {
                e.orig = pm;
                ef_ref_object_add_ref(pm);
                fn_800A3390(&e.param, params);
            } else {
                e.orig = NULL;
            }
            {
                nw4r::math::MTX34 m3, m4;
                MTX34_ctor(&m3);
                MTX34_ctor(&m4);
                ef_pm_get_mtx((void*)created, &m4);
                mtx34_inverse(&m4, &m4);
                ef_emitter_get_mtx(&e, &m3);
                mtx34_concat(&m3, &m4, &m3);
                fn_800A834C(&e, (EfParticleRec*)created, &m3);
            }
            e.parent = NULL;
            e.managerEF = NULL;
            if (e.orig != NULL) {
                fn_800A3800(pm);
                e.orig = NULL;
            }
            if (self->mLifeStatus != 1 && ef_get_life_status(created) == 1) {
                fn_800A67E8(self, (EfParticleRec*)created);
            }
        }
    }
    return 1;
}


/* -------------------------------------------------------------------------------------------------
 * 0x800A8040 - sweep this object's managers for the one matching a set of flag bytes.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void* fn_800A8040(EfEmitterObj* self, void* eh, u32 a, u32 b, s8 c, u32 d, u8 e) {
#line 700
    NW4R_POINTER_ASSERT("ef_emitter.cpp", eh, "NW4R:Pointer Error\neh(=%p) is not valid pointer.");
    void* node = ef_list_get_first(&self->particles);
    while (node != NULL) {
        EfPmView* pm = (EfPmView*)node;
        if (pm->param == eh) {
            u32 flags = pm->flags;
            if (((flags & 1) != 0) == a && ((flags & 2) != 0) == b && pm->field_0x88 == c &&
                ((flags & 4) != 0) == d && pm->field_0x89 == e) {
                return node;
            }
        }
        node = nw4r::ut::List_GetNext((nw4r::ut::List*)&self->particles, node);
    }
    return NULL;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8220 - the distance fade factor.
 * ------------------------------------------------------------------------------------------------- */

extern "C" f32 fn_800A8220(void* a, void* b, f32 p1, f32 p2, f32 p3, f32 p4) {
    f32 range = p1 - p2;
    f32 d = sqrt_f32(ef_vec3_dist_sq(b, a));
    f32 x = d - p2;
    f32 lo = range * p4;
    f32 hi = range * p3;
    if (lo > x) {
        return 1.0f;
    }
    if (hi < x) {
        return 0.0f;
    }
    f32 span = hi - lo;
    if (0.0f == span) {
        return 0.0f;
    }
    return 1.0f - (x - lo) / span;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8300 - squared distance between two packed position records.
 * ------------------------------------------------------------------------------------------------- */

extern "C" f32 ef_vec3_dist_sq(void* a, void* b) {
    return PSVECSquareDistance(fn_800508AC(a), fn_800508AC(b));
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A834C - the per-frame particle spawner.
 * ------------------------------------------------------------------------------------------------- */

#pragma fp_contract off
extern "C" void fn_800A834C(EfEmitterObj* self, EfParticleRec* pm, const nw4r::math::MTX34* mtx) {
    EfEmitterWork* work;

#line 768
    NW4R_POINTER_ASSERT("ef_emitter.cpp", pm, "NW4R:Pointer Error\npm(=%p) is not valid pointer.");
    if (self->field_0x0E0 != 0) {
        self->field_0x0E0--;
        return;
    }
    self->field_0x0E0 = self->interval;
    if (0.0f != self->rate) {
        self->field_0x0E0 += (u16)ef_truncate_float(((f32)self->interval * self->rate - 1.0f) *
                                                    ef_random_float(&self->random.mState));
    }
    if ((self->flags2 & 0x20000) != 0) {
        self->life = (f32)self->life_frames;
    } else {
        f32 step;
        if (0.0f == self->scale_step) {
            step = self->scale_0x028.scale;
        } else {
            step = self->scale_0x028.scale +
                   self->scale_0x028.scale * self->scale_step *
                       (2.0f * ef_random_float(&self->random.mState) - 1.0f);
        }
        if ((self->flags2 & 0x100) != 0) {
            EfEffectData* effect = self->managerEF->effect;
            EfVec v;
            VEC3_ctor(&v);
            mtx34_mult_vec3(&v, ef_emitter_get_mtx(self, NULL), &self->position);
            step *= self->color_b +
                    (1.0f - self->color_b) *
                        fn_800A8220(&v, &effect->ref_pos, effect->range_a, effect->range_b,
                                    self->color_g, self->color_r);
        }
        self->life += step;
        if (self->field_0x0DC != 0 && 0.0f != self->scale_0x028.scale &&
            self->life < 1.0f) {
            self->life = 1.0f;
        }
    }
    if (self->life >= 1.0f) {
        ef_resource_instance();
        work = (EfEmitterWork*)ef_res_emitter_desc(self->work);
        if (self->form != NULL) {
#line 835
            NW4R_POINTER_ASSERT("ef_emitter.cpp", self->managerEF, "NW4R:Pointer Error\nmManagerEF(=%p) is not valid pointer.");
            if (self->managerEF->field_0x48 != NULL) {
                s32 life_i = (s32)self->life;
                u32 f2 = self->flags2;
                f32 anim[6];
                u16 frames;
                f32 scale;
                nw4r::math::MTX34 mc;
                anim[0] = self->animate_0x4C[0];
                anim[1] = self->animate_0x4C[1];
                anim[2] = self->animate_0x4C[2];
                anim[3] = self->animate_0x58[0];
                anim[4] = self->animate_0x58[1];
                anim[5] = self->animate_0x58[2];
                frames = work->field_0x0A;
                scale = (f32)work->field_0x0C / 100.0f;
                ef_mtx34_copy(&mc, mtx);
                ((void (*)(EfEmitterObj*, EfParticleRec*, s32*, u32*, f32*, u16*, f32*,
                           nw4r::math::MTX34*))self->managerEF->field_0x48)(self, pm, &life_i, &f2,
                                                                        anim, &frames, &scale, &mc);
                self->form->Emission((EfWork*)self, (ParticleManager*)pm, life_i, f2, anim, frames, scale, &mc);
            } else {
                self->form->Emission((EfWork*)self, (ParticleManager*)pm, (s32)self->life, self->flags2,
                                     self->animate_0x4C, work->field_0x0A, (f32)work->field_0x0C / 100.0f, mtx);
            }
            {
                s32 a;
                s32 b;
                ef_store_word(&a, 0);
                ef_pm_handle(&b, pm);
                fn_800A898C(&a, &b);
                if ((*(u8*)((u8*)ef_emitter_tex_flags(&a) + 3) & 0x20) != 0) {
                    fn_8035B998(self->managerEF->field_0x44);
                }
            }
        }
        self->life -= (f32)(s32)self->life;
    }
    if (self->field_0x0DC != 0) {
        self->field_0x0DC = 0;
    }
}

#pragma fp_contract reset

/* -------------------------------------------------------------------------------------------------
 * 0x800A8944 / 0x800A8968 - the pooled-effect accessors.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void* ef_emitter_tex_flags(void* self) {
    return fn_800A8968((EfEmitterObj*)fn_800A5484(self));
}

extern "C" void* fn_800A8968(EfEmitterObj* self) {
    return (u8*)ef_res_emitter_desc(self) + 0x8C;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A898C / 0x800A8998 / 0x800A89A0 - small pooled-record helpers.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void* fn_800A898C(void* dst, const void* src) {
    *(u32*)dst = *(const u32*)src;
    return dst;
}

extern "C" void* ef_store_word(void* dst, s32 v) {
    *(u32*)dst = (u32)v;
    return dst;
}

extern "C" nw4r::math::MTX34* ef_mtx34_copy(nw4r::math::MTX34* dst, const nw4r::math::MTX34* src) {
    *dst = *src;
    return dst;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8A04 - the spawner's truncation (a one-argument runtime call).
 * ------------------------------------------------------------------------------------------------- */

extern "C" f32 ef_truncate_float(f32 x) {
    return ceilf(x);
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8A08 - the random block stepped and normalised to 0..1.
 * ------------------------------------------------------------------------------------------------- */

extern "C" f32 ef_random_float(u32* random) {
    fn_800A6EA4(random);
    return (f32)(*random >> 16) / 65536.0f;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8A5C - the frame's relocation pass over this object's particle managers.
 * ------------------------------------------------------------------------------------------------- */

void nw4r::ef::Emitter::CalcEmitter() {
    EfEmitterObj* self = this;
    if (fn_800A8D18(self) != 0) {
        return;
    }
    if (self->mLifeStatus != 1) {
        return;
    }
    if (self->field_0x0B4 != 0) {
        return;
    }
    if (self->field_0x0DE != 0) {
        return;
    }
    {
        u32 flags = self->flags & 4;
        u32 frame;
        if (flags == 0) {
            frame = self->field_0x0E4;
            if (frame >= self->life_max) {
                fn_800A486C(self->managerEF, self);
                return;
            }
        } else {
            frame = self->field_0x0E4;
            if (frame == 0xFFFFFFFF) {
                fn_800A486C(self->managerEF, self);
                return;
            }
        }
        s32 range = flags != 0 ? -1 : (s32)self->life_max;
        s32 found = 0;
        u16 i;
        for (i = frame == 0 ? (u16)0 : fn_800A8CE8(self->work); i < ef_emres_num_emit_track(self->work); i++) {
            EfTrack* rec = (EfTrack*)fn_800A8C34(self->work, i);
            if ((rec->flags & 8) != 0) {
                continue;
            }
            if ((u8)(rec->field_0x00 + 0x55) > 1) {
                continue;
            }
            u8 off = rec->offset;
            ef_anim_curve_f32(rec, (u8*)self + off + 0x20, frame, self->seed, range);
            if (off >= 0x70) {
                found = 1;
            }
        }
        if (found != 0) {
            fn_800A95D8(self);
        }
    }
}








/* -------------------------------------------------------------------------------------------------
 * 0x800A8D30 - the table's sweep slot: call slot 6 of every manager on the active list.
 * ------------------------------------------------------------------------------------------------- */

void nw4r::ef::Emitter::CalcParticle() {
    EfEmitterObj* self = this;
    if (fn_800A8D18(self) != 0) {
        return;
    }
#line 973
    NW4R_ASSERT(fn_800A4AF0(&self->particles) < NW4R_EF_MAX_PARTICLEMANAGER, "ef_emitter.cpp", __LINE__, "NW4R:Failed assertion UtlistSize(&mActivityList.mActiveList) < NW4R_EF_MAX_PARTICLEMANAGER");
    s32 size = fn_800A4AF0(&self->particles);
    void* list[NW4R_EF_MAX_PARTICLEMANAGER];
    u16 num = fn_8009B374(&self->particles, list, size);
    for (u16 i = 0; i < num; i++) {
        ((ParticleManager*)list[i])->Calc();
    }
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8DF8 - the table's per-frame manager tick.
 * ------------------------------------------------------------------------------------------------- */

void nw4r::ef::Emitter::CalcEmission() {
    EfEmitterObj* self = this;
    if (fn_800A8D18(self) != 0) {
        return;
    }
    if (self->field_0x0B4 != 0) {
        return;
    }
    self->field_0x0B4 = 1;
    if (self->field_0x0E8 != 0) {
        ef_effect_set_calc_flag(self->managerEF, 1);
    }
    if (self->mLifeStatus == 1) {
        EfParticleRec* pm = (EfParticleRec*)ef_list_get_first(&self->particles);
        nw4r::math::MTX34 m1, m2;
        MTX34_ctor(&m1);
        MTX34_ctor(&m2);
        ef_pm_get_mtx(pm, &m2);
        mtx34_inverse(&m2, &m2);
        ef_emitter_get_mtx(self, &m1);
        mtx34_concat(&m1, &m2, &m1);
        if (self->field_0x0DE != 0) {
            self->field_0x0DE--;
            return;
        }
        fn_800A834C(self, pm, &m1);
        self->field_0x0E4++;
    } else {
        if (self->field_0x0DE != 0) {
            self->field_0x0DE--;
            return;
        }
        self->field_0x0E4++;
    }
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A8F18 - the table's rotation-from-parent slot.
 * ------------------------------------------------------------------------------------------------- */

void nw4r::ef::Emitter::CalcBillboard() {
    EfEmitterObj* self = this;
    if (self->mLifeStatus != 1 && self->mLifeStatus != 2) {
        return;
    }
    if ((self->flags2 & 0x10000) == 0 && (self->flags2 & 0x8000) == 0) {
        return;
    }
    {
        nw4r::math::MTX34 m1, m2, m3, m4;
        MTX34_ctor(&m1);
        self->rotation.x = self->rotation.y = self->rotation.z = 0.0f;
        self->transform_dirty = 1;
        ef_emitter_get_mtx(self, &m1);
        MTX34_ctor(&m2);
        mtx34_concat(&m2, &self->managerEF->effect->ref_mtx, &m1);
        ef_mtx34_copy(&m3, &m2);
        m3.m[0][0] = ef_mtx34_column_length((const f32*)&m3, 0);
        m3.m[2][0] = 0.0f;
        m3.m[1][0] = 0.0f;
        m3.m[1][1] = ef_mtx34_column_length((const f32*)&m3, 1);
        m3.m[2][1] = 0.0f;
        m3.m[0][1] = 0.0f;
        m3.m[2][2] = ef_mtx34_column_length((const f32*)&m3, 2);
        m3.m[1][2] = 0.0f;
        m3.m[0][2] = 0.0f;
        if ((self->flags2 & 0x8000) != 0) {
            MTX34_ctor(&m4);
            ef_mtx34_rotate_xyz(&m4, 1.5707964f, 0.0f, 0.0f);
            mtx34_concat(&m3, &m3, &m4);
        }
        mtx34_inverse(&m2, &m2);
        mtx34_concat(&m2, &m2, &m3);
        fn_8009CCAC(&m2, &m2, &self->rotation);
        ef_mtx34_scale_columns(&m2, &self->vec_0x9C, &m2);
        fn_8009BCB4(&m2, &self->rotation);
        fn_800A95D8(self);
    }
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A90AC - the transform combine the parent chain uses.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void* ef_calc_inherit_mtx(void* dst, void* orig, bool a, bool b, s8 c, bool d) {
#line 1124
    NW4R_POINTER_ASSERT("ef_emitter.cpp", dst, "NW4R:Pointer Error\nresult(=%p) is not valid pointer.");
#line 1125
    NW4R_POINTER_ASSERT("ef_emitter.cpp", orig, "NW4R:Pointer Error\norig(=%p) is not valid pointer.");
    if (dst == orig) {
        nw4r::db::Panic("ef_emitter.cpp", 0x466, "NW4R:Failed assertion result != orig");
    }
    if (a != 0 && b != 0 && c == 100) {
        mtx34_copy((Mtx34*)dst, (Mtx34*)orig);
        return dst;
    }
    if (a == 0 && b == 0 && c == 0) {
        mtx34_identity(dst);
        return dst;
    }
    mtx34_identity(dst);
    {
        EfVec offset;
        VEC3_ctor(&offset);
        fn_8009BF08(orig, &offset);
        if (c == 100) {
            nw4r::math::MTX34Trans((nw4r::math::MTX34*)dst, (nw4r::math::MTX34*)dst, &offset);
        } else if (c != 0) {
            EfVec pt;
            VEC3_ctor(&pt);
            vec3_scale_by(&pt, &offset, (f32)c / 100.0f);
            nw4r::math::MTX34Trans((nw4r::math::MTX34*)dst, (nw4r::math::MTX34*)dst, &pt);
        }
        if (b != 0 || a != 0) {
            if (d != 0 && c != 100) {
                nw4r::math::MTX34Trans((nw4r::math::MTX34*)dst, (nw4r::math::MTX34*)dst, &offset);
            }
            if (b != 0) {
                nw4r::math::MTX34 m;
                MTX34_ctor(&m);
                fn_8009B650(orig, &m);
                mtx34_concat(dst, dst, &m);
            }
            if (a != 0) {
                EfVec pt;
                VEC3_ctor(&pt);
                fn_8009C040(orig, &pt);
                fn_8009CBA0(dst, dst, &pt);
            }
            if (d != 0 && c != 100) {
                EfVec pt;
                VEC3_ctor(&pt);
                vec3_scale_by(&pt, &offset, -1.0f);
                nw4r::math::MTX34Trans((nw4r::math::MTX34*)dst, (nw4r::math::MTX34*)dst, &pt);
            }
        }
    }
    return dst;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A94A4 - this object's transform, built on demand.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void* ef_emitter_get_mtx(EfEmitterObj* self, void* out) {
    if (self->transform_dirty != 0) {
        nw4r::math::MTX34 m;
        MTX34_ctor(&m);
        if (self->parent == NULL) {
            mtx34_copy_ps(&self->matrix, fn_800A60C0(self->managerEF));
        } else {
            nw4r::math::MTX34 p;
            MTX34_ctor(&p);
            ef_emitter_get_mtx(self->parent, &p);
            ef_calc_inherit_mtx(&self->matrix, &p, self->flags3 & 1, self->flags3 & 2, self->alpha,
                        self->flags3 & 4);
        }
        nw4r::math::MTX34Trans(&self->matrix, &self->matrix, &self->position);
        ef_mtx34_rotate_xyz(&m, self->rotation.x, self->rotation.y, self->rotation.z);
        mtx34_concat(&self->matrix, &self->matrix, &m);
        fn_8009CBA0(&self->matrix, &self->matrix, &self->vec_0x9C);
        self->transform_dirty = 0;
    }
    if (out != NULL) {
        mtx34_copy((Mtx34*)out, &self->matrix);
        return out;
    }
    return &self->matrix;
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A95D8 - mark this object and its descendants transform-dirty.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void fn_800A95D8(EfEmitterObj* self) {
    self->transform_dirty = 1;
    {
        void* node = NULL;
        while ((node = nw4r::ut::List_GetNext((nw4r::ut::List*)&self->particles, node)) != NULL) {
            ef_pm_set_mtx_dirty((EfPmView*)node);
        }
    }
    if (self->managerEF != NULL) {
        EfEmitterObj* e = NULL;
        while ((e = (EfEmitterObj*)nw4r::ut::List_GetNext((nw4r::ut::List*)&self->managerEF->emitters, e)) != NULL) {
            if (e->transform_dirty != 0) {
                continue;
            }
            EfEmitterObj* p = e->parent;
            while (p != NULL) {
                if (p == self) {
                    e->transform_dirty = 1;
                    {
                        void* node = NULL;
                        while ((node = nw4r::ut::List_GetNext((nw4r::ut::List*)&e->particles, node)) != NULL) {
                            ef_pm_set_mtx_dirty((EfPmView*)node);
                        }
                    }
                }
                p = p->parent;
            }
        }
    }
}

/* The id of the particle manager whose matrix was last built; a manager marked dirty clears it (`.sbss`). */
s32 ef_pm_mtx_cache_id;

/* -------------------------------------------------------------------------------------------------
 * 0x800A96C0 - mark a manager's transform dirty.
 * ------------------------------------------------------------------------------------------------- */

extern "C" void ef_pm_set_mtx_dirty(EfPmView* pm) {
    pm->dirty = 1;
    if ((s32)fn_800A9700(pm) == ef_pm_mtx_cache_id) {
        ef_pm_mtx_cache_id = -1;
    }
}

/* -------------------------------------------------------------------------------------------------
 * 0x800A9700 / 0x800A970C / 0x800A9714 / 0x800A9784 / 0x800A9790 / 0x800A98D4 - the accessor block.
 * ------------------------------------------------------------------------------------------------- */

extern "C" u16 fn_800A9700(EfPmView* pm) {
    return (u16)pm->field_0x08;
}

extern "C" u16 fn_800A970C(EfEmitterObj* self) {
    return fn_800A4AF0(&self->particles);
}

extern "C" void* fn_800A9714(EfEmitterObj* self, u16 index) {
#line 1284
    if ((u32)index >= (u32)fn_800A970C(self)) {
        nw4r::db::Panic("ef_emitter.cpp", __LINE__, "NW4R:Failed assertion idx < GetNumParticleManager()");
    }
    return nw4r::ut::List_GetNth((nw4r::ut::List*)&self->particles, index);
}

extern "C" void fn_800A9784(EfEmitterObj* self, void* cb, void* arg, s32 flag) {
    fn_800A52E4(self->managerEF, cb, arg, flag, self);
}

extern "C" void fn_800A9790(EfEmitterObj* self, EfWalkCtx* arg) {
#line 1305
    NW4R_POINTER_ASSERT("ef_emitter.cpp", self, "NW4R:Pointer Error\nptr(=%p) is not valid pointer.");
    arg->count += fn_800A98D4(self, arg->cb, arg->arg, arg->flag, 0);
}

#pragma peephole on
extern "C" s32 fn_800A98D4(EfEmitterObj* self, void* cb, void* arg, s32 flag, s32 recurse) {
    s32 count = 0;
    void* node = ef_list_get_first(&self->particles);
    while (node != NULL) {
        void* next = nw4r::ut::List_GetNext((nw4r::ut::List*)&self->particles, node);
        if (flag != 0 || ef_get_life_status(node) == 1) {
            ((void (*)(void*, void*))cb)(node, arg);
            count++;
        }
        node = next;
    }
    if (recurse != 0) {
        EfWalkCtx ctx;
        ctx.count = 0;
        ctx.cb = cb;
        ctx.arg = arg;
        ctx.flag = (u8)flag;
        fn_800A9784(self, (void*)&fn_800A9790, &ctx, 1);
        count += ctx.count;
    }
    return count;
}
