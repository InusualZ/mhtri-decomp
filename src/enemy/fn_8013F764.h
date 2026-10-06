/* Declarations `enemy/em_kind.cpp` owns from its run driver and stream helpers (0x8013F764..), C linkage like the
 * definitions. */
#ifndef MHTRI_ENEMY_FN_8013F764_H
#define MHTRI_ENEMY_FN_8013F764_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80141050 - releases the enemy `id` (the files it loaded and its slot); returns how many of its two
 * file entries were released, 0 when it was never loaded and -1 when it cannot be.  GUESS name from
 * the two file tables it walks and the slot teardown it ends with. */
s32 em_kind_release(u8 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_8013F764_H */
