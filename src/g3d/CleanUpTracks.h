/* Leaf header (docs/plan.md 6.5 rule 2): `CleanUpTracks`, which `g3d/g3d_resfile.cpp` owns.  C linkage. */
#ifndef MHTRI_G3D_CLEANUPTRACKS_H
#define MHTRI_G3D_CLEANUPTRACKS_H

#include "types.h"

#ifdef __cplusplus
namespace nw4r {
namespace g3d {
class ResFile;
}
}

extern "C" {
/* 0x800938EC (0xA4): the file clean-up the model release runs before `res_file_terminate`. */
void CleanUpTracks(nw4r::g3d::ResFile* self);
}
#endif

#endif /* MHTRI_G3D_CLEANUPTRACKS_H */
