/*
 * g3d/g3d_restex.cpp - nw4r g3d `ResTex`/`ResPltt` resources: the palette's data-cache store and revision
 *   check, the texture's checked handle, the two tex-object parameter readers, the image address, the
 *   revision check and the main-memory flush (GUESS: the file name is the `g3d_restex_ac.h` assert's stem
 *   and nw4r's own g3d_restex.cpp, which holds both classes; the dump answers `zz_` placeholders for the
 *   `fn_` rows).
 * RANGE. .text 0x8009A360-0x8009A748 (10 functions); extab 0x80009990-0x800099E0, extabindex
 *   0x80022A34-0x80022AAC, .data 0x80591830-0x80591860, .sdata 0x807912D0-0x807912D8.  Cut from
 *   `g3d/g3d_resshp.cpp` (seam request flip-resshp#1): the extabindex record at 0x80022A34 names
 *   0x8009A360 (extab 0x80009990) and the next unit's first record at 0x80022AAC names
 *   `g3d/g3d_cpu.cpp`'s 0x8009A748; the `.data` strings start 8-aligned at 0x80591830 after the ResTev
 *   strings and the `.sdata` "ref" static sits in its own 8-aligned fragment at 0x807912D0.
 * NAMES. ResTex::ref/GetTexObjParam/GetTexObjCIParam/GetTexData are named by the retail `_ac.h` accessor
 *   set.  res_pltt_check_revision and res_tex_check_revision are GUESSES (`ResFile::CheckRevision`,
 *   0x80093B0C, calls one per palette and one per texture, and each accepts the block's +0x08 word at 1 or
 *   3: the PLT0/TEX0 block revisions); res_pltt_get_revision and res_tex_get_revision are GUESSES (the
 *   +0x08 readers they call); res_pltt_dc_store is a GUESS (res_tev_dc_store's twin through
 *   `ResPltt::ref`, called by `ResFile`'s per-palette release at 0x80093A98); res_tex_dc_flush is a GUESS
 *   (`ResFile::Init` flushes every texture block's `mSize` bytes through it).  The `ResTexBlock` fields
 *   are GUESSes (the offsets are the facts).
 * RESIDUALS. The one-call stub res_tex_ref is 4 B of `.text` the linker dead-strips.
 * SHAPES. File-scope `#pragma peephole off` and `#pragma pool_data off` as in `g3d/g3d_resshp.cpp` (the
 *   TU's own strings are literals).  `ResTex::ref` is a `__declspec(noinline) inline` weak copy placed
 *   after the stub that references it (MWCC emits the copy after the first referencing function and
 *   creates its string literals at the definition).
 */
#include "types.h"
#include "nw4r/math.h"
#include "nw4r/g3d/res_common.h"
#include "g3d/g3d_cpu.h"
#include "g3d/g3d_calcview.h"
#include "g3d/g3d_resvtx.h"      /* nw4r::g3d::DC (rule 2) */
#include "g3d/g3d_resshp.h"
#include "g3d/g3d_restex.h"

#pragma pool_data off

/* The target's `-O3` schedule is retail only with the peephole pass off. */
#pragma peephole off
#pragma fp_contract off

/* nw4r::db::Panic(const char*, int, const char*, ...) - the owner's real C++ declaration, called
 * through its signature, never the mangled spelling (docs/plan.md 6.5 rule 9). */
namespace nw4r {
namespace db {

void Panic(const char* pFile, int line, const char* pFmt, ...);

}  // namespace db
}  // namespace nw4r

/* The `ResTex` resource block the unit caches (`g3d_restex_ac.h`, the file name of its assert).  Only the
 * words the bodies reach are named; the names are the best the offsets support (the offsets are the facts). */
struct ResTexBlock {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 mSize;      /* the block's size: the DCFlushRangeNoSync range        */
    /* +0x08 */ u32 mRevision;  /* the block revision, 1 or 3 (res_tex_check_revision)   */
    /* +0x0C */ u8 pad_0x0C[0x4];
    /* +0x10 */ u32 mToImage;   /* offset from the block to the image data              */
    /* +0x14 */ u8 pad_0x14[0x4];
    /* +0x18 */ u32 mFlags;     /* bit 0 gates GetTexObjParam/GetTexObjCIParam          */
    /* +0x1C */ u16 mWidth;     /* `ResTex::GetWidth`                                  */
    /* +0x1E */ u16 mHeight;    /* `ResTex::GetHeight`                                 */
    /* +0x20 */ u32 mImageSize; /* GetTexObjParam's third out                          */
    /* +0x24 */ u32 mParam;     /* `>= 2` becomes GetTexObjParam's last out             */
    /* +0x28 */ f32 mLodMin;    /* GetTexObjParam's fourth out                         */
    /* +0x2C */ f32 mLodMax;    /* GetTexObjParam's fifth out                          */
}; /* size: 0x30 (a lower bound: only the words above are reached) */

/* The neighbour this unit calls (plain map stem), its owner named beside it. */
extern "C" {
void g3d_dc_flush_range_nosync(void* pBase, u32 size);          /* owner: g3d/g3d_calcview.cpp           */
}

