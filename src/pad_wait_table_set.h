/* Leaf header (docs/plan.md 6.5 rule 2): `mh3_pad.cpp`'s `pad_wait_table_set`, for a consumer that cannot include
 * the owner's declarations.  C linkage (the map row is a plain name). */
#ifndef MHTRI_PAD_WAIT_TABLE_SET_H
#define MHTRI_PAD_WAIT_TABLE_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004422C - copies `table` (16 hold-off frames) into pad channel `chan`'s active hold-off row. */
void pad_wait_table_set(u8 chan, u8* table);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PAD_WAIT_TABLE_SET_H */
