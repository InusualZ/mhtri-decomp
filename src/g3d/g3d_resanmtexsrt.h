/* g3d/g3d_resanmtexsrt.h - `nw4r::g3d::ResFile`, a one-word `ResCommon<ResFileData>` handle whose lookups
 *   `GetResTex`/`GetResPltt` (the found `ResTex`/`ResPltt` handle as a word) `g3d/g3d_resanmtexsrt.cpp` defines. */
#ifndef MHTRI_G3D_G3D_RESANMTEXSRT_H
#define MHTRI_G3D_G3D_RESANMTEXSRT_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {

class ResFile {
public:
    u32 GetResPltt(const char* pName) const;
    u32 GetResTex(const char* pName) const;

    /* +0x0 */ void* mpData;
}; /* size: 0x4 (a one-word `ResCommon<ResFileData>` handle) */

}  // namespace g3d
}  // namespace nw4r
#endif

#endif /* MHTRI_G3D_G3D_RESANMTEXSRT_H */
