/* `pl_root_lock_ck`, defined by `Pl/pl_motion.cpp` (leaf header, rule 2). */
#ifndef MHTRI_PL_PL_ROOT_LOCK_CK_H
#define MHTRI_PL_PL_ROOT_LOCK_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8028F204 (0x48): whether the root move work's lock byte (+0x22D7) is 1. */
u32 pl_root_lock_ck(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_ROOT_LOCK_CK_H */
