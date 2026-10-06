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

#endif /* MHTRI_G3D_G3D_SCNROOT_H */
