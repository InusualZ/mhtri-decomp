/* Leaf header (docs/plan.md 6.5 rule 2): `enemy/em033_prog.cpp`'s em033_hold_kind_get */
#ifndef MHTRI_ENEMY_EM033_HOLD_KIND_GET_H
#define MHTRI_ENEMY_EM033_HOLD_KIND_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8035DD40 - the kind em033 holds (its own GUESS name). */
struct _ENEMY_WORK;
u8 em033_hold_kind_get(struct _ENEMY_WORK* self);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_ENEMY_EM033_HOLD_KIND_GET_H */
