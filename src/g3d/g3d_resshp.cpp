/*
 * g3d/g3d_resshp.cpp - nw4r g3d `ResShp`/`ResShpPrePrim`/`ResTagDL` resources (sub-resource lookups, the
 *   pre-prim attribute records, the two display-list draw paths).
 * RANGE. .text 0x80099400-0x8009A174 (42 functions); extab 0x80009880-0x80009968, extabindex
 *   0x8002289C-0x800229F8, .data 0x80591618-0x80591800, .sdata 0x807912B8-0x807912C8.  Left seam:
 *   fn_80099400 calls `ResShp::ref` (fn_80077674) on its own `this` where fn_800993B4 calls `ResNode::ref`
 *   (fn_8005D218), and the `.data` fragment opens with "g3d_resshp.cpp"; the right edge is
 *   `g3d/g3d_restev.cpp` (seam request flip-resshp#1: the extabindex record at 0x800229F8 names 0x8009A174
 *   with extab 0x80009968, the ResTev strings start 8-aligned at 0x80591800 after this unit's last string at
 *   0x805917FF, and the `.sdata` "ref" statics of this unit end at 0x807912C4 with the next fragment
 *   8-aligned at 0x807912C8).
 * NAMES. res_shp_get_vtx_clr is a GUESS; res_shp_get_vtx_nrm is a GUESS (the evidence follows).
 *   res_shp_get_vtx_pos/nrm/clr are GUESSES (nw4r's ResShp::GetResVtx*, by the asserts and the ScnMdl buffer
 *   refill).  Map stems (the dump answers `zz_` placeholders) except the nw4r members ResShp::Init/Terminate
 *   and ResShp::CallPrePrimitiveDisplayList/CallPrimitiveDisplayList.  The `ResTagDLData` fields
 *   (`mPrePrimOfs` at +0x08, `mSize`/`mDlSize`) are GUESSes; the `ResShpData` `id*` fields are spelled by
 *   res_shp_get_vtx_pos's assert (`ref().idVtxPosition >= 0`).
 * RESIDUALS. The four one-call stubs (res_shp_pre_prim_class_name/_ptr/_ref, res_tag_dl_ref) are 16 B of
 *   `.text` the linker dead-strips.  The `.sdata` fragment is 0xC B of statics where the claim is 0x10 B:
 *   the last word is the linker's 8-byte alignment pad before `g3d/g3d_restev.cpp`'s fragment.
 * SHAPES. File-scope `#pragma peephole off`: the `rlwinm` flag extracts keep retail's `cmpwi`, and fn_800997E0/
 *   fn_800998CC materialise their range check (`li r3, 0` / `li r3, 1` + `cmpwi`).
 *   `#pragma pool_data off`; the TU's own strings are literals.  The `_ac.h` accessor instances are
 *   `__declspec(noinline) inline` weak copies: MWCC emits a copy after the first function that references it
 *   (several in one function: reverse reference order), creates its string literals at its definition (weak
 *   `@STRING@` copies in reverse definition order after the TU's pool, `static char` member names forward in
 *   `.sdata`), so the definitions stand in the `.data` tail's order and a stub before each retail address
 *   places the code.  fn_80099724 is compiled with `#pragma peephole on` (base word in r0); fn_800997E0 adds the
 *   record's index before its +0x32 offset.  ResShp::Init builds each vertex resource's handle as a
 *   `ResHandleTemp` class temporary (retail's return-value temporaries, allocated after the named locals).
 */
#include "types.h"
#include "nw4r/math.h"
#include "nw4r/g3d/res_common.h"
#include "gx.h"                 /* GXWGFifo, the 0xCC008000 write window (rule 1)        */
#include "g3d/g3d_cpu.h"
#include "g3d/g3d_resmat.h"
#include "g3d/g3d_state.h"
#include "g3d/g3d_resvtx.h"
#include "g3d/g3d_resshp.h"
#include "g3d/g3d_rescommon.h" /* ResHandleTemp (rule 1) */

#pragma pool_data off

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

using nw4r::g3d::ResShpData;
using nw4r::g3d::ResTagDLData;

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

}

