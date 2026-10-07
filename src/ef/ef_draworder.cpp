/*
 * ef/ef_draworder.cpp - the nw4r::ef draw-order list: the effect's list head accessor, the list class's three
 *   virtual slots (ordered insert by the node's draw order at +0x89, removal, a walk), the object constructor,
 *   the `{two ut::Lists + counter}` sub-object constructors and the deleting destructor.
 * RANGE. .text 0x800A388C-0x800A40F4 (9 functions); extab 0x80009B60-0x80009BA0, extabindex 0x80022CEC-0x80022D4C,
 *   .data 0x805923A0-0x80592430 (the `__FILE__` string "ef_draworder.cpp" first, then the list class's table
 *   `lbl_8059241C` = [0, 0, fn_800A3B20, fn_800A3D7C, fn_800A39A4]).
 * FLAGS. `cflags_main`; file-wide `#pragma peephole off` (retail keeps `lis` + `addi r0` + `stw r0` for the three
 *   table stores and `extsh` + `cmpwi` for the deleting destructor's flag test).
 * NAMES. The map has only `fn_` stems for the range; the types and fields are GUESSes from the bodies.
 * RESIDUALS. 1 partial row:
 *  - `fn_800A39A4`: the virtual dispatch loads the node's table through the node's home register (`lwz r5,
 *    28(r29); lwz r12, 28(r5)`) where retail reuses r3 (`lwz r12, 28(r3); lwz r12, 28(r12)`); retail's shape is a
 *    virtual member call on the node.
 *   flipcheck: `.data` claimed, not emitted.
 * SHAPES. The list class's slots take the class's `this` in r3 (unused) and the real arguments in r4/r5; the
 *   class's constructor is in `ef/ef_effectsystem.cpp` (`fn_800A620C` stores `lbl_8059241C`).
 * SHAPES. The pointer guards are the `NW4R_POINTER_ASSERT` RVL address-range chain the ef units share (seven
 *   ranges, the first `if` carrying two tests), with this unit's own file string.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h" /* the canonical nw4r::ef::Effect declaration (RetireEmitterAll, rule 1) */
#include "fn_8004CAD8/mtx.h" /* the matrix helpers (rule 2) */
#include "nw4r/fn_805012C4.h" /* nw4r::ut::List_* (rule 2) */

/* `nw4r::db::Panic`, declared in its namespace so the front-end emits the map's mangling
 * (`Panic__Q24nw4r2dbFPCciPCce`). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r

/* The global `operator delete`; declaring the mangled `__dl__FPv` would be rule 9's violation. */
void operator delete(void* ptr) throw();

