/*
 * The `nw4r::g3d::ResTexSrt` handle owned by `g3d/g3d_resmat.cpp` (docs/plan.md 6.5 rules 1 and 2).
 *
 * `g3d/g3d_resmat.cpp` is the TU that defines `ResTexSrt`'s out-of-line members
 * (`SetEffectMtx__Q34nw4r3g3d9ResTexSrtFUlPCQ34nw4r4math5MTX34` and its const `GetEffectMtx` twin), so
 * the declaration belongs here, once, and every consumer includes it (`ef/eft002.cpp` still carries a
 * local handle of the same name/4-byte shape; that duplicate is on the rule-1 backlog and is not this
 * batch's to move).
 *
 * A `ResCommon<ResTexSrtData>`-style one-word handle: `mpData` is the resource pointer, the member
 * functions resolve it through the nw4r accessors. `size: 0x4`.
 */
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
