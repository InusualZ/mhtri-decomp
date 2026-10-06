/*
 * Leaf header (docs/plan.md 6.5 rule 2) for four accessors `ef/ef_particlemanager.cpp` owns: the out-of-line
 * `nw4r::ut::List` last-element getter and the emitter resource's particle-track count, table and lookup.
 */
#ifndef MHTRI_EF_EF_LIST_GET_LAST_H
#define MHTRI_EF_EF_LIST_GET_LAST_H

#include "types.h"
#include "nw4r/fn_805012C4.h"

struct EfEmitterRes;
struct EfResTrack;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800AD0C4 (0x8): Returns the list's last element, or null. */
void* ef_list_get_last(const nw4r::ut::List* list); /* untyped: caller-owned payload - the list holds elements of any type */
/* 0x800AD230 (0x24): Returns the number of particle tracks the emitter resource holds. */
u16 ef_emres_num_ptcl_track(struct EfEmitterRes* res);
/* 0x800ADDCC (0x84): Returns particle track number `index` (asserts the bound). */
struct EfResTrack* ef_emres_get_ptcl_track_at(struct EfEmitterRes* res, u16 index);
/* 0x800ADE50 (0x24): Returns the particle-track pointer table (its size table follows it). */
u8** ef_emres_get_ptcl_track_tbl(struct EfEmitterRes* res);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EF_LIST_GET_LAST_H */
