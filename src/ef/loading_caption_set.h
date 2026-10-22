/* Leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp` symbols `quest/quest_entry.cpp` calls, for a consumer that cannot
 * include the owner's full header.  C linkage (the map rows are plain names).  GUESS names, derived from the
 * call sites and bodies. */
#ifndef MHTRI_EF_LOADING_CAPTION_SET_H
#define MHTRI_EF_LOADING_CAPTION_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D0FCC - sets the loading display's caption; retail passes the quest's field text, this consumer's spelling
 * takes none. */
void loading_caption_set(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_LOADING_CAPTION_SET_H */
