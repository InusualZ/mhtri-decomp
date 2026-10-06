/*
 * ef/ef_creationqueue.cpp - the NW4R effect library's creation queue: two `Add` paths (entry type 0 and 1),
 *   `Execute` (dispatch each entry, release its manager, reset the count) and the manager's inlined
 *   reference-count pair emitted out of line.
 * RANGE. .text 0x800A3044-0x800A388C (7 functions); extab 0x80009B40-0x80009B60, extabindex 0x80022CBC-0x80022CEC,
 *   .data 0x805922C0-0x805923A0 (the `__FILE__` string `ef_creationqueue.cpp` first).
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps the unfused `clrlwi` + `cmpwi` in
 *   `fn_800A3718` and `addi` + `cmpwi` in `fn_800A3800`).
 * NAMES. The map has only `fn_` stems for the range; the types and fields are GUESSes from the panic messages
 *   (`setting`, `eh`) and from `fn_800A7750` in `ef/ef_emitter.cpp`, which asserts the same handle.
 *   GUESS: `ef_ref_object_add_ref` (0x800A337C): a referenced object's AddRef (its count at +0x10).
 *   GUESS: `ef_creation_queue_add_type0` (0x800A3044) queues entry type 0 (a particle creation).
 *   GUESS: `ef_creation_queue_add_type1` (0x800A33DC) queues type 1, which `fn_800A3718` dispatches through the
 *   emitter form's create slot. The types they share with `ef/ef_animcurve.cpp` live in
 *   `ef/ef_creationqueue.h`.
 * RESIDUALS. 2 partial rows:
 *  - `fn_800A3718`: retail reuses r12 for the form's table and slot (`lwz r12, ...; lwz r12, ...`), ours loads the
 *    table through r9;
 *  - `fn_800A3800`: the disposal call loads the table into r4 (`lwz r4, 0x1C(r31)`) where retail goes through the
 *    `this` register (`lwz r12, 0x1C(r3)`).
 *   flipcheck: `.data` claimed, not emitted.
 * SHAPES. A table member declared as a pointer to a struct of function pointers makes the virtual calls load the
 *   slot straight through the table register, as retail does.
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h"

#include "ef/ef_creationqueue.h" /* the unit's own header */
#include "ef/ef_emitter.h" /* fn_800A7750 (rule 2) */

#pragma peephole off

/* `nw4r::db::Panic`/`Warning`, declared in their namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
void Warning(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r

struct EmitterForm;

/* The emitter form's virtual slot +0x14 (the type-1 creation entry) and the manager's virtual slot
 * +0x2 (its disposal).  Declaring the vtable member as a pointer to the function-pointer table makes
 * the call load the entry straight through the vtable register, the shape the target has. */
typedef void (*EmitterFormCreate)(EmitterForm* self, EffectHandle* eh, const Setting* setting,
                                  EffectManager* manager, u16 life, const Vec3* pos);
typedef void (*EffectManagerDispose)(EffectManager* self);

struct EmitterFormVtbl {
    /* +0x00 */ u8 pad_0x00[0x14];
    /* +0x14 */ EmitterFormCreate createEmitter;
}; /* size: 0x18 */

struct EffectManagerVtbl {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ EffectManagerDispose dispose;
}; /* size: 0xC */

/* The effect manager's resource table: fn_800A3718 reaches the emitter form through `+0x20`. */
struct EmitterManager {
    /* +0x00 */ u8 pad_0x00[0x20];
    /* +0x20 */ EmitterForm* mpEmitterForm;
}; /* size: 0x24 */

/* The reference-counted object a queue entry points at (`referencedobject.h`).  The vtable sits at
 * +0x1C, the count and its disposal type at +0x10/+0x0C, and +0xC8 holds the emitter manager. */
struct EffectManager {
    /* +0x00 */ u8 pad_0x00[0xC];
    /* +0x0C */ s32 mRefCountType;
    /* +0x10 */ u32 mRefCount;
    /* +0x14 */ u8 pad_0x14[0x8];
    /* +0x1C */ EffectManagerVtbl* mpVtbl;
    /* +0x20 */ u8 pad_0x20[0xC8 - 0x20];
    /* +0xC8 */ EmitterManager* mpEmitterManager;
}; /* size: 0xCC */

/* The emitter form whose virtual slot +0x14 is the type-1 creation entry. */
struct EmitterForm {
    /* +0x00 */ u8 pad_0x00[0x1C];
    /* +0x1C */ EmitterFormVtbl* mpVtbl;
}; /* size: 0x20 */

/* NW4R_POINTER_ASSERT's RVL address-range check (MEM1/MEM2, cached and uncached, plus locked cache). */
#define NW4R_VALID_PTR(p)                                                                          \
    (((u32)(p) & 0xFF000000) == 0x80000000 || ((u32)(p) & 0xFF800000) == 0x81000000 ||             \
     ((u32)(p) & 0xF8000000) == 0x90000000 || ((u32)(p) & 0xFF000000) == 0xC0000000 ||             \
     ((u32)(p) & 0xFF800000) == 0xC1000000 || ((u32)(p) & 0xF8000000) == 0xD0000000 ||             \
     ((u32)(p) & 0xFFFFC000) == 0xE0000000)

#define NW4R_POINTER_ASSERT(p, line, msg)                                                          \
    (NW4R_VALID_PTR(p) ? (void)0 : nw4r::db::Panic(lbl_805922C0, line, msg, (p)))

#define NW4R_ASSERT(exp, line, msg) ((exp) ? (void)0 : nw4r::db::Panic(lbl_80592388, line, msg))

extern char lbl_805922C0[];
extern char lbl_805922D8[];
extern char lbl_80592310[];
extern char lbl_80592344[];
extern char lbl_80592364[];
extern char lbl_80592388[];

