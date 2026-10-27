/* ef/load_queue_reset.h - leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp`'s loader and random-ring entry
 * points, for `fn_80059550.cpp`.  C linkage (the map rows are plain names).  `load_queue_reset`, `load_queue_task`,
 * `rand_ring_seed` and `stream_bounds_arm` are GUESS names (the registry carries them). */
#ifndef MHTRI_EF_LOAD_QUEUE_RESET_H
#define MHTRI_EF_LOAD_QUEUE_RESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800CE5B4 (0x5C): zeroes the loader's side table and its work buffer. */
void load_queue_reset(void);
/* 0x800CE6A0 (0x168): the task body that steps the background load queue. */
void load_queue_task(void);
/* 0x800CEE9C (0x20): seeds the four random-ring cells with one value. */
void rand_ring_seed(u8 value);
/* 0x800CFD20 (0x1C): arms the stream bounds for the current mode. */
void stream_bounds_arm(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_LOAD_QUEUE_RESET_H */
