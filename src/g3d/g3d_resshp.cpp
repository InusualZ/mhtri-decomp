/*
 * g3d/g3d_resshp.cpp - nw4r g3d `ResShp`/`ResShpPrePrim`/`ResTagDL` resources (sub-resource lookups, the
 *   pre-prim attribute records, the two display-list draw paths) and the `ResTev`/`ResTex` cache helpers.
 * RANGE. .text 0x80099400-0x8009A748 (58 functions); extab, extabindex, .rodata 0x8056F730-0x8056F770, .data
 *   0x80591618-0x80591860, .sdata 0x807912B8-0x807912D8.  Left seam: fn_80099400 calls `ResShp::ref`
 *   (fn_80077674) on its own `this` where fn_800993B4 calls `ResNode::ref` (fn_8005D218), and the `.data`
 *   fragment opens with "g3d_resshp.cpp"; the right edge is `g3d/g3d_cpu.cpp`.
 * NAMES. Map stems (the dump answers `zz_` placeholders).  The `ResTagDLData` fields (`mPrePrimOfs` at +0x08,
 *   `mSize`/`mDlSize`) and the `ResTevData`/`ResTexData` fields are GUESSes; the `ResShpData` `id*` fields are
 *   spelled by fn_80099974's assert (`ref().idVtxPosition >= 0`).
 * RESIDUALS. fn_80099724: the base word loads into r3 where retail uses r0.
 *   fn_800997E0: `fn_80099640(...) + (attr - 9) * 0xC` accumulates in the local's register; retail uses r3, then +0x32.
 *   fn_80099C20: stack-slot order of the three `ResXxx` copies' destination handles and value temporaries.
 *   fn_8009A1E0: retail keeps `lbl_8056F730` in r31 across the `fn_800866FC` call (frame 0x20, r29-r31); ours
 *     rematerialises it after the call (frame 0x10, 8 bytes shorter); every spelling of the table read emits the same.
 *   fn_8009A278: the final `subf`'s destination register (r0 in retail, r4 here).
 *   flipcheck: `.text` 0x1340 of 0x1348; `.rodata`, `.data` and `.sdata` are claimed and not emitted.
 * SHAPES. File-scope `#pragma peephole off`: the `rlwinm` flag extracts keep retail's `cmpwi`, and fn_800997E0/
 *   fn_800998CC materialise their range check (`li r3, 0` / `li r3, 1` + `cmpwi`).
 *   The `.sdata` "ref" strings are sized externs (`extern char lbl_807912B8[4];`): MWCC then emits `li r7, @sda21`.
 */
#include "types.h"
#include "nw4r/math.h"
#include "nw4r/g3d/res_common.h"
#include "gx.h"                 /* GXWGFifo, the 0xCC008000 write window (rule 1)        */
#include "g3d/g3d_cpu.h"        /* fn_8009A748, owner g3d/g3d_cpu.cpp (rule 2)           */
#include "g3d/g3d_calcvtx.h"    /* fn_800734E4, owner g3d/g3d_calcvtx.cpp (rule 2)       */
#include "g3d/g3d_state.h"      /* fn_80089690, owner g3d/g3d_state.cpp (rule 2)         */
#include "g3d/g3d_resvtx.h" /* fn_80088E84/fn_80088FFC/fn_8008918C (rule 2) */

/* The target's `-O3` schedule is retail only with the peephole pass off: every flag extract keeps an
 * explicit `cmpwi` after the `rlwinm` instead of the folded record form `rlwinm.`. */
#pragma peephole off
#pragma fp_contract off

/* nw4r::db::Panic(const char*, int, const char*, ...) - the owner's real C++ declaration, called
 * through its signature, never the mangled spelling (docs/plan.md 6.5 rule 9). */
