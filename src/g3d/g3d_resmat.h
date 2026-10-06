/* g3d/g3d_resmat.h - `nw4r::g3d::ResTexSrt`, a one-word `ResCommon`-style handle (`mpData`, `size: 0x4`) whose
 *   `SetEffectMtx` and const `GetEffectMtx` members `g3d/g3d_resmat.cpp` defines; `ef/eft002.cpp` still carries a
 *   local copy. */
#ifndef MHTRI_NW4R_G3D_G3D_RESMAT_H
#define MHTRI_NW4R_G3D_G3D_RESMAT_H

#include "types.h"
#include "nw4r/math.h"

namespace nw4r {
namespace g3d {

class ResTexSrt {
public:
    /* +0x00 */ void* mpData;

    /* The `id`-th effect matrix slot: copy `pMtx` in (or clear it when null) and set the present bit.
     * `id` outside [0, 8) is a no-op that returns false. */
    bool SetEffectMtx(u32 id, const nw4r::math::MTX34* pMtx);
    /* The const twin: copy the `id`-th slot out. */
    bool GetEffectMtx(u32 id, nw4r::math::MTX34* pMtx) const;
}; /* size: 0x4 */

} /* namespace g3d */
} /* namespace nw4r */

#endif /* MHTRI_NW4R_G3D_G3D_RESMAT_H */
