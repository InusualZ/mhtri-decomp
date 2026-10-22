/* enemy/em_busy_net_notify.h - leaf header (docs/plan.md 6.5 rule 2): the declaration of `em_busy_net_notify` (0x8013765C), which `enemy/em_common.cpp` owns;
 * it sends the busy enemy's network notice with `arg`. */
#ifndef MHTRI_ENEMY_EM_BUSY_NET_NOTIFY_H
#define MHTRI_ENEMY_EM_BUSY_NET_NOTIFY_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

void em_busy_net_notify(struct _ENEMY_WORK* self, u32 arg);

#ifdef __cplusplus
}
#endif

#endif
