/* `system_weapon_class_set`, defined by `ef/system_core.cpp` (leaf header, rule 2). */
#ifndef MHTRI_EF_SYSTEM_WEAPON_CLASS_SET_H
#define MHTRI_EF_SYSTEM_WEAPON_CLASS_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Stores the weapon class the system work's sound selection reads (system_w +0x886). */
void system_weapon_class_set(u8 weapon_class);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_SYSTEM_WEAPON_CLASS_SET_H */
