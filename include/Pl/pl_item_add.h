/*
 * Leaf header (docs/plan.md 6.5 rule 2): `pl_item_add` (0x80272E30), defined by `src/Pl/pl_skill.cpp`.
 * `Pl/pl_skill.h` includes this one, so there is one declaration; it is separate so
 * `lobby/lb_companion_ui.cpp` can call it without taking the rest of `Pl/pl_skill.h`, whose
 * `Pl_cat_skill_ck` clashes with the lobby band's own view.
 */
#ifndef MHTRI_PL_PL_ITEM_ADD_H
#define MHTRI_PL_PL_ITEM_ADD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The item/skill value setter: adds `value` of item `item` to player `plw`'s pouch; the owner defines it
 * `extern "C" s16`. */
s16 pl_item_add(struct _PLW* plw, u16 item, s16 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ITEM_ADD_H */
