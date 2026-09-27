/* ef/ef_creationqueue.cpp - the NW4R effect library's creation queue.
 *
 * .text 0x800A3044..0x800A388C, seven functions.  The unit's own `__FILE__` string decides the
 * registration (docs/plan.md 12, evidence class 1): every Panic/Warning in the range passes
 * `lbl_805922C0`, which reads `ef_creationqueue.cpp` at 0x805922C0 - a bare source-file name, so the
 * original TU is `ef_creationqueue.cpp`.  The `.cpp` suffix and the Panic formatter
 * (`Panic__Q24nw4r2dbFPCciPCce`) make it C++ (langcheck: a `.cpp` panic string is conclusive).
 *
 * What it is: the queue the effect library fills when it wants an effect created later.  An entry is
 * 0x30 bytes and carries the effect manager, the effect handle, a 0xA-byte setting record, a life and
 * two optional VEC3 (position/velocity); `mCount` at +0 of the queue holds the live-entry count and the
 * entry array starts at +4.  The two `Add` bodies (fn_800A3044 type 0, fn_800A33DC type 1) differ only
 * in the entry's `mType` byte and in the assert/warning line numbers; `Execute` (fn_800A3718) walks the
 * queue, dispatches on `mType` (a plain call for type 0, the form's virtual slot +0x14 for type 1),
 * releases the manager and resets the count.  fn_800A337C/fn_800A3800 are the manager's inlined
 * reference-count pair emitted out of line here (the `mRefCount > 0` assert and `referencedobject.h`
 * string are theirs); fn_800A3390 is the setting record's 0xA-byte copy; fn_800A3888 is empty.
 *
 * Names: the map has only `fn_XXXXXXXX` for this range, so the functions keep the map's stems (rule 7
 * deferred).  The types and fields are named from the evidence (the panic messages name `setting` and
 * `eh`; the callee fn_800A7750 in `ef_emitter.cpp` asserts the same handle).
 *
 * Flags: the lib's `cflags_main` (`-O3 -inline noauto -Cpp_exceptions on`) is the registered home, but
 * the target's two loose ends are the *unfused* select forms - `clrlwi` + `cmpwi` for the flag test in
 * fn_800A3718 and `addi` + `cmpwi` for the ref-count decrement in fn_800A3800 - where the peephole pass
 * emits `clrlwi.`/`addic.`.  A file-scoped `#pragma peephole off` reproduces both (and leaves the other
 * five bodies byte-identical).  Its extab 0x20 + extabindex 0x30 are exact: `-Cpp_exceptions on` emits
 * the four 8-byte unwind-only records (fn_800A3044/33DC/3718/3800).
 *
 * Result: .text 0x848/0x848 (fn_800A3044, fn_800A337C, fn_800A3390, fn_800A33DC and fn_800A3888 are
 * byte-identical).  The residuals are register allocation, not source shape:
 *   - fn_800A3718 98.10 %: MWCC keeps the loop's entry pointer in r31 and the index in r30 from the
 *     `i = 0` init (the target's are r31/r30 too when the init reads the other way), and loads the
 *     form's vtable through r9 before the slot (`lwz r9, 0x1C(r3); lwz r12, 0x14(r9)`) where the target
 *     reuses r12 (`lwz r12, ...; lwz r12, ...`).
 *   - fn_800A3800 99.56 %: the disposal call loads the vtable into r4 (`lwz r4, 0x1C(r31)`) instead of
 *     through the `this` register (`lwz r12, 0x1C(r3)`); every instruction and the section size match.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * tools/symbols/dumpmap.py lookup over all seven addresses, and config/RMHE08/symbols.txt).
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h"

/* The `VEC3_ctor` macro that used to guard this include is gone: `unsplit/ef.h` no longer declares
 * the symbol (its owner `mh3_pad.h` does), so the two headers no longer clash. */
#include "unsplit/ef.h"

#pragma peephole off

/* `nw4r::db::Panic`/`Warning`: declaring the owner's real spelling makes the C++ front-end reproduce
 * the map's mangling (`Panic__Q24nw4r2dbFPCciPCce`) exactly; spelling the mangling itself would
 * re-mangle it (docs/plan.md 6.5 rule 9). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
void Warning(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r

/* The 0xA-byte effect setting record copied into a queue entry (fn_800A3390 copies it field by field:
 * a `lha` for the first halfword, then eight byte moves at +0x2..+0x9). */
struct Setting {
    /* +0x00 */ s16 mId;
    /* +0x02 */ u8 mArg0;
    /* +0x03 */ u8 mArg1;
    /* +0x04 */ u8 mArg2;
    /* +0x05 */ u8 mArg3;
    /* +0x06 */ u8 mArg4;
    /* +0x07 */ u8 mArg5;
    /* +0x08 */ u8 mArg6;
    /* +0x09 */ u8 mArg7;
}; /* size: 0xA */

struct EffectHandle;
struct EmitterForm;
struct EffectManager;

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

/* A pending effect-creation record (stride 0x30). */
struct CreationQueueEntry {
    /* +0x00 */ u8 mType;
    /* +0x01 */ u8 mFlags;
    /* +0x02 */ u16 mLife;
    /* +0x04 */ Setting mSetting;
    /* +0x0E */ u8 pad_0x0E[0x2];
    /* +0x10 */ EffectManager* mpManager;
    /* +0x14 */ EffectHandle* mpHandle;
    /* +0x18 */ Vec3 mPos;
    /* +0x24 */ Vec3 mVel;
}; /* size: 0x30 */

/* The queue: a live-entry count and the 0x400-entry array right behind it. */
struct CreationQueue {
    /* +0x00 */ s32 mCount;
    /* +0x04 */ CreationQueueEntry mEntry[0x400];
}; /* size: 0xC004 */

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
extern "C" u32 fn_800A337C(EffectManager* manager);

/* 0x800A3044 - queues a type-0 creation. */
extern "C" void fn_800A3044(CreationQueue* self, const Setting* setting, EffectManager* manager,
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
    fn_800A337C(manager);
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
extern "C" u32 fn_800A337C(EffectManager* manager) {
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

/* 0x800A33DC - queues a type-1 creation (same shape as fn_800A3044). */
extern "C" void fn_800A33DC(CreationQueue* self, const Setting* setting, EffectManager* manager,
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
    fn_800A337C(manager);
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
