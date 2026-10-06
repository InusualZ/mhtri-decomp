/*
 * g3d/g3d_anmvis.cpp - nw4r g3d `ResAnmVis` handle accessors and the `AnmObjVis` node-visibility query and walkers.
 * RANGE. .text 0x8006EAC0-0x8006EE78 (8 functions); extab, extabindex, .rodata 0x8056F628-0x8056F638, .data
 *   0x8058D6C0-0x8058D7E0, .sdata 0x807911A0-0x807911A8.  Both edges are the unclaimed run's, not proven seams.
 * NAMES. Map stems; the dump's three names in this range come from a differently laid-out build and contradict
 *   the `g3d_anmvis.cpp`/`ResAnmVis` strings.
 *   g3d_apply_vis_anm_result is a GUESS (0x8006ECB4: applies the visibility animation to the model's nodes, called
 *   by ScnMdlSimple's world pass).
 * RESIDUALS. g3d_apply_vis_anm_result: retail loads the vtable through r3 (`mr r3,r27; lwz r12,0(r3)`), ours through r27.
 *   flipcheck: `.rodata`, `.data` and `.sdata` are claimed and not emitted.
 * SHAPES. `AnmObjVis` is polymorphic with twelve placeholder slots ahead of the called one, so the call is a real
 *   virtual dispatch (`lwz r12,0x38(r12)`); a function-pointer field emits a different temp register.
 *   `#pragma peephole off` around fn_8006ED84 keeps retail's `clrlwi r0,r3,24` ahead of the `stb`.
 *   `lbl_807911A0` is a sized `char[4]` (`li r7, @sda21`); the file/format strings are extern labels, because
 *   `-str reuse` would pool the twice-used `lbl_8058D6C0` into one base register.
 */

#include "types.h"
#include "nw4r/g3d/res_common.h" /* IS_VALID_PTR, ResHandle (rule 1) */
#include "g3d/g3d_resmat.h" /* nw4r::g3d::ResMdl (rule 2) */
#include "g3d/g3d_resnode.h" /* nw4r::g3d::ResNode (rule 2) */
#include "g3d/fn_8005AA28.h" /* fn_8005AA44 (rule 2: its owner's header) */
#include "g3d/g3d_obj.h"      /* nw4r::g3d::AnmObjVis (rule 1) */

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

/* ------------------------------------------------------------------------------------------------ */
/* the unit's own functions (forward declarations; keeps the source order free)                       */
/* ------------------------------------------------------------------------------------------------ */

bool fn_8006EAC0(AnmObjVis* pSelf, s32 nodeId);
s32 fn_8006EC28(const ResHandle* pSelf);
u32 fn_8006EC3C(ResHandle* pSelf);
u32 fn_8006ECA0(const ResHandle* pSelf);
const char* fn_8006ECA8(void);
void g3d_apply_vis_anm_result(void* pModel, AnmObjVis* pSelf);
void fn_8006ED84(u8* pByteVec, void* pModel, AnmObjVis* pSelf);

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
void g3d_apply_vis_anm_result(void* pModel, AnmObjVis* pSelf)
{
    s32 numNodes = reinterpret_cast<const nw4r::g3d::ResMdl*>(pModel)->GetResNodeNumEntries();

    for (u32 i = 0; i < (u32)numNodes; i++) {
        if (fn_8006EAC0(pSelf, (s32)i)) {
            s32 handle;
            s32 node;

            node = (s32)reinterpret_cast<const nw4r::g3d::ResMdl*>(pModel)->GetResNode(i).mpData;
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
    numNodes = reinterpret_cast<const nw4r::g3d::ResMdl*>(pModel)->GetResNodeNumEntries();
    for (u32 i = 0; i < (u32)numNodes; i++) {
        if (fn_8006EAC0(pSelf, (s32)i)) {
            u8 value = (u8)pSelf->GetNodeFlag(i);

            pByteVec[i] = value;
        }
    }
}
#pragma peephole on

}  // extern "C"

/* 0x8006EE48 (0x30): returns the AnmObjVis type. */
const nw4r::g3d::G3dObj::TypeObj nw4r::g3d::AnmObjVis::GetTypeObjStatic()
{
    u32 value;

    return *reinterpret_cast<const TypeObj*>(fn_8005DC60(&value, lbl_8056F628));
}

extern "C" {

}  // extern "C"
