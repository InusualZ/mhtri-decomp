/*
 * nw4r g3d: g3d_anmvis.cpp - the ResAnmVis handle accessors and the AnmObjVis node-visibility walkers,
 * `.text` 0x8006EAC0-0x8006EE78 (8 functions, 0x3B8 bytes).
 *
 * Naming - which evidence class decided it.  Class 1 decides: the region's own `.data` pool holds the
 * bare source-file name `g3d_anmvis.cpp` (lbl_8058D6C0 at 0x8058D6C0, referenced by fn_8006EAC0 at
 * lines 54/55, by fn_8006ECB4 at line 733 and by fn_8006ED84 at line 743), the inlined-assert header
 * `g3d_resanmvis_ac.h` (lbl_8058D7CC) and the RTTI/type name `ResAnmVis` (lbl_8058D7A4).  `langcheck`
 * says C++ (the file name is a `.cpp` and the `Panic__Q24nw4r2dbFPCciPCce` relocation is a C++ mangling),
 * so the unit is `src/g3d/g3d_anmvis.cpp` in the existing g3d lib (Wii/1.3, cflags_g3d) - exactly where
 * its link neighbours sit.  Class 2 FAILS: `dumpmap.py lookup` answers `zz_006eac0_`/`zz_006ec3c_`/
 * `zz_006ecb4_`/`zz_006ed84_`/`zz_006ee48_` for five of the eight, and the three it does name
 * (`LexicalCast<b,i>(int`, `CBGetBytesAvailableForRead`, `GetOneTimerLeadGroundContactAnims(void)`) are
 * the Loading85 dump's names for a differently-laid-out build - they contradict the file's own
 * `g3d_anmvis.cpp`/`ResAnmVis` evidence, so they are not usable.
 *
 * What the unit is.  `fn_8006EC28`/`fn_8006ECA0`/`fn_8006EC3C`/`fn_8006ECA8` are the out-of-line
 * `ResCommon<ResAnmVis>` handle accessors (the `g3d_resanmvis_ac.h` inlined-assert trio: `IsValid`,
 * `ref`, and the `%s::%s: Object not valid.` guard that names the type `ResAnmVis` and the `ref`
 * accessor).  `fn_8006EAC0` is the per-node visibility/flag query and `fn_8006ECB4`/`fn_8006ED84` are the
 * two walkers that apply it over a model's node table (`fn_80097F80` = the entry count, `fn_80097F18` =
 * one node handle), one writing a byte vector and one gating a virtual per-node call; `fn_8006EE48`
 * returns the AnmObjVis type-info name.
 *
 * Language: C++ (`__FILE__` `g3d_anmvis.cpp`, C++ `Panic` mangling).  The range is one maximal
 * unclaimed run (attribute.py); its seam is unproven - it is a proposal cap, not a proven TU boundary.
 *
 * Section claim: `.text` 0x8006EAC0-0x8006EE78, `extab` 0x80007E98-0x80007EC0 (5 unwind records, one per
 * extabindex entry), `extabindex` 0x8002040C-0x80020448 (5 entries: fn_8006EAC0/EC3C/ECB4/ED84/EE48).
 *
 * Measurement path: the unit is registered here for the first time, so MAIN has no split object for the
 * range; the source is compiled with the g3d lib's real command line (cflags_g3d) and each symbol is
 * scored with objdiff `report generate` against the retired per-range object that contains it
 * (`auto_fn_8006EAC0_text.o`, `auto_03_8006EC28_text.o`, `auto_fn_8006EC3C_text.o`,
 * `auto_03_8006ECA0_text.o`, `auto_fn_8006ECB4_text.o`, `auto_fn_8006ED84_text.o`,
 * `auto_fn_8006EE48_text.o` under build/RMHE08/obj/).
 *
 * Results (objdiff `report generate`, the official metric; target bytes / our bytes):
 *   100.00: fn_8006EAC0 (360/360), fn_8006EC28 (20/20), fn_8006EC3C (100/100),
 *           fn_8006ECA0 (8/8), fn_8006ECA8 (12/12), fn_8006EE48 (48/48)
 *    99.90: fn_8006ECB4 (208/208)
 *   100.00: fn_8006ED84 (196/196)
 * 7 of 8 symbols byte-identical; the eighth is one register.
 *
 * Residuals (recorded, not worked around):
 *   * fn_8006ECB4 (99.90 %) is instruction-for-instruction except one base register: retail loads the
 *     vtable pointer through the argument register (`lwz r12,0(r3)`, after `mr r3,r27`), MWCC loads it
 *     through the parameter's home register (`lwz r12,0(r27)`).  Every other instruction, including the
 *     `lwz r12,0x38(r12); mtctr r12; bctrl` dispatch, matches byte for byte.  Register choice is the
 *     allocator's, not the source's: the identical virtual call in fn_8006ED84 does use r3, and
 *     declaration/argument permutations only move other registers.
 *
 * Two source shapes carry the range and are load-bearing:
 *   * `AnmObjVis` is declared polymorphic with twelve un-evidenced slots ahead of the called one, so the
 *     call is a real C++ virtual dispatch and emits retail's `lwz r12,0(r3); lwz r12,0x38(r12)` (a
 *     function-pointer field load emits a different temp register - 99.69 % for fn_8006ED84).
 *   * `#pragma peephole off` is scoped to fn_8006ED84: at `-O3` the peephole folds the `(u8)` truncation
 *     into the `stb`, while retail keeps the `clrlwi r0,r3,24` (97.55 % with the fold).
 * Also: `lbl_807911A0` is declared `char[4]` (its exact size) rather than an unsized array, because
 * MWCC's small-data heuristic then emits retail's `li r7, lbl_807911A0@sda21` instead of `lis`/`addi`
 * (fn_8006EC3C 93.60 % -> 100 %); and the file/format strings are `extern` map labels, not literals,
 * because `-str reuse` would pool the twice-used `lbl_8058D6C0` and share one base register.
 *
 * Naming note: the symbol map has only `fn_XXXXXXXX` for this range (checked with
 * `python tools/symbols/dumpmap.py lookup 0x8006EAC0`, which answers the `zz_006eac0_` placeholder, and
 * with `python tools/units/stylelint.py`'s own map index; every `.text` entry in 0x8006EAC0..0x8006EE78
 * in config/RMHE08/symbols.txt is a bare `fn_XXXXXXXX`).  The dump's three real names are for a
 * differently-laid-out dump build and contradict the range's own `__FILE__` evidence, so the map's stems
 * stand and are used as the identifiers.
 */

