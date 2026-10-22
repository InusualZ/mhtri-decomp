/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/player_control.cpp`'s `pl_equip_file_get`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_PL_PL_EQUIP_FILE_GET_H
#define MHTRI_PL_PL_EQUIP_FILE_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8026880C - the resource file of equipment model `slot` in the actor's file chunk `chunk` (GUESS name). */
u32 pl_equip_file_get(u8 chunk, u8 slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_EQUIP_FILE_GET_H */