/* 0x800A3390 - copies the 0xA-byte setting record. */
extern "C" void fn_800A3390(Setting* dst, const Setting* src);

/* 0x800A337C - the manager's AddRef (increments the +0x10 count, returns it). */
extern "C" u32 ef_ref_object_add_ref(EffectManager* manager);

/* 0x800A3044 (0x338): queues a type-0 (particle) creation. */
extern "C" void ef_creation_queue_add_type0(CreationQueue* self, const Setting* setting, EffectManager* manager,
                                            EffectHandle* eh, u16 life, const Vec3* pos, const Vec3* vel) {
    NW4R_POINTER_ASSERT(setting, 0x20, lbl_805922D8);
    NW4R_POINTER_ASSERT(eh, 0x21, lbl_80592310);

    if (self->mCount >= 0x400) {
        nw4r::db::Warning(lbl_805922C0, 0x25, lbl_80592344);
        return;
    }

    self->mEntry[self->mCount].mType = 0;
    self->mEntry[self->mCount].mFlags = 0;
    self->mEntry[self->mCount].mLife = life;
    fn_800A3390(&self->mEntry[self->mCount].mSetting, setting);
    self->mEntry[self->mCount].mpManager = manager;
    ef_ref_object_add_ref(manager);
    self->mEntry[self->mCount].mpHandle = eh;
    if (pos != 0) {
        self->mEntry[self->mCount].mFlags |= 1;
        copyVec3(&self->mEntry[self->mCount].mPos, pos);
    }
    if (vel != 0) {
        self->mEntry[self->mCount].mFlags |= 2;
        copyVec3(&self->mEntry[self->mCount].mVel, vel);
    }
    self->mCount++;
}

/* 0x800A337C - the manager's AddRef. */
extern "C" u32 ef_ref_object_add_ref(EffectManager* manager) {
    return ++manager->mRefCount;
}

/* 0x800A3390 - copies the 0xA-byte setting record field by field. */
extern "C" void fn_800A3390(Setting* dst, const Setting* src) {
    dst->mId = src->mId;
    dst->mArg0 = src->mArg0;
    dst->mArg1 = src->mArg1;
    dst->mArg2 = src->mArg2;
    dst->mArg3 = src->mArg3;
    dst->mArg4 = src->mArg4;
    dst->mArg5 = src->mArg5;
    dst->mArg6 = src->mArg6;
    dst->mArg7 = src->mArg7;
}

/* 0x800A33DC (0x33C): queues a type-1 (emitter) creation, the same shape as the type-0 path. */
extern "C" void ef_creation_queue_add_type1(CreationQueue* self, const Setting* setting, EffectManager* manager,
                                            EffectHandle* eh, u16 life, const Vec3* pos, const Vec3* vel) {
    NW4R_POINTER_ASSERT(setting, 0x41, lbl_805922D8);
    NW4R_POINTER_ASSERT(eh, 0x42, lbl_80592310);

    if (self->mCount >= 0x400) {
        nw4r::db::Warning(lbl_805922C0, 0x46, lbl_80592344);
        return;
    }

    self->mEntry[self->mCount].mType = 1;
    self->mEntry[self->mCount].mFlags = 0;
    self->mEntry[self->mCount].mLife = life;
    fn_800A3390(&self->mEntry[self->mCount].mSetting, setting);
    self->mEntry[self->mCount].mpManager = manager;
    ef_ref_object_add_ref(manager);
    self->mEntry[self->mCount].mpHandle = eh;
    if (pos != 0) {
        self->mEntry[self->mCount].mFlags |= 1;
        copyVec3(&self->mEntry[self->mCount].mPos, pos);
    }
    if (vel != 0) {
        self->mEntry[self->mCount].mFlags |= 2;
        copyVec3(&self->mEntry[self->mCount].mVel, vel);
    }
    self->mCount++;
}

/* 0x800A3800 - the manager's Release (asserts the count is live, decrements it and disposes on zero). */
extern "C" u32 fn_800A3800(EffectManager* manager);

/* 0x800A3718 - executes the queue: dispatch each entry, release its manager, reset the count. */
extern "C" void fn_800A3718(CreationQueue* self) {
    s32 i = 0;
    CreationQueueEntry* e = self->mEntry;

    for (; i < self->mCount; e++, i++) {
        u32 hasPos = e->mFlags & 1;
        const Vec3* pos = 0;
        if (hasPos != 0) {
            pos = &e->mPos;
        }

        switch (e->mType) {
        case 0:
            fn_800A7750(e->mpManager->mpEmitterManager->mpEmitterForm, e->mpHandle, &e->mSetting,
                        e->mpManager, e->mLife, pos);
            fn_800A3800(e->mpManager);
            break;
        case 1: {
            EmitterForm* form = e->mpManager->mpEmitterManager->mpEmitterForm;
            form->mpVtbl->createEmitter(form, e->mpHandle, &e->mSetting, e->mpManager, e->mLife,
                                        pos);
            fn_800A3800(e->mpManager);
            break;
        }
        }
    }

    self->mCount = 0;
}

/* 0x800A3800 - the manager's Release. */
extern "C" u32 fn_800A3800(EffectManager* manager) {
    NW4R_ASSERT(manager->mRefCount > 0, 0x6C, lbl_80592364);

    u32 refCount = manager->mRefCount - 1;
    manager->mRefCount = refCount;
    if (refCount == 0 && manager->mRefCountType == 2) {
        manager->mpVtbl->dispose(manager);
    }

    return manager->mRefCount;
}

/* 0x800A3888 - an empty stub. */
extern "C" void fn_800A3888(void) {
}
