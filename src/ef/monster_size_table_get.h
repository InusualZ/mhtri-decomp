/* `monster_size_table_get` (0x800CEE88), defined by `ef/system_core.cpp` (leaf header, rule 2).  C linkage
 * (unmangled map name).  GUESS name: it forwards to the command table's second entry, which hands back the monster's
 * three crown sizes. */
#ifndef MHTRI_EF_MONSTER_SIZE_TABLE_GET_H
#define MHTRI_EF_MONSTER_SIZE_TABLE_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Stores the address of monster `monster`'s crown sizes (silver, gold, small) in `*out`. */
void monster_size_table_get(u8 monster, const u16** out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_MONSTER_SIZE_TABLE_GET_H */
