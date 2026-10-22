/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `stage/stg_w.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_STAGE_STAGE_DEMO_FLAGS_SET_H
#define MHTRI_STAGE_STAGE_DEMO_FLAGS_SET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B45BC - raises bits 0x81 of the stage work's +0x2F91 flags.  GUESS name. */
void stage_demo_flags_set(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_DEMO_FLAGS_SET_H */
