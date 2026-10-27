/* Leaf header (docs/plan.md 6.5 rule 2): `eft_area_part_break_set`, which `ef/fn_80105314` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_EF_EFT_AREA_PART_BREAK_SET_H
#define MHTRI_EF_EFT_AREA_PART_BREAK_SET_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {

/* 0x801067F4 - starts a part-break effect of `kind` at `pos` (the owner's parameter order). */
void eft_area_part_break_set(u8 kind, nw4r::math::VEC3* pos, f32 scale, u8 area, u32 timer);

}
#endif

#endif /* MHTRI_EF_EFT_AREA_PART_BREAK_SET_H */
