/* `equip_blade_rec_find`, defined by `Pl/pl_act.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_EQUIP_BLADE_REC_FIND_H
#define MHTRI_PL_EQUIP_BLADE_REC_FIND_H

#include "types.h"

struct PlSkillRec;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027EC50 - the blademaster weapon row (`PlSkillRec`) of kind `kind` and id `id`. */
struct PlSkillRec* equip_blade_rec_find(u8 kind, u16 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_EQUIP_BLADE_REC_FIND_H */
