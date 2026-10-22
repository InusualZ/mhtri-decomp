/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em_common`'s `em_area_ck`, which `camera/camera_main.cpp` calls. */
#ifndef MHTRI_ENEMY_EM_AREA_CK_H
#define MHTRI_ENEMY_EM_AREA_CK_H

#include "types.h"

struct _ENEMY_WORK;

/* 0x8012D074 (enemy/em_common): whether the monster is in the player's area (`em_area_ck__FP11_ENEMY_WORK`, C++ linkage). */
u32 em_area_ck(struct _ENEMY_WORK* work);

#endif /* MHTRI_ENEMY_EM_AREA_CK_H */
