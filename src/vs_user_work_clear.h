/* vs_user_work_clear.h - leaf header (docs/plan.md 6.5 rule 2) for `src/fn_8004CAD8.cpp`'s `vs_user_work_clear`
 *   (0x8004D1F0), which the owner defines at C linkage and its own header does not declare. */
#ifndef MHTRI_VS_USER_WORK_CLEAR_H
#define MHTRI_VS_USER_WORK_CLEAR_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8004D1F0 - clears VS user block `index`. */
void vs_user_work_clear(u8 index);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_VS_USER_WORK_CLEAR_H */
