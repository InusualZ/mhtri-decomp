/*
 * ef/ef_effectsystem.cpp - the nw4r::ef effect system: its group table, its pools and its walkers.
 *
 * .text 0x800A56B0..0x800A6350, twenty-three functions.  Sections: extab 0x80009C38..0x80009CD4,
 * extabindex 0x80022E30..0x80022EE4, .text 0x800A56B0..0x800A6350, and the `.ctors` word at 0x8056F2D8,
 * which points at the file's static initializer fn_800A60C8.
 *
 * Naming note: the symbol map has only fn_XXXXXXXX for this range (checked with
 * `python tools/symbols/dumpmap.py lookup <addr>` over every one of the 23 addresses - each answers
 * `dump=zz_00a5xxx_` - and against config/RMHE08/symbols.txt, where every one is a bare `.text` fn_
 * entry; only `RetireEffect__Q34nw4r2ef12EffectSystemFPQ34nw4r2ef6Effect` carries a real name), so the
 * definitions keep the map's spelling.  The one name the map carries is written through its owner
 * (`nw4r::ef::EffectSystem::RetireEffect`, rule 9).
 *
 * Final home and name, evidence class 1 (a `__FILE__` string): every assert in the range passes
 * `lbl_80592698`, which reads "ef_effectsystem.cpp" at 0x80592698 (read from the DOL's .data - it is the
 * first item of the range's own pool, and no function below the range references it).  A bare
 * source-file name is the original TU, so the module is `ef` (both bracketing units are ef's) and the
 * extension is that name's suffix.  `langcheck.py` is conclusive for C++: the `.cpp` name, the
 * `Panic__Q24nw4r2dbFPCciPCce` / `Warning__Q24nw4r2dbFPCciPCce` callees and the range's own `.data` item
 * `lbl_8059283C` (the table the last constructor stores).  The seam is proven on both sides: the range
 * starts where `ef/ef_effect.cpp` ends (its own `__FILE__` string - "ef_effect.cpp" at 0x80592430 - heads
 * the *previous* pool, and discovery records this boundary as proposal/800A40F4's seam
 * `"ef_effectsystem.cpp" starts here`), and it stops at ef/ef_emitter.cpp's first instruction
 * (0x800A6350, whose pool starts with "ef_emitter.cpp" at 0x80592850).  The extab and extabindex runs
 * break at exactly those two addresses.
 *
 * What it is.  The NintendoWare effect library's system object - `nw4r::ef::EffectSystem`, the
 * 0xC068-byte record the symbol map carries as `lbl_806884D0` (0xC068 is the map's own size for it) and
 * that the game's effect manager holds (`fn_800D3C0C` reads the pointer out of its own +0x29074).  The
 * range reconstructs:
 *
 *   fn_800A56B0  `Initialize(maxGroupID)`: build the group table (`mActivityList`, `maxGroupID` records
 *                of 0x1C B) out of the allocator the system was handed, then bind the three borrowed
 *                engine objects and the random block.
 *   fn_800A5900/5908/590C/5940  the small helpers the group table is built with: the seed setter, the
 *                array element constructor and the two allocation shims the compiler's array form calls.
 *   fn_800A5948  retire one effect on the spot: move it to its group's retiring list, state 3.
 *   fn_800A5A90  `CreateEffect(emitter, groupID, flag)`: take an effect off the memory manager's pool,
 *                run it through the effect object's own create slot, then put it on the group's list.
 *   RetireEffect `RetireEffect(target)`: the mapped member - release the effect from its group's list
 *                and tear it down; state 1 only.
 *   fn_800A5D8C/5E6C/5F4C  the three per-group sweeps over `mActivityList[group].mActiveList` (retire
 *                the effect, retire its emitters, retire its particles), each with the 0x200-entry stack
 *                buffer `NW4R_EF_MAX_EFFECT` guards.
 *   fn_800A602C  set the system's reference transform: the reference position, the matrix and the two
 *                range floats the effect code reads at +0xC020/+0xC02C/+0xC05C/+0xC060.
 *   fn_800A60B8/60C0  the two sub-object accessors other units call (the +0x30 and +0x58 blocks; the
 *                second is the matrix ef/effect.cpp casts its result to).
 *   fn_800A60C8  the file's static initializer: the three borrowed engine objects, the 0xA0-byte record,
 *                then the system singleton and its `__register_global_object` link.
 *   fn_800A6134  the 0xA0-byte record's constructor (the game allocates 0xA0 bytes at 0x800D3D0C and
 *                calls it; the map's size for lbl_80688420 is 0xA0).
 *   fn_800A61EC/61FC/620C/6248  out-of-line copies of four other units' constructors, emitted here
 *                because this file instantiates their classes (the three borrowed objects and the
 *                `ef_draworder.cpp` list class - `lbl_8059241C` is that class's table).
 *   fn_800A6258/62C0  the emitter-side resource object's constructor and its +0x20 sub-object
 *                (ef/ef_emitter.cpp's bodies own the rest of that layout).
 *   fn_800A630C  that object's deleting destructor.
 *
 * Load-bearing source shapes:
 *   - the pointer guard is the `NW4R_POINTER_ASSERT` RVL address-range chain the ef units share (six
 *     materialised BOOLs); the file argument is this unit's own "ef_effectsystem.cpp" and the message
 *     comes from the call site.  The `__LINE__` immediates (0x4F, 0x50, 0x67, 0x7B, 0x83, 0x9F, 0xA0,
 *     0xB2, 0xC9, 0xDD) are reproduced with `#line`.
 *   - a file-scoped `#pragma peephole off` reproduces the five table stores (`lis` + `addi r0` + `stw r0`)
 *     and the deleting destructor's `extsh` + `cmpwi` flag test - with the pass on MWCC folds them (the
 *     same two shapes ef/ef_draworder.cpp and ef/ef_particle.cpp needed it for).
 *   - the group table is built with the compiler's array form: the allocator call, the shim and
 *     `__construct_new_array(block, fn_800A590C, NULL, 0x1C, maxGroupID)` sit inside one try/catch whose
 *     handler calls the empty `fn_800A5908` and rethrows.
 *   - the effect object's +0x1C is a *table*, not a sub-object: every entry is called with the effect
 *     itself as the first argument (`RetireEffect`, `fn_800A49B8` and `CreateEffect` all do).
 *   - `include/ef.h` declares `class EffectSystem` (rule 9 needs `RetireEffect` to be its member).  That
 *     declaration carries the two `virtual_0xN` placeholders ef/eft004.cpp reaches through
 *     `fn_800A4420`, i.e. it models the *memory manager* (`mMemoryManager`, this object's first word) as
 *     if it were the system, so it cannot also be the system's real layout.  The layout this file works
 *     with is therefore stated once here, as `EfSys` (rule 1 debt: the two must become one `include/ef.h`
 *     definition - booked in this unit's outbox, it would touch ef/eft004.cpp's one indirect call).
 *   - `RetireEffect` returns a value (`li r3, 1` / `li r3, 0` in the target, and the sweep adds it), so
 *     `include/ef.h`'s `void RetireEffect(Effect*)` is corrected to the mapped owner's real one; the one
 *     consumer, ef/eft004.cpp, ignores the value and its `.text` is unchanged.  The same holds for
 *     `Effect::RetireEmitterAll` (the sweep adds its result too).
 *
 * Status: all 23 symbols reconstructed, none left as a stub.  The unit is 98.678 % (official report
 * metric, .text 3232 B) and 18 of the 23 bodies are byte-identical; every symbol is at or above the
 * bar.  The residual is five functions, and each difference is a compiler shape, not a source one:
 *
 *   fn_800A56B0  97.53 %  588/592 B: the group table is built from two sizes that share the
 *                         `maxGroupID * 0x1C` sub-tree (`+ 0x20` for the allocator call, `+ 0x10` for the
 *                         placement shim), so MWCC folds that product once and hoists it into a
 *                         callee-saved register where retail materialises it twice (one `mulli` per
 *                         call, 4 B).  Every other instruction matches, including the handler.
 *   fn_800A5A90  99.68 %  376/376 B: the three indirect calls load the table and the slot through the
 *                         same register in retail (`lwz r12, 0(r3)` then `lwz r12, 0x10(r12)`) and
 *                         through an intermediate in ours.  Same instructions, register-only;
 *                         a virtual (rather than table-indexed) member call on the memory manager would
 *                         emit retail's pair, but the memory manager's own API is not named anywhere in
 *                         the map, so the table spelling the sibling units use is kept.
 *   fn_800A5D8C/ 95.96-  232/224 B each: retail keeps the scaled group index (`groupID * 0x1C`) in a
 *    5E6C/5F4C   96.05 %  callee-saved register across the `Panic` call and reuses it for all three
 *                         `&mActivityList[groupID]` uses; MWCC keeps `groupID` and re-materialises the
 *                         `mulli` per use (2 extra instructions).  Measured alternatives: a local
 *                         `EfSysActivityList* group = &self->mActivityList[groupID]` (the base is
 *                         materialised once - 212 B, 80.1 %), the `.mActiveList` member spelling
 *                         (unchanged) and an explicit scaled index local (256 B, 85.2 %).
 *
 * Everything else, including the extab of every body and the two ambiguous-source shapes (the array
 * form's `__construct_new_array` block and the EABI rethrow the handler ends with), matches byte for
 * byte.  The two load-bearing source shapes that were measured rather than guessed:
 *   - the array form's handler must end in a call the compiler knows not to return, so the rethrow is
 *     spelled `fn_80458A60(0, 0, 0)` with `__attribute__((noreturn))` (the map's own name for the EABI
 *     rethrow at 0x80458A60).  A source-level `throw;` compiles to the same call *plus* the
 *     six-instruction `__end__catch` bookkeeping and one extra extab action word (93.48 % measured).
 *   - fn_800A6134 needs its two sda2 constants in named locals, the second one declared where the
 *     first is already stored (`f32 one = ...; use; f32 zero; zero = ...; use;`): declared together they
 *     are both loaded up front (8 loads instead of 2, 86.11 %), and the store order of the two colour
 *     words is retail's own (`0x9A/0x99/0x98/0x9B`, then `0x9E/0x9D/0x9C`).
 *
 * Status detail per symbol and the probes are in .pi/notes/800a56b0-fn-800a56b0-ba01.md.
 */

