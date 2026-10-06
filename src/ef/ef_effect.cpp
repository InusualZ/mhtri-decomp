/*
 * ef/ef_effect.cpp - the nw4r::ef::Effect object (the record the memory manager's pool hands out): its
 *   initialiser and first-emitter spawn, `CreateEmitter` (warns "incomplete relocation" unless the flags' 0x40000000
 *   bit is set), the emitter retire/free paths and whole-list sweeps, the per-frame update, the flag and emitter-list
 *   accessors, `ForeachParticleManager`, the filtered walk, `SetRootMtx`, and the EffectSystem constructor, deleting
 *   destructor and singleton accessor that `ef/ef_effectsystem.cpp` calls.
 * RANGE. .text 0x800A40F4-0x800A56B0 (39 functions); extab 0x80009BA0-0x80009C38, extabindex 0x80022D4C-0x80022E30,
 *   .data 0x80592430-0x80592698 (the `__FILE__` string "ef_effect.cpp" first; the class table `lbl_80592588` = [0,
 *   0, fn_800A4464, fn_800A4470, fn_800A40F4, fn_800A5428, fn_800A4BBC, fn_800A5154]), .sdata 0x807912E0-0x807912E8,
 *   .sdata2 0x80795FF0-0x80795FF8.
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (the deleting destructor's `extsh` + `cmpwi` flag test).
 * NAMES. The map has only `fn_` stems for the range except `RetireEmitterAll`, `ForeachParticleManager` and
 *   `SetRootMtx`, written as `nw4r::ef::Effect` members.
 * RESIDUALS. The source defines `fn_800A4AF0` (8 bytes) before `RetireEmitterAll`, so the two swap places in our
 *   `.text`.
 *   6 partial rows:
 *  - `fn_800A40F4`, `fn_800A4654`, `fn_800A49B8`, `fn_800A5154`: the table dispatches load the table pointer into a
 *    scratch GPR where retail reuses r12 for both loads (`lwz r12,0(r3); lwz r12,36(r12)`), as in
 *    `ef/ef_effectsystem.cpp`;
 *  - `fn_800A4BBC`: the loop temporaries take another callee-saved rotation (r25/r26/r29/r31) and the two +0xB4
 *    load/compare pairs are scheduled differently;
 *  - `fn_800A52E4`: retail's found-flag loop is rotated (the `cmpwi r0,0` guard twice, the match test before it,
 *    `pm` itself skipped); the check-self-first loop scores higher but is 4 bytes short, so the size-exact form
 *    stays.
 *   flipcheck: `.data`, `.sdata` and `.sdata2` claimed, not emitted.
 * SHAPES. The pointer guards are the `NW4R_POINTER_ASSERT` six-BOOL chain; the file argument is the call site's
 *   own string ("ef_effect.cpp", or "activitylist.h"/"res_emitter_ac.h" for the two header asserts).
 * SHAPES. `#line` puts each `Panic`/`Warning` on retail's line (`fn_800A45DC` 134, `fn_800A51D8` 423,
 *   `RetireEmitterAll` 160, `fn_800A4AF8` 180, `fn_800A4BBC` 267/292/312/323).
 * SHAPES. `fn_800A40F4`'s fourth argument is `u16` (retail adds it with no mask), `EfEffEmitter::mField_0xB4` is
 *   `s32` (`cmpwi`), `fn_800A5114`'s bit is 0x10000, and `fn_800A559C`/`fn_800A5618` return the object.
 * SHAPES. The effect record is a local view (`EfEff`, `EfEffEmitter`, `EfEffManager`): `ef.h`'s `struct Effect` is
 *   a union of the sibling units' copies; the map-named methods cast `this` to it.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h" /* nw4r::ef::Effect (rule 9's owner) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */

namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
void Warning(const char* file, int line, const char* fmt, ...);
} // namespace db
} // namespace nw4r

/* The global `operator delete` (the map's `__dl__FPv`). */
void operator delete(void* ptr) throw();

extern "C" void OSRegisterVersion(const char* version);

/* ===================================================================================================
 * Types.  Every record states its size; the offsets are the ones the bodies load or store.
 * =================================================================================================== */

