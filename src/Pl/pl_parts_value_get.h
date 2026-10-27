/* Leaf header (docs/plan.md 6.5 rule 2): `pl_parts_value_get` (0x80269508), owned by `Pl/pl_act.cpp`.  C linkage (the
 * map row is a plain name). */
#ifndef MHTRI_PL_PL_PARTS_VALUE_GET_H
#define MHTRI_PL_PL_PARTS_VALUE_GET_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80269508 - the value stored for one part of the player the work record names, -1 with no record or no global. */
s32 pl_parts_value_get(struct _PLW* self, s32 part);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_PARTS_VALUE_GET_H */