#include "types.h"
#include "nw4r/math.h"
#include "ef.h" /* nw4r::ef::EffectSystem / nw4r::ef::Effect (rule 9's owner) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "fn_8004CAD8/mtx.h" /* the symbols deleted above (rule 2) */

namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
void Warning(const char* file, int line, const char* fmt, ...);
} // namespace db
} // namespace nw4r

/* The global `operator delete` (its compiler mangling is `__dl__FPv`; declaring that spelling would be
 * rule 9's violation - the sibling ef units spell it the same way). */
void operator delete(void* ptr) throw();

/* ===================================================================================================
 * Types.  Every record states its size; the offsets are the ones the bodies load or store.  The `EfSys`
 * prefix keeps this unit's views distinct from the sibling units' (`EfList`, `EfEmitterObj`, ...) - a
 * type more than one unit uses has to become one header definition (the rule 1 debt above).
 * =================================================================================================== */

/* The library's list record: head, tail, live count and the link offset `fn_800A4030` sets. */
typedef struct EfSysList {
    /* +0x00 */ void* head;
    /* +0x04 */ void* tail;
    /* +0x08 */ u16 numObjects;
    /* +0x0A */ u16 linkOffset;
} EfSysList; /* size: 0x0C */

/* One `mActivityList` group record: the group's live list, its retiring list and the live count.  The
 * assert names it (`mActivityList[group].mActiveList`); `fn_800A4428` zeroes exactly these words and
 * `fn_800A43E8` / `fn_800A45DC` / `fn_800A4A1C` keep them. size: 0x1C */
