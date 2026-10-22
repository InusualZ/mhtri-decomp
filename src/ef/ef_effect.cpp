/*
 * ef/ef_effect.cpp - nw4r::ef::Effect, the object the memory manager's pool hands out: its constructor and
 *   destructor, the initialiser and first-emitter spawn, `CreateEmitter` (warns "incomplete relocation" unless the
 *   flags' 0x40000000 bit is set), the emitter retire/free paths and whole-list sweeps, the per-frame update, the
 *   flag and emitter-list accessors, `ForeachParticleManager`, the filtered walk and `SetRootMtx`; the activity
 *   list and `ReferencedObject` helpers the other ef units call.
 * RANGE. .text 0x800A3F98-0x800A559C (41 functions); extab 0x80009B80-0x80009C28, extabindex 0x80022D1C-0x80022E18,
 *   .data 0x80592430-0x80592650 (the strings in first-use order, `Effect`'s and `ReferencedObject`'s tables, then the
 *   two header asserts' weak strings), .sdata2 0x80795FF0-0x80795FF8 (the 0.0f `Initialize` stores).
 *   Left edge: `Effect::Effect` and its weak helpers follow `DrawOrder::Remove`; the target's `.data` has
 *   `DrawOrder`'s table last before "ef_effect.cpp" and the base table here, so the constructor is this TU's.
 *   Right edge: `EffectSystem::EffectSystem` (0x800A559C) opens `ef/ef_effectsystem.cpp`, whose `.data` starts with
 *   the library version string it registers (0x80592650).
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (the deleting destructor's `extsh` + `cmpwi` flag test),
 *   `#pragma pool_data off` (each string its own `lis`/`addi`) and `#pragma dont_inline on` (the weak helpers stay
 *   out of line, as retail's).
 * NAMES. `Effect`'s and `ReferencedObject`'s members are nw4r's (`SendClosing`, `DestroyFunc`, `Initialize`,
 *   `CreateEmitter`, `Calc`, `Draw`, `RetireEmitterAll`, `ForeachParticleManager`, `SetRootMtx`); GUESS: their
 *   parameter types (`EffectSystem*`, `EmitterResource*`, `ResEmitter` by value, `u32` flag, `const EfDrawInfo*`)
 *   and the slot order, read from the table at 0x80592588.
 *   GUESS (from the body and its callers): `ef_get_life_status`, `RetireParticleAll` (0x800A4AF8, the nw4r name of
 *   the particle-wide sibling of `RetireEmitterAll`).
 *   GUESS: `ef_activity_list_add` (0x800A43E8), `ef_activity_list_clear` (0x800A4428): an activity list's append
 *   and reset.
 *   GUESS: `ef_system_memory_manager` (0x800A4420): the effect system's memory manager (its first word).
 *   GUESS: `ef_effect_set_calc_flag` (0x800A5114): sets or clears bit 16 of the effect's flags word.
 *   GUESS: `ef_ref_object_init` (0x800A444C): a referenced object's initialisation (no reference, state 1).
 *   GUESS (from `ef/ef_resource.cpp`'s use - the emitter resource's name word and the out-of-line
 *   GUESS: `ut::List_GetFirst`): `ef_emres_get_name`, `ef_list_get_first`.
 *   GUESS: `ef_res_emitter_desc` (0x800A4864): returns the record 8 bytes into an emitter resource, the emitter
 *   description its callers read flags and draw settings from.
 *   GUESS: `EfSysMemoryManager::GetEmitter`/`ReleaseEmitter` (the pool's slots +0x24/+0x28).
 * RESIDUALS. none.
 * SHAPES. `Effect` derives from `ReferencedObject` (`ef/ef_effect.h`: 0x1C bytes of data, then its table at +0x1C),
 *   whose key function `DestroyFunc` is defined here after `fn_800A49B8` (its retail place), so both tables are
 *   emitted here, `Effect`'s first; `SendClosing`'s weak copy is dropped for `ef/ef_creationqueue.cpp`'s.
 * SHAPES. The pointer guards are the `NW4R_POINTER_ASSERT` six-BOOL chain; the file argument is the call site's
 *   own string ("ef_effect.cpp", or "activitylist.h"/"res_emitter_ac.h" for the two header asserts, whose
 *   functions `fn_800A45DC`/`fn_800A5484` are C-linkage `inline` definitions after their first caller, so their
 *   strings are the weak `@STRING@` tail).
 * SHAPES. `#line` puts each `Panic`/`Warning` on retail's line (`fn_800A45DC` 134, `fn_800A51D8` 423,
 *   `RetireEmitterAll` 160, `RetireParticleAll` 180, `Calc` 267/292/312/323).
 * SHAPES. The constructor's helpers `fn_800A3FFC`/`fn_800A4030` are C-linkage `inline` definitions placed before
 *   it: their weak copies and the implicit `ReferencedObject` constructor follow it in retail's order.
 * SHAPES. `Initialize`'s fourth argument is `u16` (retail adds it with no mask), `EfEffEmitter::mField_0xB4` is
 *   `s32` (compared signed) and `ef_effect_set_calc_flag`'s bit is 0x10000.
 * SHAPES. The emitter (`EfEffEmitter`, its table at +0x1C) and the memory manager (`EfSysMemoryManager`) are
 *   polymorphic views whose virtual slots are declared only, so the calls load the slot through r12 as retail does
 *   and no table is emitted; `Draw` calls the draw-order object (`DrawOrderBase`, `ef/ef_draworder.h`).  The list
 *   walks of `Calc` each declare their own node and counter, and its tick pass decrements the lifetime only for an
 *   active emitter while still walking its children.  `fn_800A52E4` tests the parents of each emitter, not the
 *   emitter itself.
 * SHAPES. `Effect` is defined here in full; `ef.h` still carries the consumers' stub of the same class (the two
 *   meet only in the map's manglings).
 */

