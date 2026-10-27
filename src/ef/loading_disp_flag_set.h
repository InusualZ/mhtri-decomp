/* Leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp`'s `loading_disp_flag_set`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_EF_LOADING_DISP_FLAG_SET_H
#define MHTRI_EF_LOADING_DISP_FLAG_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D06D0 (0x10): stores `flag` in `system_w`'s loading display byte (+0x867). GUESS name. */
void loading_disp_flag_set(s8 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_LOADING_DISP_FLAG_SET_H */