typedef struct EfSysActivityList {
    /* +0x00 */ EfSysList mActiveList;
    /* +0x0C */ EfSysList mRetireList;
    /* +0x18 */ u16 mNumActive;
    /* +0x1A */ u16 pad_0x1A;
} EfSysActivityList; /* size: 0x1C */

/* `mMemoryManager`'s table: the effect pool at +0x10/+0x14 and the byte allocator at +0x60. */
typedef struct EfSysMemoryManagerVtbl {
    /* +0x00 */ u8 pad_0x00[0x10];
    /* +0x10 */ void* (*getEffect)(void* self);
    /* +0x14 */ void (*releaseEffect)(void* self, void* effect);
    /* +0x18 */ u8 pad_0x18[0x48];
    /* +0x60 */ void* (*alloc)(void* self, u32 size);
} EfSysMemoryManagerVtbl; /* size: 0x64 */

/* The allocator/pool object the system was handed (`fn_800D3D48` stores it into the system's +0x00). */
typedef struct EfSysMemoryManager {
    /* +0x00 */ EfSysMemoryManagerVtbl* vtable;
} EfSysMemoryManager; /* size: 0x04 (lower bound: the manager's own state is not this unit's) */

/* The table the effect object carries at +0x1C.  Every entry is called with the *effect* as its first
 * argument, so the field is the table itself. */
typedef struct EfSysEffectVtbl {
    /* +0x00 */ u8 pad_0x00[0x08];
    /* +0x08 */ void (*destroy)(void* effect);
    /* +0x0C */ void (*retire)(void* effect);
    /* +0x10 */ u32 (*create)(void* effect, void* manager, void* emitter, u16 flag);
    /* +0x14 */ void (*release)(void* effect);
} EfSysEffectVtbl; /* size: 0x18 */

/* The effect object the memory manager's pool hands out (`RetireEffect` asserts `mManagerES == this` on
 * it, `CreateEffect` puts it on a group list).  +0x24..+0x3F is present in the object but untouched by
 * this range's bodies, hence the padding. size: 0x44 (lower bound) */
typedef struct EfSysEffect {
    /* +0x00 */ u8 pad_0x00[0x0C];
    /* +0x0C */ s32 state; /* 1 = active, 2 = created, 3 = retired */
    /* +0x10 */ void* field_0x10;
    /* +0x14 */ u8 pad_0x14[0x08];
    /* +0x1C */ EfSysEffectVtbl* vtable;
    /* +0x20 */ void* mManagerES; /* the system that owns it - RetireEffect compares it with `this` */
    /* +0x24 */ u8 pad_0x24[0x1C];
    /* +0x40 */ u32 mGroupID; /* the index into `mActivityList` */
} EfSysEffect; /* size: 0x44 (lower bound: +0x40 is the highest field any body touches) */

/* `nw4r::ef::EffectSystem`: the object this unit's functions work on.  Only the fields this range's
 * bodies touch are named; the record's size is the symbol map's own for `lbl_806884D0`. */
