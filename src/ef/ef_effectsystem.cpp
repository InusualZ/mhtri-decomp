/*
 * ef/ef_effectsystem.cpp - the effect system (`EfSys`, nw4r's `EffectSystem`; the 0xC068-byte instance the game's
 *   effect manager holds, read by `fn_800D3C0C` at +0x29074): `Initialize` (the group table of 0x1C-byte
 *   `mActivityList` records out of the system's allocator), its array helpers, the immediate retire, `CreateEffect`,
 *   `RetireEffect`, the three per-group sweeps (0x200-entry stack buffers), the reference-transform setter and two
 *   sub-object accessors; then the unit's statics - the draw-order object, the draw-strategy and emitter-form
 *   builders, the default draw info and the system instance - whose static initializer and implicit constructors
 *   the compiler emits after them.
 * RANGE. .text 0x800A56B0-0x800A6258 (20 functions); extab 0x80009C38-0x80009CBC, extabindex 0x80022E30-0x80022EC0,
 *   .ctors 0x8056F2D8-0x8056F2DC, .data 0x80592698-0x80592850 (the `__FILE__` string "ef_effectsystem.cpp" first,
 *   `DrawOrderBase`'s table last), .bss 0x80688420-0x80694538, .sbss 0x80794910-0x80794920, .sdata2
 *   0x80795FF8-0x80796000.
 * FLAGS. `cflags_main` plus `-pool off` (configure.py: the static initializer addresses each static with its own
 *   `lis`/`addi`); `#pragma peephole off` from `Initialize` on (the table stores as `lis` + `addi r0` + `stw r0`, the
 *   sweeps' unfused `clrlwi` + `slwi`); `#pragma dont_inline on` over `Initialize` (retail calls its inline helpers)
 *   and again at the end (the implicit constructors stay out of line, as retail's).
 * NAMES. The classes are nw4r's (`DrawOrder`, `DrawStrategyBuilder`, `EmitterFormBuilder`, `DrawInfo`); `EfSys`
 *   keeps this layout's own name while `ef.h` carries an `EffectSystem` stub.  The other functions keep `fn_`
 *   stems except `RetireEffect`, written as an `nw4r::ef::EffectSystem` member, and the four inline helpers
 *   after `Initialize`, named by the compiler's manglings (`Srand__Q34nw4r2ef6RandomFUl`, `__dla__FPvPv`,
 *   `__ct__17EfSysActivityListFv`, `__nwa__FUlPv`).
 *   GUESS (from the body and its callers): `ef_draw_info_projection`.
 *   GUESS: `sDrawOrder`, `sDrawStrategyBuilder`, `sEmitterFormBuilder`, `sDrawInfo` (the statics, named for their
 *   class), `ef_system_version_registered` (the once-flag the system's constructor tests).
 * RESIDUALS. 3 partial rows:
 *  - `fn_800A5D8C`, `fn_800A5E6C`, `fn_800A5F4C` (ours 0xE8 of 0xE0 each): retail keeps `groupID * 0x1C` in a
 *    callee-saved register across the `Panic` and reuses it, ours re-materialises the `mulli` per use; a group
 *    pointer local, a scaled index local, an `int` group, hoisted `num`/`i` declarations and the expression-form
 *    assert all score the same or lower.
 *   flipcheck: `.data` and `.sdata2` are byte-identical; `.text` differs in the rows above (0xBC0 of 0xBA8).
 * SHAPES. `#line` puts each assert on retail's line (0x4F, 0x50, 0x67, 0x7B, 0x83, 0x9F, 0xA0, 0xB2, 0xC9, 0xDD).
 * SHAPES. The group table is `new (allocator->Alloc(n * 0x1C + 0x20)) EfSysActivityList[n]` with the allocation
 *   as the placement argument (a named block local shares the `mulli` and swaps the loop registers); the
 *   compiler emits the size, the `__construct_new_array` try/catch and the inline helpers after the function in
 *   the reverse order of use (`Srand`, placement `delete[]`, the record constructor, placement `new[]`).
 * SHAPES. The effect object and the memory manager are classes with virtual tables (the effect's after a 0x1C-byte
 *   head, at +0x1C): `CreateEffect` calls `GetEffect`/`ReleaseEffect`/`Create` as virtual members, dispatching
 *   through r3/r12 as retail does.
 * SHAPES. The strings are global definitions and `DrawInfo`'s constants literals, so the unit emits its `.data` and
 *   `.sdata2` as retail lays them out; each assert sits on one source line (the macro takes `__LINE__` at its end).
 * SHAPES. The statics are defined in retail's construction order; the compiler's static initializer builds them and
 *   registers the instance's destructor, and the implicit constructors come out after it in the reverse order of
 *   use.  `DrawInfo`'s constructor declares its second constant after the first is stored and stores the colour
 *   bytes in retail's order.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h" /* nw4r::ef::EffectSystem / nw4r::ef::Effect (rule 9's owner) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */
