/* The declarations `src/enemy/fn_80382310.cpp` owns (docs/plan.md 6.5 rule 2: a consumer includes the
 * owner's header, it never declares the symbol itself).
 *
 * The unit is the em009/em019 program band's shared support block 0x80382310-0x80387844.  The two
 * entries below are the ones the cockpit band above it (`menu/fn_802E4978.cpp`,
 * 0x802E4978-0x802E7408) calls out of its own frame update; they had been declared in
 * `include/unsplit/menu.h` while the address had no registered owner, and their signatures are that
 * consumer's call sites (neither body is written yet).
 */
#ifndef MHTRI_ENEMY_FN_80382310_H
#define MHTRI_ENEMY_FN_80382310_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

void fn_803839EC(void);                  /* 0x803839EC */
void fn_80383AE4(void);                  /* 0x80383AE4 */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_80382310_H */
