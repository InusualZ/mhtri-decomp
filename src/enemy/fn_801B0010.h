/* Declarations `enemy/em030_prog.cpp` owns that other units call (`fn_801B0010`, `fn_801B4348`/`fn_801B4398` in
 * their definitions' signatures).
 */
#ifndef MHTRI_ENEMY_FN_801B0010_H
#define MHTRI_ENEMY_FN_801B0010_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x801B0010 - the area predicate: r3 the area (`u32`; the body narrows it itself) and the answer in r3, which the
 * caller compares against 1. */
u32 fn_801B0010(u32 area);
/* 0x801B4348 - the seat/state reset `enemy/em034_prog.cpp` runs at its two teardown steps. */
void fn_801B4348(struct _ENEMY_WORK* self);
/* 0x801B4398 - the seat lookup that band walks: r3 the work record, r4 the kind set, r5 the `u32*`
 * out record; the answer is the record count. */
u32 fn_801B4398(struct _ENEMY_WORK* self, u32 kind, u32* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_801B0010_H */
