/* `equip_armor_ex_rec_get`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_EQUIP_ARMOR_EX_REC_GET_H
#define MHTRI_PL_EQUIP_ARMOR_EX_REC_GET_H

#include "types.h"

struct _EQUIP;
struct PlArmorExRec;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027E3F4 - `equip_armor_ex_rec_find` for the equipment record `equip`. */
struct PlArmorExRec* equip_armor_ex_rec_get(struct _EQUIP* equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_EQUIP_ARMOR_EX_REC_GET_H */
