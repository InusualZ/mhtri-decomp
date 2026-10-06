/* `enemy/em015_prog.cpp`'s `fn_801775C0` (its sub-state dispatcher, `extern "C" void (_ENEMY_WORK*)`), which the
 * master dispatcher tail-calls.
 */
#ifndef MHTRI_ENEMY_FN_80176C58_H
#define MHTRI_ENEMY_FN_80176C58_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

void fn_801775C0(struct _ENEMY_WORK* self); /* 0x801775C0 - the low sub-state dispatcher */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_FN_80176C58_H */
