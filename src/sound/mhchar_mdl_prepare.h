/* Leaf header (docs/plan.md 6.5 rule 2): `sound/mhchar.cpp`'s `mhchar_mdl_prepare`.  C linkage (the map row is a plain name). */
#ifndef MHTRI_SOUND_MHCHAR_MDL_PREPARE_H
#define MHTRI_SOUND_MHCHAR_MDL_PREPARE_H

#include "types.h"
#include "g3d/g3d_resmat.h"   /* nw4r::g3d::ResMdl */

#ifdef __cplusplus
extern "C" {

/* 0x800E38BC - prepares the model's resource model after a load (GUESS name: the rig calls it on the hair model once its materials are mapped). */
void mhchar_mdl_prepare(nw4r::g3d::ResMdl* mdl);
}
#endif

#endif /* MHTRI_SOUND_MHCHAR_MDL_PREPARE_H */
