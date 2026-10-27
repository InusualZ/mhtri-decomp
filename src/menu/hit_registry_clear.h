/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_item.cpp`'s `hit_registry_clear`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_MENU_HIT_REGISTRY_CLEAR_H
#define MHTRI_MENU_HIT_REGISTRY_CLEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80299ED8 (0x20): clears the hit registry. */
void hit_registry_clear(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_HIT_REGISTRY_CLEAR_H */
