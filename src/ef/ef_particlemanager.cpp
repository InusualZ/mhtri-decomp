/*
 * ef/ef_particlemanager.cpp - the nw4r::ef::ParticleManager bookkeeping object: its constructor `fn_800AB664`
 *   (installs the table `lbl_805934E0`, slots 2-7 = fn_800AB730, fn_800AB73C, fn_800ABA6C, fn_800AC1BC,
 *   fn_800AC2F8, fn_800ADED8), the object's lifecycle, and the static initializer `fn_800AEE14`, which constructs
 *   the file's two static matrices.
 * RANGE. .text 0x800AB658-0x800AEE48 (44 functions); extab 0x80009E6C-0x80009F74, extabindex 0x80023148-0x800232D4,
 *   .ctors 0x8056F2DC-0x8056F2E0, .data 0x80592F78-0x80593580 (the `__FILE__` string "ef_particlemanager.cpp"
 *   first), .bss 0x80694538-0x80694598, .sdata2 0x807960A0-0x807960E8.
 * FLAGS. `cflags_main`; `#pragma fp_contract off` around one body.
 * NAMES. The map has only `fn_` stems for the range.
 *   GUESS (from the body and its callers): `ef_pm_handle`, `ef_pm_get_mtx`, `ef_pm_modulate_color`.
 * RESIDUALS. 15 rows unwritten (empty bodies): 0x800ABA6C-0x800AC0E4, 0x800AC1BC-0x800AD0C4,
 *   0x800AD254-0x800AD514, 0x800AD520-0x800AD9C0, 0x800AD9CC-0x800ADA24, 0x800ADA5C-0x800ADDCC,
 *   0x800ADED8-0x800AE298, 0x800AE2A4-0x800AE500, 0x800AE628-0x800AE698, 0x800AE6A8-0x800AEE0C.  The source order
 *   differs from retail's, so `.text`, extab and extabindex run in another order.
 *   5 partial rows:
 *  - `fn_800AB664`: the table store computes into r3 (`addi r3, r3, ...@l`) where retail uses r0, and
 *    `fn_800AC178`: `extsh.` where retail keeps `extsh` + `cmpwi` (both the peephole pass's folds);
 *  - `fn_800AC0E4`, `fn_800AC100`: the argument copies take other registers and are scheduled differently;
 *  - `fn_800ADDCC`: the index scaling folds to `clrlslwi` where retail keeps `clrlwi` + `slwi`.
 *   relocdiff: our `.ctors` word carries the symbol `lbl_8056F2DC`, retail's none.
 *   flipcheck: `.bss`, `.data` and `.sdata2` claimed, not emitted (`.sdata2` is a partial pool: flipcheck names a
 *   fold with `ef/ef_particle.cpp`, one shared literal); `.text` 0x990 of 0x37F0; extab 0x90 of 0x108; extabindex
 *   0xD8 of 0x18C.
 *   Relocation names that differ from retail (pool constants, save helpers, statics): `fn_805013FC`,
 *     `MTX34RotAxisFIdx__Q24nw4r4mathFPQ34nw4r4math5MTX34PCQ34nw4r4math4VEC3f`.
 *   flipcheck: referenced but defined by nothing a flip can use: `fn_805013FC`, `fn_80501C80`.
 *   `ef_pm_get_mtx` is unwritten (an empty body).
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
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */
#include "vec3_scale.h" /* vec3_scale (rule 2) */

namespace nw4r { namespace db { void Panic(const char* file, int line, const char* fmt, ...); } }
namespace nw4r { namespace math { f32 SinFIdx(f32); } }

/* The assert strings, the class table and the pool constants of this unit's claimed data, declared, never defined. */
extern "C" {
extern const f32 lbl_807960A4;
extern const f32 lbl_807960A8;
extern const f32 lbl_807960C0;
extern const char lbl_80592F78[]; /* "ef_particlemanager.cpp" */
extern const char lbl_80592F90[]; /* "NW4R:Pointer Error\ntarget(=%p) is not valid pointer." */
extern const char lbl_80592FC8[]; /* "NW4R:Failed assertion target->mParticleManager == this" */
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
void fn_80051424(void* dst, const void* src, f32 f);
void fn_800513F0(void* dst, f32 f);
void mtx34_mult_vec3(void* dst, const void* a, const void* b);
s32 ef_get_life_status(void* self);
u8* fn_800A8BF8();
void fn_80501C80(void* self, s32 v);
void fn_805013FC(void* a, void* b, f32 f);
}

void operator delete(void* p) throw();

/* --------------------------------------------------------------------------------------------- */
/* Types                                                                                          */
/* --------------------------------------------------------------------------------------------- */

struct EfPmManager;

/* A particle on a manager's active list. Only the fields this unit touches are named; the link is
 * addressed through the list's own `linkOffset`, so no fixed link field is declared. */
