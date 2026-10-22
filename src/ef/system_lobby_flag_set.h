/* Leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp`'s `system_lobby_flag_set`, which
 * `lobby/lb_screen_step.cpp` calls.  C linkage (the map row is a plain name).  GUESS name. */
#ifndef MHTRI_EF_SYSTEM_LOBBY_FLAG_SET_H
#define MHTRI_EF_SYSTEM_LOBBY_FLAG_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D0734 - stores `value` in system_w byte +0x877, the byte `system_lobby_flag_get` returns. */
void system_lobby_flag_set(u8 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_SYSTEM_LOBBY_FLAG_SET_H */
