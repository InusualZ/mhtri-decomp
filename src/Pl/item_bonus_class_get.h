/* `item_bonus_class_get`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_ITEM_BONUS_CLASS_GET_H
#define MHTRI_PL_ITEM_BONUS_CLASS_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027403C (0x50): the bonus class of item kind (0 for an ordinary item). */
u8 item_bonus_class_get(u16 kind);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_ITEM_BONUS_CLASS_GET_H */