#include "ef/ef_draworder.h" /* nw4r::ef::DrawOrder (rule 1) */
#include "ef/ef_emform.h" /* nw4r::ef::EmitterFormBuilder (rule 1) */
#include "ef/ef_drawstrategyimpl.h" /* nw4r::ef::DrawStrategyBuilder (rule 1) */
#include "ef/ef_effectsystem.h" /* EfSys, the unit's own header */
#include "MSL/new.h" /* the placement array forms */

namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
void Warning(const char* file, int line, const char* fmt, ...);
} // namespace db
} // namespace nw4r

/* ===================================================================================================
 * Types.  The `EfSys` prefix keeps this unit's views apart from the sibling units' (`EfList`, ...).
 * =================================================================================================== */


/* The effect object's non-polymorphic head: MWCC places the vtable pointer after it, at +0x1C. */
struct EfSysEffectHead {
    /* +0x00 */ u8 pad_0x00[0x0C];
    /* +0x0C */ s32 state; /* 1 = active, 2 = created, 3 = retired */
    /* +0x10 */ void* field_0x10;
    /* +0x14 */ u8 pad_0x14[0x08];
}; /* size: 0x1C */

struct EfEmitterResource; /* the emitter resource `CreateEffect` is handed (opaque here) */

/* The effect object the memory manager's pool hands out (`RetireEffect` asserts `mManagerES == this` on
 * it, `CreateEffect` puts it on a group list).  +0x24..+0x3F is present in the object but untouched by
 * this range's bodies, hence the padding.  Its table (+0x1C) is declared only. */
class EfSysEffect : public EfSysEffectHead {
public:
    /* +0x1C: the vtable pointer */
    virtual void Destroy();
    virtual void Retire();
    virtual u32 Create(EfSys* system, EfEmitterResource* emitter, u16 flag);
    virtual void Release();

    /* +0x20 */ EfSys* mManagerES; /* the system that owns it - RetireEffect compares it with `this` */
    /* +0x24 */ u8 pad_0x24[0x1C];
    /* +0x40 */ u32 mGroupID; /* the index into `mActivityList` */
}; /* size: 0x44 (lower bound: +0x40 is the highest field any body touches) */


/* The record `ef_res_emitter_desc` hands back for an emitter: the relocation flag `CreateEffect` tests at +0x00
 * (the name the Warning prints comes from `ef_emres_get_name`, the emitter's own +0x00). size: 0x04 (lower
 * bound: only this word is read) */
typedef struct EfSysEmitterWork {
    /* +0x00 */ u32 flags;
} EfSysEmitterWork; /* size: 0x04 (lower bound) */


/* The object ef_draw_info_projection / fn_800A60C0 are called with: ef/effect.cpp casts fn_800A60C0's result to a
 * MTX34 (the effect's root matrix) and the two returned offsets are the only thing this range
 * establishes, so the +0x30 block is a named byte and the matrix is the mapped one. size: 0x88 */
