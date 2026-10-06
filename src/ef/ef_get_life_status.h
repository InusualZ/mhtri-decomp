/*
 * Leaf header (docs/plan.md 6.5 rule 2) for the referenced-object life-status getter, owned by `ef/ef_effect.cpp`.
 * GUESS name `ef_get_life_status`: the body returns the word at +0x0C, and every caller compares it with 1 (alive).
 */
#ifndef MHTRI_EF_EF_GET_LIFE_STATUS_H
#define MHTRI_EF_EF_GET_LIFE_STATUS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800A5248 (0x8): returns the object's life status (1 while alive). */
s32 ef_get_life_status(void* obj); /* untyped: opaque handle - an effect, emitter or particle read through its common header */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_GET_LIFE_STATUS_H */