typedef struct EfSys {
    /* +0x0000 */ EfSysMemoryManager* mMemoryManager;
    /* +0x0004 */ void* mDrawOrder;    /* &lbl_80794910 - the draw-order list object */
    /* +0x0008 */ void* mStrategy;     /* &lbl_80794914 - the strategy object */
    /* +0x000C */ void* mLineStrategy; /* &lbl_80794918 - the line-strategy object */
    /* +0x0010 */ u8 pad_0x0010[0xC004];
    /* +0xC014 */ u32 mMaxGroupID;
    /* +0xC018 */ EfSysActivityList* mActivityList;
    /* +0xC01C */ u32 mRandom; /* the seed fn_800A5900 writes (ef/ef_emitter.cpp reads the same word) */
    /* +0xC020 */ nw4r::math::VEC3 mRefPos; /* copyVec3 copies the caller's vector into it */
    /* +0xC02C */ nw4r::math::MTX34 mRefMtx;
    /* +0xC05C */ f32 mRangeB;
    /* +0xC060 */ f32 mRangeA;
    /* +0xC064 */ u8 mField_0xC064; /* cleared by the constructor ef/ef_effect.cpp defines */
    /* +0xC065 */ u8 pad_0xC065[0x03];
} EfSys; /* size: 0xC068 (the symbol map's size for lbl_806884D0) */

/* The record `fn_800A4864` hands back for an emitter: the relocation flag `CreateEffect` tests at +0x00
 * (the name the Warning prints comes from `fn_800A485C`, the emitter's own +0x00). size: 0x04 (lower
 * bound: only this word is read) */
typedef struct EfSysEmitterWork {
    /* +0x00 */ u32 flags;
} EfSysEmitterWork; /* size: 0x04 (lower bound) */

/* An object whose first word is its table (the three borrowed engine objects and the class
 * fn_800A6248's constructor belongs to). */
typedef struct EfSysVtblObj {
    /* +0x00 */ void* vtable;
} EfSysVtblObj; /* size: 0x04 */

/* The +0x20 sub-object of the emitter-side object `fn_800A62C0` initialises: four transform vectors,
 * cleared in this order.  The offsets are relative to the sub-object. size: 0x94 */
typedef struct EfSysResourceSub {
    /* +0x00 */ u8 pad_0x00[0x64];
    /* +0x64 */ nw4r::math::VEC3 vec_0x84;
    /* +0x70 */ nw4r::math::VEC3 vec_0x90;
    /* +0x7C */ nw4r::math::VEC3 vec_0x9C;
    /* +0x88 */ nw4r::math::VEC3 vec_0xA8;
} EfSysResourceSub; /* size: 0x94 */

/* The emitter-side object this file's constructor builds.  ef/ef_emitter.cpp owns the rest of the layout
 * (`EfEmitterObj`, the same object) - rule 1 debt, booked in the outbox. size: 0x154 (lower bound:
 * +0x124 is the highest field the constructor touches) */
typedef struct EfSysResourceObj {
    /* +0x000 */ u8 pad_0x000[0x01C];
    /* +0x01C */ void* vtable; /* the root base's table fn_800A4080 sets, then this class's */
    /* +0x020 */ EfSysResourceSub sub_0x020;
    /* +0x0B4 */ u8 pad_0x0B4[0x00C];
    /* +0x0C0 */ EfSysActivityList particles; /* the list ef/ef_emitter.cpp calls `particles` */
    /* +0x0DC */ u8 pad_0x0DC[0x02C];
    /* +0x108 */ nw4r::math::VEC3 vec_0x108;
    /* +0x114 */ nw4r::math::VEC3 vec_0x114;
    /* +0x120 */ u8 pad_0x120[0x004];
    /* +0x124 */ nw4r::math::MTX34 mtx_0x124;
} EfSysResourceObj; /* size: 0x154 (lower bound: ef/ef_emitter.cpp's own view of the object) */

/* The object fn_800A60B8 / fn_800A60C0 are called with: ef/effect.cpp casts fn_800A60C0's result to a
 * MTX34 (the effect's root matrix) and the two returned offsets are the only thing this range
 * establishes, so the +0x30 block is a named byte and the matrix is the mapped one. size: 0x88 */
typedef struct EfSysAccessObj {
    /* +0x00 */ u8 pad_0x00[0x30];
    /* +0x30 */ u8 block_0x30; /* the sub-object fn_800A60B8 returns (its type is not this range's) */
    /* +0x31 */ u8 pad_0x31[0x27];
    /* +0x58 */ nw4r::math::MTX34 mtx_0x58; /* the matrix fn_800A60C0 returns */
} EfSysAccessObj; /* size: 0x88 (lower bound: +0x58 is the highest field returned) */

/* The 0xA0-byte record `fn_800A6134` constructs (the game allocates 0xA0 bytes at 0x800D3D0C and calls
 * it as the constructor; the map's size for lbl_80688420 is 0xA0).  No `__FILE__` string or dump name
 * identifies the type, so the fields are named for what the constructor stores in them. size: 0xA0 */
