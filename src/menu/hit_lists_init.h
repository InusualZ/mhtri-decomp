/* Leaf header (docs/plan.md 6.5 rule 2): `menu/menu_item.cpp`'s `hit_lists_init`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_MENU_HIT_LISTS_INIT_H
#define MHTRI_MENU_HIT_LISTS_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80297E34 (0x2C): clears the frame's hit lists. */
void hit_lists_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_HIT_LISTS_INIT_H */
