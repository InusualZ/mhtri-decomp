/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_item.cpp`'s `hit_registry_update`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_MENU_HIT_REGISTRY_UPDATE_H
#define MHTRI_MENU_HIT_REGISTRY_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8029A358 (0x430): resolves the frame's registered hits. */
void hit_registry_update(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_HIT_REGISTRY_UPDATE_H */