typedef struct EfSysDefaultRecord {
    /* +0x00 */ nw4r::math::MTX34 mtx_0x00;
    /* +0x30 */ nw4r::math::MTX34 mtx_0x30;
    /* +0x60 */ u8 field_0x60;
    /* +0x61 */ u8 pad_0x61[0x03];
    /* +0x64 */ u32 field_0x64;
    /* +0x68 */ u32 field_0x68;
    /* +0x6C */ u8 field_0x6C;
    /* +0x6D */ u8 pad_0x6D[0x03];
    /* +0x70 */ u32 field_0x70;
    /* +0x74 */ f32 scale_0x74;
    /* +0x78 */ f32 scale_0x78;
    /* +0x7C */ f32 scale_0x7C;
    /* +0x80 */ f32 scale_0x80;
    /* +0x84 */ u32 pad_0x84; /* present in the object, untouched by the constructor */
    /* +0x88 */ f32 scale_0x88;
    /* +0x8C */ nw4r::math::VEC3 vec_0x8C;
    /* +0x98 */ u8 color_0x98; /* white */
    /* +0x99 */ u8 color_0x99;
    /* +0x9A */ u8 color_0x9A;
    /* +0x9B */ u8 color_0x9B;
    /* +0x9C */ u8 color_0x9C; /* black */
    /* +0x9D */ u8 color_0x9D;
    /* +0x9E */ u8 color_0x9E;
    /* +0x9F */ u8 color_0x9F;
} EfSysDefaultRecord; /* size: 0xA0 */

/* This file's own pooled data.  `lbl_80592698` is the `__FILE__` string (the evidence for the name), the
 * next eight are the assert messages, and `lbl_8059283C` is the zero-filled item the last constructor
 * stores (the class table at the end of this unit's .data pool).  Declared, never defined: the data pass
 * owns .data (the rule ef/ef_draworder.cpp follows). */
extern const char lbl_80592698[];
extern const char lbl_805926AC[];
extern const char lbl_805926EC[];
extern const char lbl_80592714[];
extern const char lbl_8059274C[];
extern const char lbl_80592788[];
extern const char lbl_805927AC[];
extern const char lbl_805927E0[];
extern void* lbl_8059283C[];

/* The tables and objects of the modules this unit borrows: the three engine objects the system holds,
 * the system singleton itself (0xC068 B, the map's size), its link record, the borrowed classes' tables
 * and the two float constants the 0xA0-byte record keeps. */
extern void* lbl_80794910; /* the three borrowed engine objects (their constructors are below) */
extern void* lbl_80794914;
extern void* lbl_80794918;
extern void* lbl_8059241C[]; /* ef_draworder.cpp's list class (its key function lives there)       */
extern void* lbl_80594840[]; /* the strategy table (ef_drawstrategyimpl.cpp's fn_800C5DB8)        */
extern void* lbl_80594EC4[]; /* the line-strategy table (ef_line.cpp's fn_800CCCF8)               */
extern void* lbl_80592BA8[]; /* the emitter-side object's table (ef_emitter.cpp's eight virtuals) */
extern u8 lbl_80688420[];
extern u8 lbl_806884D0[];
extern u8 lbl_806884C0[];
extern f32 lbl_80795FF8;
extern f32 lbl_80795FFC;

/* ===================================================================================================
 * Declarations.  Every callee outside this unit is still a `fn_*` in the symbol map and unsplit, so it
 * has no owner file to move to - the rule-2 gap the campaign records for an unsplit address.  They are
 * declared with C linkage: the target's relocations are plain names, not C++ spellings (docs/plan.md 6.5
 * rule 9), and this unit's own symbols come first, in address order.
 * =================================================================================================== */

