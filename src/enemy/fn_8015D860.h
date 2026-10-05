/* The action/state band `enemy/fn_8015D860.cpp` (0x8015D860..0x8015E854).
 *
 * Declarations published for the consumers (docs/plan.md 6.5 rule 2: an extern lives with the TU
 * that owns the symbol).  Moved out of `unsplit/enemy.h` when the band registered: that
 * header is a fallback for unowned addresses, and once the unit owns them its typed declarations
 * belong here.
 *
 * The signatures are the owners': `fn_8015D8F0`/`fn_8015D934` return the u32 0/1 predicates their
 * call sites compare with `cmplwi`; `fn_8015D908` returns the two-bit gate's answer;
 * `fn_8015D860`/`fn_8015DDB8`/`fn_8015DE00`/`fn_8015E05C`/`fn_8015E804` are the `void` steps and
 * dispatchers.
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
