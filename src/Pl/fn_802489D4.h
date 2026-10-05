/* The declarations `src/Pl/fn_802489D4.cpp` owns that other units call (docs/plan.md 6.5 rule 2): the
 * per-act-number state appliers the net receiver runs after it has copied a player-state message in.
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
