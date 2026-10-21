/* g3d/g3d_resanmtexsrt.h - the cross-unit declarations of `g3d/g3d_resanmtexsrt.cpp`: the resource display lists'
 *   BP-command helpers. */
#ifndef MHTRI_G3D_G3D_RESANMTEXSRT_H
#define MHTRI_G3D_G3D_RESANMTEXSRT_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {

namespace detail {

/* The BP-command helpers of the resource display lists. */
void ResReadBPCmd(const u8* pCmd, u32* pReg);
void ResWriteBPCmd(u8* pCmd, u32 reg);
void ResWriteSSMask(u8* pCmd, u32 mask);

}  // namespace detail

}  // namespace g3d
}  // namespace nw4r
#endif


#endif /* MHTRI_G3D_G3D_RESANMTEXSRT_H */
