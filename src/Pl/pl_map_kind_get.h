/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_motion.cpp`'s pl_map_kind_get */
#ifndef MHTRI_PL_PL_MAP_KIND_GET_H
#define MHTRI_PL_PL_MAP_KIND_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8028F24C - the map kind of the local player's area (GUESS name). */
u8 pl_map_kind_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_MAP_KIND_GET_H */
