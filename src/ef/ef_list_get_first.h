/*
 * Leaf header (docs/plan.md 6.5 rule 2) for two accessors `ef/ef_effect.cpp` owns: the out-of-line
 * `nw4r::ut::List` first-element getter and the emitter resource's name getter.
 */
#ifndef MHTRI_EF_EF_LIST_GET_FIRST_H
#define MHTRI_EF_EF_LIST_GET_FIRST_H

#include "types.h"
#include "nw4r/fn_805012C4.h"

struct EfEmitterRes;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800A485C (0x8): Returns the emitter resource's name (its first word). */
const char* ef_emres_get_name(struct EfEmitterRes* res);
/* 0x800A5250 (0x8): Returns the list's first element, or null. */
void* ef_list_get_first(const nw4r::ut::List* list); /* untyped: caller-owned payload - the list holds elements of any type */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_LIST_GET_FIRST_H */
