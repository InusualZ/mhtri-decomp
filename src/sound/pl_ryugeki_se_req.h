/* sound/pl_ryugeki_se_req.h - leaf header: `pl_ryugeki_se_req` (0x800DD9FC), owned by `sound/se_req.cpp`. */
#ifndef MHTRI_SOUND_PL_RYUGEKI_SE_REQ_H
#define MHTRI_SOUND_PL_RYUGEKI_SE_REQ_H

#include "types.h"
#include "nw4r/math.h"

struct _PLW;

#ifdef __cplusplus
extern "C" {
#endif
/* Claims an SE slot for the player's ryugeki positional SE (id 10, or 30 when `kind` is set) at `pos`. */
void pl_ryugeki_se_req(struct _PLW* self, nw4r::math::VEC3* pos, u8 kind);
#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_PL_RYUGEKI_SE_REQ_H */