#include "types.h"
#include "nw4r/g3d/res_common.h" /* IS_VALID_PTR, ResHandle (rule 1) */
#include "g3d/fn_8005AA28.h" /* fn_8005AA44 (rule 2: its owner's header) */

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker.  Declared in the namespace (C++ linkage), never as the mangled spelling
 * (rule 9). */
void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace nw4r

/* The panic file/format strings the target references as map symbols.  They are extern here rather than
 * literals: MWCC's `-str reuse` would pool a repeated literal (`lbl_8058D6C0` is loaded twice inside
 * fn_8006EAC0) into one blob and address it through a shared base register, while the target loads each
 * one with its own `lis`/`addi`. */
extern const char lbl_8056F628[]; /* the AnmObjVis type-info record */
extern const char lbl_8058D6C0[]; /* "g3d_anmvis.cpp" */
extern const char lbl_8058D6D0[]; /* "NW4R:Pointer Error\nthis(=%p) is not valid pointer." */
extern const char lbl_8058D704[]; /* "int(nodeId) is out of bounds(%d)\nint(nodeId) <= %d not satisfied." */
extern const char lbl_8058D748[]; /* "NW4R:Failed assertion node.GetID() == nodeID" */
extern const char lbl_8058D778[]; /* "NW4R:Pointer must not be NULL (pByteVec)" */
extern const char lbl_8058D7A4[]; /* "ResAnmVis" */
extern const char lbl_8058D7B0[]; /* "%s::%s: Object not valid." */
extern const char lbl_8058D7CC[]; /* "g3d_resanmvis_ac.h" */
extern char lbl_807911A0[4]; /* "ref" - sized (4) so MWCC's small-data heuristic emits @sda21 */

/* The node-visibility object the range walks.  Its +0x00 word is a polymorphic vtable pointer (the two
 * walkers dispatch through vtable offset +0x38), the entry count sits at +0x10 and the halfword array at
 * +0x14; only those offsets are evidenced, so the rest is padding.  The class is declared polymorphic so
 * the call compiles to the retail virtual dispatch (`lwz r12,0(r3); lwz r12,0x38(r12); mtctr; bctrl`).
 * The twelve slots ahead of the called one are un-evidenced occupancy: MWCC puts the first declared
 * virtual at vtable offset +0x08 (two hidden entries at +0x00/+0x04), so `mpfn_0x08`..`mpfn_0x34` fill
 * +0x08..+0x34 and the called slot lands at exactly +0x38.  They are never called or defined.  The type
 * is only ever used through a pointer here, so the size stays a lower bound. */
struct AnmObjVis {
    virtual u32 mpfn_0x08(u32);
    virtual u32 mpfn_0x0C(u32);
    virtual u32 mpfn_0x10(u32);
    virtual u32 mpfn_0x14(u32);
    virtual u32 mpfn_0x18(u32);
    virtual u32 mpfn_0x1C(u32);
    virtual u32 mpfn_0x20(u32);
    virtual u32 mpfn_0x24(u32);
    virtual u32 mpfn_0x28(u32);
    virtual u32 mpfn_0x2C(u32);
    virtual u32 mpfn_0x30(u32);
    virtual u32 mpfn_0x34(u32);
    /* +0x38 */ virtual u32 GetNodeFlag(u32 index);
    /* +0x04 */ u8 pad_0x04[0xC];
    /* +0x10 */ s32 mNumEntries;
    /* +0x14 */ u16* mpEntries;
}; /* size: 0x18 - lower bound, an approximation */