extern "C" {

/* The strings of this unit's `.data` (claimed, not emitted) and the two class tables of
 * `ef/ef_effect.cpp`'s `.data`. */
extern const char lbl_805923A0[]; /* "ef_draworder.cpp"                                      */
extern const char lbl_805923B4[]; /* "NW4R:Pointer Error\nef(=%p) is not valid pointer."     */
extern const char lbl_805923E8[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."     */
extern void* lbl_80592588[];      /* ef_effect's Effect table: 0, 0, then six method words   */
extern void* lbl_805925A8[];      /* the root base's vtable: 0, 0, then two method words     */

} /* extern "C" */

/* -------------------------------------------------------------------------------------------------
 * types
 * ------------------------------------------------------------------------------------------------- */

/* `nw4r::ut::List` as `List_Init` builds it: a head and tail pointer, a live count and the offset
 * of the link record inside the element (`List_GetNext`/`List_GetPrev` read `node + mOffset + 4`). */
typedef struct EfDrawOrderList {
    /* +0x00 */ void* mHead;
    /* +0x04 */ void* mTail;
    /* +0x08 */ u16 mCount;
    /* +0x0A */ u16 mOffset;
} EfDrawOrderList; /* size: 0xC */

/* The `{two lists + counter}` sub-object fn_800A3FFC constructs; every effect class embeds one
 * (ef/ef_emitter.cpp at +0xC0, ef/ef_particlemanager.cpp at +0x38, this unit at +0x24). */
typedef struct EfDrawOrderGroup {
    /* +0x00 */ EfDrawOrderList mListA;
    /* +0x0C */ EfDrawOrderList mListB;
    /* +0x18 */ u16 mCount;
} EfDrawOrderGroup; /* size: 0x1C */

struct EfDrawOrderNode;

/* The draw-order node's function table.  The walk (fn_800A39A4) reaches slot +0x1C of the table the
 * node carries at +0x1C. */
typedef struct EfDrawOrderNodeVtbl {
    /* +0x00 */ u8 pad_0x00[0x1C];
    /* +0x1C */ void (*method_0x1C)(struct EfDrawOrderNode* self, void* arg);
} EfDrawOrderNodeVtbl; /* size: 0x20 */

/* The node the effect's draw list holds (the particle manager in nw4r::ef).  The draw order is the
 * byte the ordered insert compares, at +0x89; the node continues past what this unit reads. */
typedef struct EfDrawOrderNode {
    /* +0x00 */ u8 pad_0x00[0x1C];
    /* +0x1C */ EfDrawOrderNodeVtbl* mpVtbl;
    /* +0x20 */ u8 pad_0x20[0x69];
    /* +0x89 */ u8 mDrawOrder;
} EfDrawOrderNode; /* size: 0x8A (lower bound; the record continues past what this unit reads) */

/* The object fn_800A3F98 constructs and fn_800A4090 destroys.  Base sub-object (vtable at +0x1C)
 * followed by the group at +0x24, the matrix at +0x58, the vector at +0x88 and the draw list at
 * +0x94.  Size is a lower bound: the record continues past +0xA0. */
typedef struct EfDrawOrderObject {
    /* +0x00 */ u8 pad_0x00[0x1C];
    /* +0x1C */ void** mpVtbl;
    /* +0x20 */ u8 pad_0x20[0x04];
    /* +0x24 */ EfDrawOrderGroup mGroup;
    /* +0x40 */ u8 pad_0x40[0x18];
    /* +0x58 */ MTX34 mMat_0x58;
    /* +0x88 */ VEC3 mVec_0x88;
    /* +0x94 */ EfDrawOrderList mDrawList;
} EfDrawOrderObject; /* size: 0xA0 (lower bound; the record continues past what this unit reads) */

/* NW4R_POINTER_ASSERT's RVL address-range check (MEM1/MEM2, cached and uncached, plus locked cache),
 * shared verbatim with ef/ef_creationqueue.cpp and ef/ef_drawlinestrategy.cpp. */
#define NW4R_VALID_PTR(p)                                                                          \
    (((u32)(p) & 0xFF000000) == 0x80000000 || ((u32)(p) & 0xFF800000) == 0x81000000 ||             \
     ((u32)(p) & 0xF8000000) == 0x90000000 || ((u32)(p) & 0xFF000000) == 0xC0000000 ||             \
     ((u32)(p) & 0xFF800000) == 0xC1000000 || ((u32)(p) & 0xF8000000) == 0xD0000000 ||             \
     ((u32)(p) & 0xFFFFC000) == 0xE0000000)

#define NW4R_POINTER_ASSERT(p, line, msg)                                                          \
    (NW4R_VALID_PTR(p) ? (void)0 : nw4r::db::Panic(lbl_805923A0, line, msg, (p)))

extern "C" {

/* Callees outside this unit: two particle-manager functions. */
void fn_800AE628(void* pm);                                    /* particle manager: world matrix */
void fn_800AE6A4(void* pm);                                    /* particle manager: tail (a `blr`) */

/* This unit's own forward declarations (address order puts two bodies after their callers). */
EfDrawOrderGroup* fn_800A3FFC(EfDrawOrderGroup* group, u32 offset);
void fn_800A4030(EfDrawOrderGroup* group, u32 offset);
void fn_800A4080(EfDrawOrderObject* self);

} /* extern "C" */

/* -------------------------------------------------------------------------------------------------
 * the functions, in address order
 * ------------------------------------------------------------------------------------------------- */

#pragma peephole off

extern "C" {

/* 0x800A388C - the effect's draw-order list head (the list at +0x94). */
void* fn_800A388C(EfDrawOrderObject* ef) {
    NW4R_POINTER_ASSERT(ef, 27, lbl_805923B4);
    return &ef->mDrawList;
}

/* 0x800A39A4 - the walk slot: head to tail, rebuilding each node's world matrix before the node's
 * own slot +0x1C runs. */
void fn_800A39A4(EfDrawOrderObject* self, EfDrawOrderObject* ef, void* arg) {
    NW4R_POINTER_ASSERT(ef, 35, lbl_805923B4);

    (void)self;
    EfDrawOrderList* list = (EfDrawOrderList*)fn_800A388C(ef);
    EfDrawOrderNode* node = NULL;
    while ((node = (EfDrawOrderNode*)nw4r::ut::List_GetNext((nw4r::ut::List*)list, node)) != NULL) {
        fn_800AE628(node);
        node->mpVtbl->method_0x1C(node, arg);
        fn_800AE6A4(node);
    }
}

/* 0x800A3B20 - the insert slot: links the node in behind the last element whose draw order (+0x89)
 * is not greater than its own (the head when there is none). */
void fn_800A3B20(EfDrawOrderObject* self, EfDrawOrderObject* ef, EfDrawOrderNode* node) {
    NW4R_POINTER_ASSERT(ef, 49, lbl_805923B4);
    NW4R_POINTER_ASSERT(node, 50, lbl_805923E8);

    (void)self;
    EfDrawOrderList* list = (EfDrawOrderList*)fn_800A388C(ef);
    EfDrawOrderNode* prev = NULL;
    while ((prev = (EfDrawOrderNode*)nw4r::ut::List_GetPrev((nw4r::ut::List*)list, prev)) != NULL) {
        if (prev->mDrawOrder <= node->mDrawOrder) {
            break;
        }
    }
    nw4r::ut::List_Insert((nw4r::ut::List*)list, nw4r::ut::List_GetNext((nw4r::ut::List*)list, prev), node);
}

/* 0x800A3D7C - the vtable's remove slot: unlink one node from the effect's draw list. */
void fn_800A3D7C(EfDrawOrderObject* self, EfDrawOrderObject* ef, EfDrawOrderNode* node) {
    NW4R_POINTER_ASSERT(ef, 70, lbl_805923B4);
    NW4R_POINTER_ASSERT(node, 71, lbl_805923E8);

    (void)self;
    nw4r::ut::List_Remove((nw4r::ut::List*)fn_800A388C(ef), node);
}

/* 0x800A3F98 - the constructor: root base vtable, own vtable, group, matrix, vector, draw list. */
EfDrawOrderObject* fn_800A3F98(EfDrawOrderObject* self) {
    fn_800A4080(self);
    self->mpVtbl = lbl_80592588;
    fn_800A3FFC(&self->mGroup, 0x14);
    MTX34_ctor(&self->mMat_0x58);
    VEC3_ctor(&self->mVec_0x88);
    nw4r::ut::List_Init((nw4r::ut::List*)&self->mDrawList, 0x30);
    return self;
}

/* 0x800A3FFC - the group constructor: build the sub-object and hand it back. */
EfDrawOrderGroup* fn_800A3FFC(EfDrawOrderGroup* group, u32 offset) {
    fn_800A4030(group, (u16)offset);
    return group;
}

/* 0x800A4030 - initialises both of the group's lists and clear its counter. */
void fn_800A4030(EfDrawOrderGroup* group, u32 offset) {
    nw4r::ut::List_Init((nw4r::ut::List*)&group->mListA, (u16)offset);
    nw4r::ut::List_Init((nw4r::ut::List*)&group->mListB, (u16)offset);
    group->mCount = 0;
}

/* 0x800A4080 - the root base's vtable setter, shared by every effect-class constructor
 * (ef/ef_emitter.cpp's fn_800A6258, ef/ef_particlemanager.cpp's fn_800AB664, ...). */
void fn_800A4080(EfDrawOrderObject* self) {
    self->mpVtbl = lbl_805925A8;
}

/* 0x800A4090 - the deleting destructor: reset the vtable, retire the effect's emitters and, for a
 * positive flag, free the object. */
void* fn_800A4090(EfDrawOrderObject* self, s16 flag) {
    if (self != NULL) {
        self->mpVtbl = lbl_80592588;
        ((nw4r::ef::Effect*)self)->RetireEmitterAll();
        if (flag > 0) {
            operator delete(self);
        }
    }
    return self;
}

} /* extern "C" */
