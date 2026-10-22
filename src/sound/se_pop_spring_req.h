/* Leaf header (docs/plan.md 6.5 rule 2): `sound/se_req.cpp`'s se_pop_spring_req */
#ifndef MHTRI_SOUND_SE_POP_SPRING_REQ_H
#define MHTRI_SOUND_SE_POP_SPRING_REQ_H

#include "types.h"
#include "nw4r/math.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800DD504 - requests the spring point's bubbling sound at `pos` (GUESS name). */
void se_pop_spring_req(nw4r::math::VEC3* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_POP_SPRING_REQ_H */
