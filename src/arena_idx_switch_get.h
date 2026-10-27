/* arena_idx_switch_get.h - leaf header for `arena_idx_switch_get`, which `src/draw_shape.cpp` defines with C linkage
 * (docs/plan.md 6.5 rule 2).  A leaf because the owner's header does not declare it. */
#ifndef MHTRI_ARENA_IDX_SWITCH_GET_H
#define MHTRI_ARENA_IDX_SWITCH_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80055E50 - whether the arena swaps its own layout set in (`set_arena_idx_switch` stores it). */
u8 arena_idx_switch_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ARENA_IDX_SWITCH_GET_H */
