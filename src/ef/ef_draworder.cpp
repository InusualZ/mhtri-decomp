/*
 * ef/ef_draworder.cpp - the nw4r::ef draw-order list helpers.
 *
 * .text 0x800A388C..0x800A40F4, nine functions.  Final home and name, evidence class 1 (a `__FILE__`
 * string): every `Panic` in the range passes `lbl_805923A0`, which reads "ef_draworder.cpp" at
 * 0x805923A0 (read from the DOL's .data).  A bare source-file name is the original TU, so the module
 * is `ef` and the extension is the name's suffix.  `langcheck.py`'s evidence is conclusive for C++:
 * the `.cpp` name and the `Panic__Q24nw4r2dbFPCciPCce` callees (3 of them).  The seam is proven on
 * both sides: the run starts where ef/ef_creationqueue.cpp ends and stops at the next original TU
 * (0x800A40F4, whose own `__FILE__` string is "ef_effect.cpp" at 0x80592430), and the extabindex table
 * breaks at exactly those two addresses (0x80022CEC / 0x80022D4C).  Sections: extab
 * 0x80009B60..0x80009BA0 (8 unwind records, one per non-leaf body), extabindex
 * 0x80022CEC..0x80022D4C, .text 0x800A388C..0x800A40F4.
 *
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>` over all nine addresses and with
 * config/RMHE08/symbols.txt - every one is a bare `.text` fn_ name; only the two vtables carry labels).
 *
 * What it is.  The draw-order side of the effect library.  Three of the nine functions are the whole
 * vtable of a class - `lbl_8059241C` is [0, 0, fn_800A3B20, fn_800A3D7C, fn_800A39A4], a 2-word
 * head plus three virtuals: ordered insert, removal and a walk - and they are emitted here because
 * their first non-inline virtual lives here (a vtable lands in its key function's TU).  The rest is
 * the small object layer beside them:
 *
 *   fn_800A388C  `ef->mDrawList` - the draw-order list head the effect keeps at +0x94 - with the
 *                `NW4R_POINTER_ASSERT(ef, ...)` guard every ef entry point carries.
 *   fn_800A39A4  walk the list head->tail, calling `fn_800AE628` (the particle manager's world-matrix
 *                rebuild), the node's own vtable slot +0x1C and `fn_800AE6A4` per element.
 *   fn_800A3B20  ordered insert: walk tail->head while the node's draw order (+0x89) is greater than
 *                the new one and link the new node in behind the first that is not.
 *   fn_800A3D7C  unlink one node from the list.
 *   fn_800A3F98  the object constructor: the root base's vtable setter (fn_800A4080), the object's
 *                own vtable, the pooled-list head at +0x24, the matrix at +0x58, the vector at +0x88
 *                and the draw list at +0x94 (link offset 0x30).
 *   fn_800A3FFC/ fn_800A4030  the `{two ut::Lists + counter}` sub-object constructor (the effect
 *                classes each embed one; its link offset is the 0x14 argument).
 *   fn_800A4090  the deleting destructor: reset the vtable, `Effect::RetireEmitterAll()` and, for a
 *                positive flag, `operator delete`.
 *
 * Load-bearing source shapes:
 *   - the pointer guards are the `NW4R_POINTER_ASSERT` RVL address-range chain the ef units share
 *     (seven ranges, six materialised BOOLs, the first `if` carrying two tests); the file argument is
 *     this unit's own "ef_draworder.cpp" and the message comes from the call site.
 *   - `lbl_8059241C`'s slots are virtuals of a class whose definition is here and whose constructor is
 *     in `ef/ef_effectsystem.cpp` (fn_800A620C stores `lbl_8059241C` into +0x00), so each of the three
 *     takes the class's `this` in r3 - which the bodies never touch - and its real arguments in r4/r5.
 *     The reconstruction spells that first parameter out rather than defining C++ virtuals (a real
 *     vtable would re-emit `.data`, which this unit does not claim).
 *   - `lbl_805925A8` is the root base's vtable and `lbl_80592588` this class's; both are referenced,
 *     never defined (the data pass owns `.data`).
 *   - a file-scoped `#pragma peephole off` reproduces the three vtable stores (`lis` + `addi r0` +
 *     `stw r0`) and the deleting destructor's `extsh` + `cmpwi` flag test - with the pass on, MWCC
 *     folds the `addi` destination into the base register and drops the `extsh` (the same two shapes
 *     ef/ef_particle.cpp needed it for).
 *
 * Status: .text 0x868/0x868, extab 0x40/0x40, extabindex 0x60/0x60.  Eight of the nine bodies are
 * byte-identical; the ninth is
 *   fn_800A39A4  99.84 %: the virtual dispatch loads the node's table through the node's home
 *                register (`lwz r5, 28(r29); lwz r12, 28(r5)`) where retail reuses r3 for both
 *                (`lwz r12, 28(r3); lwz r12, 28(r12)`).  Every instruction and the section size match;
 *                a virtual member call on the node is what produces retail's shape, and a real
 *                virtual cannot be declared without re-emitting the class's `.data` vtable.
 * The per-symbol numbers and the pragma/flag probes are in
 * `.pi/notes/800a388c-fn-800a388c-e2cb.md`.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h" /* the canonical nw4r::ef::Effect declaration (RetireEmitterAll, rule 1) */

