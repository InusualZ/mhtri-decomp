/* enemy/em_route_entry_get.h - the declaration of `em_route_entry_get`, which `enemy/em_common.cpp` owns (docs/plan.md 6.5 rule 2); the signature is the one its callers use. */
#ifndef MHTRI_ENEMY_EM_ROUTE_ENTRY_GET_H
#define MHTRI_ENEMY_EM_ROUTE_ENTRY_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void* em_route_entry_get(struct _ENEMY_WORK* self, u8 code); /* the struct EmActionInfo record */
#ifdef __cplusplus
}
#endif

#endif
