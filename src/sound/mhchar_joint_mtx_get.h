/* sound/mhchar_joint_mtx_get.h - leaf header: `mhchar_joint_mtx_get` (0x800E0A14), owned by `sound/mhchar.cpp`. C linkage:
 * the map carries the plain name. */
#ifndef MHTRI_SOUND_MHCHAR_JOINT_MTX_GET_H
#define MHTRI_SOUND_MHCHAR_JOINT_MTX_GET_H

#include "types.h"
#include "nw4r/math.h"

struct MHchar;

#ifdef __cplusplus
extern "C" {
#endif
/* Copies the world matrix of joint `joint` of `chr` into `out`. */
void mhchar_joint_mtx_get(struct MHchar* chr, u32 joint, MTX34* out);
#ifdef __cplusplus
}
#endif

#endif