typedef struct EfSysAccessObj {
    /* +0x00 */ u8 pad_0x00[0x30];
    /* +0x30 */ u8 block_0x30; /* the sub-object ef_draw_info_projection returns (its type is not this range's) */
    /* +0x31 */ u8 pad_0x31[0x27];
    /* +0x58 */ nw4r::math::MTX34 mtx_0x58; /* the matrix fn_800A60C0 returns */
} EfSysAccessObj; /* size: 0x88 (lower bound: +0x58 is the highest field returned) */


/* This unit's `.data` strings, in retail order: the `__FILE__` string and the assert messages (`DrawOrderBase`'s
 * table follows them). */
char ef_effectsystem_file_name[] = "ef_effectsystem.cpp";
char ef_effectsystem_err_memory_manager[] = "NW4R:Pointer Error\nmMemoryManager(=%p) is not valid pointer.";
char ef_effectsystem_err_max_group[] = "NW4R:Failed assertion maxGroupID > 0";
char ef_effectsystem_err_target[] = "NW4R:Pointer Error\ntarget(=%p) is not valid pointer.";
char ef_effectsystem_err_group_id[] = "NW4R:Failed assertion groupID >= 0 && groupID < mMaxGroupID";
char ef_effectsystem_warn_relocation[] = "incomplete relocation (emitter:%s)";
char ef_effectsystem_err_manager[] = "NW4R:Failed assertion target->mManagerES == this";
char ef_effectsystem_err_list_size[] = "NW4R:Failed assertion UtlistSize(&mActivityList[group].mActiveList) < NW4R_EF_MAX_EFFECT";


/* ===================================================================================================
 * Declarations, with C linkage (retail's relocations carry plain map names): this unit's own symbols in
 * address order, then the callees.
 * =================================================================================================== */

extern "C" {
/* this unit's own symbols */
u32 fn_800A56B0(EfSys* self, u32 maxGroupID);
u32 fn_800A5948(EfSys* self, EfSysEffect* target);
EfSysEffect* fn_800A5A90(EfSys* self, void* emitter, u32 groupID, u16 flag);
u32 fn_800A5D8C(EfSys* self, u32 groupID);
u32 fn_800A5E6C(EfSys* self, u32 groupID);
u32 fn_800A5F4C(EfSys* self, u32 groupID);
void fn_800A602C(EfSys* self, const nw4r::math::VEC3* pos, const nw4r::math::MTX34* src, f32 a, f32 b);
void* ef_draw_info_projection(EfSysAccessObj* self);
nw4r::math::MTX34* fn_800A60C0(EfSysAccessObj* self);

/* the callees */
void fn_800A4030(void* list, u16 linkOffset);
void* ef_system_memory_manager(void* p);
void ef_activity_list_clear(void* list);
void ef_activity_list_add(void* list, void* node);
void fn_800A45DC(void* list, void* node);
void fn_800A49B8(void* effect);
void fn_800A4A1C(void* list, void* node);
u16 fn_800A4AF0(void* list);
u32 fn_800A4AF8(void* effect);
const char* ef_emres_get_name(void* p);
void* ef_res_emitter_desc(void* p);
u16 fn_8009B374(void* list, void** buf, u16 size);
void* mtx34_identity(void* mtx);
void* mtx34_get_ptr(void* mtx);
void* mtx34_const_ptr(const void* src);
void PSMTXCopy(const void* src, void* dst);
}

/* ===================================================================================================
 * Assert shapes
 * =================================================================================================== */

/* `NW4R_ASSERT(expr, msg)`: the file is "ef_effectsystem.cpp" and the message is the pooled literal; the
 * `__LINE__` value comes from a `#line` directive at the call site. */
#define NW4R_ASSERT(expr, msg)                                                                     \
    if (!(expr))                                                                                   \
    nw4r::db::Panic(ef_effectsystem_file_name, __LINE__, msg)

