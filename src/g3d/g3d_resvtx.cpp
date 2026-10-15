/* g3d/g3d_resvtx - nw4r g3d vertex-array resources: binding (`SetArray`), querying (`GetArray`), copying and
 *   cache-storing the `ResVtxPos`/`Nrm`/`Clr`/`TexCoord`/`FurVec`/`FurPos` blocks.
 * RANGE. .text 0x80088E24-0x800898B0 (35 functions); extab, extabindex, .data 0x8058FCE8-0x8058FDC8 (the accessor
 *   asserts' strings), .sdata 0x80791258-0x80791268 (their "ref" names).
 * FLAGS. cflags_g3d (docs/g3d.md).
 * NAMES. The classes and members are nw4r's (`g3d_resvtx.h`); the file name is a GUESS from the class names (the
 *   `.data` pool carries only the accessor header's name).
 *   res_vtx_tex_coord_ref, res_vtx_tex_coord_ref_const, res_vtx_fur_vec_ref, res_vtx_fur_pos_ref are GUESSES
 *   (dead-stripped referencers of the inline accessors; no symbols.txt row).
 * RESIDUALS. The calls to accessor copies other units keep (`fn_80069754`, `fn_800736F0`, ...) relocate against their
 *   mangled names until those rows are renamed.  flipcheck: the four dead-stripped referencers are object-only
 *   (0x10 bytes, stripped by the link).
 * SHAPES. The accessors are declared by `NW4R_G3D_RESOURCE_FUNC_DECL` (g3d/g3d_rescommon.h) and defined here as
 *   `inline` under `#pragma dont_inline on`, only for the copies this TU keeps: their strings are weak `@STRING@`
 *   objects (the inline tail, in reverse definition order: `ref() const`, `ref()`, then `GetClassName`), and
 *   each weak copy follows its first referencer - a frameless dead-stripped wrapper, as retail's were.  The
 *   member name is `__FUNCTION__`, a function-local static: `.sdata` holds the four "ref" copies in forward
 *   definition order (a `"ref"` literal is a weak `@STRING@` object and comes out reversed).
 *   ResVtxFurPos::GetData reads `toFurPosArray` into a local and declares `inRange` before `hasLayer`
 *   (retail's r6/r5/r4 allocation).
 */

#include "types.h"
#include "g3d/g3d_resvtx.h"
#include "RVLGX/GXAttr.h"
#include "g3d/g3d_cpu.h"
#include "unsplit/OS.h"

