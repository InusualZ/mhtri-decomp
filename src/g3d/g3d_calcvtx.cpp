/*
 * g3d/g3d_calcvtx.cpp - nw4r g3d vertex calculation: the shape-blend driver fn_8007270C (three inlined
 *   ResVtxPos/ResVtxNrm/ResVtxClr passes, each blending an animated-shape key list over the vertex array) and the
 *   `g3d_resvtx_ac.h` accessor family.
 * RANGE. .text 0x8007270C-0x800736F8 (35 functions); extab, extabindex, .data 0x8058DD68-0x8058E178 (opens on
 *   "g3d_calcvtx.cpp", fn_8007270C's assert), .sdata 0x807911B8-0x807911D0, .sdata2 0x80795DB8-0x80795DC0.  The
 *   `.extabindex` run ends with fn_800736A4; the seam at 0x800736F8 gives the scale setter fn_800736F8 and the flag
 *   helpers fn_800737AC..fn_800737C4 to `g3d/g3d_calcworld.cpp` (the helpers carry no data reference to confirm it).
 * NAMES. Map stems.
 * RESIDUALS. fn_8007270C: 132 bytes short (2340 of 2472); the first divergence is the `mdl` spill (retail stores
 *   it at 0x8(r1) and reloads it, ours keeps it in r14), then `lwz` against `lwzx` addressing; the rest unmeasured.
 *   fn_80073404, fn_800735A8: the masked test folds to the record form where retail keeps `clrlwi` + `cmpwi`.
 *   fn_80073614: one instruction more than retail.
 *   flipcheck: `.text` 0xF64 of 0xFEC; `.data` is claimed and not emitted; `.sdata` is 0x4 of 0x18, `.sdata2` 0x4
 *   of 0x8.
 * SHAPES. Each blend pass accumulates into a `f32 v[3]` with an inner `for (j < 3)` (pass 3 reads a rolling
 *   three-float window advancing 4 bytes).  File-scope `#pragma fp_contract off` (retail has no `fmadds`/`fmsubs`);
 *   `#pragma peephole off` around fn_800731EC keeps retail's `clrlwi` + `cmpwi` (playbook 32).
 */


#include "types.h"
#include "nw4r/g3d/res_common.h"
#include "unsplit/g3d.h" /* unsplit g3d neighbours (rule 2) */
#include "g3d/fn_800680CC.h" /* fn_8006946C..fn_80069768, owned by fn_800680CC.cpp (rule 2) */

/* The target object contains no fused multiply-add at all while `cflags_g3d` passes
 * `-fp_contract on`, so the original file carried the pragma. File-scoped (see header). */
#pragma fp_contract off