/* `NW4R_POINTER_ASSERT`'s RVL address-range check (six materialised BOOLs), shared verbatim with the
 * sibling ef units; the message differs by call site. */
#define NW4R_POINTER_ASSERT(ptr, msg)                                                              \
    {                                                                                              \
        BOOL ok1_ = TRUE, ok2_ = TRUE, ok3_ = TRUE, ok4_ = TRUE, ok5_ = TRUE, ok6_ = TRUE;         \
        u32 top_ = (u32)(ptr) & 0xFF000000u;                                                       \
        if (!(top_ == 0x80000000u) && !(((u32)(ptr) & 0xFF800000u) == 0x81000000u))                \
            ok6_ = FALSE;                                                                          \
        if (!ok6_ && !(((u32)(ptr) & 0xF8000000u) == 0x90000000u))                                 \
            ok5_ = FALSE;                                                                          \
        if (!ok5_ && !(top_ == 0xC0000000u))                                                       \
            ok4_ = FALSE;                                                                          \
        if (!ok4_ && !(((u32)(ptr) & 0xFF800000u) == 0xC1000000u))                                 \
            ok3_ = FALSE;                                                                          \
        if (!ok3_ && !(((u32)(ptr) & 0xF8000000u) == 0xD0000000u))                                 \
            ok2_ = FALSE;                                                                          \
        if (!ok2_ && !(((u32)(ptr) & 0xFFFFC000u) == 0xE0000000u))                                 \
            ok1_ = FALSE;                                                                          \
        if (!ok1_)                                                                                 \
            nw4r::db::Panic(ef_effectsystem_file_name, __LINE__, msg, (ptr));                                   \
    }

#define NW4R_EF_MAX_EFFECT 0x200

namespace nw4r {
namespace ef {

/* The default draw info the system draws with: `nw4r::ef::DrawInfo`'s constructor sets both matrices to the
 * identity, lighting and fog off, a zero depth offset and origin and a white material over a black ambient colour. */
class DrawInfo : public EfDrawInfo {
public:
    DrawInfo() {
        MTX34_ctor(&view_mtx);
        MTX34_ctor(&mtx_0x30);
        VEC3_ctor(&depth_origin);
        mtx34_identity(&view_mtx);
        mtx34_identity(&mtx_0x30);
        light_enable = false;
        light_mask = 0;
        light_mask1 = 0;
        is_spot_light = true;
        fog_type = 0;
        f32 one;
        f32 zero = 0.0f;

        fog_start_z = zero;
        one = 1.0f;
        fog_end_z = one;
        fog_near_z = zero;
        fog_far_z = one;
        depth_offset = zero;
        depth_origin.x = zero;
        depth_origin.y = zero;
        depth_origin.z = zero;
        mat_color.b = 0xFF;
        mat_color.g = 0xFF;
        mat_color.r = 0xFF;
        mat_color.a = 0xFF;
        amb_color.b = 0;
        amb_color.g = 0;
        amb_color.r = 0;
        amb_color.a = 0xFF;
    }
}; /* size: 0xA0 */

/* The objects the system hands out, in construction order (the static initializer builds them). */
static DrawOrder sDrawOrder;
static DrawStrategyBuilder sDrawStrategyBuilder;
static EmitterFormBuilder sEmitterFormBuilder;
static DrawInfo sDrawInfo;

} // namespace ef
} // namespace nw4r

/* Set once the library version has been registered. */
u32 ef_system_version_registered;

/* The system singleton (its constructor and destructor are `ef/ef_effect.cpp`'s). */
EfSys ef_system_instance;

/* The system's allocator, reached through `ef_system_memory_manager` (the map's `GetMemoryManager__Q34nw4r2ef12EffectSystemCFv`
 * sits at an .init offset, not here). */
static inline EfSysMemoryManager* EfGetMemoryManager(EfSys* self) {
    return (EfSysMemoryManager*)ef_system_memory_manager(self);
}

#pragma peephole off

