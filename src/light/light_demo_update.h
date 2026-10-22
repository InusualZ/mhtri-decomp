/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `light/light.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LIGHT_LIGHT_DEMO_UPDATE_H
#define MHTRI_LIGHT_LIGHT_DEMO_UPDATE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802C2338 - moves demo `demo`'s point light by the demo frame.  GUESS name. */
void light_demo_update(u8 demo);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LIGHT_LIGHT_DEMO_UPDATE_H */
