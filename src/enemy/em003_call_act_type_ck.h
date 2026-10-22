/* enemy/em003_call_act_type_ck.h - leaf header (docs/plan.md 6.5 rule 2): `enemy/em003_prog.cpp`'s
 *   `em003_call_act_type_ck`, for `menu/menu_placeinfo.cpp`.  C++ linkage (the map row is the mangling
 *   `em003_call_act_type_ck__FP11_ENEMY_WORK`). */
#ifndef MHTRI_ENEMY_EM003_CALL_ACT_TYPE_CK_H
#define MHTRI_ENEMY_EM003_CALL_ACT_TYPE_CK_H

#include "types.h"

struct _ENEMY_WORK;

/* 0x8015D5BC: the kind of reinforcement call the current action makes (0/1: none). */
u8 em003_call_act_type_ck(struct _ENEMY_WORK* self);

#endif /* MHTRI_ENEMY_EM003_CALL_ACT_TYPE_CK_H */