#include "types.h"
#include "nw4r/math.h"
#define MHTRI_EF_EFFECT_OWNER /* the full nw4r::ef::Effect of ef/ef_effect.h */
#include "ef/ef_effect.h" /* nw4r::ef::Effect and ReferencedObject, the unit's own header */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */
#include "ef/ef_effectsystem.h" /* EfSys and its memory manager (rule 1) */
#include "nw4r/ut_list.h" /* nw4r::ut::List_* (rule 2) */

namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
void Warning(const char* file, int line, const char* fmt, ...);
} // namespace db
} // namespace nw4r

/* The global `operator delete` (the map's `__dl__FPv`). */
void operator delete(void* ptr) throw();

/* ===================================================================================================
 * Types.  Every record states its size; the offsets are the ones the bodies load or store.
 * =================================================================================================== */

/* The work record `ef_res_emitter_desc` hands back for an emitter: the flags word at its first byte. */
typedef struct EfEffEmitterWork {
    /* +0x00 */ u32 flags;
} EfEffEmitterWork; /* size: 0x04 (lower bound: only this word is read) */

/* The bodies' spelling of the effect. */
typedef nw4r::ef::Effect EfEff;

/* An emitter/particle-manager record: the objects the effect's +0x24 list holds. */
typedef struct EfEffEmitter {
    /* +0x000 */ const char* mName; /* ef_emres_get_name hands it back for the Warning */
    /* +0x004 */ u8 pad_0x004[0x04];
    /* +0x008 */ u32 mFlags_0x08; /* ef_res_emitter_desc hands the record at +0x08 back */
    /* +0x00C */ s32 mState;
    /* +0x010 */ void* mField_0x10;
    /* +0x014 */ u8 pad_0x014[0x08];
    /* +0x01C: the vtable pointer (the emitter's virtual slots +0x08..+0x24, called through r12) */
    virtual void method_0x08();
    virtual void method_0x0C();
    virtual u32 method_0x10(void* a, void* b, u32 c);
    virtual void method_0x14();
    virtual void method_0x18();
    virtual void method_0x1C();
    virtual void method_0x20();
    virtual void method_0x24();
    /* +0x020 */ u8 pad_0x020[0x04];
    /* +0x024 */ u32 mField_0x24; /* the two flag bits fn_800A4BBC tests */
    /* +0x028 */ u8 pad_0x028[0x8C];
    /* +0x0B4 */ s32 mField_0xB4;
    /* +0x0B8 */ u8 pad_0x0B8[0x04];
    /* +0x0BC */ void* mField_0xBC; /* freed by fn_800A4474 */
    /* +0x0C0 */ EfEffList mChildren; /* the child list fn_800A4BBC walks */
    /* +0x0CC */ u8 pad_0x0CC[0x18];
    /* +0x0E4 */ u32 mField_0xE4;
    /* +0x0E8 */ u16 mField_0xE8; /* the lifetime count */
    /* +0x0EA */ u8 pad_0x0EA[0x0A];
    /* +0x0F4 */ void* mField_0xF4; /* the parent/link block (freed by fn_800A4474) */
    /* +0x0F8 */ void* mField_0xF8; /* freed by fn_800A4474 */
    /* +0x0FC */ u8 pad_0x0FC[0x0C];
    /* +0x108 */ VEC3 mField_0x108;
    /* +0x114 */ VEC3 mField_0x114;
} EfEffEmitter; /* size: 0x120 (lower bound: +0x114 is the highest field any body touches) */

