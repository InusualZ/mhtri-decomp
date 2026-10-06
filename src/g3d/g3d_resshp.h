/* g3d/g3d_resshp.h - nw4r g3d's shape, tev, texture and palette resource handles whose members the
 *   `g3d/g3d_resshp.cpp` range defines (`ResShp::Init`/`Terminate`, the `ResTev`/`ResTex` helpers). */
#ifndef MHTRI_G3D_G3D_RESSHP_H
#define MHTRI_G3D_G3D_RESSHP_H

#include "types.h"
#include "g3d/g3d_rescommon.h"

namespace nw4r {
namespace g3d {

/* The shape block.  size: 0x64 (a lower bound) */
struct ResShpData {
    /* +0x00 */ u32 size;
};

/* The tev block.  size: 0x20 (a lower bound) */
struct ResTevData {
    /* +0x00 */ u32 size;
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
};

/* size: 0x4 */
class ResTev : public ResCommon<ResTevData> {
public:
    /* untyped: opaque handle */
    explicit ResTev(void* pData);
    ResTev& operator=(const ResTev& rhs);
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

#endif /* MHTRI_G3D_G3D_RESSHP_H */
