/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `nw4r::g3d::PlayPolicy_Loop` (0x800648B0), owned by `g3d/g3d_anmobj.cpp`,
 * whose own header carries only its C-linkage symbols.
 */
#ifndef MHTRI_G3D_PLAYPOLICY_LOOP_H
#define MHTRI_G3D_PLAYPOLICY_LOOP_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {

/* 0x800648B0 (0xCC): Folds `frame` into the frame range by the remainder; the looping play policy. */
f32 PlayPolicy_Loop(f32 start, f32 end, f32 frame);

}  // namespace g3d
}  // namespace nw4r
#endif

#endif /* MHTRI_G3D_PLAYPOLICY_LOOP_H */
