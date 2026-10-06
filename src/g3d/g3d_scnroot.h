/* g3d/g3d_scnroot.h - `nw4r::g3d::ScnRoot`, declaring only the two camera methods its consumers call; with no
 *   virtuals MWCC emits nothing for the class. */
#ifndef MHTRI_G3D_G3D_SCNROOT_H
#define MHTRI_G3D_G3D_SCNROOT_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {
/* size: 0x4 (approximation: declared only for the two camera methods its consumers call) */
class ScnRoot {
public:
    int GetCamera(int index);
    void SetCurrentCamera(int camera);
};
} /* namespace g3d */
} /* namespace nw4r */
#endif

#ifdef __cplusplus
extern "C" {
#endif

void VEC2_ctor(void* p); /* 0x800834F0 - constructs one 8-byte sub-object (ef_particle's parameter record) */
u16 fn_80082F18(f32 value); /* 0x80082F18 - the frame-round helper (callers: g3d_resanm.c, g3d_resanmchr.cpp) */
s32 fn_80082BCC(s32 model); /* 0x80082BCC - the camera handle lookup (callers: eft019.cpp, em_effect_ctrl.cpp) */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_G3D_G3D_SCNROOT_H */
