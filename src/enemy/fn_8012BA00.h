/* The enemy motion-cost unit `enemy/fn_8012BA00.c` (0x8012BA00..0x8012BDF4).
 *
 * Declarations moved here from the consumer units' `src/` files (docs/plan.md 6.5 rule 2:
 * an extern lives with the TU that owns the symbol).  The signature set is what the
 * consumers used; where only the parameter spelling differed the wider form is kept.
 */
#ifndef MHTRI_ENEMY_FN_8012BA00_H
#define MHTRI_ENEMY_FN_8012BA00_H

#include "types.h"
#include "nw4r/math.h"

struct EnemyActionTable;
struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

s32 fn_8012BA00(struct _ENEMY_WORK* enemy, struct EnemyActionTable* table, void* work, s32 kind, u16 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_8012BA00_H */
