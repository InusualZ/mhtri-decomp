/* Leaf header (docs/plan.md 6.5 rule 2): `ef/system_core.cpp`'s `loading_disp_flag_get`, which `fn_80056F24.cpp` calls. */
#ifndef MHTRI_EF_LOADING_DISP_FLAG_GET_H
#define MHTRI_EF_LOADING_DISP_FLAG_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D06E0 (0x10): returns `system_w`'s loading display byte (+0x867) that `loading_disp_flag_set` stores. GUESS name. */
u8 loading_disp_flag_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_LOADING_DISP_FLAG_GET_H */
