/* g3d/g3d_fog.h - the declarations `g3d/g3d_fog.cpp` owns: nw4r g3d's fog record and handle, and the C entry points
 *   its consumers call by their map names. */
#ifndef MHTRI_G3D_G3D_FOG_H
#define MHTRI_G3D_G3D_FOG_H

#include "types.h"
#include "nw4r/math.h" /* nw4r::math::MTX44 */

#ifdef __cplusplus
#include "gx.h"
#include "nw4r/math.h"
#include "RVLGX/GXPixel.h"

namespace nw4r {
namespace g3d {

/* One fog: the GX fog type, range, clip planes and colour, and the range adjustment.  size: 0x30 */
struct FogData {
    /* +0x00 */ u32 type;               /* GXFogType; 0 = no fog */
    /* +0x04 */ f32 startz;
    /* +0x08 */ f32 endz;
    /* +0x0C */ f32 Near;
    /* +0x10 */ f32 Far;
    /* +0x14 */ GXColor color;
    /* +0x18 */ u8 adjEnable;
    /* +0x19 */ u8 pad_0x19;
    /* +0x1A */ u16 adjCenter;
    /* +0x1C */ GXFogAdjTable adjTable;
};

/* A one-word handle on a FogData.  size: 0x4 */
class Fog {
public:
    /* untyped: byte range - the copy's destination */
    Fog CopyTo(void* pDst) const;
    const FogData* ptr() const;
    void SetFogRangeAdjParam(u16 width, u16 center, const math::MTX44& projMtx);
    void SetGP() const;

    /* +0x00 */ FogData* mpFogData;
};

}  // namespace g3d
}  // namespace nw4r
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80079FB4 (0x30): constructs a fog handle at `pSelf` on the record `pData` and returns `pSelf`. */
/* untyped: opaque handle - the FogData record */
s32 fog_ctor(s32 pSelf, void* pData);
/* 0x80079FEC (0xC8): resets the fog the handle at `pSelf` names. */
void fog_init(s32 pSelf);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_FOG_H */
