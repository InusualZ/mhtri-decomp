/* `enemy/em_common.cpp`'s nearest-part-spot query (0x80127308), for the units that place an enemy on a part spot
 * (`enemy/em010_prog.cpp`).  The signature is the owner's definition. */
#ifndef MHTRI_ENEMY_EM_PART_SPOT_NEAREST_GET_H
#define MHTRI_ENEMY_EM_PART_SPOT_NEAREST_GET_H

#include "types.h"
#include "nw4r/math.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80127308 */
void em_part_spot_nearest_get(struct _ENEMY_WORK* self, nw4r::math::VEC3* pos, nw4r::math::VEC3* out, u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM_PART_SPOT_NEAREST_GET_H */