extern "C" {
/* this unit's own symbols */
u32 fn_800A56B0(EfSys* self, u32 maxGroupID);
void fn_800A5900(u32* random, u32 seed);
void fn_800A5908(void* array, void* block);
void* fn_800A590C(void* element);
void* fn_800A5940(u32 size, void* block);
u32 fn_800A5948(EfSys* self, EfSysEffect* target);
EfSysEffect* fn_800A5A90(EfSys* self, void* emitter, u32 groupID, u16 flag);
u32 fn_800A5D8C(EfSys* self, u32 groupID);
u32 fn_800A5E6C(EfSys* self, u32 groupID);
u32 fn_800A5F4C(EfSys* self, u32 groupID);
void fn_800A602C(EfSys* self, const nw4r::math::VEC3* pos, const nw4r::math::MTX34* src, f32 a, f32 b);
void* fn_800A60B8(EfSysAccessObj* self);
nw4r::math::MTX34* fn_800A60C0(EfSysAccessObj* self);
void fn_800A60C8(void);
void* fn_800A6134(EfSysDefaultRecord* self);
EfSysVtblObj* fn_800A61EC(EfSysVtblObj* self);
EfSysVtblObj* fn_800A61FC(EfSysVtblObj* self);
EfSysVtblObj* fn_800A620C(EfSysVtblObj* self);
EfSysVtblObj* fn_800A6248(EfSysVtblObj* self);
EfSysResourceObj* fn_800A6258(EfSysResourceObj* self);
EfSysResourceSub* fn_800A62C0(EfSysResourceSub* self);
void* fn_800A630C(void* self, s16 flag);

/* the callees */
void fn_800A4030(void* list, u16 linkOffset);
void* fn_800A3FFC(void* list, u32 linkOffset);
void fn_800A4080(void* self);
void* fn_800A4420(void* p);
void fn_800A4428(void* list);
void fn_800A43E8(void* list, void* node);
void fn_800A45DC(void* list, void* node);
void fn_800A49B8(void* effect);
void fn_800A4A1C(void* list, void* node);
u16 fn_800A4AF0(void* list);
u32 fn_800A4AF8(void* effect);
const char* fn_800A485C(void* p);
void* fn_800A4864(void* p);
u16 fn_8009B374(void* list, void** buf, u16 size);
void* fn_800504D4(void* mtx);
void* fn_80050508(void* mtx);
void* fn_80051570(const void* src);
void fn_800A559C(EfSys* self); /* the system's constructor lives in ef/ef_effect.cpp */
void fn_800A5618(EfSys* self, s16 flag); /* ... and so does its destructor */
void* __construct_new_array(void* block, void* (*ctor)(void*), void (*dtor)(void*), u32 size,
                            u32 count);
void fn_80458A60(void*, void*, void*) __attribute__((noreturn)); /* the EABI rethrow */
void __register_global_object(void* object, void* dtor, void* link);
void PSMTXCopy(const void* src, void* dst);
}

/* ===================================================================================================
 * Assert shapes
 * =================================================================================================== */

/* `NW4R_ASSERT(expr, msg)`: the file is "ef_effectsystem.cpp" and the message is the pooled literal; the
 * `__LINE__` value comes from a `#line` directive at the call site. */
#define NW4R_ASSERT(expr, msg)                                                                     \
    if (!(expr))                                                                                   \
    nw4r::db::Panic(lbl_80592698, __LINE__, msg)

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
            nw4r::db::Panic(lbl_80592698, __LINE__, msg, (ptr));                                   \
    }

#define NW4R_EF_MAX_EFFECT 0x200

/* The system's allocator.  The target reaches it through `fn_800A4420` (the map also carries a
 * `GetMemoryManager__Q34nw4r2ef12EffectSystemCFv` name for the class, but at an .init offset, so the
 * call keeps the map's own spelling for 0x800A4420). */
static inline EfSysMemoryManager* EfGetMemoryManager(EfSys* self) {
    return (EfSysMemoryManager*)fn_800A4420(self);
}

/* The file-scoped pragma: the five table stores materialise their address as `lis` + `addi r0` and the
 * deleting destructor keeps its `extsh` + `cmpwi` pair - the peephole pass folds both shapes (the same
 * two shapes ef/ef_draworder.cpp and ef/ef_particle.cpp needed it for).  It also keeps the index
 * expression in the three sweeps unfused (`clrlwi` + `slwi`, not `clrlslwi`). */
#pragma peephole off

/* ===================================================================================================
 * 0x800A56B0 - `Initialize`: build the group table and bind the borrowed objects.
 * =================================================================================================== */

extern "C" u32 fn_800A56B0(EfSys* self, u32 maxGroupID) {
    EfSysActivityList* list;
    void* block;

#line 79
    NW4R_POINTER_ASSERT(self->mMemoryManager, lbl_805926AC);
#line 80
    NW4R_ASSERT(maxGroupID > 0, lbl_805926EC);
    self->mMaxGroupID = maxGroupID;
    block = self->mMemoryManager->vtable->alloc(self->mMemoryManager, maxGroupID * 0x1C + 0x20);
    list = (EfSysActivityList*)fn_800A5940(maxGroupID * sizeof(EfSysActivityList) + 0x10, block);
    if (list != NULL) {
        try {
            list = (EfSysActivityList*)__construct_new_array(list, fn_800A590C, NULL,
                                                            sizeof(EfSysActivityList), maxGroupID);
        } catch (...) {
            fn_800A5908(list, block);
            fn_80458A60(0, 0, 0);
        }
    }
    self->mActivityList = list;
    for (u32 i = 0; i < self->mMaxGroupID; i++) {
        fn_800A4030(&self->mActivityList[i], 0x14);
        fn_800A4428(&self->mActivityList[i]);
    }
    fn_800A5900(&self->mRandom, 0);
    self->mDrawOrder = &lbl_80794910;
    self->mStrategy = &lbl_80794914;
    self->mLineStrategy = &lbl_80794918;
    return 1;
}

/* 0x800A5900 - the random block's seed setter (ef/ef_emitter.cpp calls it with the same shape). */
extern "C" void fn_800A5900(u32* random, u32 seed) {
    *random = seed;
}

/* 0x800A5908 - the array form's release hook (empty in this build). */
extern "C" void fn_800A5908(void* array, void* block) {
    (void)array;
    (void)block;
}

/* 0x800A590C - the array element constructor the compiler's array form calls. */
extern "C" void* fn_800A590C(void* element) {
    fn_800A4030(element, 0);
    return element;
}