/* `nw4r::db::Panic`: declaring the owner's real spelling makes the C++ front-end reproduce the map's
 * mangling (`Panic__Q24nw4r2dbFPCciPCce`) exactly; spelling the mangling itself would re-mangle it
 * (docs/plan.md 6.5 rule 9). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace nw4r

/* The global `operator delete`; declaring the mangled `__dl__FPv` would be rule 9's violation. */
void operator delete(void* ptr) throw();

extern "C" {

/* The pooled data this range references but does not own (`.data` 0x805923A0..0x805925C8).  Declared,
 * never defined; the data pass owns the section. */
extern const char lbl_805923A0[]; /* "ef_draworder.cpp"                                      */
extern const char lbl_805923B4[]; /* "NW4R:Pointer Error\nef(=%p) is not valid pointer."     */
extern const char lbl_805923E8[]; /* "NW4R:Pointer Error\npm(=%p) is not valid pointer."     */
extern void* lbl_80592588[];      /* this class's vtable: 0, 0, then six method words        */
extern void* lbl_805925A8[];      /* the root base's vtable: 0, 0, then two method words     */

} /* extern "C" */

/* -------------------------------------------------------------------------------------------------
 * types
 * ------------------------------------------------------------------------------------------------- */

/* `nw4r::ut::List` as `MEMInitList` builds it: a head and tail pointer, a live count and the offset
 * of the link record inside the element (`fn_80501C60`/`fn_80501C80` read `node + mOffset + 4`). */
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

/* Callees outside this unit.  The first three belong to other registered units (rule 2 would want
 * their owner's headers; ef_emitter.cpp and fn_800AEE48.cpp declare them the same way) and the
 * MEMInitList family is unsplit, so the prototypes stay here. */
void fn_800AE628(void* pm);                                    /* particle manager: world matrix */
void fn_800AE6A4(void* pm);                                    /* particle manager: tail (a `blr`) */
void fn_8005050C(void* mtx);                                   /* matrix helper (fn_8004CAD8.cpp) */
void fn_80501AD4(void* list, void* before, void* elem);        /* ut::List insert-before/append */
void* fn_80501C60(void* list, void* node);                     /* ut::List GetNext (head when 0) */
void* fn_80501C80(void* list, void* node);                     /* ut::List GetPrev (tail when 0) */
void fn_80501BF4(void* list, void* node);                      /* ut::List unlink */
void MEMInitList(void* list, u16 offset);                      /* ut::List initialiser */

/* This unit's own forward declarations (address order puts two bodies after their callers). */
EfDrawOrderGroup* fn_800A3FFC(EfDrawOrderGroup* group, u32 offset);
void fn_800A4030(EfDrawOrderGroup* group, u32 offset);
void fn_800A4080(EfDrawOrderObject* self);

} /* extern "C" */

