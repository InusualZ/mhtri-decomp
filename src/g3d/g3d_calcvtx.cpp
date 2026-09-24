/*
 * nw4r g3d: g3d_calcvtx.cpp - the `g3d_resvtx_ac.h` accessor copies, `.text`
 * 0x80073398-0x800736F8 (18 functions).
 *
 * Re-cut from `auto/80073398_fn_80073398.cpp` (docs/plan.md 12 item 5, bulk attribution spanning three
 * original TUs).  The panic strings the block references (`lbl_8058E098`/`lbl_8058E138`/... = 
 * `g3d_resvtx_ac.h`) sit in the `g3d_calcvtx.cpp` data fragment (0x8058DD68-0x8058E184), and the retail
 * `.extabindex` run 0x800206B8-0x8002073C (11 EH functions) ends with `fn_800736A4`.  The seam at
 * 0x800736F8 puts the `g3d_resvtx_ac.h` scale setter `fn_800736F8` and the flag helpers
 * `fn_800737AC`..`fn_800737C4` in `g3d/g3d_calcworld.cpp`; the extabindex table pins the seam to the
 * whole-function start, but the flag-helper group carries no data reference of its own
 * (`.pi/notes/tuboundary-defects-2026-09-23.md` - measure to settle).
 *
 * rule 7 deferred: the map carries only `fn_XXXXXXXX` names in this range (docs/plan.md 6.5 rule 7);
 * renaming a symbol needs the map and the source in one edit (playbook 31).
 *
 * `#pragma fp_contract off` is file-scoped: the retail object contains no `fmadds`/`fmsubs` while
 * `cflags_g3d` passes `-fp_contract on`.  Registered `Object(NonMatching, ...)` in lib g3d; the
 * per-function residual is in the objdiff report.
 */


#include "types.h"
#include "nw4r/g3d/res_common.h"
#include "unsplit/g3d.h" /* unsplit g3d neighbours (rule 2) */

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
/* fn_800696E4/fn_80069748/fn_80069754/fn_800731EC/fn_800732F0/fn_80073354 come from
 * include/unsplit/g3d.h (unsplit g3d units, rule 2). */

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

} /* extern "C" */
