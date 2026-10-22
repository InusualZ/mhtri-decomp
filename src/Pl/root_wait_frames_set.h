/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `Pl/pl_motion.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_PL_ROOT_WAIT_FRAMES_SET_H
#define MHTRI_PL_ROOT_WAIT_FRAMES_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8028D5B8 - sets move work 0's wait countdown (+0x114).  GUESS name. */
void root_wait_frames_set(s32 frames);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_ROOT_WAIT_FRAMES_SET_H */
