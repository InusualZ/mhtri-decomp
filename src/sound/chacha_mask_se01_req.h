/* `chacha_mask_se01_req`, defined by `sound/fn_800D7F54.cpp` (leaf header, rule 2). */
#ifndef MHTRI_SOUND_CHACHA_MASK_SE01_REQ_H
#define MHTRI_SOUND_CHACHA_MASK_SE01_REQ_H

#include "types.h"
#include "nw4r/math.h"

struct _se_w;

#ifdef __cplusplus
extern "C" {
#endif

/* Requests the Cha-Cha mask's sound 1 at `pos` on sound work `work`. */
void chacha_mask_se01_req(struct _se_w* work, nw4r::math::VEC3* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_CHACHA_MASK_SE01_REQ_H */