/* 0x800A5940 - the allocation shim the compiler's array form calls; it hands the block back. */
extern "C" void* fn_800A5940(u32 size, void* block) {
    (void)size;
    return block;
}

/* 0x800A5948 - retire one effect on the spot: move it to its group's retiring list and mark it done. */
extern "C" u32 fn_800A5948(EfSys* self, EfSysEffect* target) {
#line 103
    NW4R_POINTER_ASSERT(target, lbl_80592714);
    fn_800A45DC(&self->mActivityList[target->mGroupID], target);
    target->state = 3;
    return 1;
}

/* 0x800A5A90 - `CreateEffect(emitter, groupID, flag)`. */
extern "C" EfSysEffect* fn_800A5A90(EfSys* self, void* emitter, u32 groupID, u16 flag) {
    if (groupID >= self->mMaxGroupID) {
#line 123
        nw4r::db::Panic(lbl_80592698, __LINE__, lbl_8059274C);
    }
    if (emitter == NULL) {
        return NULL;
    }
    if ((((EfSysEmitterWork*)fn_800A4864(emitter))->flags & 0x40000000u) == 0) {
#line 131
        nw4r::db::Warning(lbl_80592698, __LINE__, lbl_80592788, fn_800A485C(emitter));
        return NULL;
    }
    EfSysEffect* effect;
    {
        EfSysMemoryManager* mm = EfGetMemoryManager(self);
        effect = (EfSysEffect*)mm->vtable->getEffect(mm);
    }
    if (effect == NULL) {
        return NULL;
    }
    if (effect->vtable->create(effect, self, emitter, flag) == 0) {
        EfSysMemoryManager* mm = EfGetMemoryManager(self);
        mm->vtable->releaseEffect(mm, effect);
        return NULL;
    }
    effect->mGroupID = groupID;
    fn_800A43E8(&self->mActivityList[groupID], effect);
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
    NW4R_POINTER_ASSERT(target, lbl_80592714);
    if (target->mManagerES != self) {
#line 160
        nw4r::db::Panic(lbl_80592698, __LINE__, lbl_805927AC);
    }
    if (target->state != 1) {
        return 0;
    }
    fn_800A4A1C(&self->mActivityList[target->mGroupID], target);
    fn_800A49B8(target);
    return 1;
}

/* 0x800A5D8C - retire every effect on one group's active list. */
extern "C" u32 fn_800A5D8C(EfSys* self, u32 groupID) {
    u32 count = 0;
    EfSysEffect* list[NW4R_EF_MAX_EFFECT];

#line 178
    NW4R_ASSERT((u16)fn_800A4AF0(&self->mActivityList[groupID].mActiveList) < NW4R_EF_MAX_EFFECT,
                lbl_805927E0);
    u16 num = fn_8009B374(&self->mActivityList[groupID].mActiveList, (void**)list,
                          (u16)fn_800A4AF0(&self->mActivityList[groupID].mActiveList));
    for (u16 i = 0; i < num; i++) {
        if (list[i]->state == 1) {
            count += ((nw4r::ef::EffectSystem*)self)->RetireEffect((nw4r::ef::Effect*)list[i]);
        }
    }
    return count;
}

/* 0x800A5E6C - retire the emitters of every effect on one group's active list. */
extern "C" u32 fn_800A5E6C(EfSys* self, u32 groupID) {
    u32 count = 0;
    EfSysEffect* list[NW4R_EF_MAX_EFFECT];

#line 201
    NW4R_ASSERT((u16)fn_800A4AF0(&self->mActivityList[groupID].mActiveList) < NW4R_EF_MAX_EFFECT,
                lbl_805927E0);
    u16 num = fn_8009B374(&self->mActivityList[groupID].mActiveList, (void**)list,
                          (u16)fn_800A4AF0(&self->mActivityList[groupID].mActiveList));
    for (u16 i = 0; i < num; i++) {
        count += ((nw4r::ef::Effect*)list[i])->RetireEmitterAll();
    }
    return count;
}

/* 0x800A5F4C - retire the particles of every effect on one group's active list. */
extern "C" u32 fn_800A5F4C(EfSys* self, u32 groupID) {
    u32 count = 0;
    EfSysEffect* list[NW4R_EF_MAX_EFFECT];

#line 221
    NW4R_ASSERT((u16)fn_800A4AF0(&self->mActivityList[groupID].mActiveList) < NW4R_EF_MAX_EFFECT,
                lbl_805927E0);
    u16 num = fn_8009B374(&self->mActivityList[groupID].mActiveList, (void**)list,
                          (u16)fn_800A4AF0(&self->mActivityList[groupID].mActiveList));
    for (u16 i = 0; i < num; i++) {
        count += fn_800A4AF8(list[i]);
    }
    return count;
}

