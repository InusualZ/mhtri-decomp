/* Leaf header (docs/plan.md 6.5 rule 2): `menu/get_pop_dat_ptr.cpp`'s `option_nop`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_MENU_OPTION_NOP_H
#define MHTRI_MENU_OPTION_NOP_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x803BED38 (0x4): an empty option hook. */
void option_nop(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_OPTION_NOP_H */