/* `nw4r::ut::List` as the ef units build it: head, tail, live count and the link offset. */
typedef struct EfEffList {
    /* +0x00 */ void* head;
    /* +0x04 */ void* tail;
    /* +0x08 */ u16 numObjects;
    /* +0x0A */ u16 linkOffset;
} EfEffList; /* size: 0xC */

/* The effect's emitter group: the live list, the retired list and the live count. */
typedef struct EfEffActivityList {
    /* +0x00 */ EfEffList mActiveList;
    /* +0x0C */ EfEffList mRetireList;
    /* +0x18 */ u16 mNumActive;
    /* +0x1A */ u16 pad_0x1A;
} EfEffActivityList; /* size: 0x1C */

/* The table a record carries at +0x1C.  Every entry is called with the record itself as `self`. */
typedef struct EfEffTable {
    /* +0x00 */ u8 pad_0x00[0x08];
    /* +0x08 */ void (*method_0x08)(void* self);
    /* +0x0C */ void (*method_0x0C)(void* self);
    /* +0x10 */ u32 (*method_0x10)(void* self, void* a, void* b, u32 c);
    /* +0x14 */ u8 pad_0x14[0x04];
    /* +0x18 */ void (*method_0x18)(void* self);
    /* +0x1C */ void (*method_0x1C)(void* self);
    /* +0x20 */ void (*method_0x20)(void* self);
    /* +0x24 */ void (*method_0x24)(void* self);
} EfEffTable; /* size: 0x28 */

/* The memory manager fn_800A4420 hands back: a table with the effect-pool getter/releaser. */
typedef struct EfEffMemMgrVtbl {
    /* +0x00 */ u8 pad_0x00[0x24];
    /* +0x24 */ void* (*getEmitter)(void* self);
    /* +0x28 */ void (*releaseEmitter)(void* self, void* emitter);
} EfEffMemMgrVtbl; /* size: 0x2C */

typedef struct EfEffMemMgr {
    /* +0x00 */ EfEffMemMgrVtbl* vtable;
} EfEffMemMgr; /* size: 0x04 (lower bound: the manager's own state is not this unit's) */

/* The object the effect keeps at +0x20 (`mManagerES`): its first word is the record `fn_800A4420`
 * dereferences and its +0x04 word is the effect's +0xA0 back-pointer. */
typedef struct EfEffManager {
    /* +0x00 */ void* field_0x00;
    /* +0x04 */ void* field_0x04;
    /* +0x08 */ u8 pad_0x08[0x08];
    /* +0x10 */ u32 field_0x10; /* the pool link fn_800A5104 tests and fn_800A3718 resets */
} EfEffManager; /* size: 0x14 (lower bound: only +0x00/+0x04/+0x10 are touched) */

/* The object the effect keeps at +0xA0; one virtual call reaches its slot +0x10. */
typedef struct EfEffA0Obj {
    /* +0x00 */ void** vtable;
} EfEffA0Obj; /* size: 0x04 */

/* The effect object this unit works on (`nw4r::ef::Effect`).  A local view (rule 1 debt above). */
typedef struct EfEff {
    /* +0x00 */ void* mMemoryManager; /* fn_800A4420 dereferences it */
    /* +0x04 */ u8 pad_0x04[0x04];
    /* +0x08 */ u16 mNumEmitter; /* the head word fn_800A4AF0 reads through +0x24 */
    /* +0x0A */ u16 pad_0x0A;
    /* +0x0C */ s32 mState; /* 1 = active, 2 = created, 3 = retired */
    /* +0x10 */ void* mField_0x10; /* cleared by fn_800A444C, tested by fn_800A49B8 */
    /* +0x14 */ u8 pad_0x14[0x04];
    /* +0x18 */ u16 mField_0x18;
    /* +0x1A */ u16 pad_0x1A;
    /* +0x1C */ EfEffTable* mTable; /* the record's own table (fn_800A49B8 reaches it) */
    /* +0x20 */ EfEffManager* mManagerES; /* the memory/effect manager that owns it */
    /* +0x24 */ EfEffActivityList mEmitters;
    /* +0x40 */ u32 mGroupID;
    /* +0x44 */ u8 pad_0x44[0x04];
    /* +0x48 */ u32 mField_0x48; /* four words cleared by fn_800A40F4 */
    /* +0x4C */ u32 mField_0x4C;
    /* +0x50 */ u32 mField_0x50;
    /* +0x54 */ u32 mFlags_0x54; /* bit 0, bit 1 and bit 15 the accessors touch */
    /* +0x58 */ MTX34 mRootMtx; /* SetRootMtx and fn_800A504D4 write it */
    /* +0x88 */ f32 mField_0x88;
    /* +0x8C */ f32 mField_0x8C;
    /* +0x90 */ f32 mField_0x90;
    /* +0x94 */ u8 pad_0x94[0x0C];
    /* +0xA0 */ void* mField_0xA0; /* the object fn_800A5154 dispatches to */
} EfEff; /* size: 0xA4 (lower bound: +0xA0 is the highest field any body touches) */

