/* ef/eft013_fx.h - the declarations `ef/eft013_fx.cpp` owns that other units call (docs/plan.md 6.5 rule 2). */
#ifndef MHTRI_EF_EFT013_FX_H
#define MHTRI_EF_EFT013_FX_H

#include "types.h"

#ifdef __cplusplus

struct _PLW;

/* 0x80107250 - the player setter: spawns the eft013 type `type` in the player's area; C++ scope, so this is the
 * callable spelling of the map name `eft013_set__FP4_PLWUc` (rule 9). */
void eft013_set(struct _PLW* plw, u8 type);

#endif

#ifdef __cplusplus
extern "C" {
#endif
void eft013_set_pl_model(struct _EFT013_PL* self, u8 type);
void eft013_spark_spawn(struct _EFT013_PL* self);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT013_FX_H */
