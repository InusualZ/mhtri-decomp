/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `equip_blade_dye_ck`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_PL_EQUIP_BLADE_DYE_CK_H
#define MHTRI_PL_EQUIP_BLADE_DYE_CK_H

#include "types.h"

struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8027E8D0 - the dye flag of a blade-kind equipment piece's record, 0 for other kinds (GUESS name). */
u8 equip_blade_dye_ck(struct _EQUIP* equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_EQUIP_BLADE_DYE_CK_H */
