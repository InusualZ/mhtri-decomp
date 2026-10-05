/*
 * g3d/g3d_scnroot.h - `nw4r::g3d::ScnRoot`, the scene-graph root `src/g3d/g3d_scnroot.cpp` owns.  Only the
 * two camera methods its consumers call are declared, and the class carries no virtuals, so MWCC emits
 * nothing for it (docs/plan.md 6.5 rule 9: the map names are manglings of these members, never spelled
 * as callables).  Moved here from `stage/fn_802B2AA0.h` (rules 1 and 2) when
 * `quest/arenatask.cpp` became the second consumer.
 */
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
