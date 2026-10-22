/* Leaf header (docs/plan.md 6.5 rule 2): `stage/stg_w.cpp`'s `stage_climate_kind_get`, the one declaration `ai/ai_npc.cpp` needs.
 * GUESS name, from the body (listed on the NAMES line of ai/ai_npc.cpp). */
#ifndef MHTRI_STAGE_STAGE_CLIMATE_KIND_GET_H
#define MHTRI_STAGE_STAGE_CLIMATE_KIND_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802B0608 (0x60): classifies the current stage map: 1 for the hot maps (2, 5, 10, 16), 2 for the cold ones (4, 13, 15), else 0 */
u8 stage_climate_kind_get(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_STAGE_STAGE_CLIMATE_KIND_GET_H */
