/* The declarations `src/enemy/fn_8013F764.cpp` owns that other units call (docs/plan.md 6.5 rule 2).
 * The owner's definitions are C linkage (its own `extern "C"` block). */
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
