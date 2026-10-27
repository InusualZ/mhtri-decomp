/* Leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s `.sbss` hook `set_demo_equip_data_func`. */
#ifndef MHTRI_PL_SET_DEMO_EQUIP_DATA_FUNC_H
#define MHTRI_PL_SET_DEMO_EQUIP_DATA_FUNC_H

#include "types.h"

/* .sbss 0x80794B30 (0x8): the demo equipment hook `pl_net_slot_bind` reads; `pad_connect.cpp`'s boot clears it. */
extern s32 set_demo_equip_data_func;

#endif /* MHTRI_PL_SET_DEMO_EQUIP_DATA_FUNC_H */
