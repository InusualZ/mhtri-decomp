/* Leaf header (docs/plan.md 6.5 rule 2): `vec3_segment_box_ck`, which `fn_8004CAD8` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_VEC3_SEGMENT_BOX_CK_H
#define MHTRI_VEC3_SEGMENT_BOX_CK_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {

/* 0x80052034 - whether `pos` lies in the box around the segment `a`..`b`. */
u8 vec3_segment_box_ck(nw4r::math::VEC3* pos, nw4r::math::VEC3* a, nw4r::math::VEC3* b, f32 height, f32 radius);

}
#endif

#endif /* MHTRI_VEC3_SEGMENT_BOX_CK_H */
