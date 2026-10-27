/* enemy/em_hit_kind_accept_ck.h - leaf header: `em_hit_kind_accept_ck` (0x8012F110), owned by `enemy/em_common.cpp`; C linkage. */
#ifndef MHTRI_ENEMY_EM_HIT_KIND_ACCEPT_CK_H
#define MHTRI_ENEMY_EM_HIT_KIND_ACCEPT_CK_H

#include "types.h"

struct _ENEMY_WORK;

#ifdef __cplusplus
extern "C" {
#endif

/* Whether an attack of the given kind can hit the enemy now. */
s32 em_hit_kind_accept_ck(struct _ENEMY_WORK* self, u16 kind);

#ifdef __cplusplus
}
#endif

#endif
