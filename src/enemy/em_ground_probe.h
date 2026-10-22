/* enemy/em_ground_probe.h - the declaration of `em_ground_probe`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_GROUND_PROBE_H
#define MHTRI_ENEMY_EM_GROUND_PROBE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void em_ground_probe(struct _ENEMY_WORK* work);
#ifdef __cplusplus
}
#endif

#endif