/* -------------------------------------------------------------------------------------------------- */
/* The `_ac.h` accessor instances the target keeps as weak out-of-line copies.  MWCC emits such a copy  */
/* after the first function that references it (several in one function: in reverse reference order)  */
/* and creates its string literals where it is defined: the weak `@STRING@` copies come out in reverse */
/* definition order and the `static` member names forward, so the definitions stand in the order the   */
/* `.data` tail needs, and a one-call stub at each retail address places the code (the linker strips   */
/* the stubs).                                                                                        */
/* -------------------------------------------------------------------------------------------------- */

/* `ResTagDL::ref` with the `g3d_rescommon_ac.h` assert. */
extern "C" __declspec(noinline) inline ResTagDLData* fn_80099514(ResHandle* pSelf) {
    static char sMember[] = "ref";

    if (fn_8009958C(pSelf) == 0) {
        nw4r::db::Panic("g3d_rescommon_ac.h", 0x151, "%s::%s: Object not valid.", (const char*)fn_80099580(), sMember);
    }
    return fn_80099578(pSelf);
}

/* `ResTagDL::ref` with the `g3d_rescommon_ac.h` assert (the const instance). */
extern "C" __declspec(noinline) inline ResTagDLData* fn_80099740(ResHandle* pSelf) {
    static char sMember[] = "ref";

    if (fn_8009958C(pSelf) == 0) {
        nw4r::db::Panic("g3d_rescommon_ac.h", 0x151, "%s::%s: Object not valid.", (const char*)fn_80099580(), sMember);
    }
    return fn_800997A4(pSelf);
}

/* The class-name string the `%s::%s: Object not valid.` assert passes for ResTagDL. */
extern "C" __declspec(noinline) inline u32 fn_80099580(void) {
    return (u32)"ResTagDL";
}

