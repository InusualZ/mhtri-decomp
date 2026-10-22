/* Leaf header (docs/plan.md 6.5 rule 2): `menu/get_pop_dat_ptr.cpp`'s pop_dat_ptrs_bind */
#ifndef MHTRI_MENU_POP_DAT_PTRS_BIND_H
#define MHTRI_MENU_POP_DAT_PTRS_BIND_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803BE798 - copies the ten `pop_dat_ptrs` words into their `.sbss` slots (GUESS name). */
void pop_dat_ptrs_bind(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_POP_DAT_PTRS_BIND_H */