/* The effect system `SendClosing` hands the effect back to. */
typedef EfSys EfEffSys; /* the effect system, `ef/ef_effectsystem.h` */

/* ===================================================================================================
 * The declarations of the callees and of this unit's own functions.
 * =================================================================================================== */

extern "C" {

/* Callees outside this unit, with C linkage: retail's relocations carry their plain map names. */
void fn_800A3718(void* p);
void fn_800A3800(void* p);
u32 fn_800A5948(EfEffSys* self, EfEff* target);
u32 fn_800A6350(void* p);
void ef_emitter_get_mtx(void* node, MTX34* mtx);
void fn_800A95D8(void* node);
u32 fn_800A98D4(void* pm, void (*cb)(void*, u32), u32 arg, bool flag, u32 zero);
void fn_800AE500(void* pm, u32 flag);
void fn_800AE5B0(void* pm);
void mtx34_identity(void* mtx);
void mtx34_copy_ps(MTX34* dst, const MTX34* src);
u16 fn_8009B374(void* list, void** buf, u16 size);

/* This unit's own symbols (address order). */
inline EfEffActivityList* fn_800A3FFC(EfEffActivityList* list, u16 offset);
inline void fn_800A4030(EfEffActivityList* list, u16 offset);
u32 fn_800A4474(EfEff* self, EfEffEmitter* em);
void fn_800A45DC(EfEffActivityList* list, void* node);
u32 fn_800A4654(EfEff* self, EfEffEmitter* emitter, u8 a, u16 b);
void* ef_emres_get_name(void* p);
u32 fn_800A486C(EfEff* self, EfEffEmitter* em);
void fn_800A49B8(EfEffEmitter* em);
void fn_800A4A1C(EfEffActivityList* list, void* node);
u16 fn_800A4AF0(EfEffList* list);
u32 RetireParticleAll(EfEff* self);
u32 fn_800A50EC(EfEff* self);
u32 fn_800A5104(void* p);
u32 fn_800A513C(EfEff* self);
u32 fn_800A51B0(EfEff* self);
void* fn_800A51C8(EfEff* self);
u16 ef_effect_emitter_count(EfEff* self);
void* ef_effect_emitter_get(EfEff* self, u16 idx);
s32 ef_get_life_status(void* p);
void* ef_list_get_first(EfEffList* list);
u32 fn_800A52E4(EfEff* self, void (*cb)(void*, void*), void* arg, u32 flag, EfEffEmitter* match);
void* fn_800A5484(nw4r::ef::ResEmitter* res);

} /* extern "C" */

/* ===================================================================================================
 * Assert shapes
 * =================================================================================================== */

#define NW4R_EF_MAX_EMITTER 0x200

/* `NW4R_POINTER_ASSERT`'s RVL address-range check (six materialised BOOLs), shared verbatim with the
 * sibling ef units; the file is the call site's own pooled name. */
#define NW4R_POINTER_ASSERT(ptr, file, msg)                                                        \
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
            nw4r::db::Panic(file, __LINE__, msg, (ptr));                                           \
    }

#pragma peephole off
#pragma pool_data off
#pragma dont_inline on

/* 0x800A3FFC (0x34): builds an activity list whose elements carry their link at `offset`. */
extern "C" inline EfEffActivityList* fn_800A3FFC(EfEffActivityList* list, u16 offset) {
    fn_800A4030(list, offset);
    return list;
}

/* 0x800A4030 (0x50): sets both lists' link offset and clears the live count. */
extern "C" inline void fn_800A4030(EfEffActivityList* list, u16 offset) {
    nw4r::ut::List_Init((nw4r::ut::List*)&list->mActiveList, offset);
    nw4r::ut::List_Init((nw4r::ut::List*)&list->mRetireList, offset);
    list->mNumActive = 0;
}

/* 0x800A3F98 (0x64): builds the effect: the base, its emitter group, the root matrix and velocity, the draw list. */
nw4r::ef::Effect::Effect() {
    fn_800A3FFC(&mEmitters, 0x14);
    MTX34_ctor(&mRootMtx);
    VEC3_ctor(&mVelocity);
    nw4r::ut::List_Init((nw4r::ut::List*)&mDrawList, 0x30);
}

