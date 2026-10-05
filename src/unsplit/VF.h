/*
 * VF / prfile2 declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * The three file-system entry points at 0x80521CA0/0x80521D70/0x80521DE0 - initialize the console's
 * file system with this work buffer, shut it down again, and report whether it is up - sit in the
 * NCD/VF band that no registered unit covers (the nearest registered `.text` ranges are
 * `NWC24/nwc24_io.c` below and the game-UI band at 0x805482CC above, different modules, so
 * stylelint's rule 2 cannot place them).  Their own bodies name the library - `VFSysInit`,
 * `VFipdm_init_diskmanager`, `VFipf2_init_prfile2`, `dHash_InitHashTable` and
 * `VFSysSetTimeStampCallback` under one mutex - so `VFipf2*` is the scheme they are named in.  The
 * three names are a GUESS in that scheme, not names recovered from the SDK: a later pass that writes
 * the bodies, or that reaches the runtime dump, may confirm or correct them.  No `include/VF/` owner
 * exists, so this file is their home, the way `include/unsplit/SO.h` is the SO band's.
 *
 * Added with the `DWCi/DWCi_Np_CPUCopyFast.c` friend-code getter, whose body calls all three.
 */
#ifndef MHTRI_UNSPLIT_VF_H
#define MHTRI_UNSPLIT_VF_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_VF_H */