/* 0x800A602C - set the system's reference transform and its two range floats. */
extern "C" void fn_800A602C(EfSys* self, const nw4r::math::VEC3* pos, const nw4r::math::MTX34* src,
                            f32 a, f32 b) {
    copyVec3(&self->mRefPos, pos);
    nw4r::math::MTX34* dst = (nw4r::math::MTX34*)fn_80050508(&self->mRefMtx);
    PSMTXCopy(fn_80051570(src), dst);
    self->mRangeA = a;
    self->mRangeB = b;
}

/* 0x800A60B8 - the +0x30 block accessor (ef_drawstrategyimpl.cpp's fn_800C7CE0 calls it). */
extern "C" void* fn_800A60B8(EfSysAccessObj* self) {
    return &self->block_0x30;
}

/* 0x800A60C0 - the +0x58 matrix accessor the effect code (ef/effect.cpp, ef/eft004.cpp) calls. */
extern "C" nw4r::math::MTX34* fn_800A60C0(EfSysAccessObj* self) {
    return &self->mtx_0x58;
}

/* 0x800A60C8 - the file's static initializer: the three borrowed engine objects, the 0xA0-byte record,
 * then the system singleton and the link that registers its destructor. */
extern "C" void fn_800A60C8(void) {
    fn_800A620C((EfSysVtblObj*)&lbl_80794910);
    fn_800A61FC((EfSysVtblObj*)&lbl_80794914);
    fn_800A61EC((EfSysVtblObj*)&lbl_80794918);
    fn_800A6134((EfSysDefaultRecord*)lbl_80688420);
    fn_800A559C((EfSys*)lbl_806884D0);
    __register_global_object(lbl_806884D0, (void*)fn_800A5618, lbl_806884C0);
}

/* The `.ctors` word (0x8056F2D8) the split assigns to this unit - it points at the initializer. */
__declspec(section ".ctors") void* const lbl_8056F2D8 = (void*)fn_800A60C8;

/* The .data pool's own functions want the pass on: fn_800A6134 keeps its two sda2 constants in f1/f0
 * across the stores and schedules the byte stores 0x9A/0x99/0x98/0x9B (measured: with the pass off it
 * reloads both constants for every store, 8 loads against the target's 2). */
#pragma peephole on

/* 0x800A6134 - the constructor of the 0xA0-byte record the game allocates at 0x800D3D0C. */
extern "C" void* fn_800A6134(EfSysDefaultRecord* self) {
    MTX34_ctor(&self->mtx_0x00);
    MTX34_ctor(&self->mtx_0x30);
    VEC3_ctor(&self->vec_0x8C);
    fn_800504D4(&self->mtx_0x00);
    fn_800504D4(&self->mtx_0x30);
    self->field_0x60 = 0;
    self->field_0x64 = 0;
    self->field_0x68 = 0;
    self->field_0x6C = 1;
    self->field_0x70 = 0;
    f32 zero;
    f32 one = lbl_80795FF8;

    self->scale_0x74 = one;
    zero = lbl_80795FFC;
    self->scale_0x78 = zero;
    self->scale_0x7C = one;
    self->scale_0x80 = zero;
    self->scale_0x88 = one;
    self->vec_0x8C.x = one;
    self->vec_0x8C.y = one;
    self->vec_0x8C.z = one;
    self->color_0x9A = 0xFF;
    self->color_0x99 = 0xFF;
    self->color_0x98 = 0xFF;
    self->color_0x9B = 0xFF;
    self->color_0x9E = 0;
    self->color_0x9D = 0;
    self->color_0x9C = 0;
    self->color_0x9F = 0xFF;
    return self;
}

#pragma peephole off

/* 0x800A61EC - the line-strategy object's constructor (ef_line.cpp's class; its key function is there). */
extern "C" EfSysVtblObj* fn_800A61EC(EfSysVtblObj* self) {
    self->vtable = lbl_80594EC4;
    return self;
}

/* 0x800A61FC - the strategy object's constructor (ef_drawstrategyimpl.cpp's class). */
extern "C" EfSysVtblObj* fn_800A61FC(EfSysVtblObj* self) {
    self->vtable = lbl_80594840;
    return self;
}

/* 0x800A620C - the draw-order list object's constructor (ef_draworder.cpp's class). */
extern "C" EfSysVtblObj* fn_800A620C(EfSysVtblObj* self) {
    fn_800A6248(self);
    self->vtable = lbl_8059241C;
    return self;
}

/* 0x800A6248 - that class's base constructor. */
extern "C" EfSysVtblObj* fn_800A6248(EfSysVtblObj* self) {
    self->vtable = lbl_8059283C;
    return self;
}

/* 0x800A6258 - the emitter-side resource object's constructor (ef/ef_emitter.cpp owns its layout). */
extern "C" EfSysResourceObj* fn_800A6258(EfSysResourceObj* self) {
    fn_800A4080(self);
    self->vtable = lbl_80592BA8;
    fn_800A62C0(&self->sub_0x020);
    fn_800A3FFC(&self->particles, 0x14);
    VEC3_ctor(&self->vec_0x108);
    VEC3_ctor(&self->vec_0x114);
    MTX34_ctor(&self->mtx_0x124);
    return self;
}

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
