/*
 * `pad_acc_hist_get` (0x80043968) and the three accelerometer accessors beside it, defined by `src/mh3_pad.cpp`
 * (leaf header, rule 2): the remote's and the Nunchuk's sample history and the Y movement since the last frame.
 */
#ifndef MHTRI_MH3_PAD_PAD_ACC_HIST_GET_H
#define MHTRI_MH3_PAD_PAD_ACC_HIST_GET_H

#include "types.h"

/* The sample record, spelled as `mh3_pad/vec3.h` spells it. */
#ifdef __cplusplus
namespace nw4r { namespace math { struct VEC3; } }
#define MHTRI_PAD_ACC_VEC3 nw4r::math::VEC3
#else
struct VEC3;
#define MHTRI_PAD_ACC_VEC3 struct VEC3
#endif

#ifdef __cplusplus
extern "C" {
#endif

void pad_acc_hist_get(s32 chan, MHTRI_PAD_ACC_VEC3* out, u8 count);
void pad_fs_acc_hist_get(s32 chan, MHTRI_PAD_ACC_VEC3* out, u8 count);
s32 pad_acc_y_delta(s32 chan);
s32 pad_fs_acc_y_delta(s32 chan);

#ifdef __cplusplus
}
#endif

#undef MHTRI_PAD_ACC_VEC3

#endif /* MHTRI_MH3_PAD_PAD_ACC_HIST_GET_H */