namespace nw4r {
namespace db {

/* `Panic(const char* pFile, int line, const char* pFmt, ...)` - the trailing `e` in the map's mangling
 * is MWCC's vararg marker. */
void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace db


/* ------------------------------------------------------------------------------------------------ */
/* externs: the SDK and the neighbouring units this one calls (the map owns their names)            */
/* ------------------------------------------------------------------------------------------------ */

extern "C" void DCStoreRange(void* pBase, u32 size);
/* fn_800696E4/fn_80069748/fn_80069754/fn_800695D4/fn_800695DC/fn_80069768/fn_800696C0/fn_8006946C come from
 * `g3d/fn_800680CC.h`; fn_800731EC/fn_800732F0/fn_80073354 are this unit's, in its forward-declaration block. */

/* The panic file/format strings the target references as map symbols. They are extern here rather
 * than literals: MWCC's `-str reuse` would pool a literal into one blob and address it through a
 * shared base register, while the target loads each one with its own `lis`/`addi`. */
extern const char lbl_8058E070[];
extern const char lbl_8058E098[];
extern const char lbl_8058E0B4[];
extern const char lbl_8058E0D0[];
extern const char lbl_8058E110[];
extern const char lbl_8058E138[];
extern const char lbl_8058E148[];
extern const char lbl_8058E168[];

/* The 0x8007270C cluster's panic strings, same data fragment, same reason (extern rather than a
 * literal: MWCC's `-str reuse` would otherwise pool them behind one base register). */
extern const char lbl_8058E008[]; /* "ResVtxClr" */
extern const char lbl_8058E040[]; /* "%s::%s: Object not valid." */
extern const char lbl_8058E060[]; /* "g3d_resvtx_ac.h" */

/* The 0x8007270C driver's panic strings (all in the same fragment). */
extern const char lbl_8058DD68[]; /* "g3d_calcvtx.cpp" */
extern const char lbl_8058DD78[]; /* "NW4R:Pointer must not be NULL (pAnmObjShp)" */
extern const char lbl_8058DDA4[]; /* "NW4R:Failed assertion mdl.IsValid()" */
extern const char lbl_8058DDC8[]; /* "NW4R:Pointer must not be NULL (vtxPosTable)" */
extern const char lbl_8058DDF8[]; /* "NW4R:Failed assertion mdl.GetResVtxPos(vtxPosID)" */
extern const char lbl_8058DE48[]; /* "NW4R:Pointer must not be NULL (pResult)" */
extern const char lbl_8058DE70[]; /* "NW4R:Failed assertion resVtxPos.IsValid()" */
extern const char lbl_8058DE9C[]; /* "NW4R:Failed assertion resVtxPos.GetID() == vtxPosID" */
extern const char lbl_8058DED0[]; /* "NW4R:Pointer must not be NULL (pBaseVtx)" */
extern const char lbl_8058DEFC[]; /* "NW4R:Failed assertion vtxStride == VTX_STRIDE" */
extern const char lbl_8058DF2C[]; /* "NW4R:Failed assertion key.IsValid()" */
extern const char lbl_8058DF50[]; /* "NW4R:Failed assertion numKeyShape > 0" */
extern const char lbl_8058DF78[]; /* "NW4R:Failed assertion resVtxNrm.IsValid()" */
extern const char lbl_8058DFA4[]; /* "NW4R:Failed assertion resVtxClr.IsValid()" */

/* The 0x80073180-0x80073398 gap's panic strings (same data fragment). */
extern const char lbl_8058DFD0[]; /* "NW4R:Failed assertion !((u32)p & 0x3)" */
extern const char lbl_8058DFF8[]; /* "g3d_resvtx_ac.h" */
extern const char lbl_8058E014[]; /* "%s::%s: Object not valid." */
extern const char lbl_8058E030[]; /* "g3d_resvtx_ac.h" */
extern const char lbl_8058E0A8[]; /* "ResVtxNrm" */
extern const char lbl_8058E0E0[]; /* "%s::%s: Object not valid." */
extern const char lbl_8058E100[]; /* "g3d_resvtx_ac.h" */

/* ------------------------------------------------------------------------------------------------ */
/* types                                                                                             */
/* ------------------------------------------------------------------------------------------------ */

/* The resource block `fn_800732F0` / `fn_800696E4` hand back. Only the offsets this unit reads are
 * named; the leading words are the resource header (`g3d_rescommon.h`'s `ResCommonData`). */
struct ResVtxNrmBlock {
    /* +0x00 */ u32 mSize;
    /* +0x04 */ u32 mUnk04;
    /* +0x08 */ u32 mToArray; /* byte offset from the block start to the vertex array */
    /* +0x0C */ u32 mUnk0C;
    /* +0x10 */ u32 mUnk10;
    /* +0x14 */ u32 mUnk14;
    /* +0x18 */ u32 mUnk18;
    /* +0x1C */ u16 mUnk1C;
    /* +0x1E */ u16 mCount;
}; /* size: 0x20 */

/* One entry of the 0x10-byte array `fn_80073614` builds: a handle and two more one-word sub-objects
 * (`fn_800731EC` owns the third, it lives in the previous unit). */
struct VtxEntry {
    /* +0x0 */ ResHandle mNrm;
    /* +0x4 */ ResHandle mClr;
    /* +0x8 */ ResHandle mTexCoord;
    /* +0xC */ u32 mUnused0C;
}; /* size: 0x10 */

/* The block `fn_80073614` builds: the entry array's header plus the 32 entries at +0x18. */
struct VtxEntryArray {
    /* +0x00 */ u32 mUnk00;
    /* +0x04 */ u32 mUnk04;
    /* +0x08 */ VtxEntry mHeaderEntry;
    /* +0x18 */ VtxEntry mEntries[32];
}; /* size: 0x218 */

/* The `ResVtxClr` resource block shares `g3d_resvtx_ac.h`'s header with `ResVtxNrmBlock` (same layout:
 * `mToArray` +0x08, `mUnk10` +0x10, `mCount` +0x1E), so the block accessors reuse that type. */

/* One key of the animated-shape blend node: the normal/colour/texcoord handles plus the key weight. */
struct ShpKey {
    /* +0x00 */ ResHandle mNrm;
    /* +0x04 */ ResHandle mClr;
    /* +0x08 */ ResHandle mTex;
    /* +0x0C */ f32 mWeight;
}; /* size: 0x10 */

/* The node the animated-shape virtual call (`vtable[0x38/4]`) hands back: a flag word, the base key
 * and the key list.  `mBase` is the node's own contribution; `mKeys` follows immediately, so the
 * node is variable-length (only `mKeys[0]` is ever named). */
struct ShpNode {
    /* +0x00 */ u32 mFlags;
    /* +0x04 */ s32 mNumKey;
    /* +0x08 */ ShpKey mBase;
    /* +0x18 */ ShpKey mKeys[1];
}; /* size: 0x28; `mKeys` is variable-length, so only `mKeys[0]` is ever written */

/* The {vertex pointer, weight} pair the blend walks; the pointer is advanced in place per vertex. */
struct BlendKey {
    /* +0x00 */ const f32* mpVtx;
    /* +0x04 */ f32 mWeight;
}; /* size: 0x8 */

/* The animated-shape object's virtual that fills `pArr` and returns the node for shape `id`. */
typedef const ShpNode* (*ShpCalcFn)(void* pSelf, VtxEntryArray* pArr, s32 id);

extern "C" {

/* ------------------------------------------------------------------------------------------------ */
/* forward declarations (one per function this unit defines; keeps the source order free)             */
/* ------------------------------------------------------------------------------------------------ */

void* fn_800733FC(ResHandle* pSelf);
void fn_80073468(ResHandle* pSelf, u32 value);
bool fn_80073494(ResHandle* pSelf);
void* fn_80073398(ResHandle* pSelf);
ResHandle* fn_80073404(ResHandle* pSelf, u32 pData);
void fn_800734D8(ResHandle* pSelf, const ResHandle* pRhs);
ResHandle* fn_800734A8(ResHandle* pSelf, const ResHandle* pRhs);
void fn_800734E4(void* pBase, u32 size);
u32 fn_80073470(ResHandle* pSelf);
u16 fn_800734E8(ResHandle* pSelf);
void* fn_800736F0(ResHandle* pSelf);
ResVtxNrmBlock* fn_80073544(ResHandle* pSelf);
void fn_8007360C(ResHandle* pSelf, u32 value);
ResHandle* fn_800735A8(ResHandle* pSelf, u32 pData);
void* fn_8007350C(ResHandle* pSelf);
VtxEntry* fn_800736A4(VtxEntry* pSelf);
VtxEntry* fn_80073674(VtxEntry* pSelf);
VtxEntryArray* fn_80073614(VtxEntryArray* pSelf);

/* The 0x80073180-0x80073398 gap and the 0x8007270C cluster (this unit's earlier half). */
const char* fn_8007313C(void);
ResVtxNrmBlock* fn_800730D8(ResHandle* pSelf);
u16 fn_800730B4(ResHandle* pSelf);
void* fn_80073148(ResHandle* pSelf);
void* fn_80073180(ResHandle* pSelf);
void* fn_800731E4(ResHandle* pSelf);
ResHandle* fn_800731EC(ResHandle* pSelf, u32 pData);
void fn_80073250(ResHandle* pSelf, u32 value);
u32 fn_80073258(ResHandle* pSelf);
bool fn_8007327C(ResHandle* pSelf);
ResHandle* fn_80073290(ResHandle* pSelf, const ResHandle* pRhs);
void fn_800732C0(ResHandle* pSelf, const ResHandle* pRhs);
u16 fn_800732CC(ResHandle* pSelf);
void* fn_800732F0(ResHandle* pSelf);
const char* fn_80073354(void);
void* fn_80073360(ResHandle* pSelf);
void fn_8007270C(void* mdl, void* pAnmObjShp, const void** vtxPosTable, const void** pClrTable,
                 const void** pTexTable);

/* The 0x8007270C driver's callees in the unsplittable band (rule 2 gap: the bracketing registered
 * units name different modules, so symbols.txt gives no sound header). */
bool fn_8005D2FC(const void* mdl);
u32 fn_80098098(const void* mdl);
u32 fn_80098030(const void* mdl, s32 id);

/* ------------------------------------------------------------------------------------------------ */
/* g3d_resvtx_ac.h: the out-of-line accessor inlines                                                 */
/* ------------------------------------------------------------------------------------------------ */

void* fn_800733FC(ResHandle* pSelf) {
    return pSelf->mpData;
}

void fn_80073468(ResHandle* pSelf, u32 value) {
    pSelf->mpData = (void*)value;
}

bool fn_80073494(ResHandle* pSelf) {
    return pSelf->mpData != NULL;
}

/* The line-98 instantiation: assert validity, then return the data word. */
void* fn_80073398(ResHandle* pSelf) {
    if (!fn_80073494(pSelf)) {
        nw4r::db::Panic(lbl_8058E0D0, 98, lbl_8058E0B4, fn_80073354(), "ref");
    }
    return fn_800733FC(pSelf);
}

/* The line-98 alignment-checked setter. */
ResHandle* fn_80073404(ResHandle* pSelf, u32 pData) {
    fn_80073468(pSelf, pData);
    if ((pData & 3) != 0) {
        nw4r::db::Panic(lbl_8058E098, 98, lbl_8058E070);
    }
    return pSelf;
}

/* The line-98 copy assignment. */
void fn_800734D8(ResHandle* pSelf, const ResHandle* pRhs) {
    pSelf->mpData = pRhs->mpData;
}

ResHandle* fn_800734A8(ResHandle* pSelf, const ResHandle* pRhs) {
    fn_800734D8(pSelf, pRhs);
    return pSelf;
}

void fn_800734E4(void* pBase, u32 size) {
    DCStoreRange(pBase, size);
}

u32 fn_80073470(ResHandle* pSelf) {
    ResVtxNrmBlock* pBlock = (ResVtxNrmBlock*)fn_800732F0(pSelf);
    return pBlock->mUnk10;
}

u16 fn_800734E8(ResHandle* pSelf) {
    ResVtxNrmBlock* pBlock = (ResVtxNrmBlock*)fn_800696E4(pSelf);
    return pBlock->mCount;
}

/* The line-39 instantiation: return the data word (the validity assert lives in `fn_80073544`). */
void* fn_800736F0(ResHandle* pSelf) {
    return pSelf->mpData;
}

ResVtxNrmBlock* fn_80073544(ResHandle* pSelf) {
    if (!fn_80069754(pSelf)) {
        nw4r::db::Panic(lbl_8058E168, 39, lbl_8058E148, fn_80069748(), "ref");
    }
    return (ResVtxNrmBlock*)fn_800736F0(pSelf);
}

/* The line-39 setter. */
void fn_8007360C(ResHandle* pSelf, u32 value) {
    pSelf->mpData = (void*)value;
}

ResHandle* fn_800735A8(ResHandle* pSelf, u32 pData) {
    fn_8007360C(pSelf, pData);
    if ((pData & 3) != 0) {
        nw4r::db::Panic(lbl_8058E138, 39, lbl_8058E110);
    }
    return pSelf;
}

/* The resource's vertex array: the header's byte offset, or NULL when there is none. */
void* fn_8007350C(ResHandle* pSelf) {
    ResVtxNrmBlock* pBlock = fn_80073544(pSelf);
    if (pBlock->mToArray != 0) {
        return (u8*)pBlock + pBlock->mToArray;
    }
    return NULL;
}

/* Construct one 0x10-byte entry: the three sub-handles cleared. */
VtxEntry* fn_800736A4(VtxEntry* pSelf) {
    fn_800735A8(&pSelf->mNrm, 0);
    fn_80073404(&pSelf->mClr, 0);
    fn_800731EC(&pSelf->mTexCoord, 0);
    return pSelf;
}

VtxEntry* fn_80073674(VtxEntry* pSelf) {
    fn_800736A4(pSelf);
    return pSelf;
}

/* Construct the header entry and the 32-entry array. */
VtxEntryArray* fn_80073614(VtxEntryArray* pSelf) {
    fn_800736A4(&pSelf->mHeaderEntry);
    for (VtxEntry* pEntry = pSelf->mEntries; pEntry < pSelf->mEntries + 32; pEntry++) {
        fn_80073674(pEntry);
    }
    return pSelf;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x8007270C: the `ResVtxClr` accessor cluster (where the g3d_calcvtx.cpp text starts)             */
/* ------------------------------------------------------------------------------------------------ */

/* The class-name string the `ResVtxClr` asserts print (`g3d_resvtx_ac.h`'s pointer assert). */
const char* fn_8007313C(void) {
    return lbl_8058E008;
}

/* The `ResVtxClr` block accessor: assert the handle, then hand back the resource block. */
ResVtxNrmBlock* fn_800730D8(ResHandle* pSelf) {
    if (!fn_8007327C(pSelf)) {
        nw4r::db::Panic(lbl_8058E060, 154, lbl_8058E040, fn_8007313C(), "ref");
    }
    return (ResVtxNrmBlock*)fn_800695D4(pSelf);
}

/* The block's vertex count. */
u16 fn_800730B4(ResHandle* pSelf) {
    return fn_800730D8(pSelf)->mCount;
}

/* The block's vertex array: the header's byte offset, or NULL when there is none. The `fn_80073360`
 * shape for this block's sibling resource. */
void* fn_80073148(ResHandle* pSelf) {
    ResVtxNrmBlock* pBlock = (ResVtxNrmBlock*)fn_80073180(pSelf);
    if (pBlock->mToArray != 0) {
        return (u8*)pBlock + pBlock->mToArray;
    }
    return NULL;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x80073180-0x80073398: the `g3d_resvtx_ac.h` accessor gap (absorbed from the 80073180 handover)   */
/* ------------------------------------------------------------------------------------------------ */

/* The line-154 instantiation: assert validity, then return the data word. */
void* fn_80073180(ResHandle* pSelf) {
    if (!fn_8007327C(pSelf)) {
        nw4r::db::Panic(lbl_8058E030, 154, lbl_8058E014, fn_8007313C(), "ref");
    }
    return fn_800731E4(pSelf);
}

void* fn_800731E4(ResHandle* pSelf) {
    return pSelf->mpData;
}

/* The line-154 alignment-checked setter; `peephole off` keeps retail's `clrlwi`+`cmpwi`+`beq` where `-O3` folds
 * the masked compare into `clrlwi.` (playbook 32). */
#pragma peephole off
ResHandle* fn_800731EC(ResHandle* pSelf, u32 pData) {
    fn_80073250(pSelf, pData);
    if ((pData & 3) != 0) {
        nw4r::db::Panic(lbl_8058DFF8, 154, lbl_8058DFD0);
    }
    return pSelf;
}
#pragma peephole on

void fn_80073250(ResHandle* pSelf, u32 value) {
    pSelf->mpData = (void*)value;
}

u32 fn_80073258(ResHandle* pSelf) {
    ResVtxNrmBlock* pBlock = fn_800730D8(pSelf);
    return pBlock->mUnk10;
}

bool fn_8007327C(ResHandle* pSelf) {
    return pSelf->mpData != NULL;
}

ResHandle* fn_80073290(ResHandle* pSelf, const ResHandle* pRhs) {
    fn_800732C0(pSelf, pRhs);
    return pSelf;
}

void fn_800732C0(ResHandle* pSelf, const ResHandle* pRhs) {
    pSelf->mpData = pRhs->mpData;
}

u16 fn_800732CC(ResHandle* pSelf) {
    ResVtxNrmBlock* pBlock = (ResVtxNrmBlock*)fn_800732F0(pSelf);
    return pBlock->mCount;
}

void* fn_800732F0(ResHandle* pSelf) {
    if (!fn_80073494(pSelf)) {
        nw4r::db::Panic(lbl_8058E100, 98, lbl_8058E0E0, fn_80073354(), "ref");
    }
    return fn_800695DC(pSelf);
}

const char* fn_80073354(void) {
    return lbl_8058E0A8;
}

void* fn_80073360(ResHandle* pSelf) {
    ResVtxNrmBlock* pBlock = (ResVtxNrmBlock*)fn_80073398(pSelf);
    if (pBlock->mToArray != 0) {
        return (u8*)pBlock + pBlock->mToArray;
    }
    return NULL;
}

/* ------------------------------------------------------------------------------------------------ */
/* 0x8007270C: the animated-shape vertex driver (the largest function of the TU)                     */
/* ------------------------------------------------------------------------------------------------ */

/* Blends each animated shape's base key and key list over the position, normal and colour vertex arrays (three
 * passes, flag bits 1/2/3, differing only in the accessor pair and the per-vertex width). */
void fn_8007270C(void* mdl, void* pAnmObjShp, const void** vtxPosTable, const void** pClrTable,
                 const void** pTexTable) {
    if (pAnmObjShp == NULL) {
        nw4r::db::Panic(lbl_8058DD68, 36, lbl_8058DD78);
    }
    if (!fn_8005D2FC(mdl)) {
        nw4r::db::Panic(lbl_8058DD68, 37, lbl_8058DDA4);
    }
    s32 numVtxPos = (s32)fn_80098098(mdl);

    for (s32 i = 0; i < numVtxPos; i++) {
        if (!fn_8006946C(pAnmObjShp, i)) {
            continue;
        }
        if (vtxPosTable == NULL) {
            nw4r::db::Panic(lbl_8058DD68, 49, lbl_8058DDC8);
        }
        ResHandle vtxPos;
        vtxPos.mpData = (void*)fn_80098030(mdl, i);
        if (fn_800736F0(&vtxPos) != vtxPosTable[i]) {
            nw4r::db::Panic(lbl_8058DD68, 50, lbl_8058DDF8);
        }
        VtxEntryArray arr;
        fn_80073614(&arr);
        const ShpNode* pNode = ((ShpCalcFn*)(*(void**)pAnmObjShp))[14](pAnmObjShp, &arr, i);
        if (pNode == NULL) {
            nw4r::db::Panic(lbl_8058DD68, 55, lbl_8058DE48);
        }
        if ((pNode->mFlags & 1) == 0) {
            continue;
        }

        /* pass 1: the position (ResVtxPos) arrays */
        if ((pNode->mFlags & 2) != 0) {
            ResHandle nrm;
            fn_80069768(&nrm, &pNode->mBase.mNrm);
            if (!fn_80069754(&nrm)) {
                nw4r::db::Panic(lbl_8058DD68, 66, lbl_8058DE70);
            }
            if (fn_800696C0(&nrm) != (u32)i) {
                nw4r::db::Panic(lbl_8058DD68, 67, lbl_8058DE9C);
            }
            ResHandle dst;
            fn_800735A8(&dst, (u32)vtxPosTable[i]);
            f32* dstVtx = (f32*)fn_8007350C(&dst);
            BlendKey keys[32];
            s32 numKeyShape = 0;
            if (pNode->mBase.mWeight != 0.0f) {
                const void* pBaseVtx;
                u8 vtxStride;
                fn_80088E84(&nrm, &pBaseVtx, &vtxStride);
                if (pBaseVtx == NULL) {
                    nw4r::db::Panic(lbl_8058DD68, 89, lbl_8058DED0);
                }
                if (vtxStride != 0xC) {
                    nw4r::db::Panic(lbl_8058DD68, 91, lbl_8058DEFC);
                }
                keys[0].mpVtx = (const f32*)pBaseVtx;
                keys[0].mWeight = pNode->mBase.mWeight;
                numKeyShape = 1;
            }
            for (s32 k = 0; k < pNode->mNumKey; k++) {
                if (pNode->mKeys[k].mWeight != 0.0f) {
                    ResHandle keyVtx;
                    fn_80069768(&keyVtx, &pNode->mKeys[k].mNrm);
                    if (!fn_80069754(&keyVtx)) {
                        nw4r::db::Panic(lbl_8058DD68, 103, lbl_8058DF2C);
                    }
                    keys[numKeyShape].mpVtx = (const f32*)fn_8007350C(&keyVtx);
                    keys[numKeyShape].mWeight = pNode->mKeys[k].mWeight;
                    numKeyShape++;
                }
            }
            if (numKeyShape <= 0) {
                nw4r::db::Panic(lbl_8058DD68, 111, lbl_8058DF50);
            }
            u16 count = fn_800734E8(&nrm);
            f32* out = dstVtx;
            f32* end = out + count * 3;
            while (out < end) {
                f32 v[3];
                for (s32 j = 0; j < 3; j++) {
                    v[j] = keys[0].mpVtx[j] * keys[0].mWeight;
                }
                keys[0].mpVtx += 3;
                for (s32 k = 1; k < numKeyShape; k++) {
                    for (s32 j = 0; j < 3; j++) {
                        v[j] += keys[k].mpVtx[j] * keys[k].mWeight;
                    }
                    keys[k].mpVtx += 3;
                }
                for (s32 j = 0; j < 3; j++) {
                    out[j] = v[j];
                }
                out += 3;
            }
            DCStoreRange(dstVtx, (u32)count * 0xC);
        }

        /* pass 2: the normal (ResVtxNrm) arrays */
        if ((pNode->mFlags & 4) != 0 && pClrTable != NULL) {
            ResHandle nrm;
            fn_800734A8(&nrm, &pNode->mBase.mNrm);
            if (!fn_80073494(&nrm)) {
                nw4r::db::Panic(lbl_8058DD68, 0xC4, lbl_8058DF78);
            }
            ResHandle dst;
            fn_80073404(&dst, (u32)pClrTable[fn_80073470(&nrm)]);
            f32* dstVtx = (f32*)fn_80073360(&dst);
            BlendKey keys[32];
            s32 numKeyShape = 0;
            if (pNode->mBase.mWeight != 0.0f) {
                const void* pBaseVtx;
                u8 vtxStride;
                fn_80088FFC(&nrm, &pBaseVtx, &vtxStride);
                if (pBaseVtx == NULL) {
                    nw4r::db::Panic(lbl_8058DD68, 0xDA, lbl_8058DED0);
                }
                if (vtxStride != 0xC) {
                    nw4r::db::Panic(lbl_8058DD68, 0xDC, lbl_8058DEFC);
                }
                keys[0].mpVtx = (const f32*)pBaseVtx;
                keys[0].mWeight = pNode->mBase.mWeight;
                numKeyShape = 1;
            }
            for (s32 k = 0; k < pNode->mNumKey; k++) {
                if (pNode->mKeys[k].mWeight != 0.0f) {
                    ResHandle keyVtx;
                    fn_800734A8(&keyVtx, &pNode->mKeys[k].mNrm);
                    if (!fn_80073494(&keyVtx)) {
                        nw4r::db::Panic(lbl_8058DD68, 0xE8, lbl_8058DF2C);
                    }
                    keys[numKeyShape].mpVtx = (const f32*)fn_80073360(&keyVtx);
                    keys[numKeyShape].mWeight = pNode->mKeys[k].mWeight;
                    numKeyShape++;
                }
            }
            if (numKeyShape <= 0) {
                nw4r::db::Panic(lbl_8058DD68, 0xF0, lbl_8058DF50);
            }
            u16 count = fn_800732CC(&nrm);
            f32* out = dstVtx;
            f32* end = out + count * 3;
            while (out < end) {
                f32 v[3];
                for (s32 j = 0; j < 3; j++) {
                    v[j] = keys[0].mpVtx[j] * keys[0].mWeight;
                }
                keys[0].mpVtx += 3;
                for (s32 k = 1; k < numKeyShape; k++) {
                    for (s32 j = 0; j < 3; j++) {
                        v[j] += keys[k].mpVtx[j] * keys[k].mWeight;
                    }
                    keys[k].mpVtx += 3;
                }
                for (s32 j = 0; j < 3; j++) {
                    out[j] = v[j];
                }
                out += 3;
            }
            DCStoreRange(dstVtx, (u32)count * 0xC);
        }

        /* pass 3: the colour (ResVtxClr) arrays - one float per vertex */
        if ((pNode->mFlags & 8) != 0 && pTexTable != NULL) {
            ResHandle clr;
            fn_80073290(&clr, &pNode->mBase.mClr);
            if (!fn_8007327C(&clr)) {
                nw4r::db::Panic(lbl_8058DD68, 0x146, lbl_8058DFA4);
            }
            ResHandle dst;
            fn_800731EC(&dst, (u32)pTexTable[fn_80073258(&clr)]);
            f32* dstVtx = (f32*)fn_80073148(&dst);
            BlendKey keys[32];
            s32 numKeyShape = 0;
            if (pNode->mBase.mWeight != 0.0f) {
                const void* pBaseVtx;
                u8 vtxStride;
                fn_8008918C(&clr, &pBaseVtx, &vtxStride);
                if (pBaseVtx == NULL) {
                    nw4r::db::Panic(lbl_8058DD68, 0x15C, lbl_8058DED0);
                }
                if (vtxStride != 0x4) {
                    nw4r::db::Panic(lbl_8058DD68, 0x15E, lbl_8058DEFC);
                }
                keys[0].mpVtx = (const f32*)pBaseVtx;
                keys[0].mWeight = pNode->mBase.mWeight;
                numKeyShape = 1;
            }
            for (s32 k = 0; k < pNode->mNumKey; k++) {
                if (pNode->mKeys[k].mWeight != 0.0f) {
                    ResHandle keyVtx;
                    fn_80073290(&keyVtx, &pNode->mKeys[k].mClr);
                    if (!fn_8007327C(&keyVtx)) {
                        nw4r::db::Panic(lbl_8058DD68, 0x16A, lbl_8058DF2C);
                    }
                    keys[numKeyShape].mpVtx = (const f32*)fn_80073148(&keyVtx);
                    keys[numKeyShape].mWeight = pNode->mKeys[k].mWeight;
                    numKeyShape++;
                }
            }
            if (numKeyShape <= 0) {
                nw4r::db::Panic(lbl_8058DD68, 0x172, lbl_8058DF50);
            }
            u16 count = fn_800730B4(&clr);
            f32* out = dstVtx;
            f32* end = out + count;
            while (out < end) {
                f32 v[3];
                for (s32 j = 0; j < 3; j++) {
                    v[j] = keys[0].mpVtx[j] * keys[0].mWeight;
                }
                keys[0].mpVtx += 1;
                for (s32 k = 1; k < numKeyShape; k++) {
                    for (s32 j = 0; j < 3; j++) {
                        v[j] += keys[k].mpVtx[j] * keys[k].mWeight;
                    }
                    keys[k].mpVtx += 1;
                }
                out[0] = v[0];
                out[1] = v[1];
                out[2] = v[2];
                out += 1;
            }
            DCStoreRange(dstVtx, (u32)count * 4);
        }
    }
}

} /* extern "C" */