/* Seeds the random state word. */
inline void nw4r::ef::Random::Srand(u32 seed) {
    mState = seed;
}

/* Initialises the record's live list with a zero link offset. */
inline EfSysActivityList::EfSysActivityList() {
    fn_800A4030(this, 0);
}

#pragma dont_inline on

/* ===================================================================================================
 * 0x800A56B0 - `Initialize`: build the group table and bind the borrowed objects.
 * =================================================================================================== */

extern "C" u32 fn_800A56B0(EfSys* self, u32 maxGroupID) {
#line 79
    NW4R_POINTER_ASSERT(self->mMemoryManager, ef_effectsystem_err_memory_manager);
#line 80
    NW4R_ASSERT(maxGroupID > 0, ef_effectsystem_err_max_group);
    self->mMaxGroupID = maxGroupID;
    self->mActivityList = new (self->mMemoryManager->Alloc(maxGroupID * 0x1C + 0x20)) EfSysActivityList[maxGroupID];
    for (u32 i = 0; i < self->mMaxGroupID; ++i) {
        fn_800A4030(&self->mActivityList[i], 0x14);
        ef_activity_list_clear(&self->mActivityList[i]);
    }
    self->mRandom.Srand(0);
    self->mDrawOrder = &nw4r::ef::sDrawOrder;
    self->mStrategy = &nw4r::ef::sDrawStrategyBuilder;
    self->mLineStrategy = &nw4r::ef::sEmitterFormBuilder;
    return 1;
}

#pragma dont_inline reset

/* 0x800A5948 - retires one effect on the spot: move it to its group's retiring list and mark it done. */
extern "C" u32 fn_800A5948(EfSys* self, EfSysEffect* target) {
#line 103
    NW4R_POINTER_ASSERT(target, ef_effectsystem_err_target);
    fn_800A45DC(&self->mActivityList[target->mGroupID], target);
    target->state = 3;
    return 1;
}

/* 0x800A5A90 - `CreateEffect(emitter, groupID, flag)`. */
extern "C" EfSysEffect* fn_800A5A90(EfSys* self, void* emitter, u32 groupID, u16 flag) {
    if (groupID >= self->mMaxGroupID) {
#line 123
        nw4r::db::Panic(ef_effectsystem_file_name, __LINE__, ef_effectsystem_err_group_id);
    }
    if (emitter == NULL) {
        return NULL;
    }
    if ((((EfSysEmitterWork*)ef_res_emitter_desc(emitter))->flags & 0x40000000u) == 0) {
#line 131
        nw4r::db::Warning(ef_effectsystem_file_name, __LINE__, ef_effectsystem_warn_relocation, ef_emres_get_name(emitter));
        return NULL;
    }
    EfSysEffect* effect;
    {
        EfSysMemoryManager* mm = EfGetMemoryManager(self);
        effect = mm->GetEffect();
    }
    if (effect == NULL) {
        return NULL;
    }
    if (effect->Create(self, (EfEmitterResource*)emitter, flag) == 0) {
        EfSysMemoryManager* mm = EfGetMemoryManager(self);
        mm->ReleaseEffect(effect);
        return NULL;
    }
    effect->mGroupID = groupID;
    ef_activity_list_add(&self->mActivityList[groupID], effect);
    effect->state = 1;
    fn_800A4A1C(&self->mActivityList[groupID], effect);
    effect->state = 2;
    return effect;
}

/* 0x800A5C08 - `nw4r::ef::EffectSystem::RetireEffect`: release one active effect from its group's list
 * and tear it down.  The `this` is the system; the effect's own table at +0x1C does the teardown. */
u32 nw4r::ef::EffectSystem::RetireEffect(Effect* target_) {
    EfSys* self = (EfSys*)this;
    EfSysEffect* target = (EfSysEffect*)target_;

#line 159
    NW4R_POINTER_ASSERT(target, ef_effectsystem_err_target);
    if (target->mManagerES != self) {
#line 160
        nw4r::db::Panic(ef_effectsystem_file_name, __LINE__, ef_effectsystem_err_manager);
    }
    if (target->state != 1) {
        return 0;
    }
    fn_800A4A1C(&self->mActivityList[target->mGroupID], target);
    fn_800A49B8(target);
    return 1;
}

