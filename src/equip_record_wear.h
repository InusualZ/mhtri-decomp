/* `equip_record_wear`, defined by `src/fn_8004C9A0.cpp` (leaf header, rule 2). */
#ifndef MHTRI_EQUIP_RECORD_WEAR_H
#define MHTRI_EQUIP_RECORD_WEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Wears box record `id` in the slot its kind selects; returns 1 when worn. */
/* untyped: the save block, seen by each caller through its own record view */
u32 equip_record_wear(void* userdata, u16 id, s8* class_change);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EQUIP_RECORD_WEAR_H */