struct EfPmParticle {
    /* +0x00 */ u8 pad_0x00[0x0C];
    /* +0x0C */ s32 state;            /* 1 = created (ad/retire), 3 = retired */
    /* +0x10 */ u8 pad_0x10[0xB8];
    /* +0xC8 */ EfPmManager* manager; /* the ParticleManager that owns this particle */
    /* +0xCC */ u8 pad_0xCC[0x0C];
    /* +0xD8 */ s32 retireFlag;       /* distinguished by value 1 and 3 in the frame walk */
    /* +0xDC */ u8 pad_0xDC[0x06];
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
    /* +0x00 */ s8 dirMode;
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
    /* +0x2C */ void* drawStrategy;
    /* +0x30 */ u8 pad_0x30[0x08];
    /* +0x38 */ EfPmList list;
    /* +0x58 */ EfPmStateA stateA;
    /* +0x88 */ f32 scale;
    /* +0x8C */ nw4r::math::VEC3 pos;
    /* +0x98 */ u32 field_0x98;
    /* +0x9C */ u32 field_0x9C;
    /* +0xA0 */ u8 pad_0xA0[0x0C];
    /* +0xAC */ nw4r::math::VEC3 accel;
    /* +0xB8 */ u8 pad_0xB8[0x0C];
    /* +0xC4 */ f32 accelScale;
}; /* size: 0xC8 */

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
extern "C" u8* fn_800ADE50(void* self);
extern "C" u16 fn_800AD230(void* self);
extern "C" void fn_800AC100(EfPmManager* self, u8 a, void* b, void* c, const nw4r::math::VEC3* d, f32 f);
extern "C" void fn_800AEE14();

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
    MTX34_ctor((MTX34*)&self->pos);
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
    fn_800AC100(self, p->mode, &p->colorPri, &p->colorSec, &p->vec, p->scale);
}

extern "C" void fn_800AC100(EfPmManager* self, u8 a, void* b, void* c, const nw4r::math::VEC3* d, f32 f);

/* The setter fn_800AC0E4 forwards to. */
extern "C" void fn_800AC100(EfPmManager* self, u8 a, void* b, void* c, const nw4r::math::VEC3* d, f32 f) {
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
extern "C" void fn_800AD0C4(void* self) {
    fn_80501C80(self, 0);
}

/* The resource-table accessors: the table's count, its first array base and its third u16. */
extern "C" u16 fn_800AD230(void* self) {
    (void)self;
    return *(u16*)fn_800A8BF8();
}

extern "C" u16 fn_800ADE74(void* self) {
    (void)self;
    return *(u16*)(fn_800A8BF8() + 2);
}

extern "C" u8* fn_800ADE50(void* self) {
    (void)self;
    return fn_800A8BF8() + 4;
}

/* Bounds-checked resource lookup (index into the emitter resource table). */
extern "C" u8* fn_800ADDCC(void* self, u32 index) {
    u8* base = fn_800ADE50(self);
    u16 count = fn_800AD230(self);
    if ((u16)index >= count)
        nw4r::db::Panic(lbl_8059356C, 0x29E, lbl_80593540);
    return ((u8**)base)[(u16)index];
}

#pragma fp_contract off
/* Add a scaled direction into the manager's accumulated acceleration. */
extern "C" nw4r::math::VEC3* fn_800AD0CC(EfPmManager* self, nw4r::math::VEC3* add) {
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
extern "C" s32 fn_800ADA24(void* out, EfPmDirParam* p, void* v) {
    fn_80051424(out, v, p->rate - lbl_807960A8);
    return 1;
}

/* Set the manager's scale and copy a source position into the live block. */
extern "C" void fn_800AE298(EfPmManager* self, f32 f, const nw4r::math::VEC3* src) {
    self->scale = f;
    copyVec3(&self->pos, src);
}

/* Walk the list, aging every particle. */
extern "C" void fn_800AE500(EfPmManager* self, s32 arg1) {
    self->list.activeCount = 0;
    EfPmParticle* node = self->list.head;
    while (node != NULL) {
        if (arg1 == 0 || node->life != 0) {
            u16 life = node->life;
            if (life != 0)
                node->life = (u16)(life - 1);
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

/* --------------------------------------------------------------------------------------------- */
/* Not yet reconstructed                                                                          */
/* --------------------------------------------------------------------------------------------- */

extern "C" void fn_800ABA6C() {}
extern "C" void fn_800AC1BC() {}
extern "C" void fn_800AC2F8() {}
extern "C" void fn_800AD254() {}
extern "C" void fn_800AD384() {}
extern "C" void fn_800AD420() {}
extern "C" void fn_800AD47C() {}
extern "C" void fn_800AD520() {}
extern "C" void fn_800AD9CC() {}
extern "C" void fn_800ADA5C() {}
extern "C" void fn_800ADED8() {}
extern "C" void fn_800AE2A4() {}
extern "C" void ef_pm_get_mtx(void* target, MTX34* out) { (void)target; (void)out; }
extern "C" void fn_800AE628() {}
extern "C" void ef_pm_modulate_color() {}
