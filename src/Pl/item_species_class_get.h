/* `item_species_class_get`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_ITEM_SPECIES_CLASS_GET_H
#define MHTRI_PL_ITEM_SPECIES_CLASS_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027F0F4 - the slot class (1..3) of the decoration species of item `id`. */
u8 item_species_class_get(u16 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_ITEM_SPECIES_CLASS_GET_H */
