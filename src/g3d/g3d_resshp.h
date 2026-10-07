/* g3d/g3d_resshp.h - nw4r g3d's shape, tev, texture and palette resource handles whose members the
 *   `g3d/g3d_resshp.cpp` range defines (`ResShp::Init`/`Terminate`, the `ResTev`/`ResTex` helpers). */
#ifndef MHTRI_G3D_G3D_RESSHP_H
#define MHTRI_G3D_G3D_RESSHP_H

#include "types.h"
#include "g3d/g3d_rescommon.h"

namespace nw4r {
namespace g3d {

/* The `ResTagDL` block embedded in the `ResShp` data at +0x18 and +0x24.  +0x08 is the offset from the tag's
 * own address to the display list it marks.  size: 0xC */
struct ResTagDLData {
    /* +0x00 */ u32 mSize;       /* the marked block's total size */
    /* +0x04 */ u32 mDlSize;     /* the display-list size */
    /* +0x08 */ u32 mPrePrimOfs; /* the offset from this tag to the block it marks */
};

/* The shape block: the model offset, the current matrix index (bit 31: several matrices), the vertex
 * description the pre-primitive list loads, the two display-list tags, the vertex attribute bitmap, the flags
 * and the per-vertex-resource ids (-1 = none).  size: 0x64 (a lower bound: only the fields below are reached) */
struct ResShpData {
    /* +0x00 */ u32 size;
    /* +0x04 */ u32 mToResMdlData;
    /* +0x08 */ s32 curMtxIdx;
    /* +0x0C */ u32 vtxDesc[3];
    /* +0x18 */ ResTagDLData mTag;            /* the pre-primitive tag */
    /* +0x24 */ ResTagDLData mDlTag;          /* the primitive display-list tag */
    /* +0x30 */ u32 vtxAttrFlags;             /* bit n: GX vertex attribute n is present */
    /* +0x34 */ u32 flag;                     /* bit 1: invisible */
    /* +0x38 */ u8 pad_0x38[0x10];
    /* +0x48 */ s16 idVtxPosition;            /* vertex-position id (asserted `>= 0`) */
    /* +0x4A */ s16 idVtxNrm;
    /* +0x4C */ s16 idVtxClr[2];
    /* +0x50 */ s16 idVtxTexCoord[8];
    /* +0x60 */ u8 pad_0x60[0x2];
    /* +0x62 */ s16 idTex;
};

/* The tev block.  size: 0x20 (a lower bound) */
struct ResTevData {
    /* +0x00 */ u32 size;
    /* +0x04 */ u8 pad_0x04[0xC];
    /* +0x10 */ u8 texMapID[8];   /* the texture map each texture coordinate samples, 0xFF for none */
};

/* The texture block.  size: 0x30 (a lower bound) */
struct ResTexData {
    /* +0x00 */ u32 signature;
};

/* The palette block.  size: 0x20 (a lower bound) */
struct ResPlttData {
    /* +0x00 */ u32 signature;
    /* +0x04 */ u8 pad_0x04[0x10 - 0x4];
    /* +0x10 */ s32 toPlttData;   /* offset to the colour data, 0 when absent */
};

/* size: 0x4 */
class ResShp : public ResCommon<ResShpData> {
public:
    /* untyped: opaque handle */
    explicit ResShp(void* pData);
    void Init();
    void Terminate();
    static const char* GetClassName();
    bool IsValid() const;
    ResShpData* ptr();
    const ResShpData* ptr() const;
    ResShpData& ref();
    const ResShpData& ref() const;
    bool IsVtxAttrEnabled(u32 attr) const;
    void CallPrePrimitiveDisplayList(bool bSync, bool bSkipHeader) const;
    void CallPrimitiveDisplayList(bool bSync) const;
};

/* size: 0x4 */
class ResTev : public ResCommon<ResTevData> {
public:
    /* untyped: opaque handle */
    explicit ResTev(void* pData);
    ResTev& operator=(const ResTev& rhs);
    static const char* GetClassName();
    bool IsValid() const;
    const ResTevData* ptr() const;
    const ResTevData& ref() const;
    void CallDisplayList(bool bSync) const;
};

/* size: 0x4 */
class ResTex : public ResCommon<ResTexData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResTex)

    bool IsCIFmt() const;
    u16 GetWidth() const;
    u16 GetHeight() const;
    const void* GetTexData() const; /* untyped: byte range */
    bool GetTexObjParam(void** ppTexData, u16* pWidth, u16* pHeight, u32* pFormat, f32* pMinLod, f32* pMaxLod,
                        u8* pMipmap) const; /* untyped: byte range */
    bool GetTexObjCIParam(void** ppTexData, u16* pWidth, u16* pHeight, u32* pFormatCI, f32* pMinLod,
                          f32* pMaxLod, u8* pMipmap) const; /* untyped: byte range */
};

/* size: 0x4 */
class ResPltt : public ResCommon<ResPlttData> {
public:
    NW4R_G3D_RESOURCE_FUNC_DECL(ResPltt)

    u16 GetNumEntries() const;
    u32 GetFmt() const;
    void* GetPlttData() const; /* untyped: byte range */
    void* GetPlttData(); /* untyped: byte range */
};

}  // namespace g3d
}  // namespace nw4r

extern "C" {
/* The shape's vertex resources (a null handle when the shape has none) and the tev block's copy and range
 * store; each takes the resource's one-word handle. */
u32 res_shp_get_vtx_pos(struct ResHandle* pSelf);
u32 res_shp_get_vtx_nrm(struct ResHandle* pSelf);
u32 res_shp_get_vtx_clr(struct ResHandle* pSelf, u32 idx);
u32 res_tev_copy_to(struct ResHandle* pSelf, void* pDst); /* untyped: byte range */
void res_tev_dc_store(struct ResHandle* pSelf, s32 flag);
}

#endif /* MHTRI_G3D_G3D_RESSHP_H */