/* 0x800A4090 (0x64): retires the effect's emitters. */
nw4r::ef::Effect::~Effect() {
    RetireEmitterAll();
}

/* ===================================================================================================
 * 0x800A40F4 - the effect initialiser + first-emitter spawn.
 * =================================================================================================== */

u32 nw4r::ef::Effect::Initialize(EffectSystem* system, EmitterResource* eh, u16 n) {
    EfEff* self = this;
    EfEffManager* mgr = (EfEffManager*)system;
    EfSysMemoryManager* mm;
    EfEffEmitter* em;

#line 41
    NW4R_POINTER_ASSERT(mgr, "ef_effect.cpp", "NW4R:Pointer Error\nmanager(=%p) is not valid pointer.");
#line 42
    NW4R_POINTER_ASSERT(eh, "ef_effect.cpp", "NW4R:Pointer Error\neh(=%p) is not valid pointer.");

    ef_ref_object_init(self);
    ef_activity_list_clear(&self->mEmitters);
    self->mDrawOrder = (nw4r::ef::DrawOrderBase*)mgr->field_0x04;
    mtx34_identity(&self->mRootMtx);
    self->mManagerES = mgr;
    self->mField_0x48 = 0;
    self->mField_0x4C = 0;
    self->mField_0x50 = 0;
    self->mFlags_0x54 = 0;
    f32 zero = 0.0f;
    self->mVelocity.x = zero;
    self->mVelocity.y = zero;
    self->mVelocity.z = zero;

    mm = (EfSysMemoryManager*)ef_system_memory_manager(mgr);
    em = (EfEffEmitter*)mm->GetEmitter();
    if (em == NULL) {
        return 0;
    }
    if (em->method_0x10(self, eh, 128) == 0) {
        mm = (EfSysMemoryManager*)ef_system_memory_manager(mgr);
        mm->ReleaseEmitter((EfSysEmitter*)em);
        return 0;
    }
    ef_activity_list_add(&self->mEmitters, em);
    em->mState = 1;
    em->mField_0xE8 += n;
    return 1;
}

/* 0x800A43E8 - appends an emitter to the group and bump the live count. */
/* untyped: opaque handle - the list and its node are each caller's own record views */
extern "C" void ef_activity_list_add(void* group, void* node) {
    EfEffActivityList* list = (EfEffActivityList*)group;
    nw4r::ut::List_Append((nw4r::ut::List*)&list->mActiveList, node);
    list->mNumActive++;
}

/* 0x800A4420 - the manager getter: the record's first word. */
extern "C" EfMemoryManager* ef_system_memory_manager(void* p) {
    return *(EfMemoryManager**)p;
}

/* 0x800A4428 - clears an activity group (both lists and the count). */
/* untyped: opaque handle - the list is each caller's own record view */
extern "C" void ef_activity_list_clear(void* group) {
    EfEffActivityList* list = (EfEffActivityList*)group;
    list->mActiveList.head = NULL;
    list->mActiveList.numObjects = 0;
    list->mActiveList.tail = NULL;
    list->mRetireList.head = NULL;
    list->mRetireList.numObjects = 0;
    list->mRetireList.tail = NULL;
    list->mNumActive = 0;
}

/* 0x800A444C - resets the effect's state to active. */
/* untyped: opaque handle - the object is each caller's own record view */
extern "C" u32 ef_ref_object_init(void* object) {
    nw4r::ef::ReferencedObject* self = (nw4r::ef::ReferencedObject*)object;
    self->mRefCount = 0;
    self->mLifeStatus = 1;
    return 1;
}

/* 0x800A4464 - the table's direct-retire slot: hand the effect to its system. */
void nw4r::ef::Effect::SendClosing() {
    fn_800A5948((EfEffSys*)mManagerES, this);
}

/* 0x800A4470 - the table's retire-all slot. */
void nw4r::ef::Effect::DestroyFunc() {
    RetireEmitterAll();
}

/* 0x800A4474 - retires one emitter and free its blocks. */
extern "C" u32 fn_800A4474(EfEff* self, EfEffEmitter* em) {
#line 93
    NW4R_POINTER_ASSERT(em, "ef_effect.cpp", "NW4R:Pointer Error\ntarget(=%p) is not valid pointer.");

    if (em->mField_0xF4 != NULL) {
        fn_800A3800(em->mField_0xF4);
    }
    if (em->mField_0xF8 != NULL) {
        fn_800A3800(em->mField_0xF8);
        em->mField_0xF8 = NULL;
    }
    fn_800A3800(em->mField_0xBC);
    fn_800A45DC(&self->mEmitters, em);
    em->mState = 3;
    return 1;
}