extern "C" {

/* ------------------------------------------------------------------------------------------------ */
/* callees (module-ambiguous unsplit addresses: rule 2's documented gap, so they are declared here)  */
/* ------------------------------------------------------------------------------------------------ */

u32 fn_8005D050(const void* pSelf);
void fn_8005D2C0(void* pOut, const void* pIn);
u32* fn_8005DC60(u32* pOut, const void* value);
s32 fn_80097F18(void* pA, u32 idx);
s32 fn_80097F80(void* pA);

/* ------------------------------------------------------------------------------------------------ */
/* the unit's own functions (forward declarations; keeps the source order free)                       */
/* ------------------------------------------------------------------------------------------------ */

bool fn_8006EAC0(AnmObjVis* pSelf, s32 nodeId);
s32 fn_8006EC28(const ResHandle* pSelf);
u32 fn_8006EC3C(ResHandle* pSelf);
u32 fn_8006ECA0(const ResHandle* pSelf);
const char* fn_8006ECA8(void);
void fn_8006ECB4(void* pModel, AnmObjVis* pSelf);
void fn_8006ED84(u8* pByteVec, void* pModel, AnmObjVis* pSelf);
u32 fn_8006EE48(void);

/* The per-node visibility test: guard `this` with the library pointer check, bound `nodeId` by the entry
 * count, then report the top two bits of the entry halfword. */
bool fn_8006EAC0(AnmObjVis* pSelf, s32 nodeId)
{
    u32 valid = IS_VALID_PTR(pSelf);

    if (!valid) {
        nw4r::db::Panic(lbl_8058D6C0, 54, lbl_8058D6D0, pSelf);
    }
    if (nodeId > pSelf->mNumEntries - 1) {
        nw4r::db::Panic(lbl_8058D6C0, 55, lbl_8058D704, nodeId, pSelf->mNumEntries - 1);
    }
    return (pSelf->mpEntries[nodeId] & 0xC000) == 0;
}

/* `ResCommon<ResAnmVis>::IsValid()`: the handle's resource pointer is non-NULL. */
s32 fn_8006EC28(const ResHandle* pSelf)
{
    u32 data = (u32)pSelf->mpData;

    return ((u32)(-(s32)data) | data) >> 31;
}

/* `ResCommon<ResAnmVis>::ref()` guarded by the `g3d_resanmvis_ac.h` inlined assert. */
u32 fn_8006EC3C(ResHandle* pSelf)
{
    if (fn_8006EC28(pSelf) == 0) {
        nw4r::db::Panic(lbl_8058D7CC, 39, lbl_8058D7B0, fn_8006ECA8(), lbl_807911A0);
    }
    return fn_8006ECA0(pSelf);
}

/* `ResCommon<ResAnmVis>::ref()`: the handle's resource pointer. */
u32 fn_8006ECA0(const ResHandle* pSelf)
{
    return (u32)pSelf->mpData;
}

/* `ResCommon<ResAnmVis>`'s type name, spelled out of line by the `_ac.h` assert. */
const char* fn_8006ECA8(void)
{
    return lbl_8058D7A4;
}

/* Walk the model's node table and, for every node the object marks visible, query the node handle's ID
 * (asserting it equals the index) and forward the per-node virtual result into the handle. */
void fn_8006ECB4(void* pModel, AnmObjVis* pSelf)
{
    s32 numNodes = fn_80097F80(pModel);

    for (u32 i = 0; i < (u32)numNodes; i++) {
        if (fn_8006EAC0(pSelf, (s32)i)) {
            s32 handle;
            s32 node;

            node = fn_80097F18(pModel, i);
            fn_8005D2C0(&handle, &node);
            if ((u32)fn_8005D050(&handle) != i) {
                nw4r::db::Panic(lbl_8058D6C0, 733, lbl_8058D748);
            }
            fn_8005AA44(&handle, pSelf->GetNodeFlag(i));
        }
    }
}

/* Walk the model's node table and write one byte per visible node into `pByteVec`. */
#pragma peephole off
void fn_8006ED84(u8* pByteVec, void* pModel, AnmObjVis* pSelf)
{
    s32 numNodes;

    if (pByteVec == NULL) {
        nw4r::db::Panic(lbl_8058D6C0, 743, lbl_8058D778);
    }
    numNodes = fn_80097F80(pModel);
    for (u32 i = 0; i < (u32)numNodes; i++) {
        if (fn_8006EAC0(pSelf, (s32)i)) {
            u8 value = (u8)pSelf->GetNodeFlag(i);

            pByteVec[i] = value;
        }
    }
}
#pragma peephole on

/* The AnmObjVis type-info name, through the one-word association helper the SDK uses for it. */
u32 fn_8006EE48(void)
{
    u32 value;

    return *fn_8005DC60(&value, lbl_8056F628);
}

}  // extern "C"
