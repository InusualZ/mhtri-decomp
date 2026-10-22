/* `enemy_net_work_reset`, defined by `enemy/em_prog_support.cpp` (leaf header, rule 2). */
#ifndef MHTRI_ENEMY_ENEMY_NET_WORK_RESET_H
#define MHTRI_ENEMY_ENEMY_NET_WORK_RESET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Clears the enemy network work when no session is running, else only its step bytes. */
void enemy_net_work_reset(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_ENEMY_NET_WORK_RESET_H */