/* 0x800A45DC - moves an emitter from the live list to the retired list. */
extern "C" inline void fn_800A45DC(EfEffActivityList* list, void* node) {
#line 133
    if (list->mActiveList.numObjects < list->mNumActive) {
        nw4r::db::Panic("activitylist.h", __LINE__, "NW4R:Failed assertion mActiveList.numObjects >= mNumActive");
    }
    nw4r::ut::List_Remove((nw4r::ut::List*)&list->mActiveList, node);
    nw4r::ut::List_Append((nw4r::ut::List*)&list->mRetireList, node);
}

/* 0x800A4654 - creates an emitter on the effect's list. */
extern "C" u32 fn_800A4654(EfEff* self, EfEffEmitter* emitter, u8 a, u16 b) {
    EfSysMemoryManager* mm;
    EfEffEmitter* em;

#line 114
    NW4R_POINTER_ASSERT(emitter, "ef_effect.cpp", "NW4R:Pointer Error\neh(=%p) is not valid pointer.");

    if ((((EfEffEmitterWork*)ef_res_emitter_desc(emitter))->flags & 0x40000000u) == 0) {
#line 118
        nw4r::db::Warning("ef_effect.cpp", __LINE__, "incomplete relocation (emitter:%s)", ef_emres_get_name(emitter));
        return 0;
    }
    mm = (EfSysMemoryManager*)ef_system_memory_manager(self->mManagerES);
    em = (EfEffEmitter*)mm->GetEmitter();
    if (em == NULL) {
        return 0;
    }
    if (em->method_0x10(self, emitter, a) == 0) {
        mm = (EfSysMemoryManager*)ef_system_memory_manager(self->mManagerES);
        mm->ReleaseEmitter((EfSysEmitter*)em);
        return 0;
    }
    ef_activity_list_add(&self->mEmitters, em);
    em->mState = 1;
    em->mField_0xE8 += b;
    return (u32)em;
}

/* 0x800A485C - the record's name getter (its first word). */
extern "C" void* ef_emres_get_name(void* p) {
    return *(void**)p;
}

/* 0x800A4864 - the emitter's work record (the object at +0x08). */
extern "C" EfEmitterDesc* ef_res_emitter_desc(void* p) {
    return (EfEmitterDesc*)((u8*)p + 8);
}

/* 0x800A486C - retires one live emitter. */
extern "C" u32 fn_800A486C(EfEff* self, EfEffEmitter* em) {
#line 144
    NW4R_POINTER_ASSERT(em, "ef_effect.cpp", "NW4R:Pointer Error\ntarget(=%p) is not valid pointer.");

    if (em->mState != 1) {
        return 0;
    }
    fn_800A4A1C(&self->mEmitters, em);
    fn_800A49B8(em);
    return 1;
}

/* 0x800A49B8 - runs an emitter's teardown: table slot 0x0C, state 2, then slot 0x08 when detached. */
extern "C" void fn_800A49B8(EfEffEmitter* em) {
    em->method_0x0C();
    em->mState = 2;
    if (em->mField_0x10 == NULL) {
        em->method_0x08();
    }
}

/* 0x800A4A18 - the base table's empty slot. */
void nw4r::ef::ReferencedObject::DestroyFunc() {
}

/* 0x800A4A1C - drops one from the group's live count. */
extern "C" void fn_800A4A1C(EfEffActivityList* list, void* node) {
    (void)node;
    list->mNumActive--;
}

/* ===================================================================================================
 * 0x800A4A2C - `Effect::RetireEmitterAll`: retire every live emitter on the effect's list.
 * =================================================================================================== */

u32 nw4r::ef::Effect::RetireEmitterAll() {
    EfEff* self = this;
    u32 count = 0;
    EfEffEmitter* list[NW4R_EF_MAX_EMITTER];

#line 159
    if ((u16)fn_800A4AF0(&self->mEmitters.mActiveList) >= NW4R_EF_MAX_EMITTER) {
        nw4r::db::Panic("ef_effect.cpp", __LINE__, "NW4R:Failed assertion UtlistSize(&mActivityList.mActiveList) < NW4R_EF_MAX_EMITTER");
    }
    u16 num = fn_8009B374(&self->mEmitters.mActiveList, (void**)list,
                          (u16)fn_800A4AF0(&self->mEmitters.mActiveList));
    for (u16 i = 0; i < num; i++) {
        if (list[i]->mState == 1) {
            count += fn_800A486C(self, list[i]);
        }
    }
    return count;
}

