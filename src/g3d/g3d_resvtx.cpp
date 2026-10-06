/* g3d/g3d_resvtx - nw4r g3d vertex-array resources: binding (`SetArray`), querying (`GetArray`), copying and
 *   cache-storing the `ResVtxPos`/`Nrm`/`Clr`/`TexCoord`/`FurVec`/`FurPos` blocks.
 * RANGE. .text 0x80088E24-0x800898B0 (35 functions); extab, extabindex, .data 0x8058FCE8-0x8058FDC8 (the accessor
 *   asserts' strings), .sdata 0x80791258-0x80791268 (their "ref" names).
 * FLAGS. cflags_g3d (docs/g3d.md).
 * NAMES. The classes and members are nw4r's (`g3d_resvtx.h`); the file name is a GUESS from the class names (the
 *   `.data` pool carries only the accessor header's name).
 * RESIDUALS. ResVtxFurPos::GetData: the two materialised bools take r6/r5 where retail takes r5/r6.
 *   The calls to accessor copies other units keep (`fn_80069754`, `fn_800736F0`, ...) relocate against their
 *   mangled names until those rows are renamed. flipcheck: `.data`/`.sdata` hold the accessor strings in their
 *   own order, not measured.
 * SHAPES. the accessors are declared by `NW4R_G3D_RESOURCE_FUNC_DECL` (g3d/g3d_rescommon.h) and defined here, in
 *   retail's order, only for the copies this TU keeps; every accessor call stays out of line.
 */

#include "types.h"
#include "g3d/g3d_resvtx.h"
#include "EXI/ProbeBarnacle.h"
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

NW4R_G3D_RESOURCE_REF(ResVtxTexCoord, "g3d_resvtx_ac.h", 211)
NW4R_G3D_RESOURCE_PTR(ResVtxTexCoord)
NW4R_G3D_RESOURCE_CLASS_NAME(ResVtxTexCoord)
NW4R_G3D_RESOURCE_IS_VALID(ResVtxTexCoord)
NW4R_G3D_RESOURCE_REF_CONST(ResVtxTexCoord, "g3d_resvtx_ac.h", 211)
NW4R_G3D_RESOURCE_PTR_CONST(ResVtxTexCoord)

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

NW4R_G3D_RESOURCE_REF(ResVtxFurVec, "g3d_resvtx_ac.h", 270)
NW4R_G3D_RESOURCE_PTR(ResVtxFurVec)
NW4R_G3D_RESOURCE_CLASS_NAME(ResVtxFurVec)
NW4R_G3D_RESOURCE_IS_VALID(ResVtxFurVec)

NW4R_G3D_RESOURCE_REF(ResVtxFurPos, "g3d_resvtx_ac.h", 318)
NW4R_G3D_RESOURCE_PTR(ResVtxFurPos)

/* Returns fur layer `layer`'s position array, or NULL when the block has none or the layer is out of range. */
/* untyped: byte range */
void* ResVtxFurPos::GetData(u16 layer) {
    ResVtxFurPosData& r = ref();
    bool hasLayer = false;
    bool inRange = false;
    if (r.toFurPosArray != 0 && (s32)layer < (s32)r.numLayer) {
        hasLayer = true;
    }
    if (hasLayer && (s32)layer >= 0) {
        inRange = true;
    }
    if (inRange) {
        return reinterpret_cast<u8*>(&r) + r.toFurPosArray + layer * r.ofsLayer;
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
