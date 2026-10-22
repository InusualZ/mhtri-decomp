/* Leaf header (docs/plan.md 6.5 rule 2): `sound/mhchar.cpp`'s `mhchar_wld_mtx_array_get`.  C linkage (the map row is a
 * plain name). */
#ifndef MHTRI_SOUND_MHCHAR_WLD_MTX_ARRAY_GET_H
#define MHTRI_SOUND_MHCHAR_WLD_MTX_ARRAY_GET_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
namespace nw4r { namespace g3d { class ScnMdlSimple; } }

extern "C" {

/* 0x800E0A8C - the world matrix array of scene model `mdl` (its +0xEC word; GUESS name). */
nw4r::math::MTX34* mhchar_wld_mtx_array_get(nw4r::g3d::ScnMdlSimple* mdl);
}
#endif

#endif /* MHTRI_SOUND_MHCHAR_WLD_MTX_ARRAY_GET_H */