/* The work record `fn_800A4864` hands back for an emitter: the flags word at its first byte. */
typedef struct EfEffEmitterWork {
    /* +0x00 */ u32 flags;
} EfEffEmitterWork; /* size: 0x04 (lower bound: only this word is read) */

/* An emitter/particle-manager record: the objects the effect's +0x24 list holds. */
typedef struct EfEffEmitter {
    /* +0x000 */ const char* mName; /* fn_800A485C hands it back for the Warning */
    /* +0x004 */ u8 pad_0x004[0x04];
    /* +0x008 */ u32 mFlags_0x08; /* fn_800A4864 hands the record at +0x08 back */
    /* +0x00C */ s32 mState;
    /* +0x010 */ void* mField_0x10;
    /* +0x014 */ u8 pad_0x014[0x08];
    /* +0x01C */ EfEffTable* mTable;
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

/* The EffectSystem the constructor/destructor work on: only the words they touch. */
typedef struct EfEffSys {
    /* +0x0000 */ void* mMemoryManager;
    /* +0x0004 */ u8 pad_0x0004[0x0C];
    /* +0x0010 */ u8 field_0x10;
    /* +0x0011 */ u8 pad_0x0011[0xC003];
    /* +0xC014 */ u32 mMaxGroupID;
    /* +0xC018 */ u8 pad_0xC018[0x08];
    /* +0xC020 */ VEC3 mRefPos;
    /* +0xC02C */ MTX34 mRefMtx;
    /* +0xC05C */ u8 pad_0xC05C[0x08];
    /* +0xC064 */ u8 mField_0xC064;
} EfEffSys; /* size: 0xC068 (the symbol map's size for lbl_806884D0) */

/* ===================================================================================================
 * The strings and constants of this unit's claimed data, `ef/ef_effectsystem.cpp`'s once-flag and singleton
 * (all declared, never defined) and the declarations of the callees.
 * =================================================================================================== */

extern "C" {

extern const char lbl_80592430[]; /* "ef_effect.cpp"                                     */
extern const char lbl_80592440[]; /* "NW4R:Pointer Error\nmanager(=%p) is not valid pointer." */
extern const char lbl_80592478[]; /* "NW4R:Pointer Error\neh(=%p) is not valid pointer."     */
extern const char lbl_805924AC[]; /* "NW4R:Pointer Error\ntarget(=%p) is not valid pointer." */
extern const char lbl_805924E4[]; /* "incomplete relocation (emitter:%s)"                    */
extern const char lbl_80592508[]; /* "...UtlistSize(&mActivityList.mActiveList) < ..."       */
extern const char lbl_8059255C[]; /* "NW4R:Failed assertion idx < GetNumEmitter()"           */
extern const char lbl_805925B8[]; /* "NW4R:Pointer Error\nmData(=%p) is not valid pointer."  */
extern const char lbl_805925EC[]; /* "res_emitter_ac.h"                                      */
extern const char lbl_80592600[]; /* "...mActiveList.numObjects >= mNumActive"               */
extern const char lbl_8059263C[]; /* "activitylist.h"                                        */
extern f32 lbl_80795FF0;          /* 0.0f (the three cleared floats)                         */
extern void* lbl_807912E0;        /* the version string registered once                      */
extern u32 lbl_8079491C;          /* the once-only init flag fn_800A559C sets                */
extern u8 lbl_806884D0[];         /* the EffectSystem singleton (0xC068 B)                   */

/* Callees outside this unit, with C linkage: retail's relocations carry their plain map names. */
void fn_800A2FA4(void* p);
void fn_800A3718(void* p);
void fn_800A3800(void* p);
u32 fn_800A5948(EfEffSys* self, EfEff* target);
void fn_800A5D8C(EfEffSys* self, u32 groupID);
u32 fn_800A6350(void* p);
void fn_800A94A4(void* node, MTX34* mtx);
void fn_800A95D8(void* node);
u32 fn_800A98D4(void* pm, void (*cb)(void*, u32), u32 arg, bool flag, u32 zero);
void fn_800AE500(void* pm, u32 flag);
void fn_800AE5B0(void* pm);
void mtx34_identity(void* mtx);
void fn_8007100C(MTX34* dst, const MTX34* src);
u16 fn_8009B374(void* list, void** buf, u16 size);
void fn_80501A64(void* list, void* node);
void fn_80501BF4(void* list, void* node);
void* fn_80501C60(void* list, void* node);
void* fn_80501C9C(void* list, u16 idx);

/* This unit's own symbols (address order). */
u32 fn_800A40F4(EfEff* self, EfEffManager* mgr, void* eh, u16 n);
void fn_800A43E8(EfEffActivityList* list, void* node);
void* fn_800A4420(void* p);
void fn_800A4428(EfEffActivityList* list);
u32 fn_800A444C(EfEff* self);
u32 fn_800A4464(EfEff* self);
u32 fn_800A4470(EfEff* self);
u32 fn_800A4474(EfEff* self, EfEffEmitter* em);
void fn_800A45DC(EfEffActivityList* list, void* node);
u32 fn_800A4654(EfEff* self, EfEffEmitter* emitter, u8 a, u16 b);
void* fn_800A485C(void* p);
void* fn_800A4864(void* p);
u32 fn_800A486C(EfEff* self, EfEffEmitter* em);
void fn_800A49B8(EfEffEmitter* em);
void fn_800A4A18(void* p);
void fn_800A4A1C(EfEffActivityList* list, void* node);
u16 fn_800A4AF0(EfEffList* list);
u32 fn_800A4AF8(EfEff* self);
void fn_800A4BBC(EfEff* self, u32 flag);
u32 fn_800A50EC(EfEff* self);
u32 fn_800A5104(void* p);
void fn_800A5114(EfEff* self, u32 on);
u32 fn_800A513C(EfEff* self);
void fn_800A5154(EfEff* self, void* arg);
u32 fn_800A51B0(EfEff* self);
void* fn_800A51C8(EfEff* self);
u16 fn_800A51D0(EfEff* self);
void* fn_800A51D8(EfEff* self, u16 idx);
s32 fn_800A5248(void* p);
void* fn_800A5250(EfEffList* list);
u32 fn_800A52E4(EfEff* self, void (*cb)(void*, void*), void* arg, u32 flag, EfEffEmitter* match);
u32 fn_800A5428(EfEff* self, void** pp, u8 a, u16 b);
void* fn_800A5484(void** pp);
EfEffSys* fn_800A559C(EfEffSys* self);
void* fn_800A5618(EfEffSys* self, s16 flag);
void* fn_800A56A4(void);

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

/* ===================================================================================================
 * 0x800A40F4 - the effect initialiser + first-emitter spawn.
 * =================================================================================================== */

extern "C" u32 fn_800A40F4(EfEff* self, EfEffManager* mgr, void* eh, u16 n) {
    EfEffMemMgr* mm;
    EfEffEmitter* em;

#line 41
    NW4R_POINTER_ASSERT(mgr, lbl_80592430, lbl_80592440);
#line 42
    NW4R_POINTER_ASSERT(eh, lbl_80592430, lbl_80592478);

    fn_800A444C(self);
    fn_800A4428(&self->mEmitters);
    self->mField_0xA0 = mgr->field_0x04;
    mtx34_identity(&self->mRootMtx);
    self->mManagerES = mgr;
    self->mField_0x48 = 0;
    self->mField_0x4C = 0;
    self->mField_0x50 = 0;
    self->mFlags_0x54 = 0;
    f32 zero = lbl_80795FF0;
    self->mField_0x88 = zero;
    self->mField_0x8C = zero;
    self->mField_0x90 = zero;

    mm = (EfEffMemMgr*)fn_800A4420(mgr);
    em = (EfEffEmitter*)mm->vtable->getEmitter(mm);
    if (em == NULL) {
        return 0;
    }
    if (em->mTable->method_0x10(em, self, eh, 128) == 0) {
        mm = (EfEffMemMgr*)fn_800A4420(mgr);
        mm->vtable->releaseEmitter(mm, em);
        return 0;
    }
    fn_800A43E8(&self->mEmitters, em);
    em->mState = 1;
    em->mField_0xE8 += n;
    return 1;
}

/* 0x800A43E8 - appends an emitter to the group and bump the live count. */
extern "C" void fn_800A43E8(EfEffActivityList* list, void* node) {
    fn_80501A64(&list->mActiveList, node);
    list->mNumActive++;
}

/* 0x800A4420 - the manager getter: the record's first word. */
extern "C" void* fn_800A4420(void* p) {
    return *(void**)p;
}

/* 0x800A4428 - clears an activity group (both lists and the count). */
extern "C" void fn_800A4428(EfEffActivityList* list) {
    list->mActiveList.head = NULL;
    list->mActiveList.numObjects = 0;
    list->mActiveList.tail = NULL;
    list->mRetireList.head = NULL;
    list->mRetireList.numObjects = 0;
    list->mRetireList.tail = NULL;
    list->mNumActive = 0;
}

/* 0x800A444C - resets the effect's state to active. */
extern "C" u32 fn_800A444C(EfEff* self) {
    self->mField_0x10 = NULL;
    self->mState = 1;
    return 1;
}

/* 0x800A4464 - the table's direct-retire slot: hand the effect to its system. */
extern "C" u32 fn_800A4464(EfEff* self) {
    return fn_800A5948((EfEffSys*)self->mManagerES, self);
}

/* 0x800A4470 - the table's retire-all slot. */
extern "C" u32 fn_800A4470(EfEff* self) {
    return ((nw4r::ef::Effect*)self)->RetireEmitterAll();
}

/* 0x800A4474 - retires one emitter and free its blocks. */
extern "C" u32 fn_800A4474(EfEff* self, EfEffEmitter* em) {
#line 93
    NW4R_POINTER_ASSERT(em, lbl_80592430, lbl_805924AC);

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
extern "C" void fn_800A45DC(EfEffActivityList* list, void* node) {
#line 133
    if (list->mActiveList.numObjects < list->mNumActive) {
        nw4r::db::Panic(lbl_8059263C, __LINE__, lbl_80592600);
    }
    fn_80501BF4(&list->mActiveList, node);
    fn_80501A64(&list->mRetireList, node);
}

/* 0x800A4654 - creates an emitter on the effect's list. */
extern "C" u32 fn_800A4654(EfEff* self, EfEffEmitter* emitter, u8 a, u16 b) {
    EfEffMemMgr* mm;
    EfEffEmitter* em;

#line 114
    NW4R_POINTER_ASSERT(emitter, lbl_80592430, lbl_80592478);

    if ((((EfEffEmitterWork*)fn_800A4864(emitter))->flags & 0x40000000u) == 0) {
#line 118
        nw4r::db::Warning(lbl_80592430, __LINE__, lbl_805924E4, fn_800A485C(emitter));
        return 0;
    }
    mm = (EfEffMemMgr*)fn_800A4420(self->mManagerES);
    em = (EfEffEmitter*)mm->vtable->getEmitter(mm);
    if (em == NULL) {
        return 0;
    }
    if (em->mTable->method_0x10(em, self, emitter, a) == 0) {
        mm = (EfEffMemMgr*)fn_800A4420(self->mManagerES);
        mm->vtable->releaseEmitter(mm, em);
        return 0;
    }
    fn_800A43E8(&self->mEmitters, em);
    em->mState = 1;
    em->mField_0xE8 += b;
    return (u32)em;
}

/* 0x800A485C - the record's name getter (its first word). */
extern "C" void* fn_800A485C(void* p) {
    return *(void**)p;
}

/* 0x800A4864 - the emitter's work record (the object at +0x08). */
extern "C" void* fn_800A4864(void* p) {
    return (u8*)p + 8;
}

/* 0x800A486C - retires one live emitter. */
extern "C" u32 fn_800A486C(EfEff* self, EfEffEmitter* em) {
#line 144
    NW4R_POINTER_ASSERT(em, lbl_80592430, lbl_805924AC);

    if (em->mState != 1) {
        return 0;
    }
    fn_800A4A1C(&self->mEmitters, em);
    fn_800A49B8(em);
    return 1;
}

/* 0x800A49B8 - runs an emitter's teardown: table slot 0x0C, state 2, then slot 0x08 when detached. */
extern "C" void fn_800A49B8(EfEffEmitter* em) {
    em->mTable->method_0x0C(em);
    em->mState = 2;
    if (em->mField_0x10 == NULL) {
        em->mTable->method_0x08(em);
    }
}

/* 0x800A4A18 - the base table's empty slot. */
extern "C" void fn_800A4A18(void* p) {
    (void)p;
}

/* 0x800A4A1C - drops one from the group's live count. */
extern "C" void fn_800A4A1C(EfEffActivityList* list, void* node) {
    (void)node;
    list->mNumActive--;
}

/* 0x800A4AF0 - the list's live count. */
extern "C" u16 fn_800A4AF0(EfEffList* list) {
    return list->numObjects;
}

/* ===================================================================================================
 * 0x800A4A2C - `Effect::RetireEmitterAll`: retire every live emitter on the effect's list.
 * =================================================================================================== */

u32 nw4r::ef::Effect::RetireEmitterAll() {
    EfEff* self = (EfEff*)this;
    u32 count = 0;
    EfEffEmitter* list[NW4R_EF_MAX_EMITTER];

#line 159
    if ((u16)fn_800A4AF0(&self->mEmitters.mActiveList) >= NW4R_EF_MAX_EMITTER) {
        nw4r::db::Panic(lbl_80592430, __LINE__, lbl_80592508);
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

/* 0x800A4AF8 - retires every particle on every live emitter. */
extern "C" u32 fn_800A4AF8(EfEff* self) {
    u32 count = 0;
    EfEffEmitter* list[NW4R_EF_MAX_EMITTER];

#line 179
    if ((u16)fn_800A4AF0(&self->mEmitters.mActiveList) >= NW4R_EF_MAX_EMITTER) {
        nw4r::db::Panic(lbl_80592430, __LINE__, lbl_80592508);
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

extern "C" void fn_800A4BBC(EfEff* self, u32 flag) {
    EfEffEmitter* list[NW4R_EF_MAX_EMITTER];
    EfEffEmitter* node;
    u16 num;
    u16 i;

    if (flag & 1u) {
        node = NULL;
        while ((node = (EfEffEmitter*)fn_80501C60(&self->mEmitters.mActiveList, node)) != NULL) {
            if ((u32)(node->mState - 1) <= 1u) {
                if ((node->mField_0x24 & 0x10000u) || (node->mField_0x24 & 0x8000u)) {
                    node->mTable->method_0x24(node);
                }
            }
        }
        return;
    }

    if (fn_800A513C(self) != 0) {
        return;
    }

    if ((flag & 2u) == 0u) {
        node = NULL;
        while ((node = (EfEffEmitter*)fn_80501C60(&self->mEmitters.mActiveList, node)) != NULL) {
            if ((u32)(node->mState - 1) <= 1u && node->mField_0xB4 == 1u) {
                node->mField_0xB4 = 0;
            }
            copyVec3(&node->mField_0x114, &node->mField_0x108);
            EfEffEmitter* child = NULL;
            while ((child = (EfEffEmitter*)fn_80501C60(&node->mChildren, child)) != NULL) {
                fn_800AE500(child, 0);
            }
        }
    }

    fn_800A5114(self, 1);

    while (fn_800A50EC(self) != 0) {
        fn_800A5114(self, 0);

        for (;;) {
#line 266
            if ((u16)fn_800A4AF0(&self->mEmitters.mActiveList) >= NW4R_EF_MAX_EMITTER) {
                nw4r::db::Panic(lbl_80592430, __LINE__, lbl_80592508);
            }
            num = fn_8009B374(&self->mEmitters.mActiveList, (void**)list,
                              (u16)fn_800A4AF0(&self->mEmitters.mActiveList));
            for (i = 0; i < num; i++) {
                list[i]->mTable->method_0x18(list[i]);
            }

            node = NULL;
            while ((node = (EfEffEmitter*)fn_80501C60(&self->mEmitters.mActiveList, node)) !=
                   NULL) {
                if ((u32)(node->mState - 1) <= 1u) {
                    if ((node->mField_0x24 & 0x10000u) || (node->mField_0x24 & 0x8000u)) {
                        node->mTable->method_0x24(node);
                    }
                }
            }

#line 291
            if ((u16)fn_800A4AF0(&self->mEmitters.mActiveList) >= NW4R_EF_MAX_EMITTER) {
                nw4r::db::Panic(lbl_80592430, __LINE__, lbl_80592508);
            }
            num = fn_8009B374(&self->mEmitters.mActiveList, (void**)list,
                              (u16)fn_800A4AF0(&self->mEmitters.mActiveList));
            for (i = 0; i < num; i++) {
                EfEffEmitter* e = list[i];
                if (e->mField_0xB4 == 0u) {
                    e->mField_0xE4++;
                }
                e->mTable->method_0x1C(e);
                if (e->mField_0xB4 == 0u) {
                    e->mField_0xE4--;
                }
            }

#line 311
            if ((u16)fn_800A4AF0(&self->mEmitters.mActiveList) >= NW4R_EF_MAX_EMITTER) {
                nw4r::db::Panic(lbl_80592430, __LINE__, lbl_80592508);
            }
            num = fn_8009B374(&self->mEmitters.mActiveList, (void**)list,
                              (u16)fn_800A4AF0(&self->mEmitters.mActiveList));
            for (i = 0; i < num; i++) {
                list[i]->mTable->method_0x20(list[i]);
            }

#line 322
            if ((u16)fn_800A4AF0(&self->mEmitters.mActiveList) >= NW4R_EF_MAX_EMITTER) {
                nw4r::db::Panic(lbl_80592430, __LINE__, lbl_80592508);
            }
            num = fn_8009B374(&self->mEmitters.mActiveList, (void**)list,
                              (u16)fn_800A4AF0(&self->mEmitters.mActiveList));
            for (i = 0; i < num; i++) {
                list[i]->mTable->method_0x1C(list[i]);
            }

            if (fn_800A5104(&((EfEffManager*)self->mManagerES)->field_0x10) != 0) {
                break;
            }
            fn_800A3718(&((EfEffManager*)self->mManagerES)->field_0x10);
        }

        if (fn_800A50EC(self) != 0) {
            node = NULL;
            while ((node = (EfEffEmitter*)fn_80501C60(&self->mEmitters.mActiveList, node)) !=
                   NULL) {
                if (node->mState == 1) {
                    if (node->mField_0xE8 != 0) {
                        node->mField_0xE8--;
                        if (node->mField_0xB4 == 1u) {
                            node->mField_0xB4 = 0;
                        }
                    }
                    EfEffEmitter* child = NULL;
                    while ((child = (EfEffEmitter*)fn_80501C60(&node->mChildren, child)) != NULL) {
                        fn_800AE500(child, 1);
                    }
                }
            }
        }
    }

    node = NULL;
    while ((node = (EfEffEmitter*)fn_80501C60(&self->mEmitters.mActiveList, node)) != NULL) {
        if ((u32)(node->mState - 1) <= 1u && node->mField_0xB4 == 3u) {
            node->mField_0xB4 = 1;
        }
        EfEffEmitter* child = NULL;
        while ((child = (EfEffEmitter*)fn_80501C60(&node->mChildren, child)) != NULL) {
            fn_800AE5B0(child);
        }
        MTX34 mtx;
        MTX34_ctor(&mtx);
        fn_800A94A4(node, &mtx);
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
extern "C" void fn_800A5114(EfEff* self, u32 on) {
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
extern "C" void fn_800A5154(EfEff* self, void* arg) {
    if (fn_800A51B0(self) == 0) {
        EfEffA0Obj* o = (EfEffA0Obj*)self->mField_0xA0;
        ((void (*)(EfEffA0Obj*, EfEff*, void*))o->vtable[4])(o, self, arg);
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
extern "C" u16 fn_800A51D0(EfEff* self) {
    return fn_800A4AF0(&self->mEmitters.mActiveList);
}

/* 0x800A51D8 - the indexed live emitter. */
extern "C" void* fn_800A51D8(EfEff* self, u16 idx) {
#line 422
    if ((u16)idx >= (u16)fn_800A51D0(self)) {
        nw4r::db::Panic(lbl_80592430, __LINE__, lbl_8059255C);
    }
    return fn_80501C9C(&self->mEmitters.mActiveList, idx);
}

/* 0x800A5248 - the record's state getter. */
extern "C" s32 fn_800A5248(void* p) {
    return *(s32*)((u8*)p + 0x0C);
}

/* 0x800A5250 - the emitter list's first live element. */
extern "C" void* fn_800A5250(EfEffList* list) {
    return fn_80501C60(list, NULL);
}

/* 0x800A5258 - walks the particle managers with `cb` and returns the count the walk accumulates. */
u32 nw4r::ef::Effect::ForeachParticleManager(void (*cb)(void*, u32), u32 arg, bool flag) {
    EfEff* self = (EfEff*)this;
    u32 count = 0;
    EfEffEmitter* pm = (EfEffEmitter*)fn_800A5250(&self->mEmitters.mActiveList);
    while (pm != NULL) {
        EfEffEmitter* next =
            (EfEffEmitter*)fn_80501C60(&self->mEmitters.mActiveList, pm);
        count += fn_800A98D4(pm, cb, arg, flag, 0);
        pm = next;
    }
    return count;
}

/* 0x800A52E4 - walks the particle managers, keeping those whose +0xF4 chain holds `match`. */
extern "C" u32 fn_800A52E4(EfEff* self, void (*cb)(void*, void*), void* arg, u32 flag,
                           EfEffEmitter* match) {
    u32 count = 0;
    EfEffEmitter* pm = (EfEffEmitter*)fn_800A5250(&self->mEmitters.mActiveList);
    while (pm != NULL) {
        EfEffEmitter* next =
            (EfEffEmitter*)fn_80501C60(&self->mEmitters.mActiveList, pm);
        if (flag != 0u || fn_800A5248(pm) == 1) {
            BOOL found = FALSE;
            EfEffEmitter* p = pm;
            while (!found && p != NULL) {
                if (p == match) {
                    found = TRUE;
                }
                p = (EfEffEmitter*)p->mField_0xF4;
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
    EfEff* self = (EfEff*)this;
    fn_8007100C(&self->mRootMtx, &mtx);
    EfEffEmitter* node = NULL;
    while ((node = (EfEffEmitter*)fn_80501C60(&self->mEmitters.mActiveList, node)) != NULL) {
        if (node->mField_0xF4 == NULL) {
            fn_800A95D8(node);
        }
    }
}

/* 0x800A5428 - the table's spawn slot. */
extern "C" u32 fn_800A5428(EfEff* self, void** pp, u8 a, u16 b) {
    return fn_800A4654(self, (EfEffEmitter*)fn_800A5484(pp), a, b);
}

/* 0x800A5484 - the spawn slot's emitter getter (validated first word). */
extern "C" void* fn_800A5484(void** pp) {
#line 95
    NW4R_POINTER_ASSERT(*pp, lbl_805925EC, lbl_805925B8);
    return *pp;
}

/* 0x800A559C - the EffectSystem constructor. */
extern "C" EfEffSys* fn_800A559C(EfEffSys* self) {
    fn_800A2FA4(&self->field_0x10);
    VEC3_ctor(&self->mRefPos);
    MTX34_ctor(&self->mRefMtx);
    if (lbl_8079491C == 0) {
        lbl_8079491C = 1;
        OSRegisterVersion((const char*)lbl_807912E0);
    }
    self->mMemoryManager = NULL;
    self->mMaxGroupID = 0;
    self->mField_0xC064 = 0;
    return self;
}

/* 0x800A5618 - the EffectSystem deleting destructor. */
extern "C" void* fn_800A5618(EfEffSys* self, s16 flag) {
    if (self != NULL) {
        for (u32 i = 0; i < self->mMaxGroupID; i++) {
            fn_800A5D8C(self, i);
        }
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

/* 0x800A56A4 - the singleton accessor. */
extern "C" void* fn_800A56A4(void) {
    return lbl_806884D0;
}