namespace nw4r {
namespace g3d {

/* Binds the position array to the GX vertex attribute. */
void ResVtxPos::SetArray() {
    if (IsValid()) {
        GXSetArray(9 /* GX_VA_POS */, GetData(), ptr()->stride);
    }
}

/* Returns the array base and stride through whichever out pointer is non-NULL. */
/* untyped: byte range */
void ResVtxPos::GetArray(const void** ppBase, u8* pStride) const {
    if (ppBase != NULL) {
        *ppBase = GetData();
    }
    if (pStride != NULL) {
        *pStride = ref().stride;
    }
}

/* Returns the vertex data, or NULL when the block has none. */
/* untyped: byte range */
const void* ResVtxPos::GetData() const {
    const ResVtxPosData& r = ref();
    return r.toVtxPosArray != 0 ? reinterpret_cast<const u8*>(&r) + r.toVtxPosArray : NULL;
}

/* Copies the whole block to `pDst` and stores it out of the data cache. */
/* untyped: byte range */
void ResVtxPos::CopyTo(void* pDst) const {
    if (IsValid()) {
        u32 size = GetSize();
        detail::Copy32ByteBlocks(pDst, ptr(), size);
        DC::StoreRange(pDst, size);
    }
}

/* Binds the normal array to the GX vertex attribute. */
void ResVtxNrm::SetArray() {
    if (IsValid()) {
        GXSetArray(10 /* GX_VA_NRM */, GetData(), ptr()->stride);
    }
}

/* Returns the array base and stride through whichever out pointer is non-NULL. */
/* untyped: byte range */
void ResVtxNrm::GetArray(const void** ppBase, u8* pStride) const {
    if (ppBase != NULL) {
        *ppBase = GetData();
    }
    if (pStride != NULL) {
        *pStride = ref().stride;
    }
}

/* Returns the vertex data, or NULL when the block has none. */
/* untyped: byte range */
const void* ResVtxNrm::GetData() const {
    const ResVtxNrmData& r = ref();
    return r.toVtxNrmArray != 0 ? reinterpret_cast<const u8*>(&r) + r.toVtxNrmArray : NULL;
}

/* Copies the whole block to `pDst` and stores it out of the data cache. */
/* untyped: byte range */
void ResVtxNrm::CopyTo(void* pDst) const {
    if (IsValid()) {
        u32 size = GetSize();
        detail::Copy32ByteBlocks(pDst, ptr(), size);
        DC::StoreRange(pDst, size);
    }
}

/* Binds the colour array to GX_VA_CLR0 or GX_VA_CLR1; any other attribute is ignored. */
void ResVtxClr::SetArray(u32 attr) {
    if (IsValid() && attr - 11 <= 1) {
        GXSetArray(attr, GetData(), ptr()->stride);
    }
}

/* Returns the array base and stride through whichever out pointer is non-NULL. */
/* untyped: byte range */
void ResVtxClr::GetArray(const void** ppBase, u8* pStride) const {
    if (ppBase != NULL) {
        *ppBase = GetData();
    }
    if (pStride != NULL) {
        *pStride = ref().stride;
    }
}

/* Returns the vertex data, or NULL when the block has none. */
/* untyped: byte range */
const void* ResVtxClr::GetData() const {
    const ResVtxClrData& r = ref();
    return r.toClrArray != 0 ? reinterpret_cast<const u8*>(&r) + r.toClrArray : NULL;
}

/* Copies the whole block to `pDst` and stores it out of the data cache. */
/* untyped: byte range */
void ResVtxClr::CopyTo(void* pDst) const {
    if (IsValid()) {
        u32 size = GetSize();
        detail::Copy32ByteBlocks(pDst, ptr(), size);
        DC::StoreRange(pDst, size);
    }
}

/* The `g3d_resvtx_ac.h` accessors this TU keeps: `inline`, called out of line (`dont_inline` from here on). */
#pragma dont_inline on

inline const ResVtxTexCoordData& ResVtxTexCoord::ref() const {
    if (!IsValid()) nw4r::db::Panic("g3d_resvtx_ac.h", 211, "%s::%s: Object not valid.", GetClassName(), __FUNCTION__);
    return *ptr();
}
inline ResVtxTexCoordData& ResVtxTexCoord::ref() {
    if (!IsValid()) nw4r::db::Panic("g3d_resvtx_ac.h", 211, "%s::%s: Object not valid.", GetClassName(), __FUNCTION__);
    return *ptr();
}
inline ResVtxTexCoordData* ResVtxTexCoord::ptr() { return mpData; }
inline bool ResVtxTexCoord::IsValid() const { return mpData != NULL; }
inline const ResVtxTexCoordData* ResVtxTexCoord::ptr() const { return mpData; }
inline const char* ResVtxTexCoord::GetClassName() { return "ResVtxTexCoord"; }

/* Returns the texture-coordinate block's data; dead-stripped, it references the inline accessor. */
ResVtxTexCoordData& res_vtx_tex_coord_ref(ResVtxTexCoord* pSelf) {
    return pSelf->ref();
}

/* Returns the texture-coordinate block's data; dead-stripped, it references the inline accessor. */
const ResVtxTexCoordData& res_vtx_tex_coord_ref_const(const ResVtxTexCoord* pSelf) {
    return pSelf->ref();
}

/* Returns the array base and stride through whichever out pointer is non-NULL. */
/* untyped: byte range */
void ResVtxTexCoord::GetArray(const void** ppBase, u8* pStride) const {
    if (ppBase != NULL) {
        *ppBase = GetData();
    }
    if (pStride != NULL) {
        *pStride = ref().stride;
    }
}

/* Returns the vertex data, or NULL when the block has none. */
/* untyped: byte range */
const void* ResVtxTexCoord::GetData() const {
    const ResVtxTexCoordData& r = ref();
    return r.toTexCoordArray != 0 ? reinterpret_cast<const u8*>(&r) + r.toTexCoordArray : NULL;
}

inline ResVtxFurVecData& ResVtxFurVec::ref() {
    if (!IsValid()) nw4r::db::Panic("g3d_resvtx_ac.h", 270, "%s::%s: Object not valid.", GetClassName(), __FUNCTION__);
    return *ptr();
}
inline ResVtxFurVecData* ResVtxFurVec::ptr() { return mpData; }
inline const char* ResVtxFurVec::GetClassName() { return "ResVtxFurVec"; }
inline bool ResVtxFurVec::IsValid() const { return mpData != NULL; }

/* Returns the fur-vector block's data; dead-stripped, it references the inline accessor. */
ResVtxFurVecData& res_vtx_fur_vec_ref(ResVtxFurVec* pSelf) {
    return pSelf->ref();
}

inline ResVtxFurPosData& ResVtxFurPos::ref() {
    if (!IsValid()) nw4r::db::Panic("g3d_resvtx_ac.h", 318, "%s::%s: Object not valid.", GetClassName(), __FUNCTION__);
    return *ptr();
}
inline ResVtxFurPosData* ResVtxFurPos::ptr() { return mpData; }


/* Returns the fur-position block's data; dead-stripped, it references the inline accessor. */
ResVtxFurPosData& res_vtx_fur_pos_ref(ResVtxFurPos* pSelf) {
    return pSelf->ref();
}

/* Returns fur layer `layer`'s position array, or NULL when the block has none or the layer is out of range. */
/* untyped: byte range */
void* ResVtxFurPos::GetData(u16 layer) {
    ResVtxFurPosData& r = ref();
    bool inRange = false;
    bool hasLayer = false;
    u32 ofs = r.toFurPosArray;
    if (ofs != 0 && (s32)layer < (s32)r.numLayer) {
        hasLayer = true;
    }
    if (hasLayer && (s32)layer >= 0) {
        inRange = true;
    }
    if (inRange) {
        return reinterpret_cast<u8*>(&r) + ofs + layer * r.ofsLayer;
    }
    return NULL;
}

/* Binds fur layer `layer`'s position array to GX_VA_POS. */
void ResVtxFurPos::SetArray(u32 layer) {
    if (IsValid()) {
        void* pData = GetData(layer);
        GXSetArray(9 /* GX_VA_POS */, pData, ptr()->stride);
    }
}

/* Stores the block out of the data cache, waiting for the store when `sync` is set. */
void ResVtxPos::DCStore(bool sync) {
    void* p = &ref();
    u32 size = ref().size;
    if (sync) {
        DC::StoreRange(p, size);
    } else {
        DC::StoreRangeNoSync(p, size);
    }
}

/* Stores a range out of the data cache without waiting. */
/* untyped: byte range */
void DC::StoreRangeNoSync(void* pStart, u32 size) {
    DCStoreRangeNoSync(pStart, size);
}

/* Stores the block out of the data cache, waiting for the store when `sync` is set. */
void ResVtxNrm::DCStore(bool sync) {
    void* p = &ref();
    u32 size = ref().size;
    if (sync) {
        DC::StoreRange(p, size);
    } else {
        DC::StoreRangeNoSync(p, size);
    }
}

/* Stores the block out of the data cache, waiting for the store when `sync` is set. */
void ResVtxClr::DCStore(bool sync) {
    void* p = &ref();
    u32 size = ref().size;
    if (sync) {
        DC::StoreRange(p, size);
    } else {
        DC::StoreRangeNoSync(p, size);
    }
}

/* Stores the block out of the data cache, waiting for the store when `sync` is set. */
void ResVtxTexCoord::DCStore(bool sync) {
    void* p = &ref();
    u32 size = ref().size;
    if (sync) {
        DC::StoreRange(p, size);
    } else {
        DC::StoreRangeNoSync(p, size);
    }
}

/* Stores the block out of the data cache, waiting for the store when `sync` is set. */
void ResVtxFurVec::DCStore(bool sync) {
    void* p = &ref();
    u32 size = ref().size;
    if (sync) {
        DC::StoreRange(p, size);
    } else {
        DC::StoreRangeNoSync(p, size);
    }
}

/* Stores the block out of the data cache, waiting for the store when `sync` is set. */
void ResVtxFurPos::DCStore(bool sync) {
    void* p = &ref();
    u32 size = ref().size;
    if (sync) {
        DC::StoreRange(p, size);
    } else {
        DC::StoreRangeNoSync(p, size);
    }
}

}  // namespace g3d
}  // namespace nw4r
