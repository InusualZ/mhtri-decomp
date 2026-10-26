/*
 * Leaf header (docs/plan.md 6.5 rule 2) for `nw4r::g3d::G3dInit` (0x8007A400), owned by `g3d/g3d_init.cpp`: nw4r g3d's
 * library start-up, called by the resource manager's start-up.
 */
#ifndef MHTRI_G3D_G3DINIT_H
#define MHTRI_G3D_G3DINIT_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {

/* 0x8007A400 (0x68): Registers the library version and sets up the locked cache and the fast-cast quantisers. */
void G3dInit(bool enableLockedCache);

}  // namespace g3d
}  // namespace nw4r
#endif

#endif /* MHTRI_G3D_G3DINIT_H */
