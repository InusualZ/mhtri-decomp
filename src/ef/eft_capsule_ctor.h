/* ef/eft_capsule_ctor.h - the declaration of `eft_capsule_ctor`, which `ef/effect.cpp` owns (docs/plan.md 6.5 rule 2),
 * with the callers' `nw4r::math::CAPSULE` view of the two-vector record (the same pointer register). */
#ifndef MHTRI_EF_EFT_CAPSULE_CTOR_H
#define MHTRI_EF_EFT_CAPSULE_CTOR_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif
/* 0x800FA3B8 - zeroes both vectors of the record and returns it. */
nw4r::math::CAPSULE* eft_capsule_ctor(nw4r::math::CAPSULE* self);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT_CAPSULE_CTOR_H */