/* 0x800A4AF0 - the list's live count. */
extern "C" u16 fn_800A4AF0(EfEffList* list) {
    return list->numObjects;
}

/* 0x800A4AF8 - retires every particle on every live emitter. */
extern "C" u32 RetireParticleAll(EfEff* self) {
    u32 count = 0;
    EfEffEmitter* list[NW4R_EF_MAX_EMITTER];

#line 179
    if ((u16)fn_800A4AF0(&self->mEmitters.mActiveList) >= NW4R_EF_MAX_EMITTER) {
        nw4r::db::Panic("ef_effect.cpp", __LINE__, "NW4R:Failed assertion UtlistSize(&mActivityList.mActiveList) < NW4R_EF_MAX_EMITTER");
    }
    u16 num = fn_8009B374(&self->mEmitters.mActiveList, (void**)list,
                          (u16)fn_800A4AF0(&self->mEmitters.mActiveList));
    for (u16 i = 0; i < num; i++) {
        count += fn_800A6350(list[i]);
    }
    return count;
}

/* ===================================================================================================
 * 0x800A4BBC - the per-frame update.
 * =================================================================================================== */

void nw4r::ef::Effect::Calc(u32 flag) {
    EfEff* self = this;
    EfEffEmitter* list[NW4R_EF_MAX_EMITTER];
    u16 num;

    if (flag & 1u) {
        EfEffEmitter* node = NULL;
        while ((node = (EfEffEmitter*)nw4r::ut::List_GetNext((const nw4r::ut::List*)&self->mEmitters.mActiveList, node)) != NULL) {
            if ((u32)(node->mState - 1) <= 1u) {
                if ((node->mField_0x24 & 0x10000u) || (node->mField_0x24 & 0x8000u)) {
                    node->method_0x24();
                }
            }
        }
        return;
    }

    if (fn_800A513C(self) != 0) {
        return;
    }

    if ((flag & 2u) == 0u) {
        EfEffEmitter* node = NULL;
        while ((node = (EfEffEmitter*)nw4r::ut::List_GetNext((const nw4r::ut::List*)&self->mEmitters.mActiveList, node)) != NULL) {
            if ((u32)(node->mState - 1) <= 1u && node->mField_0xB4 == 1) {
                node->mField_0xB4 = 0;
            }
            copyVec3(&node->mField_0x114, &node->mField_0x108);
            EfEffEmitter* child = NULL;
            while ((child = (EfEffEmitter*)nw4r::ut::List_GetNext((const nw4r::ut::List*)&node->mChildren, child)) != NULL) {
                fn_800AE500(child, 0);
            }
        }
    }

    ef_effect_set_calc_flag(self, 1);

    while (fn_800A50EC(self) != 0) {
        ef_effect_set_calc_flag(self, 0);

        for (;;) {
#line 266
            if ((u16)fn_800A4AF0(&self->mEmitters.mActiveList) >= NW4R_EF_MAX_EMITTER) {
                nw4r::db::Panic("ef_effect.cpp", __LINE__, "NW4R:Failed assertion UtlistSize(&mActivityList.mActiveList) < NW4R_EF_MAX_EMITTER");
            }
            num = fn_8009B374(&self->mEmitters.mActiveList, (void**)list,
                              (u16)fn_800A4AF0(&self->mEmitters.mActiveList));
            for (u16 i = 0; i < num; i++) {
                list[i]->method_0x18();
            }

            EfEffEmitter* node = NULL;
            while ((node = (EfEffEmitter*)nw4r::ut::List_GetNext((const nw4r::ut::List*)&self->mEmitters.mActiveList, node)) !=
                   NULL) {
                if ((u32)(node->mState - 1) <= 1u) {
                    if ((node->mField_0x24 & 0x10000u) || (node->mField_0x24 & 0x8000u)) {
                        node->method_0x24();
                    }
                }
            }

#line 291
            if ((u16)fn_800A4AF0(&self->mEmitters.mActiveList) >= NW4R_EF_MAX_EMITTER) {
                nw4r::db::Panic("ef_effect.cpp", __LINE__, "NW4R:Failed assertion UtlistSize(&mActivityList.mActiveList) < NW4R_EF_MAX_EMITTER");
            }
            num = fn_8009B374(&self->mEmitters.mActiveList, (void**)list,
                              (u16)fn_800A4AF0(&self->mEmitters.mActiveList));
            for (u16 i = 0; i < num; i++) {
                EfEffEmitter* e = list[i];
                if (e->mField_0xB4 == 0) {
                    e->mField_0xE4++;
                }
                e->method_0x1C();
                if (e->mField_0xB4 == 0) {
                    e->mField_0xE4--;
                }
            }

#line 311
            if ((u16)fn_800A4AF0(&self->mEmitters.mActiveList) >= NW4R_EF_MAX_EMITTER) {
                nw4r::db::Panic("ef_effect.cpp", __LINE__, "NW4R:Failed assertion UtlistSize(&mActivityList.mActiveList) < NW4R_EF_MAX_EMITTER");
            }
            num = fn_8009B374(&self->mEmitters.mActiveList, (void**)list,
                              (u16)fn_800A4AF0(&self->mEmitters.mActiveList));
            for (u16 i = 0; i < num; i++) {
                list[i]->method_0x20();
            }

#line 322
            if ((u16)fn_800A4AF0(&self->mEmitters.mActiveList) >= NW4R_EF_MAX_EMITTER) {
                nw4r::db::Panic("ef_effect.cpp", __LINE__, "NW4R:Failed assertion UtlistSize(&mActivityList.mActiveList) < NW4R_EF_MAX_EMITTER");
            }
            num = fn_8009B374(&self->mEmitters.mActiveList, (void**)list,
                              (u16)fn_800A4AF0(&self->mEmitters.mActiveList));
            for (u16 i = 0; i < num; i++) {
                list[i]->method_0x1C();
            }

            if (fn_800A5104(&((EfEffManager*)self->mManagerES)->field_0x10) != 0) {
                break;
            }
            fn_800A3718(&((EfEffManager*)self->mManagerES)->field_0x10);
        }

        if (fn_800A50EC(self) != 0) {
            EfEffEmitter* node = NULL;
            while ((node = (EfEffEmitter*)nw4r::ut::List_GetNext((const nw4r::ut::List*)&self->mEmitters.mActiveList, node)) !=
                   NULL) {
                if (node->mState == 1 && node->mField_0xE8 != 0) {
                    node->mField_0xE8--;
                    if (node->mField_0xB4 == 1) {
                        node->mField_0xB4 = 0;
                    }
                }
                EfEffEmitter* child = NULL;
                while ((child = (EfEffEmitter*)nw4r::ut::List_GetNext((const nw4r::ut::List*)&node->mChildren, child)) != NULL) {
                    fn_800AE500(child, 1);
                }
            }
        }
    }

    EfEffEmitter* node = NULL;
    while ((node = (EfEffEmitter*)nw4r::ut::List_GetNext((const nw4r::ut::List*)&self->mEmitters.mActiveList, node)) != NULL) {
        if ((u32)(node->mState - 1) <= 1u && node->mField_0xB4 == 3) {
            node->mField_0xB4 = 1;
        }
        EfEffEmitter* child = NULL;
        while ((child = (EfEffEmitter*)nw4r::ut::List_GetNext((const nw4r::ut::List*)&node->mChildren, child)) != NULL) {
            fn_800AE5B0(child);
        }
        MTX34 mtx;
        MTX34_ctor(&mtx);
        ef_emitter_get_mtx(node, &mtx);
        node->mField_0x108.x = mtx.m[0][3];
        node->mField_0x108.y = mtx.m[1][3];
        node->mField_0x108.z = mtx.m[2][3];
    }
}

