/* g3d/g3d_resvtx.h - nw4r g3d's vertex-array resources (`ResVtxPos`, `ResVtxNrm`, `ResVtxClr`, `ResVtxTexCoord`,
 *   `ResVtxFurVec`, `ResVtxFurPos`) that `g3d/g3d_resvtx.cpp` owns, and the data-cache wrappers they store with. */
#ifndef MHTRI_G3D_G3D_RESVTX_H
#define MHTRI_G3D_G3D_RESVTX_H

#include "types.h"
#include "nw4r/math.h"
#include "g3d/g3d_rescommon.h"

namespace nw4r {
namespace g3d {

/* The data-cache wrappers the resources store their blocks through (a tail call each). */
namespace DC {
void StoreRange(void* pStart, u32 size);       /* untyped: byte range */
void StoreRangeNoSync(void* pStart, u32 size); /* untyped: byte range */
}  // namespace DC

/* The vertex-position array block.  size: 0x38 */
struct ResVtxPosData {
    /* +0x00 */ u32 size;           /* the whole block, stored and copied as one range */
    /* +0x04 */ s32 toResMdlData;   /* back offset to the owning model */
    /* +0x08 */ s32 toVtxPosArray;  /* offset to the vertex data, 0 when absent */
    /* +0x0C */ s32 name;
    /* +0x10 */ u32 id;
    /* +0x14 */ u32 cmpcnt;         /* GXCompCnt */
    /* +0x18 */ u32 tp;             /* GXCompType */
    /* +0x1C */ u8 frac;
    /* +0x1D */ u8 stride;          /* the GXSetArray stride */
    /* +0x1E */ u16 numPos;
    /* +0x20 */ math::VEC3 min;
    /* +0x2C */ math::VEC3 max;
};

/* The vertex-normal array block.  size: 0x20 */
struct ResVtxNrmData {
    /* +0x00 */ u32 size;
    /* +0x04 */ s32 toResMdlData;
    /* +0x08 */ s32 toVtxNrmArray;
    /* +0x0C */ s32 name;
    /* +0x10 */ u32 id;
    /* +0x14 */ u32 cmpcnt;
    /* +0x18 */ u32 tp;
    /* +0x1C */ u8 frac;
    /* +0x1D */ u8 stride;
    /* +0x1E */ u16 numNrm;
};

/* The vertex-colour array block.  size: 0x20 */
struct ResVtxClrData {
    /* +0x00 */ u32 size;
    /* +0x04 */ s32 toResMdlData;
    /* +0x08 */ s32 toClrArray;
    /* +0x0C */ s32 name;
    /* +0x10 */ u32 id;
    /* +0x14 */ u32 cmpcnt;
    /* +0x18 */ u32 tp;
    /* +0x1C */ u8 stride;
    /* +0x1D */ u8 pad_0x1D;
    /* +0x1E */ u16 numClr;
};

/* The texture-coordinate array block.  size: 0x30 */
struct ResVtxTexCoordData {
    /* +0x00 */ u32 size;
    /* +0x04 */ s32 toResMdlData;
    /* +0x08 */ s32 toTexCoordArray;
    /* +0x0C */ s32 name;
    /* +0x10 */ u32 id;
    /* +0x14 */ u32 cmpcnt;
    /* +0x18 */ u32 tp;
    /* +0x1C */ u8 frac;
    /* +0x1D */ u8 stride;
    /* +0x1E */ u16 numTexCoord;
    /* +0x20 */ f32 min[2];
    /* +0x28 */ f32 max[2];
};

/* The fur-direction array block.  size: 0x20 */
struct ResVtxFurVecData {
    /* +0x00 */ u32 size;
    /* +0x04 */ s32 toResMdlData;
    /* +0x08 */ s32 toFurVecArray;
    /* +0x0C */ s32 name;
    /* +0x10 */ u32 id;
    /* +0x14 */ u8 pad_0x14[0x1E - 0x14];
    /* +0x1E */ u16 numFurVec;
};

/* The fur-layer position arrays block: `numLayer` arrays of `ofsLayer` bytes each.  size: 0x28 */
struct ResVtxFurPosData {
    /* +0x00 */ u32 size;
    /* +0x04 */ s32 toResMdlData;
    /* +0x08 */ s32 toFurPosArray;
    /* +0x0C */ s32 name;
    /* +0x10 */ u32 id;
    /* +0x14 */ u32 cmpcnt;
    /* +0x18 */ u32 tp;
    /* +0x1C */ u8 frac;
    /* +0x1D */ u8 stride;
    /* +0x1E */ u16 numFurPos;
    /* +0x20 */ u32 numLayer;
    /* +0x24 */ u32 ofsLayer;
};

/* size: 0x4 */
class ResVtxPos : public ResCommon<ResVtxPosData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResVtxPos)
    explicit ResVtxPos(const ResVtxPos* pRhs);
    ResVtxPos() {}

    void Init();
    u32 GetID() const;
    u16 GetNumVtxPos() const;

    void SetArray();
    void GetArray(const void** ppBase, u8* pStride) const; /* untyped: byte range */
    void CopyTo(void* pDst) const; /* untyped: byte range */
    void DCStore(bool sync);

    u32 GetSize() const;
    void* GetData(); /* untyped: byte range */
    const void* GetData() const; /* untyped: byte range */
};

/* size: 0x4 */
class ResVtxNrm : public ResCommon<ResVtxNrmData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResVtxNrm)
    explicit ResVtxNrm(const ResVtxNrm* pRhs);

    void Init();
    u32 GetID() const;
    u16 GetNumVtxNrm() const;

    void SetArray();
    void GetArray(const void** ppBase, u8* pStride) const; /* untyped: byte range */
    void CopyTo(void* pDst) const; /* untyped: byte range */
    void DCStore(bool sync);

    u32 GetSize() const;
    void* GetData(); /* untyped: byte range */
    const void* GetData() const; /* untyped: byte range */
};

/* size: 0x4 */
class ResVtxClr : public ResCommon<ResVtxClrData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResVtxClr)
    explicit ResVtxClr(const ResVtxClr* pRhs);

    void Init();
    u32 GetID() const;
    u16 GetNumVtxClr() const;

    void SetArray(u32 attr);
    void GetArray(const void** ppBase, u8* pStride) const; /* untyped: byte range */
    void CopyTo(void* pDst) const; /* untyped: byte range */
    void DCStore(bool sync);

    u32 GetSize() const;
    void* GetData(); /* untyped: byte range */
    const void* GetData() const; /* untyped: byte range */
};

/* size: 0x4 */
class ResVtxTexCoord : public ResCommon<ResVtxTexCoordData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResVtxTexCoord)

    void Init();

    void GetArray(const void** ppBase, u8* pStride) const; /* untyped: byte range */
    void DCStore(bool sync);

    const void* GetData() const; /* untyped: byte range */
};

/* size: 0x4 */
class ResVtxFurVec : public ResCommon<ResVtxFurVecData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResVtxFurVec)

    void Init();

    void DCStore(bool sync);
};

/* size: 0x4 */
class ResVtxFurPos : public ResCommon<ResVtxFurPosData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResVtxFurPos)

    void Init();

    void* GetData(u16 layer); /* untyped: byte range */
    void SetArray(u32 layer);
    void DCStore(bool sync);
};

}  // namespace g3d
}  // namespace nw4r

#endif /* MHTRI_G3D_G3D_RESVTX_H */