/* -------------------------------------------------------------------------------------------------
 * the functions, in address order
 * ------------------------------------------------------------------------------------------------- */

/* The file-scoped pragma: the three vtable stores materialise the address as `lis` + `addi r0` and
 * the deleting destructor's flag test keeps its `extsh` + `cmpwi` pair (the same two shapes
 * ef/ef_particle.cpp needed it for).  With the peephole pass on, MWCC folds them. */
#pragma peephole off

extern "C" {

/* 0x800A388C - the effect's draw-order list head (the list at +0x94). */
void* fn_800A388C(EfDrawOrderObject* ef) {
    NW4R_POINTER_ASSERT(ef, 27, lbl_805923B4);
    return &ef->mDrawList;
}

/* 0x800A39A4 - the vtable's walk slot: head->tail, rebuilding each node's world matrix before its
 * own slot +0x1C runs.  `self` is the vtable owner the compiler passes in r3; the body works from the
 * `ef` argument the slot takes in r4. */
void fn_800A39A4(EfDrawOrderObject* self, EfDrawOrderObject* ef, void* arg) {
    NW4R_POINTER_ASSERT(ef, 35, lbl_805923B4);

    (void)self;
    EfDrawOrderList* list = (EfDrawOrderList*)fn_800A388C(ef);
    EfDrawOrderNode* node = NULL;
    while ((node = (EfDrawOrderNode*)fn_80501C60(list, node)) != NULL) {
        fn_800AE628(node);
        node->mpVtbl->method_0x1C(node, arg);
        fn_800AE6A4(node);
    }
}

/* 0x800A3B20 - the vtable's insert slot: keep the list ordered by the node's draw order (+0x89).
 * Walk tail->head while the element's order is greater than the new node's, then link in behind the
 * first one that is not (the head when the walk runs off the end). */
void fn_800A3B20(EfDrawOrderObject* self, EfDrawOrderObject* ef, EfDrawOrderNode* node) {
    NW4R_POINTER_ASSERT(ef, 49, lbl_805923B4);
    NW4R_POINTER_ASSERT(node, 50, lbl_805923E8);

    (void)self;
    EfDrawOrderList* list = (EfDrawOrderList*)fn_800A388C(ef);
    EfDrawOrderNode* prev = NULL;
    while ((prev = (EfDrawOrderNode*)fn_80501C80(list, prev)) != NULL) {
        if (prev->mDrawOrder <= node->mDrawOrder) {
            break;
        }
    }
    fn_80501AD4(list, fn_80501C60(list, prev), node);
}

/* 0x800A3D7C - the vtable's remove slot: unlink one node from the effect's draw list. */
void fn_800A3D7C(EfDrawOrderObject* self, EfDrawOrderObject* ef, EfDrawOrderNode* node) {
    NW4R_POINTER_ASSERT(ef, 70, lbl_805923B4);
    NW4R_POINTER_ASSERT(node, 71, lbl_805923E8);

    (void)self;
    fn_80501BF4(fn_800A388C(ef), node);
}

/* 0x800A3F98 - the constructor: root base vtable, own vtable, group, matrix, vector, draw list. */
EfDrawOrderObject* fn_800A3F98(EfDrawOrderObject* self) {
    fn_800A4080(self);
    self->mpVtbl = lbl_80592588;
    fn_800A3FFC(&self->mGroup, 0x14);
    fn_8005050C(&self->mMat_0x58);
    fn_80043EA8(&self->mVec_0x88);
    MEMInitList(&self->mDrawList, 0x30);
    return self;
}

/* 0x800A3FFC - the group constructor: build the sub-object and hand it back. */
EfDrawOrderGroup* fn_800A3FFC(EfDrawOrderGroup* group, u32 offset) {
    fn_800A4030(group, (u16)offset);
    return group;
}

/* 0x800A4030 - initialise both of the group's lists and clear its counter. */
void fn_800A4030(EfDrawOrderGroup* group, u32 offset) {
    MEMInitList(&group->mListA, (u16)offset);
    MEMInitList(&group->mListB, (u16)offset);
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
