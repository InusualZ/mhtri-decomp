/* ef/eft013_spark_set_em.h - the declaration of `eft013_spark_set_em`, which `ef/eft013_fx.cpp` owns (docs/plan.md 6.5 rule 2);
 * the signature is the one its callers use: the scale arrives in f1 and is handed on to the spawn. */
#ifndef MHTRI_EF_EFT013_SPARK_SET_EM_H
#define MHTRI_EF_EFT013_SPARK_SET_EM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif
void eft013_spark_set_em(struct _ENEMY_WORK* self, u32 id, u32 joint, f32 scale);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_EF_EFT013_SPARK_SET_EM_H */