namespace nw4r {
namespace db {

void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace nw4r

/* The `ResTagDL` block embedded in the `ResShp` data at +0x18 (fn_80099740's assert names the class
 * through `.sdata` 0x80591794).  +0x08 is the offset fn_800996E8 resolves from the tag's own address;
 * the tail half also reads +0x00 (fn_80099DD4) and +0x04 (fn_8009A018), so all three words are named. */
struct ResTagDLData {
    /* +0x00 */ u32 mSize;       /* the marked block's total size (fn_80099DD4)                  */
    /* +0x04 */ u32 mDlSize;     /* the display-list size (fn_8009A018, fn_8009A018's callers)    */
    /* +0x08 */ u32 mPrePrimOfs; /* the offset from this tag to the block it marks               */
}; /* size: 0xC */

/* The `ResShp` resource block `ResShp::ref` hands back.  +0x04 is the offset fn_80099400 turns into a
 * `ResMdl` handle (fn_8007B878's assert header is `g3d_resmdl_ac.h`); +0x18 is the embedded pre-prim
 * tag and +0x24 the shape's own display-list tag.  The `id*` half are the shape's per-vertex-resource
 * ids: the first is named by fn_80099974's own assert string (`ref().idVtxPosition >= 0`), the rest
 * follow it and are `-1` when the shape has no resource of that kind. */
struct ResShpData {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 mToResMdlData;
    /* +0x08 */ u8 pad_0x08[0x18 - 0x8];
    /* +0x18 */ ResTagDLData mTag;            /* the pre-prim tag (fn_800995A0, fn_800996AC) */
    /* +0x24 */ ResTagDLData mDlTag;          /* the shape's display-list tag (fn_80099DF8) */
    /* +0x30 */ u8 pad_0x30[0x18];
    /* +0x48 */ s16 idVtxPosition;            /* vertex-position id (asserted `>= 0`)        */
    /* +0x4A */ s16 idVtxNrm;                 /* vertex-normal id, -1 = none                 */
    /* +0x4C */ s16 idVtxClr[2];              /* the colour-channel ids, -1 = none           */
    /* +0x50 */ s16 idVtxTexCoord[8];         /* the tex-coord ids, -1 = none                */
    /* +0x60 */ u8 pad_0x60[0x2];
    /* +0x62 */ s16 idTex;                    /* texture id, -1 = none                       */
}; /* size: 0x64 (a lower bound: only the fields above are reached) */

/* The `ResTev`/`ResTex` resource blocks the tail half caches (`g3d_restev_ac.h` /
 * `g3d_restex_ac.h`, the file names of their asserts).  Only the words the tail's bodies reach are
 * named; the names are the best the offsets support (the offsets are the facts). */
struct ResTevData {
    /* +0x00 */ u32 mSize;     /* the block's size, the DC range fn_8009A2F4 stores/flushes */
    /* +0x04 */ u32 mDlSize;   /* the display-list size fn_8009A278 rebases by the copy     */
    /* +0x08 */ u8 pad_0x08[0x4];
    /* +0x0C */ u8 mPrimCount; /* the primitive count fn_8009A1E0 indexes the size table by */
    /* +0x0D */ u8 pad_0x0D[0x13];
    /* +0x20 */ u8 mDl[];      /* the embedded display list (fn_8009A1E0 calls it at +0x20) */
}; /* size: 0x20+ (a lower bound: only the words above are reached) */

struct ResTexData {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 mSize;      /* the block's size: the DCFlushRangeNoSync range        */
    /* +0x08 */ u32 mFmt;       /* compared against 1 and 3 (fn_8009A3CC)                */
    /* +0x0C */ u8 pad_0x0C[0x4];
    /* +0x10 */ u32 mToImage;   /* offset from the block to the image data              */
    /* +0x14 */ u8 pad_0x14[0x4];
    /* +0x18 */ u32 mFlags;     /* bit 0 gates fn_8009A490/fn_8009A5C4                  */
    /* +0x1C */ u16 mWidth;     /* fn_8005348C                                         */
    /* +0x1E */ u16 mHeight;    /* fn_80053468                                         */
    /* +0x20 */ u32 mImageSize; /* fn_8009A490's third out                             */
    /* +0x24 */ u32 mParam;     /* `>= 2` becomes fn_8009A490's last out                */
    /* +0x28 */ f32 mLodMin;    /* fn_8009A490's fourth out                            */
    /* +0x2C */ f32 mLodMax;    /* fn_8009A490's fifth out                             */
}; /* size: 0x30 (a lower bound: only the words above are reached) */

/* The pooled constants this unit reads (this unit's data, claimed, not emitted). */
extern "C" {
extern const char lbl_805916E0[]; /* "NW4R:Failed assertion !((u32)p & 0x3)"   .data */
extern const char lbl_80591708[]; /* "g3d_resshp_ac.h"                        .data */
extern const char lbl_80591718[]; /* "ResShpPrePrim"                          .data */
extern const char lbl_80591728[]; /* "%s::%s: Object not valid."              .data */
extern const char lbl_80591748[]; /* "g3d_resshp_ac.h"                        .data */
extern const char lbl_80591758[]; /* "NW4R:Failed assertion !((u32)p & 0x3)"   .data */
extern const char lbl_80591780[]; /* "g3d_rescommon_ac.h"                     .data */
extern const char lbl_80591794[]; /* "ResTagDL"                               .data */
extern const char lbl_805917A0[]; /* "%s::%s: Object not valid."              .data */
extern const char lbl_805917BC[]; /* "g3d_rescommon_ac.h"                     .data */
extern const char lbl_805917D0[]; /* "%s::%s: Object not valid."              .data */
extern const char lbl_805917EC[]; /* "g3d_rescommon_ac.h"                     .data */
}

/* The `"ref"` member-name strings of the accessor asserts (`.sdata`).  Declared as *sized*
 * 4-byte arrays so MWCC's small-data heuristic emits the target's `li rN, @sda21` address form (an
 * unsized `extern const char[]` is assumed large and gets `lis`/`addi` - the same lever as
 * `g3d/g3d_resfile.cpp`'s `lbl_80791290`). */
extern char lbl_807912B8[4]; /* "ref" */
extern char lbl_807912BC[4]; /* "ref" */
extern char lbl_807912C0[4]; /* "ref" */

/* The neighbours this unit calls (plain map stems), each owner named beside it. */
extern "C" {
ResShpData* fn_80077674(const ResHandle* pSelf);         /* owner: src/g3d/fn_80075DCC.cpp */
ResShpData* fn_80077398(const ResHandle* pSelf);         /* owner: src/g3d/fn_80075DCC.cpp */
void* fn_8007B878(void* pOut, u32 value);                /* owner: src/g3d/fn_80075DCC.cpp */
}

/* The unit's own functions, in address order (the forward declarations keep the source order free).
 * They are C linkage: the map spells every one `fn_XXXXXXXX`. */
extern "C" {

u32 fn_80099400(ResHandle* pSelf);
u32 fn_8009943C(ResHandle* pSelf, u32 ofs);
u32 fn_80099488(void);
s32 fn_80099494(const ResHandle* pSelf);
ResHandle* fn_800994A8(ResHandle* pSelf, u32 value);
void fn_8009950C(ResHandle* pSelf, u32 value);
ResTagDLData* fn_80099514(ResHandle* pSelf);
ResTagDLData* fn_80099578(const ResHandle* pSelf);
u32 fn_80099580(void);
s32 fn_8009958C(const ResHandle* pSelf);
u32 fn_800995A0(ResHandle* pSelf);
ResHandle* fn_800995D4(ResHandle* pSelf, u32 value);
void fn_80099638(ResHandle* pSelf, u32 value);
u32 fn_80099640(ResHandle* pSelf);
u32 fn_800996A4(const ResHandle* pSelf);
u32 fn_800996AC(ResHandle* pSelf);
u32 fn_800996E8(ResHandle* pSelf);
u32 fn_80099724(const ResHandle* pSelf, u32 ofs);
ResTagDLData* fn_80099740(ResHandle* pSelf);
ResTagDLData* fn_800997A4(const ResHandle* pSelf);
u32 fn_800997AC(ResHandle* pSelf);

/* -------------------------------------------------------------------------------------------------- */

/* The `ResShp` body before the range's ResShpPrePrim cluster: build the model handle from the shape's
 * +0x04 offset. */
u32 fn_80099400(ResHandle* pSelf) {
    ResShpData* pData = fn_80077674(pSelf);
    return fn_8009943C(pSelf, pData->mToResMdlData);
}

/* Box `ResShp::ref + ofs` (0 for a null offset) and return the boxed word. */
u32 fn_8009943C(ResHandle* pSelf, u32 ofs) {
    u32 base = (u32)pSelf->mpData;
    if (ofs != 0) {
        u32 present;
        return *(u32*)fn_8007B878(&present, base + ofs);
    }
    u32 absent;
    return *(u32*)fn_8007B878(&absent, 0);
}

/* The class-name string the `%s::%s: Object not valid.` assert passes for ResShpPrePrim. */
u32 fn_80099488(void) {
    return (u32)lbl_80591718;
}

/* ResShpPrePrim's validity word. */
s32 fn_80099494(const ResHandle* pSelf) {
    return (u32)pSelf->mpData != 0;
}

/* Box a ResShpPrePrim pointer and assert it is 4-byte aligned (the `g3d_resshp_ac.h` `Ptr` setter). */
ResHandle* fn_800994A8(ResHandle* pSelf, u32 value) {
    fn_8009950C(pSelf, value);
    if ((value & 0x3) != 0) {
        nw4r::db::Panic(lbl_80591708, 0x2C, lbl_805916E0);
    }
    return pSelf;
}

/* The raw handle store the Ptr setter expands to. */
void fn_8009950C(ResHandle* pSelf, u32 value) {
    pSelf->mpData = (void*)value;
}

/* `ResTagDL::ref` with the `g3d_rescommon_ac.h` assert. */
ResTagDLData* fn_80099514(ResHandle* pSelf) {
    if (fn_8009958C(pSelf) == 0) {
        nw4r::db::Panic(lbl_805917EC, 0x151, lbl_805917D0, (const char*)fn_80099580(), lbl_807912B8);
    }
    return fn_80099578(pSelf);
}

/* The raw ref getter. */
ResTagDLData* fn_80099578(const ResHandle* pSelf) {
    return (ResTagDLData*)pSelf->mpData;
}

/* The class-name string the `%s::%s: Object not valid.` assert passes for ResTagDL. */
u32 fn_80099580(void) {
    return (u32)lbl_80591794;
}

/* ResTagDL's validity word. */
s32 fn_8009958C(const ResHandle* pSelf) {
    return (u32)pSelf->mpData != 0;
}

/* Box the ResShp data's embedded tag (its +0x18 block). */
u32 fn_800995A0(ResHandle* pSelf) {
    ResShpData* pData = fn_80077674(pSelf);
    ResHandle tag;
    return (u32)fn_800995D4(&tag, (u32)&pData->mTag)->mpData;
}

/* Box a pointer and assert it is 4-byte aligned (the `g3d_rescommon_ac.h` `Ptr` setter). */
ResHandle* fn_800995D4(ResHandle* pSelf, u32 value) {
    fn_80099638(pSelf, value);
    if ((value & 0x3) != 0) {
        nw4r::db::Panic(lbl_80591780, 0x151, lbl_80591758);
    }
    return pSelf;
}

/* The raw handle store the ResTagDL Ptr setter expands to. */
void fn_80099638(ResHandle* pSelf, u32 value) {
    pSelf->mpData = (void*)value;
}

/* Assert ResShpPrePrim validity, then hand back the pointer (the `g3d_resshp_ac.h` ref form). */
u32 fn_80099640(ResHandle* pSelf) {
    if (fn_80099494(pSelf) == 0) {
        nw4r::db::Panic(lbl_80591748, 0x2C, lbl_80591728, (const char*)fn_80099488(), lbl_807912C0);
    }
    return fn_800996A4(pSelf);
}

/* The raw pre-prim-array pointer getter. */
u32 fn_800996A4(const ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* Resolve the shape's pre-prim array: the tag's +0x08 offset, from the tag's own address. */
u32 fn_800996AC(ResHandle* pSelf) {
    ResHandle array;
    ResHandle tag;
    tag.mpData = (void*)fn_800997AC(pSelf);
    return (u32)fn_800994A8(&array, fn_800996E8(&tag))->mpData;
}

/* The tag's stored offset, resolved from the tag itself. */
u32 fn_800996E8(ResHandle* pSelf) {
    ResTagDLData* pTag = fn_80099740(pSelf);
    return fn_80099724(pSelf, pTag->mPrePrimOfs);
}

/* `base + ofs` for a boxed base, 0 for a null offset. */
u32 fn_80099724(const ResHandle* pSelf, u32 ofs) {
    u32 base = (u32)pSelf->mpData;
    if (ofs) {
        base += ofs;
        return base;
    }
    return 0;
}

/* `ResTagDL::ref` with the `g3d_rescommon_ac.h` assert (the second out-of-line copy). */
ResTagDLData* fn_80099740(ResHandle* pSelf) {
    if (fn_8009958C(pSelf) == 0) {
        nw4r::db::Panic(lbl_805917BC, 0x151, lbl_805917A0, (const char*)fn_80099580(), lbl_807912BC);
    }
    return fn_800997A4(pSelf);
}

/* The raw ref getter (the second out-of-line copy). */
ResTagDLData* fn_800997A4(const ResHandle* pSelf) {
    return (ResTagDLData*)pSelf->mpData;
}

/* Box the ResShp data's embedded tag through the other `ResShp::ref` copy. */
u32 fn_800997AC(ResHandle* pSelf) {
    ResShpData* pData = fn_80077398(pSelf);
    ResHandle tag;
    return (u32)fn_800995D4(&tag, (u32)&pData->mTag)->mpData;
}

}  /* extern "C" */

/* ================================================================================================== */
/* The tail: 0x800997E0-0x8009A748 (37 functions) - the `ResShp`/`ResShpPrePrim` attribute and draw  */
/* half plus the `ResTev`/`ResTex` cache helpers.  Same TU, same section claim (widened below).      */
/* ================================================================================================== */

/* The pooled `__FILE__`/assert strings this half passes (this unit's `.data`, claimed, not emitted). */
extern const char lbl_80591618[]; /* "g3d_resshp.cpp"                                              */
extern const char lbl_80591628[]; /* "attr is out of bounds(%d)\n%d <= attr <= %d not satisfied."    */
extern const char lbl_80591664[]; /* "NW4R:Failed assertion ref().idVtxPosition >= 0"               */
extern const char lbl_80591694[]; /* "NW4R:Failed assertion idx == 0 || idx == 1"                   */
extern const char lbl_805916C0[]; /* "NW4R:Failed assertion idx < 8"                                */
extern const char lbl_80591800[]; /* "%s::%s: Object not valid."                                   */
extern const char lbl_80591820[]; /* "g3d_restev_ac.h"                                             */
extern const char lbl_80591830[]; /* "%s::%s: Object not valid."                                   */
extern const char lbl_80591850[]; /* "g3d_restex_ac.h"                                             */

/* The `"ref"` member-name strings of the `ResTev`/`ResTex` asserts, declared as *sized* 4-byte arrays
 * so MWCC's small-data heuristic emits the target's `li rN, @sda21` form (the lever the head records). */
extern char lbl_807912C8[4]; /* "ref" */
extern char lbl_807912D0[4]; /* "ref" */

/* The pre-prim display-list size table `ResTev`'s draw path indexes by the block's primitive count
 * (`lbl_8056F730[0..15]` = 0xA0, 0xA0, 0xC0, 0xC0, 0x100, ..., 0x1E0).  `.data`, so not small data. */
extern u32 lbl_8056F730[];

/* The neighbours this half calls (plain map stems), each owner named beside it. */
extern "C" {
ResShpData* fn_80077398(const ResHandle* pSelf);  /* owner: g3d/fn_80075DCC.cpp            */
s32 fn_80076814(const ResHandle* pSelf);          /* owner: g3d/fn_80075DCC.cpp            */
void fn_800783EC(ResHandle* pSelf, u32 pData);    /* owner: g3d/fn_80075DCC.cpp            */
ResHandle* fn_800731EC(ResHandle* pSelf, u32 pData); /* owner: g3d/g3d_calcvtx.cpp         */
s32 fn_8007327C(ResHandle* pSelf);                /* owner: g3d/g3d_calcvtx.cpp            */
ResHandle* fn_80073290(ResHandle* pSelf, const ResHandle* pRhs); /* owner: g3d/g3d_calcvtx.cpp */
ResHandle* fn_80073404(ResHandle* pSelf, u32 pData); /* owner: g3d/g3d_calcvtx.cpp          */
s32 fn_80073494(ResHandle* pSelf);                /* owner: g3d/g3d_calcvtx.cpp            */
ResHandle* fn_800734A8(ResHandle* pSelf, const ResHandle* pRhs); /* owner: g3d/g3d_calcvtx.cpp */
u32 fn_800866FC(ResHandle* pSelf);                /* owner: g3d/g3d_state.cpp              */
u32* fn_80086768(void);                           /* owner: g3d/g3d_state.cpp              */
s32 fn_8008931C(ResHandle* pSelf);                /* owner: g3d/g3d_state.cpp              */
void fn_8008939C(ResHandle* pSelf, const void** ppBaseVtx, u8* pStride); /* g3d/g3d_state.cpp */
void fn_80091F70(void* pDst, u32 arg1, u32 arg2);  /* owner: g3d/g3d_resanmtexsrt.cpp       */
u32 fn_80097FC8(ResHandle* pMdl, s32 idx);        /* owner: g3d/g3d_resmat.cpp             */
u32 fn_800980E0(ResHandle* pMdl, s32 idx);        /* owner: g3d/g3d_resmat.cpp             */
u32 fn_800981F8(ResHandle* pMdl, s32 idx);        /* owner: g3d/g3d_resmat.cpp             */
ResHandle* fn_80098310(ResHandle* pSelf, u32 value); /* owner: g3d/g3d_resmat.cpp (Ptr setter) */
ResHandle* fn_800985B0(ResHandle* pSelf, u32 value); /* owner: g3d/g3d_resmat.cpp (Ptr setter) */
u32 fn_8009837C(ResHandle* pMdl, s32 idx);        /* owner: g3d/g3d_resmat.cpp             */
u32 fn_8009861C(ResHandle* pMdl, s32 idx);        /* owner: g3d/g3d_resmat.cpp             */
void fn_80071008(void* pBase, u32 size);          /* owner: g3d/g3d_calcview.cpp           */
u32 fn_80052910(ResHandle* pSelf);                /* owner: fn_8004CAD8.cpp                */
u32 fn_8005297C(void);                            /* owner: fn_8004CAD8.cpp                */
s32 fn_80052984(ResHandle* pSelf);                /* owner: fn_8004CAD8.cpp                */
u32 fn_80052998(ResHandle* pSelf);                /* owner: fn_8004CAD8.cpp                */
u32 fn_80052DBC(ResHandle* pSelf);                /* owner: fn_8004CAD8.cpp                */
u32 fn_80052E8C(ResHandle* pSelf);                /* owner: fn_8004CAD8.cpp                */
u16 fn_80053468(ResHandle* pSelf);                /* owner: fn_8004CAD8.cpp                */
u16 fn_8005348C(ResHandle* pSelf);                /* owner: fn_8004CAD8.cpp                */
void GXCallDisplayList(void* pList, u32 size);    /* the SDK's own symbol                  */
void PPCSync(void);                               /* the SDK's own symbol                  */
void* memset(void* pDst, int value, u32 size);    /* the runtime's own symbol              */
}

/* The tail's own functions, in address order (the forward declarations keep the source order free).
 * They are C linkage: the map spells every one `fn_XXXXXXXX`. */
extern "C" {
void fn_800997E0(ResHandle* pSelf, s32 attr, u32 pData, u32 size);
void fn_800998CC(ResHandle* pSelf, u32 attr);
u32 fn_80099974(ResHandle* pSelf);
u32 fn_800999E8(ResHandle* pSelf);
u32 fn_80099A58(ResHandle* pSelf, u32 idx);
u32 fn_80099B10(ResHandle* pSelf, u32 idx);
u32 fn_80099BB0(ResHandle* pSelf);
void fn_80099C20(ResHandle* pSelf);
u32 fn_80099DD4(ResHandle* pSelf);
u32 fn_80099DF8(ResHandle* pSelf);
ResHandle* fn_80099E2C(ResHandle* pSelf, const ResHandle* pRhs);
void fn_80099E5C(ResHandle* pSelf, const ResHandle* pRhs);
void fn_80099E68(ResHandle* pSelf);
void fn_80099F1C(ResHandle* pSelf, s32 pSync, s32 pSkipPrePrimHeader);
void fn_8009A000(u32 addr, u32 size);
u32 fn_8009A018(ResHandle* pSelf);
ResHandle* fn_8009A03C(ResHandle* pSelf, const ResHandle* pRhs);
void fn_8009A06C(ResHandle* pSelf, const ResHandle* pRhs);
void fn_8009A078(ResHandle* pSelf, s32 flag);
u32 fn_8009A0F8(ResHandle* pSelf);
void fn_8009A12C(ResHandle* pSelf, s32 flag);
u32 fn_8009A174(ResHandle* pSelf);
u32 fn_8009A1D8(ResHandle* pSelf);
void fn_8009A1E0(ResHandle* pSelf, s32 flag);
u8 fn_8009A254(ResHandle* pSelf);
u32 fn_8009A278(ResHandle* pSelf, void* pDst);
void fn_8009A2F4(ResHandle* pSelf, s32 flag);
void fn_8009A360(ResHandle* pSelf, s32 flag);
u32 fn_8009A3CC(ResHandle* pSelf);
u32 fn_8009A408(ResHandle* pSelf);
u32 fn_8009A42C(ResHandle* pSelf);
u32 fn_8009A490(ResHandle* pSelf, u32* pOutVtxData, u16* pOutWidth, u16* pOutHeight, u32* pOutImageSize,
                 f32* pOutLodMin, f32* pOutLodMax, u8* pOutFlag);
u32 fn_8009A58C(ResHandle* pSelf);
u32 fn_8009A5C4(ResHandle* pSelf, u32* pOutVtxData, u16* pOutWidth, u16* pOutHeight, u32* pOutImageSize,
                 f32* pOutLodMin, f32* pOutLodMax, u8* pOutFlag);
u32 fn_8009A6C0(ResHandle* pSelf);
u32 fn_8009A6FC(ResHandle* pSelf);
void fn_8009A720(ResHandle* pSelf);
}

/* -------------------------------------------------------------------------------------------------- */
/* The `ResShp` attribute records: one 0xC-byte record per GX attribute in the pre-prim block's        */
/* attribute table (the block's own +0x32, indexed by `attr - GX_VA_POS`).                             */
/* -------------------------------------------------------------------------------------------------- */

/* 0x800997E0 - writes one GX vertex attribute's size/data words (asserting GX_VA_POS..GX_VA_TEX7): `pData` is the
 * base-vertex address biased by 0x80000000, `size` the stride/count byte; attribute 25 takes the single form. */
extern "C" void fn_800997E0(ResHandle* pSelf, s32 attr, u32 pData, u32 size) {
    bool inRange = false;
    if ((u32)(attr - 9) <= 11) {
        inRange = true;
    }

    if (!inRange) {
        nw4r::db::Panic(lbl_80591618, 0x2D4, lbl_80591628, attr, 9, 20);
    }

    u32 prePrim = fn_800996AC(pSelf);
    u8* pRecord = (u8*)fn_80099640((ResHandle*)&prePrim) + (attr - 9) * 0xC + 0x32;
    u32 count = (attr == 25) ? 1 : (attr - 9);

    fn_80091F70(pRecord, (u8)(count + 0xA0), pData + 0x80000000);
    fn_80091F70(pRecord + 6, (u8)(count + 0xB0), size & 0xFF);
}

/* 0x800998CC - clear one attribute's record (the array form fn_80099E68 walks attr 9..20 with). */
extern "C" void fn_800998CC(ResHandle* pSelf, u32 attr) {
    bool inRange = false;
    if ((u32)(attr - 9) <= 11) {
        inRange = true;
    }

    if (!inRange) {
        nw4r::db::Panic(lbl_80591618, 0x30F, lbl_80591628, attr, 9, 20);
    }

    u32 prePrim = fn_800996AC(pSelf);
    void* pRecord = (u8*)fn_80099640((ResHandle*)&prePrim) + (attr - 9) * 0xC + 0x32;

    memset(pRecord, 0, 0xC);
}

/* 0x80099974 - the shape's vertex-position resource: resolve the owning model's position array at the
 * shape's own `idVtxPosition` (the assert the target bakes into `ref().idVtxPosition >= 0`). */
extern "C" u32 fn_80099974(ResHandle* pSelf) {
    if (fn_80077674(pSelf)->idVtxPosition < 0) {
        nw4r::db::Panic(lbl_80591618, 0x31E, lbl_80591664);
    }

    u32 mdl = fn_80099400(pSelf);
    return fn_80097FC8((ResHandle*)&mdl, fn_80077674(pSelf)->idVtxPosition);
}

/* 0x800999E8 - the vertex-normal resource, or a null handle when the shape has none (`idVtxNrm == -1`). */
extern "C" u32 fn_800999E8(ResHandle* pSelf) {
    ResShpData* pData = fn_80077674(pSelf);

    if (pData->idVtxNrm != -1) {
        u32 mdl = fn_80099400(pSelf);
        return fn_800980E0((ResHandle*)&mdl, pData->idVtxNrm);
    }

    ResHandle ret;
    return (u32)fn_80073404(&ret, 0)->mpData;
}

/* 0x80099A58 - one colour-channel resource (two per shape, hence the `idx == 0 || idx == 1` assert). */
extern "C" u32 fn_80099A58(ResHandle* pSelf, u32 idx) {
    ResShpData* pData = fn_80077674(pSelf);
    bool inRange = (idx == 0 || idx == 1);

    if (!inRange) {
        nw4r::db::Panic(lbl_80591618, 0x330, lbl_80591694);
    }

    s16* pId = &pData->idVtxClr[idx];

    if (*pId != -1) {
        u32 mdl = fn_80099400(pSelf);
        return fn_800981F8((ResHandle*)&mdl, *pId);
    }

    ResHandle ret;
    return (u32)fn_800731EC(&ret, 0)->mpData;
}

/* 0x80099B10 - one tex-coord resource (eight per shape, hence the `idx < 8` assert). */
extern "C" u32 fn_80099B10(ResHandle* pSelf, u32 idx) {
    if (idx >= 8) {
        nw4r::db::Panic(lbl_80591618, 0x33B, lbl_805916C0);
    }

    s16* pId = &fn_80077674(pSelf)->idVtxTexCoord[idx];

    if (*pId != -1) {
        u32 mdl = fn_80099400(pSelf);
        return fn_8009837C((ResHandle*)&mdl, *pId);
    }

    ResHandle ret;
    return (u32)fn_80098310(&ret, 0)->mpData;
}

/* 0x80099BB0 - the shape's texture, or a null handle when it has none (`idTex == -1`). */
extern "C" u32 fn_80099BB0(ResHandle* pSelf) {
    ResShpData* pData = fn_80077674(pSelf);

    if (pData->idTex != -1) {
        u32 mdl = fn_80099400(pSelf);
        return fn_8009861C((ResHandle*)&mdl, pData->idTex);
    }

    ResHandle ret;
    return (u32)fn_800985B0(&ret, 0)->mpData;
}

/* 0x80099C20 - rebuilds the pre-prim attribute records, one per vertex resource the shape has, then invalidates
 * the pre-prim and display-list blocks in the data cache. */
extern "C" void fn_80099C20(ResHandle* pSelf) {
    const void* pBaseVtx;
    u8 stride;

    u32 vtxPos = fn_80099974(pSelf);
    fn_80088E84(&vtxPos, &pBaseVtx, &stride);
    fn_800997E0(pSelf, 9, (u32)pBaseVtx, stride);

    u32 vtxNrm = fn_800999E8(pSelf);
    ResHandle nrm;
    fn_800734A8(&nrm, (const ResHandle*)&vtxNrm);

    if (fn_80073494(&nrm)) {
        fn_80088FFC(&nrm, &pBaseVtx, &stride);
        fn_800997E0(pSelf, 10, (u32)pBaseVtx, stride);
    }

    for (u32 i = 0; i < 2; i++) {
        u32 vtxClr = fn_80099A58(pSelf, i);
        ResHandle clr;
        fn_80073290(&clr, (const ResHandle*)&vtxClr);

        if (fn_8007327C(&clr)) {
            fn_8008918C(&clr, &pBaseVtx, &stride);
            fn_800997E0(pSelf, i + 11, (u32)pBaseVtx, stride);
        }
    }

    for (u32 i = 0; i < 8; i++) {
        u32 vtxTex = fn_80099B10(pSelf, i);
        ResHandle tex;
        fn_80099E2C(&tex, (const ResHandle*)&vtxTex);

        if (fn_8008931C(&tex)) {
            fn_8008939C(&tex, &pBaseVtx, &stride);
            fn_800997E0(pSelf, i + 13, (u32)pBaseVtx, stride);
        }
    }

    u32 prePrim = fn_800996AC(pSelf);
    fn_8009A12C((ResHandle*)&prePrim, 0);

    /* Two copies of the shape's own display-list tag (the inlined accessor reached twice): the first
     * supplies the size, the second the address the tag resolves to. */
    u32 tagSize = fn_80099DF8(pSelf);
    u32 tagAddr = fn_80099DF8(pSelf);
    u32 size = fn_80099DD4((ResHandle*)&tagSize);
    fn_80089690((void*)fn_800996E8((ResHandle*)&tagAddr), size);
}

/* 0x80099DD4 - the display-list tag's total size (`ResTagDL::ref` then its +0x00 word). */
extern "C" u32 fn_80099DD4(ResHandle* pSelf) {
    return fn_80099514(pSelf)->mSize;
}

/* 0x80099DF8 - box the shape's own display-list tag (the ResShp data's +0x24 block). */
extern "C" u32 fn_80099DF8(ResHandle* pSelf) {
    ResShpData* pData = fn_80077398(pSelf);
    ResHandle tag;
    return (u32)fn_800995D4(&tag, (u32)&pData->mDlTag)->mpData;
}

/* 0x80099E2C - the handle copy-assignment the shape accessors' `_ac.h` inlines instantiate (the
 * target emits it out of line, and fn_80099F1C's local tag is built through it). */
extern "C" ResHandle* fn_80099E2C(ResHandle* pSelf, const ResHandle* pRhs) {
    fn_80099E5C(pSelf, pRhs);
    return pSelf;
}

/* 0x80099E5C - the raw word copy that assignment expands to. */
extern "C" void fn_80099E5C(ResHandle* pSelf, const ResHandle* pRhs) {
    pSelf->mpData = pRhs->mpData;
}

/* 0x80099E68 - clear every GX attribute record of the pre-prim block.  The target's 12 calls are the
 * unrolled `attr = GX_VA_POS..GX_VA_TEX7` walk, so the walk is written out. */
extern "C" void fn_80099E68(ResHandle* pSelf) {
    fn_800998CC(pSelf, 9);
    fn_800998CC(pSelf, 10);
    fn_800998CC(pSelf, 11);
    fn_800998CC(pSelf, 12);
    fn_800998CC(pSelf, 13);
    fn_800998CC(pSelf, 14);
    fn_800998CC(pSelf, 15);
    fn_800998CC(pSelf, 16);
    fn_800998CC(pSelf, 17);
    fn_800998CC(pSelf, 18);
    fn_800998CC(pSelf, 19);
    fn_800998CC(pSelf, 20);
}

/* 0x80099F1C - draws the pre-prim block: `pSync` picks the SDK's `GXCallDisplayList` over the inline pipe
 * command, `pSkipPrePrimHeader` skips the block's 0x20-byte header. */
extern "C" void fn_80099F1C(ResHandle* pSelf, s32 pSync, s32 pSkipPrePrimHeader) {
    ResHandle hTag;
    u32 tag = fn_800995A0(pSelf);
    fn_8009A03C(&hTag, (const ResHandle*)&tag);

    if (pSkipPrePrimHeader != 0) {
        if (pSync != 0) {
            u32 size = fn_8009A018(&hTag);
            GXCallDisplayList((void*)(fn_800996E8(&hTag) + 0x20), size - 0x20);
        } else {
            u32 size = fn_8009A018(&hTag);
            fn_8009A000(fn_800996E8(&hTag) + 0x20, size - 0x20);
        }
    } else {
        if (pSync != 0) {
            u32 size = fn_8009A018(&hTag);
            GXCallDisplayList((void*)fn_800996E8(&hTag), size);
        } else {
            u32 size = fn_8009A018(&hTag);
            fn_8009A000(fn_800996E8(&hTag), size);
        }
    }
}

/* 0x8009A000 - the inline `GXCallDisplayList` pipe command (opcode 0x40, then address and size). */
extern "C" void fn_8009A000(u32 addr, u32 size) {
    GXWGFifo.u8 = 0x40;
    GXWGFifo.u32 = addr;
    GXWGFifo.u32 = size;
}

/* 0x8009A018 - the display-list tag's display-list size (its +0x04 word). */
extern "C" u32 fn_8009A018(ResHandle* pSelf) {
    return fn_80099514(pSelf)->mDlSize;
}

/* 0x8009A03C - the second handle copy-assignment instantiation (fn_8009A078's local tag). */
extern "C" ResHandle* fn_8009A03C(ResHandle* pSelf, const ResHandle* pRhs) {
    fn_8009A06C(pSelf, pRhs);
    return pSelf;
}

/* 0x8009A06C - the raw word copy that assignment expands to. */
extern "C" void fn_8009A06C(ResHandle* pSelf, const ResHandle* pRhs) {
    pSelf->mpData = pRhs->mpData;
}

/* 0x8009A078 - draw the shape's own display list (the +0x24 tag), `flag` picking the SDK call over
 * the inline pipe command. */
extern "C" void fn_8009A078(ResHandle* pSelf, s32 flag) {
    ResHandle hTag;
    u32 tag = fn_8009A0F8(pSelf);
    fn_8009A03C(&hTag, (const ResHandle*)&tag);

    if (flag != 0) {
        u32 size = fn_8009A018(&hTag);
        GXCallDisplayList((void*)fn_800996E8(&hTag), size);
    } else {
        u32 size = fn_8009A018(&hTag);
        fn_8009A000(fn_800996E8(&hTag), size);
    }
}

/* 0x8009A0F8 - the same +0x24 tag through the first `ResShp::ref` copy (the head's fn_800997AC is
 * the +0x18 one through the second). */
extern "C" u32 fn_8009A0F8(ResHandle* pSelf) {
    ResShpData* pData = fn_80077674(pSelf);
    ResHandle tag;
    return (u32)fn_800995D4(&tag, (u32)&pData->mDlTag)->mpData;
}

/* 0x8009A12C - store or invalidate the pre-prim block's 0xE0 bytes in the data cache. */
extern "C" void fn_8009A12C(ResHandle* pSelf, s32 flag) {
    u32 data = fn_80099640(pSelf);

    if (flag != 0) {
        fn_800734E4((void*)data, 0xE0);
    } else {
        fn_80089690((void*)data, 0xE0);
    }
}

/* -------------------------------------------------------------------------------------------------- */
/* The `ResTev` block: its checked handle, its primitive-count and the display-list call it drives.     */
/* -------------------------------------------------------------------------------------------------- */

/* 0x8009A174 - `ResTev::ref` with the `g3d_restev_ac.h` validity assert. */
extern "C" u32 fn_8009A174(ResHandle* pSelf) {
    if (fn_80076814(pSelf) == 0) {
        nw4r::db::Panic(lbl_80591820, 0x25, lbl_80591800, (const char*)fn_80086768(), lbl_807912C8);
    }
    return fn_8009A1D8(pSelf);
}

/* 0x8009A1D8 - the raw `ref()` word. */
extern "C" u32 fn_8009A1D8(ResHandle* pSelf) {
    return (u32)pSelf->mpData;
}

/* 0x8009A1E0 - call the block's display list with the size its primitive count names: `PPCSync` first
 * when the caller asks for it, then `lbl_8056F730[count - 1]` bytes at the block's +0x20. */
extern "C" void fn_8009A1E0(ResHandle* pSelf, s32 flag) {
    if (flag != 0) {
        PPCSync();
    }

    u32 prim = (u8)fn_8009A254(pSelf) - 1;

    GXCallDisplayList((void*)(fn_800866FC(pSelf) + 0x20), lbl_8056F730[prim]);
}

/* 0x8009A254 - the block's primitive count (its +0x0C byte). */
extern "C" u8 fn_8009A254(ResHandle* pSelf) {
    return *(u8*)(fn_800866FC(pSelf) + 0xC);
}

/* 0x8009A278 - copy the block to a 0x20-aligned destination, rebase its recorded size by the amount
 * the copy moved it by, and store the copy back to the cache. */
extern "C" u32 fn_8009A278(ResHandle* pSelf, void* pDst) {
    u32 src = fn_8009A174(pSelf);
    fn_8009A748(pDst, (const void*)src, 0x200);

    ResHandle copy;
    fn_800783EC(&copy, (u32)pDst);

    ResTevData* pData = (ResTevData*)fn_8009A174(&copy);
    u32 size = pData->mDlSize;
    size -= (u32)pDst - src;
    pData->mDlSize = size;

    fn_8009A2F4(&copy, 0);
    return (u32)copy.mpData;
}

/* 0x8009A2F4 - store or invalidate the block's own `mSize` bytes. */
extern "C" void fn_8009A2F4(ResHandle* pSelf, s32 flag) {
    u32 base = fn_8009A174(pSelf);
    u32 size = *(u32*)fn_8009A174(pSelf);

    if (flag != 0) {
        fn_800734E4((void*)base, size);
    } else {
        fn_80089690((void*)base, size);
    }
}

/* -------------------------------------------------------------------------------------------------- */
/* The `ResTex` block: its checked handle, its format test and the two resource-info shapes.           */
/* -------------------------------------------------------------------------------------------------- */

/* 0x8009A360 - store or invalidate the block's `mSize` bytes (the second `ref` copy's twin of
 * fn_8009A2F4). */
extern "C" void fn_8009A360(ResHandle* pSelf, s32 flag) {
    u32 base = fn_80052E8C(pSelf);
    u32 size = *(u32*)(fn_80052E8C(pSelf) + 0x4);

    if (flag != 0) {
        fn_800734E4((void*)base, size);
    } else {
        fn_80089690((void*)base, size);
    }
}

/* 0x8009A3CC - whether the block's format is one of the two the caller treats specially. */
extern "C" u32 fn_8009A3CC(ResHandle* pSelf) {
    u32 fmt = fn_8009A408(pSelf);
    bool result = (fmt == 3 || fmt == 1);
    return result;
}

/* 0x8009A408 - the block's format word (its +0x08). */
extern "C" u32 fn_8009A408(ResHandle* pSelf) {
    return *(u32*)(fn_80052DBC(pSelf) + 0x8);
}

/* 0x8009A42C - `ResTex::ref` with the `g3d_restex_ac.h` validity assert. */
extern "C" u32 fn_8009A42C(ResHandle* pSelf) {
    if (fn_80052984(pSelf) == 0) {
        nw4r::db::Panic(lbl_80591850, 0x26, lbl_80591830, (const char*)fn_8005297C(), lbl_807912D0);
    }
    return fn_80052998(pSelf);
}

/* 0x8009A490 - the block's resource description: its flags must say the description is readable, and
 * each non-null out-parameter takes one field.  `0` is the refusal, `1` the success. */
extern "C" u32 fn_8009A490(ResHandle* pSelf, u32* pOutVtxData, u16* pOutWidth, u16* pOutHeight,
                            u32* pOutImageSize, f32* pOutLodMin, f32* pOutLodMax, u8* pOutFlag) {
    ResTexData* pData = (ResTexData*)fn_80052910(pSelf);

    if ((pData->mFlags & 1) != 0) {
        return 0;
    }

    if (pOutVtxData != NULL) {
        *pOutVtxData = fn_8009A58C(pSelf);
    }
    if (pOutWidth != NULL) {
        *pOutWidth = fn_8005348C(pSelf);
    }
    if (pOutHeight != NULL) {
        *pOutHeight = fn_80053468(pSelf);
    }
    if (pOutImageSize != NULL) {
        *pOutImageSize = pData->mImageSize;
    }
    if (pOutLodMin != NULL) {
        *pOutLodMin = pData->mLodMin;
    }
    if (pOutLodMax != NULL) {
        *pOutLodMax = pData->mLodMax;
    }
    if (pOutFlag != NULL) {
        *pOutFlag = pData->mParam > 1;
    }

    return 1;
}

/* 0x8009A58C - the block's image address: its own base plus the +0x10 offset, or 0 when it has none. */
extern "C" u32 fn_8009A58C(ResHandle* pSelf) {
    ResTexData* pData = (ResTexData*)fn_80052910(pSelf);
    s32 ofs = pData->mToImage;

    if (ofs != 0) {
        pData = (ResTexData*)((u8*)pData + ofs);
    } else {
        pData = NULL;
    }
    return (u32)pData;
}

/* 0x8009A5C4 - fn_8009A490's twin with the flag test inverted (the second `_ac.h` copy: same body,
 * `ref` reached through the other instantiation). */
extern "C" u32 fn_8009A5C4(ResHandle* pSelf, u32* pOutVtxData, u16* pOutWidth, u16* pOutHeight,
                            u32* pOutImageSize, f32* pOutLodMin, f32* pOutLodMax, u8* pOutFlag) {
    ResTexData* pData = (ResTexData*)fn_80052910(pSelf);

    if ((pData->mFlags & 1) == 0) {
        return 0;
    }

    if (pOutVtxData != NULL) {
        *pOutVtxData = fn_8009A58C(pSelf);
    }
    if (pOutWidth != NULL) {
        *pOutWidth = fn_8005348C(pSelf);
    }
    if (pOutHeight != NULL) {
        *pOutHeight = fn_80053468(pSelf);
    }
    if (pOutImageSize != NULL) {
        *pOutImageSize = pData->mImageSize;
    }
    if (pOutLodMin != NULL) {
        *pOutLodMin = pData->mLodMin;
    }
    if (pOutLodMax != NULL) {
        *pOutLodMax = pData->mLodMax;
    }
    if (pOutFlag != NULL) {
        *pOutFlag = pData->mParam > 1;
    }

    return 1;
}

/* 0x8009A6C0 - fn_8009A3CC's twin through the +0x08 reader's other instantiation. */
extern "C" u32 fn_8009A6C0(ResHandle* pSelf) {
    u32 fmt = fn_8009A6FC(pSelf);
    bool result = (fmt == 3 || fmt == 1);
    return result;
}

/* 0x8009A6FC - the block's format word through the `fn_80052910` instantiation. */
extern "C" u32 fn_8009A6FC(ResHandle* pSelf) {
    return *(u32*)(fn_80052910(pSelf) + 0x8);
}

/* 0x8009A720 - write the block's `mSize` bytes at its own address out to main memory (the `ResTex`
 * store the loader ends with; fn_80071008 is the SDK's `DCFlushRangeNoSync`). */
extern "C" void fn_8009A720(ResHandle* pSelf) {
    ResTexData* pData = (ResTexData*)fn_8009A42C(pSelf);
    fn_80071008(pData, pData->mSize);
}
