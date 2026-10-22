/* menu/option_cfg_max.h - leaf header (docs/plan.md 6.5 rule 2): `menu/get_pop_dat_ptr.cpp`'s option maxima, for `menu/movie.cpp`.  GUESS name (the registry carries it). */
#ifndef MHTRI_MENU_OPTION_CFG_MAX_H
#define MHTRI_MENU_OPTION_CFG_MAX_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x805F8528 (.data) - the highest value of each option. */
extern u8 option_cfg_max[0x20];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_OPTION_CFG_MAX_H */