/* 0x800A5D8C - retires every effect on one group's active list. */
extern "C" u32 fn_800A5D8C(EfSys* self, u32 groupID) {
    u32 count = 0;
    EfSysEffect* list[NW4R_EF_MAX_EFFECT];

#line 178
    NW4R_ASSERT((u16)fn_800A4AF0(&self->mActivityList[groupID].mActiveList) < NW4R_EF_MAX_EFFECT, ef_effectsystem_err_list_size);
    u16 num = fn_8009B374(&self->mActivityList[groupID].mActiveList, (void**)list,
                          (u16)fn_800A4AF0(&self->mActivityList[groupID].mActiveList));
    for (u16 i = 0; i < num; i++) {
        if (list[i]->state == 1) {
            count += ((nw4r::ef::EffectSystem*)self)->RetireEffect((nw4r::ef::Effect*)list[i]);
        }
    }
    return count;
}

/* 0x800A5E6C - retires the emitters of every effect on one group's active list. */
extern "C" u32 fn_800A5E6C(EfSys* self, u32 groupID) {
    u32 count = 0;
    EfSysEffect* list[NW4R_EF_MAX_EFFECT];

#line 201
    NW4R_ASSERT((u16)fn_800A4AF0(&self->mActivityList[groupID].mActiveList) < NW4R_EF_MAX_EFFECT, ef_effectsystem_err_list_size);
    u16 num = fn_8009B374(&self->mActivityList[groupID].mActiveList, (void**)list,
                          (u16)fn_800A4AF0(&self->mActivityList[groupID].mActiveList));
    for (u16 i = 0; i < num; i++) {
        count += ((nw4r::ef::Effect*)list[i])->RetireEmitterAll();
    }
    return count;
}

/* 0x800A5F4C - retires the particles of every effect on one group's active list. */
extern "C" u32 fn_800A5F4C(EfSys* self, u32 groupID) {
    u32 count = 0;
    EfSysEffect* list[NW4R_EF_MAX_EFFECT];

#line 221
    NW4R_ASSERT((u16)fn_800A4AF0(&self->mActivityList[groupID].mActiveList) < NW4R_EF_MAX_EFFECT, ef_effectsystem_err_list_size);
    u16 num = fn_8009B374(&self->mActivityList[groupID].mActiveList, (void**)list,
                          (u16)fn_800A4AF0(&self->mActivityList[groupID].mActiveList));
    for (u16 i = 0; i < num; i++) {
        count += fn_800A4AF8(list[i]);
    }
    return count;
}

/* 0x800A602C - sets the system's reference transform and its two range floats. */
extern "C" void fn_800A602C(EfSys* self, const nw4r::math::VEC3* pos, const nw4r::math::MTX34* src,
                            f32 a, f32 b) {
    copyVec3(&self->mRefPos, pos);
    nw4r::math::MTX34* dst = (nw4r::math::MTX34*)mtx34_get_ptr(&self->mRefMtx);
    PSMTXCopy(mtx34_const_ptr(src), dst);
    self->mRangeA = a;
    self->mRangeB = b;
}

/* 0x800A60B8 - the +0x30 block accessor (ef_drawstrategyimpl.cpp's fn_800C7CE0 calls it). */
extern "C" void* ef_draw_info_projection(EfSysAccessObj* self) {
    return &self->block_0x30;
}

/* 0x800A60C0 - the +0x58 matrix accessor (callers include ef/effect.cpp, ef/fn_800FD864_fx.cpp, ef/ef_emitter.cpp). */
extern "C" nw4r::math::MTX34* fn_800A60C0(EfSysAccessObj* self) {
    return &self->mtx_0x58;
}






#pragma dont_inline on
