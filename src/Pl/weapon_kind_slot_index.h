/* `weapon_kind_slot_index`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_WEAPON_KIND_SLOT_INDEX_H
#define MHTRI_PL_WEAPON_KIND_SLOT_INDEX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the `_PLW::equipA` slot of weapon kind `kind` (1-6): `kind - 1`. */
u8 weapon_kind_slot_index(u8 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_WEAPON_KIND_SLOT_INDEX_H */
