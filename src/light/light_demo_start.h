/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `light/light.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LIGHT_LIGHT_DEMO_START_H
#define MHTRI_LIGHT_LIGHT_DEMO_START_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802C22B8 - sets up demo `demo`'s lights.  GUESS name. */
void light_demo_start(u8 demo);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LIGHT_LIGHT_DEMO_START_H */
