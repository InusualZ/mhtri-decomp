/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `stage/stg_w.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_STAGE_STAGE_DEMO_START_H
#define MHTRI_STAGE_STAGE_DEMO_START_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B4824 - re-inits the stage models for demo `demo`.  GUESS name. */
void stage_demo_start(u8 demo);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_DEMO_START_H */
