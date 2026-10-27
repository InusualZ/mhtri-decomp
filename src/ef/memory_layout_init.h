/* Leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp`'s `memory_layout_init`, for `font/flfnt.cpp`.  C linkage (the map row is a plain
 * name). */
#ifndef MHTRI_EF_MEMORY_LAYOUT_INIT_H
#define MHTRI_EF_MEMORY_LAYOUT_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800CF3E4 (0x238): lays the game's resident memory blocks out (the font buffer among them). */
void memory_layout_init(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_MEMORY_LAYOUT_INIT_H */