/* 0x800A50EC - the +0x54 bit-15 accessor. */
extern "C" u32 fn_800A50EC(EfEff* self) {
    return (self->mFlags_0x54 & 0x10000u) != 0;
}

/* 0x800A5104 - the null test of the manager's +0x10 word. */
extern "C" u32 fn_800A5104(void* p) {
    return *(u32*)p == 0;
}

/* 0x800A5114 - sets/clears the +0x54 bit 0. */
/* untyped: opaque handle - the effect is each caller's own record view */
extern "C" void ef_effect_set_calc_flag(void* effect, u32 on) {
    EfEff* self = (EfEff*)effect;
    if (on) {
        self->mFlags_0x54 |= 0x10000u;
    } else {
        self->mFlags_0x54 &= ~0x10000u;
    }
}

/* 0x800A513C - the +0x54 bit-0 accessor. */
extern "C" u32 fn_800A513C(EfEff* self) {
    return (self->mFlags_0x54 & 1u) != 0;
}

/* 0x800A5154 - the table's detach slot: forward to the +0xA0 object when not already detached. */
void nw4r::ef::Effect::Draw(const EfDrawInfo* info) {
    if (fn_800A51B0(this) == 0) {
        mDrawOrder->Draw(this, info);
    }
}

/* 0x800A51B0 - the +0x54 bit-1 accessor. */
extern "C" u32 fn_800A51B0(EfEff* self) {
    return (self->mFlags_0x54 & 2u) != 0;
}

