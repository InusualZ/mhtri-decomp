/* stage/stage_zone_hit_ck.h - leaf header for two `stage/stg_w.cpp` functions the enemy part-flinch test calls
 * (docs/plan.md 6.5 rule 2); kept out of `stage/stg_w.h`, which carries no vector type. */
#ifndef MHTRI_STAGE_STAGE_ZONE_HIT_CK_H
#define MHTRI_STAGE_STAGE_ZONE_HIT_CK_H

#include "types.h"
#include "nw4r/math.h"

extern "C" {
/* 0x802AFE5C - the address of word `j` of map `id`'s group table (NULL for the unmapped id 0xFF). */
u32* stage_group_word_get(u8 id, u8 j);
/* 0x802B2574 - whether `pos` lies in a zone of the group table `table` (`out` receives the zone word). */
u32 stage_zone_hit_ck(u32* table, u32* out, nw4r::math::VEC3* pos);
}

#endif /* MHTRI_STAGE_STAGE_ZONE_HIT_CK_H */
