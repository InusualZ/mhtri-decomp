/* Leaf header (docs/plan.md 6.5 rule 2): the `mh3_pad.cpp` symbol `quest/quest_entry.cpp` calls, for a consumer
 * that cannot include the owner's full header.  C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_STAGE_MAP_SET_H
#define MHTRI_STAGE_MAP_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80040DE8 - hands the system the map `map` the quest plays on. */
void stage_map_set(u8 map);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_MAP_SET_H */
