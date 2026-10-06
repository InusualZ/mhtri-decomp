/* Declarations `enemy/em008_prog.cpp` owns from its 0x8015D860-0x8015E854 action/state band: `fn_8015D8F0`/
 * `fn_8015D934` return the u32 0/1 predicates their callers compare with `cmplwi`, `fn_8015D908` the two-bit gate's
 * answer, and `fn_8015D860`/`fn_8015DDB8`/`fn_8015DE00`/`fn_8015E05C`/`fn_8015E804` are `void` steps and dispatchers.
 */
#ifndef MHTRI_ENEMY_FN_8015D860_H
#define MHTRI_ENEMY_FN_8015D860_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

void fn_8015D860(struct _ENEMY_WORK* self);
u32 fn_8015D8F0(struct _ENEMY_WORK* self, u8 mask);
u32 fn_8015D908(struct _ENEMY_WORK* self);
u32 fn_8015D934(struct _ENEMY_WORK* self);
void fn_8015DDB8(struct _ENEMY_WORK* self);
void fn_8015DE00(struct _ENEMY_WORK* self);
void fn_8015E05C(struct _ENEMY_WORK* self);
void fn_8015E804(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_8015D860_H */
