/* Leaf declarations of `Pl/pl_act_step.cpp`'s per-act-number state appliers (0x8024CE54-0x8024EE88), which the net
 * receiver runs after it has copied a player-state message in.
 */
#ifndef MHTRI_PL_FN_802489D4_H
#define MHTRI_PL_FN_802489D4_H

#include "types.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif

void pl_act_net_hook_b(struct _PLW* self);
void pl_act_net_hook_c(struct _PLW* self);
void pl_act_net_hook_a(struct _PLW* self);
void pl_act_net_hook_main(struct _PLW* self); /* the owner's definition takes a second, unread `s32` */
void pl_act_net_hook_d(struct _PLW* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_FN_802489D4_H */
