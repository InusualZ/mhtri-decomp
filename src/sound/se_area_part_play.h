/* Leaf header (docs/plan.md 6.5 rule 2): `se_area_part_play`, which `sound/fn_800D7F54` owns; declared for `stage/stg_w.cpp`.  C linkage. */
#ifndef MHTRI_SOUND_SE_AREA_PART_PLAY_H
#define MHTRI_SOUND_SE_AREA_PART_PLAY_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {

/* 0x800DCFEC - plays the part-break sound of part `index` at `pos`. */
void se_area_part_play(u8 index, nw4r::math::VEC3* pos);

}
#endif

#endif /* MHTRI_SOUND_SE_AREA_PART_PLAY_H */
