/* Declarations `enemy/em016_prog.cpp` owns from its aim/action group (0x80191598..), which `enemy/em018_prog.cpp`'s
 * `fn_8019D8B8` and `fn_8019D9BC` call.
 */
#ifndef MHTRI_ENEMY_FN_80191598_H
#define MHTRI_ENEMY_FN_80191598_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80192370 - r3 (`self`) and r4, a selector; the return is compared against 0 by its caller, so
 * it reports a status word rather than a pointer. */
u32 em018_aim_bits_ck(struct _ENEMY_WORK* self, u32 a);
/* 0x80192618 - r3 (`self`) only, no return; the tail every action of this band runs. */
void em018_aim_arm(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_80191598_H */