/* 0x8009A42C (0x64): `ResTex::ref` with the `g3d_restex_ac.h` validity assert. */
__declspec(noinline) inline nw4r::g3d::ResTexData& nw4r::g3d::ResTex::ref() {
    if (!IsValid()) {
        nw4r::db::Panic("g3d_restex_ac.h", 0x26, "%s::%s: Object not valid.", GetClassName(), __FUNCTION__);
    }
    return *ptr();
}

/* 0x8009A360 - store or invalidate the palette block's `mSize` bytes (res_tev_dc_store's twin through
 * `ResPltt::ref`). */
extern "C" void res_pltt_dc_store(ResHandle* pSelf, s32 flag) {
    u32 base = (u32)&reinterpret_cast<nw4r::g3d::ResPltt*>(pSelf)->ref();
    u32 size = *(u32*)((u32)&reinterpret_cast<nw4r::g3d::ResPltt*>(pSelf)->ref() + 0x4);

    if (flag != 0) {
        nw4r::g3d::DC::StoreRange((void*)base, size);
    } else {
        nw4r::g3d::DC::StoreRangeNoSync((void*)base, size);
    }
}

/* 0x8009A3CC - whether the palette block's revision is one the library reads (1 or 3). */
extern "C" u32 res_pltt_check_revision(ResHandle* pSelf) {
    u32 revision = res_pltt_get_revision(pSelf);
    bool result = (revision == 3 || revision == 1);
    return result;
}

/* 0x8009A408 - the palette block's revision word (its +0x08). */
extern "C" u32 res_pltt_get_revision(ResHandle* pSelf) {
    return *(u32*)((u32)&reinterpret_cast<const nw4r::g3d::ResPltt*>(pSelf)->ref() + 0x8);
}

/* The texture block's checked handle; dead-stripped, it places the weak copy of `ResTex::ref` here. */
extern "C" nw4r::g3d::ResTexData* res_tex_ref(ResHandle* pSelf) {
    return &reinterpret_cast<nw4r::g3d::ResTex*>(pSelf)->ref();
}

/* 0x8009A490 - the block's resource description: its flags must say the description is readable, and
 * each non-null out-parameter takes one field.  `0` is the refusal, `1` the success. */
/* untyped: byte range - the image the texture block points at */
bool nw4r::g3d::ResTex::GetTexObjParam(void** pOutVtxData, u16* pOutWidth, u16* pOutHeight, u32* pOutImageSize,
                                       f32* pOutLodMin, f32* pOutLodMax, u8* pOutFlag) const {
    const ResTexBlock* pData = reinterpret_cast<const ResTexBlock*>(&ref());

    if ((pData->mFlags & 1) != 0) {
        return 0;
    }

    if (pOutVtxData != NULL) {
        *pOutVtxData = const_cast<void*>(GetTexData());
    }
    if (pOutWidth != NULL) {
        *pOutWidth = GetWidth();
    }
    if (pOutHeight != NULL) {
        *pOutHeight = GetHeight();
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
/* untyped: byte range - the image the texture block points at */
const void* nw4r::g3d::ResTex::GetTexData() const {
    const ResTexBlock* pData = reinterpret_cast<const ResTexBlock*>(&ref());
    s32 ofs = pData->mToImage;

    if (ofs != 0) {
        pData = (const ResTexBlock*)((const u8*)pData + ofs);
    } else {
        pData = NULL;
    }
    return pData;
}

/* 0x8009A5C4 - GetTexObjParam's twin with the flag test inverted (the second `_ac.h` copy: same body,
 * `ref` reached through the other instantiation). */
/* untyped: byte range - the image the texture block points at */
bool nw4r::g3d::ResTex::GetTexObjCIParam(void** pOutVtxData, u16* pOutWidth, u16* pOutHeight, u32* pOutImageSize,
                                       f32* pOutLodMin, f32* pOutLodMax, u8* pOutFlag) const {
    const ResTexBlock* pData = reinterpret_cast<const ResTexBlock*>(&ref());

    if ((pData->mFlags & 1) == 0) {
        return 0;
    }

    if (pOutVtxData != NULL) {
        *pOutVtxData = const_cast<void*>(GetTexData());
    }
    if (pOutWidth != NULL) {
        *pOutWidth = GetWidth();
    }
    if (pOutHeight != NULL) {
        *pOutHeight = GetHeight();
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

/* 0x8009A6C0 - whether the texture block's revision is one the library reads (1 or 3). */
extern "C" u32 res_tex_check_revision(ResHandle* pSelf) {
    u32 revision = res_tex_get_revision(pSelf);
    bool result = (revision == 3 || revision == 1);
    return result;
}

/* 0x8009A6FC - the texture block's revision word (its +0x08) through the const `ResTex::ref`. */
extern "C" u32 res_tex_get_revision(ResHandle* pSelf) {
    return *(u32*)((u32)&reinterpret_cast<const nw4r::g3d::ResTex*>(pSelf)->ref() + 0x8);
}

/* 0x8009A720 - write the texture block's `mSize` bytes at its own address out to main memory (the
 * `ResTex` flush `ResFile::Init` ends with; g3d_dc_flush_range_nosync wraps the SDK's `DCFlushRangeNoSync`). */
extern "C" void res_tex_dc_flush(ResHandle* pSelf) {
    ResTexBlock* pData = reinterpret_cast<ResTexBlock*>(&reinterpret_cast<nw4r::g3d::ResTex*>(pSelf)->ref());
    g3d_dc_flush_range_nosync(pData, pData->mSize);
}