/* 0x800A51C8 - the emitter list's head word. */
extern "C" void* fn_800A51C8(EfEff* self) {
    return self->mEmitters.mActiveList.head;
}

/* 0x800A51D0 - the number of live emitters. */
extern "C" u16 ef_effect_emitter_count(EfEff* self) {
    return fn_800A4AF0(&self->mEmitters.mActiveList);
}

/* 0x800A51D8 - the indexed live emitter. */
extern "C" void* ef_effect_emitter_get(EfEff* self, u16 idx) {
#line 422
    if ((u16)idx >= (u16)ef_effect_emitter_count(self)) {
        nw4r::db::Panic("ef_effect.cpp", __LINE__, "NW4R:Failed assertion idx < GetNumEmitter()");
    }
    return nw4r::ut::List_GetNth((const nw4r::ut::List*)&self->mEmitters.mActiveList, idx);
}

/* 0x800A5248 - the record's state getter. */
extern "C" s32 ef_get_life_status(void* p) {
    return *(s32*)((u8*)p + 0x0C);
}

/* 0x800A5250 - the emitter list's first live element. */
extern "C" void* ef_list_get_first(EfEffList* list) {
    return nw4r::ut::List_GetNext((const nw4r::ut::List*)list, NULL);
}

/* 0x800A5258 - walks the particle managers with `cb` and returns the count the walk accumulates. */
u32 nw4r::ef::Effect::ForeachParticleManager(void (*cb)(void*, u32), u32 arg, bool flag) {
    EfEff* self = this;
    u32 count = 0;
    EfEffEmitter* pm = (EfEffEmitter*)ef_list_get_first(&self->mEmitters.mActiveList);
    while (pm != NULL) {
        EfEffEmitter* next =
            (EfEffEmitter*)nw4r::ut::List_GetNext((const nw4r::ut::List*)&self->mEmitters.mActiveList, pm);
        count += fn_800A98D4(pm, cb, arg, flag, 0);
        pm = next;
    }
    return count;
}

/* 0x800A52E4 - walks the particle managers, keeping those whose +0xF4 chain holds `match`. */
extern "C" u32 fn_800A52E4(EfEff* self, void (*cb)(void*, void*), void* arg, u32 flag,
                           EfEffEmitter* match) {
    u32 count = 0;
    EfEffEmitter* pm = (EfEffEmitter*)ef_list_get_first(&self->mEmitters.mActiveList);
    while (pm != NULL) {
        EfEffEmitter* next =
            (EfEffEmitter*)nw4r::ut::List_GetNext((const nw4r::ut::List*)&self->mEmitters.mActiveList, pm);
        if (flag != 0u || ef_get_life_status(pm) == 1) {
            EfEffEmitter* p = pm;
            BOOL found = FALSE;
            while (!found && (p = (EfEffEmitter*)p->mField_0xF4) != NULL) {
                if (p == match) {
                    found = TRUE;
                }
            }
            if (found) {
                cb(pm, arg);
                count++;
            }
        }
        pm = next;
    }
    return count;
}

/* 0x800A53BC - concatenates the root matrix and pushes it to the parentless emitters. */
void nw4r::ef::Effect::SetRootMtx(const nw4r::math::MTX34& mtx) {
    EfEff* self = this;
    mtx34_copy_ps(&self->mRootMtx, &mtx);
    EfEffEmitter* node = NULL;
    while ((node = (EfEffEmitter*)nw4r::ut::List_GetNext((const nw4r::ut::List*)&self->mEmitters.mActiveList, node)) != NULL) {
        if (node->mField_0xF4 == NULL) {
            fn_800A95D8(node);
        }
    }
}

/* 0x800A5428 - the table's spawn slot. */
u32 nw4r::ef::Effect::CreateEmitter(ResEmitter resource, u8 a, u16 b) {
    return fn_800A4654(this, (EfEffEmitter*)fn_800A5484(&resource), a, b);
}

/* 0x800A5484 - the spawn slot's emitter getter (validated first word). */
extern "C" inline void* fn_800A5484(nw4r::ef::ResEmitter* res) {
#line 95
    NW4R_POINTER_ASSERT(res->mData, "res_emitter_ac.h", "NW4R:Pointer Error\nmData(=%p) is not valid pointer.");
    return res->mData;
}