/* Box a pointer and assert it is 4-byte aligned (the `g3d_rescommon_ac.h` `Ptr` setter). */
extern "C" __declspec(noinline) inline ResHandle* fn_800995D4(ResHandle* pSelf, u32 value) {
    fn_80099638(pSelf, value);
    if ((value & 0x3) != 0) {
        nw4r::db::Panic("g3d_rescommon_ac.h", 0x151, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return pSelf;
}

/* Assert ResShpPrePrim validity, then hand back the pointer (the `g3d_resshp_ac.h` ref form). */
extern "C" __declspec(noinline) inline u32 fn_80099640(ResHandle* pSelf) {
    static char sMember[] = "ref";

    if (fn_80099494(pSelf) == 0) {
        nw4r::db::Panic("g3d_resshp_ac.h", 0x2C, "%s::%s: Object not valid.", (const char*)fn_80099488(), sMember);
    }
    return fn_800996A4(pSelf);
}

/* The class-name string the `%s::%s: Object not valid.` assert passes for ResShpPrePrim. */
extern "C" __declspec(noinline) inline u32 fn_80099488(void) {
    return (u32)"ResShpPrePrim";
}

/* Box a ResShpPrePrim pointer and assert it is 4-byte aligned (the `g3d_resshp_ac.h` `Ptr` setter). */
extern "C" __declspec(noinline) inline ResHandle* fn_800994A8(ResHandle* pSelf, u32 value) {
    fn_8009950C(pSelf, value);
    if ((value & 0x3) != 0) {
        nw4r::db::Panic("g3d_resshp_ac.h", 0x2C, "NW4R:Failed assertion !((u32)p & 0x3)");
    }
    return pSelf;
}

/* The string-free instances whose place comes from the same rule: `ResTagDL::ref`'s callees follow it
 * in reverse reference order (ptr, class name, validity), and `fn_80099724` precedes the const `ref`
 * instance that fn_800996E8 references after it. */

/* The raw ref getter. */
extern "C" __declspec(noinline) inline ResTagDLData* fn_80099578(const ResHandle* pSelf) {
    return (ResTagDLData*)pSelf->mpData;
}

/* ResTagDL's validity word. */
extern "C" __declspec(noinline) inline s32 fn_8009958C(const ResHandle* pSelf) {
    return (u32)pSelf->mpData != 0;
}

#pragma peephole on
/* `base + ofs` for a boxed base, 0 for a null offset. */
extern "C" __declspec(noinline) inline u32 fn_80099724(const ResHandle* pSelf, u32 ofs) {
    u32 base = (u32)pSelf->mpData;
    if (ofs) {
        base += ofs;
        return base;
    }
    return 0;
}
#pragma peephole off

/* -------------------------------------------------------------------------------------------------- */

extern "C" {

/* The `ResShp` body before the range's ResShpPrePrim cluster: build the model handle from the shape's
 * +0x04 offset. */
u32 fn_80099400(ResHandle* pSelf) {
    const ResShpData* pData = &reinterpret_cast<const nw4r::g3d::ResShp*>(pSelf)->ref();
    return fn_8009943C(pSelf, pData->mToResMdlData);
}

/* Box `ResShp::ref + ofs` (0 for a null offset) and return the boxed word. */
u32 fn_8009943C(ResHandle* pSelf, u32 ofs) {
    u32 base = (u32)pSelf->mpData;
    if (ofs != 0) {
        return *(u32*)&nw4r::g3d::ResMdl((void*)(base + ofs));
    }
    return *(u32*)&nw4r::g3d::ResMdl((void*)0);
}

/* ResShpPrePrim's class name; dead-stripped, it places the weak copy of fn_80099488 here. */
u32 res_shp_pre_prim_class_name(void) {
    return fn_80099488();
}

/* ResShpPrePrim's validity word. */
s32 fn_80099494(const ResHandle* pSelf) {
    return (u32)pSelf->mpData != 0;
}

/* A ResShpPrePrim handle's boxing; dead-stripped, it places the weak copy of fn_800994A8 here. */
ResHandle* res_shp_pre_prim_ptr(ResHandle* pSelf, u32 value) {
    return fn_800994A8(pSelf, value);
}

/* The raw handle store the Ptr setter expands to. */
void fn_8009950C(ResHandle* pSelf, u32 value) {
    pSelf->mpData = (void*)value;
}

/* A ResTagDL handle's checked block; dead-stripped, it places the weak copy of fn_80099514 here (and
 * its three callees after it). */
ResTagDLData* res_tag_dl_ref(ResHandle* pSelf) {
    return fn_80099514(pSelf);
}

/* Box the ResShp data's embedded tag (its +0x18 block). */
u32 fn_800995A0(ResHandle* pSelf) {
    const ResShpData* pData = &reinterpret_cast<const nw4r::g3d::ResShp*>(pSelf)->ref();
    ResHandle tag;
    return (u32)fn_800995D4(&tag, (u32)&pData->mTag)->mpData;
}

/* The raw handle store the ResTagDL Ptr setter expands to. */
void fn_80099638(ResHandle* pSelf, u32 value) {
    pSelf->mpData = (void*)value;
}

/* A ResShpPrePrim handle's checked pointer; dead-stripped, it places the weak copy of fn_80099640 here. */
u32 res_shp_pre_prim_ref(ResHandle* pSelf) {
    return fn_80099640(pSelf);
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

/* The raw ref getter (the const instance). */
ResTagDLData* fn_800997A4(const ResHandle* pSelf) {
    return (ResTagDLData*)pSelf->mpData;
}

/* Box the ResShp data's embedded tag through the other `ResShp::ref` copy. */
u32 fn_800997AC(ResHandle* pSelf) {
    ResShpData* pData = &reinterpret_cast<nw4r::g3d::ResShp*>(pSelf)->ref();
    ResHandle tag;
    return (u32)fn_800995D4(&tag, (u32)&pData->mTag)->mpData;
}

}  /* extern "C" */

/* ================================================================================================== */
/* The tail: 0x800997E0-0x8009A174 (21 functions) - the `ResShp`/`ResShpPrePrim` attribute and draw  */
/* half.  Same TU, same section claim.                                                               */
/* ================================================================================================== */

/* The neighbours this half calls (plain map stems), each owner named beside it. */
extern "C" {
void fn_80091F70(void* pDst, u32 arg1, u32 arg2);  /* owner: g3d/g3d_resanmtexsrt.cpp       */
void GXCallDisplayList(void* pList, u32 size);    /* the SDK's own symbol                  */
void* memset(void* pDst, int value, u32 size);    /* the runtime's own symbol              */
}

/* The tail's own functions, in address order (the forward declarations keep the source order free).
 * They are C linkage: the map spells every one `fn_XXXXXXXX`. */
extern "C" {
void fn_800997E0(ResHandle* pSelf, s32 attr, u32 pData, u32 size);
void fn_800998CC(ResHandle* pSelf, u32 attr);
u32 res_shp_get_vtx_pos(ResHandle* pSelf);
u32 res_shp_get_vtx_nrm(ResHandle* pSelf);
u32 res_shp_get_vtx_clr(ResHandle* pSelf, u32 idx);
u32 fn_80099B10(ResHandle* pSelf, u32 idx);
u32 res_shp_get_vtx_fur_pos(ResHandle* pSelf);
u32 fn_80099DD4(ResHandle* pSelf);
u32 fn_80099DF8(ResHandle* pSelf);
ResHandle* fn_80099E2C(ResHandle* pSelf, const ResHandle* pRhs);
void fn_80099E5C(ResHandle* pSelf, const ResHandle* pRhs);
void fn_8009A000(u32 addr, u32 size);
u32 fn_8009A018(ResHandle* pSelf);
ResHandle* fn_8009A03C(ResHandle* pSelf, const ResHandle* pRhs);
void fn_8009A06C(ResHandle* pSelf, const ResHandle* pRhs);
u32 fn_8009A0F8(ResHandle* pSelf);
void fn_8009A12C(ResHandle* pSelf, s32 flag);
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
        nw4r::db::Panic("g3d_resshp.cpp", 0x2D4, "attr is out of bounds(%d)\n%d <= attr <= %d not satisfied.", attr, 9, 20);
    }

    u32 prePrim = fn_800996AC(pSelf);
    u8* pBase = (u8*)fn_80099640((ResHandle*)&prePrim) + (attr - 9) * 0xC;
    u8* pRecord = pBase + 0x32;
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
        nw4r::db::Panic("g3d_resshp.cpp", 0x30F, "attr is out of bounds(%d)\n%d <= attr <= %d not satisfied.", attr, 9, 20);
    }

    u32 prePrim = fn_800996AC(pSelf);
    void* pRecord = (u8*)fn_80099640((ResHandle*)&prePrim) + (attr - 9) * 0xC + 0x32;

    memset(pRecord, 0, 0xC);
}

/* 0x80099974 - the shape's vertex-position resource: resolve the owning model's position array at the
 * shape's own `idVtxPosition` (the assert the target bakes into `ref().idVtxPosition >= 0`). */
extern "C" u32 res_shp_get_vtx_pos(ResHandle* pSelf) {
    if (reinterpret_cast<const nw4r::g3d::ResShp*>(pSelf)->ref().idVtxPosition < 0) {
        nw4r::db::Panic("g3d_resshp.cpp", 0x31E, "NW4R:Failed assertion ref().idVtxPosition >= 0");
    }

    u32 mdl = fn_80099400(pSelf);
    return (u32)reinterpret_cast<nw4r::g3d::ResMdl*>(&mdl)->GetResVtxPos((int)reinterpret_cast<const nw4r::g3d::ResShp*>(pSelf)->ref().idVtxPosition).mpData;
}

/* 0x800999E8 - the vertex-normal resource, or a null handle when the shape has none (`idVtxNrm == -1`). */
extern "C" u32 res_shp_get_vtx_nrm(ResHandle* pSelf) {
    const ResShpData* pData = &reinterpret_cast<const nw4r::g3d::ResShp*>(pSelf)->ref();

    if (pData->idVtxNrm != -1) {
        u32 mdl = fn_80099400(pSelf);
        return (u32)reinterpret_cast<nw4r::g3d::ResMdl*>(&mdl)->GetResVtxNrm((int)pData->idVtxNrm).mpData;
    }

    return (u32)nw4r::g3d::ResVtxNrm((void*)NULL).mpData;
}

/* 0x80099A58 - one colour-channel resource (two per shape, hence the `idx == 0 || idx == 1` assert). */
extern "C" u32 res_shp_get_vtx_clr(ResHandle* pSelf, u32 idx) {
    const ResShpData* pData = &reinterpret_cast<const nw4r::g3d::ResShp*>(pSelf)->ref();
    bool inRange = (idx == 0 || idx == 1);

    if (!inRange) {
        nw4r::db::Panic("g3d_resshp.cpp", 0x330, "NW4R:Failed assertion idx == 0 || idx == 1");
    }

    const s16* pId = &pData->idVtxClr[idx];

    if (*pId != -1) {
        u32 mdl = fn_80099400(pSelf);
        return (u32)reinterpret_cast<nw4r::g3d::ResMdl*>(&mdl)->GetResVtxClr((int)*pId).mpData;
    }

    return (u32)nw4r::g3d::ResVtxClr((void*)NULL).mpData;
}

/* 0x80099B10 - one tex-coord resource (eight per shape, hence the `idx < 8` assert). */
extern "C" u32 fn_80099B10(ResHandle* pSelf, u32 idx) {
    if (idx >= 8) {
        nw4r::db::Panic("g3d_resshp.cpp", 0x33B, "NW4R:Failed assertion idx < 8");
    }

    const s16* pId = &reinterpret_cast<const nw4r::g3d::ResShp*>(pSelf)->ref().idVtxTexCoord[idx];

    if (*pId != -1) {
        u32 mdl = fn_80099400(pSelf);
        return (u32)reinterpret_cast<nw4r::g3d::ResMdl*>(&mdl)->GetResVtxTexCoord((int)*pId).mpData;
    }

    return (u32)nw4r::g3d::ResVtxTexCoord((void*)NULL).mpData;
}

/* 0x80099BB0 - the shape's texture, or a null handle when it has none (`idTex == -1`). */
extern "C" u32 res_shp_get_vtx_fur_pos(ResHandle* pSelf) {
    const ResShpData* pData = &reinterpret_cast<const nw4r::g3d::ResShp*>(pSelf)->ref();

    if (pData->idTex != -1) {
        u32 mdl = fn_80099400(pSelf);
        return (u32)reinterpret_cast<nw4r::g3d::ResMdl*>(&mdl)->GetResVtxFurPos((int)pData->idTex).mpData;
    }

    return (u32)nw4r::g3d::ResVtxFurPos((void*)NULL).mpData;
}

/* 0x80099C20 - rebuilds the pre-prim attribute records, one per vertex resource the shape has, then invalidates
 * the pre-prim and display-list blocks in the data cache. */
void nw4r::g3d::ResShp::Init() {
    const void* pBaseVtx;
    u8 stride;

    reinterpret_cast<nw4r::g3d::ResVtxPos*>(ResHandleTemp().set(res_shp_get_vtx_pos(reinterpret_cast<ResHandle*>(this))))
        ->GetArray(&pBaseVtx, &stride);
    fn_800997E0(reinterpret_cast<ResHandle*>(this), 9, (u32)pBaseVtx, stride);

    nw4r::g3d::ResVtxNrm nrm(reinterpret_cast<const nw4r::g3d::ResVtxNrm*>(
        ResHandleTemp().set(res_shp_get_vtx_nrm(reinterpret_cast<ResHandle*>(this)))));

    if (nrm.IsValid()) {
        nrm.GetArray(&pBaseVtx, &stride);
        fn_800997E0(reinterpret_cast<ResHandle*>(this), 10, (u32)pBaseVtx, stride);
    }

    for (u32 i = 0; i < 2; i++) {
        nw4r::g3d::ResVtxClr clr(reinterpret_cast<const nw4r::g3d::ResVtxClr*>(
            ResHandleTemp().set(res_shp_get_vtx_clr(reinterpret_cast<ResHandle*>(this), i))));

        if (clr.IsValid()) {
            clr.GetArray(&pBaseVtx, &stride);
            fn_800997E0(reinterpret_cast<ResHandle*>(this), i + 11, (u32)pBaseVtx, stride);
        }
    }

    for (u32 i = 0; i < 8; i++) {
        ResHandle tex;
        fn_80099E2C(&tex, reinterpret_cast<const ResHandle*>(ResHandleTemp().set(fn_80099B10(reinterpret_cast<ResHandle*>(this), i))));

        if (reinterpret_cast<nw4r::g3d::ResVtxTexCoord*>(&tex)->IsValid()) {
            reinterpret_cast<nw4r::g3d::ResVtxTexCoord*>(&tex)->GetArray(&pBaseVtx, &stride);
            fn_800997E0(reinterpret_cast<ResHandle*>(this), i + 13, (u32)pBaseVtx, stride);
        }
    }

    fn_8009A12C(reinterpret_cast<ResHandle*>(ResHandleTemp().set(fn_800996AC(reinterpret_cast<ResHandle*>(this)))), 0);

    /* Two copies of the shape's own display-list tag (the inlined accessor reached twice): the first
     * supplies the size, the second the address the tag resolves to. */
    nw4r::g3d::DC::StoreRangeNoSync(
        (void*)fn_800996E8(reinterpret_cast<ResHandle*>(ResHandleTemp().set(fn_80099DF8(reinterpret_cast<ResHandle*>(this))))),
        fn_80099DD4(reinterpret_cast<ResHandle*>(ResHandleTemp().set(fn_80099DF8(reinterpret_cast<ResHandle*>(this))))));
}

/* 0x80099DD4 - the display-list tag's total size (`ResTagDL::ref` then its +0x00 word). */
extern "C" u32 fn_80099DD4(ResHandle* pSelf) {
    return fn_80099514(pSelf)->mSize;
}

/* 0x80099DF8 - box the shape's own display-list tag (the ResShp data's +0x24 block). */
extern "C" u32 fn_80099DF8(ResHandle* pSelf) {
    ResShpData* pData = &reinterpret_cast<nw4r::g3d::ResShp*>(pSelf)->ref();
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
void nw4r::g3d::ResShp::Terminate() {
    fn_800998CC(reinterpret_cast<ResHandle*>(this), 9);
    fn_800998CC(reinterpret_cast<ResHandle*>(this), 10);
    fn_800998CC(reinterpret_cast<ResHandle*>(this), 11);
    fn_800998CC(reinterpret_cast<ResHandle*>(this), 12);
    fn_800998CC(reinterpret_cast<ResHandle*>(this), 13);
    fn_800998CC(reinterpret_cast<ResHandle*>(this), 14);
    fn_800998CC(reinterpret_cast<ResHandle*>(this), 15);
    fn_800998CC(reinterpret_cast<ResHandle*>(this), 16);
    fn_800998CC(reinterpret_cast<ResHandle*>(this), 17);
    fn_800998CC(reinterpret_cast<ResHandle*>(this), 18);
    fn_800998CC(reinterpret_cast<ResHandle*>(this), 19);
    fn_800998CC(reinterpret_cast<ResHandle*>(this), 20);
}

/* 0x80099F1C - draws the pre-prim block: `pSync` picks the SDK's `GXCallDisplayList` over the inline pipe
 * command, `pSkipPrePrimHeader` skips the block's 0x20-byte header. */
void nw4r::g3d::ResShp::CallPrePrimitiveDisplayList(bool bSync, bool bSkipHeader) const {
    ResHandle hTag;
    u32 tag = fn_800995A0(reinterpret_cast<ResHandle*>(const_cast<ResShp*>(this)));
    fn_8009A03C(&hTag, (const ResHandle*)&tag);

    if (bSkipHeader) {
        if (bSync) {
            u32 size = fn_8009A018(&hTag);
            GXCallDisplayList((void*)(fn_800996E8(&hTag) + 0x20), size - 0x20);
        } else {
            u32 size = fn_8009A018(&hTag);
            fn_8009A000(fn_800996E8(&hTag) + 0x20, size - 0x20);
        }
    } else {
        if (bSync) {
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
void nw4r::g3d::ResShp::CallPrimitiveDisplayList(bool bSync) const {
    ResHandle hTag;
    u32 tag = fn_8009A0F8(reinterpret_cast<ResHandle*>(const_cast<ResShp*>(this)));
    fn_8009A03C(&hTag, (const ResHandle*)&tag);

    if (bSync) {
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
    const ResShpData* pData = &reinterpret_cast<const nw4r::g3d::ResShp*>(pSelf)->ref();
    ResHandle tag;
    return (u32)fn_800995D4(&tag, (u32)&pData->mDlTag)->mpData;
}

/* 0x8009A12C - store or invalidate the pre-prim block's 0xE0 bytes in the data cache. */
extern "C" void fn_8009A12C(ResHandle* pSelf, s32 flag) {
    u32 data = fn_80099640(pSelf);

    if (flag != 0) {
        nw4r::g3d::DC::StoreRange((void*)data, 0xE0);
    } else {
        nw4r::g3d::DC::StoreRangeNoSync((void*)data, 0xE0);
    }
}
