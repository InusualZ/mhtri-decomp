/* Leaf header (docs/plan.md 6.5 rule 2): `menu/get_pop_dat_ptr.cpp`'s pop_dat_f_load */
#ifndef MHTRI_MENU_POP_DAT_F_LOAD_H
#define MHTRI_MENU_POP_DAT_F_LOAD_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803BE640 - loads the map's `f_pop.dat` population file (GUESS name). */
void pop_dat_f_load(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_POP_DAT_F_LOAD_H */
