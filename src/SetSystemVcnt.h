/* Leaf header (docs/plan.md 6.5 rule 2): `main.cpp`'s `SetSystemVcnt`, which `Pl/pl_motion.cpp` calls. */
#ifndef MHTRI_SETSYSTEMVCNT_H
#define MHTRI_SETSYSTEMVCNT_H

#include "types.h"

#ifdef __cplusplus
/* 0x8003F50C (0x18): sets the frame's vertical-count interval (C++ scope: `SetSystemVcnt__Fl`). */
void SetSystemVcnt(s32 count);
#endif

#endif /* MHTRI_SETSYSTEMVCNT_H */
