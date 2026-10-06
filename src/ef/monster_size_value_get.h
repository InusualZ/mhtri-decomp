/*
 * ef/monster_size_value_get.h - leaf header (docs/plan.md 6.5 rule 2) for `ef/system_core.cpp`'s
 *   `monster_size_value_get` (0x800CEE74).
 */
#ifndef MHTRI_EF_MONSTER_SIZE_VALUE_GET_H
#define MHTRI_EF_MONSTER_SIZE_VALUE_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forwards (`monster`, `size`, `out`) to the command table's first entry, which writes the size's value in `out`
 * (the body narrows the first two and passes r5 through).  GUESS name. */
void monster_size_value_get(u8 monster, u16 size, f32* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_MONSTER_SIZE_VALUE_GET_H */
