/* `em_part_sphere_get`, defined by `menu/menu_item.cpp` (leaf header, rule 2). */
#ifndef MHTRI_MENU_EM_PART_SPHERE_GET_H
#define MHTRI_MENU_EM_PART_SPHERE_GET_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8029D894 (0x230): fills the hit sphere (centre, then radius) of hit part part; 0 when filled. */
struct _ENEMY_WORK;
u8 em_part_sphere_get(struct _ENEMY_WORK* em, s16* part, nw4r::math::CAPSULE* capsule, nw4r::math::VEC3* sphere);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_EM_PART_SPHERE_GET_H */
