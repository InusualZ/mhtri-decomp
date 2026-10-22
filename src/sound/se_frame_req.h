/* sound/se_frame_req.h - leaf header (docs/plan.md 6.5 rule 2): `sound/fn_800D7F54.cpp`'s 0x800D9CC8, for `menu/menu_placeinfo.cpp`.
 *   C linkage (the map row is a plain name); the signature is `sound/se.h`'s.  GUESS name (registry). */
#ifndef MHTRI_SOUND_SE_FRAME_REQ_H
#define MHTRI_SOUND_SE_FRAME_REQ_H

#include "types.h"

struct _se_w;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800D9CC8: starts a frame sound `id` on SE work `work`; `param` packs the owner in its high byte and the kind below it. */
void se_frame_req(struct _se_w* work, s32 id, s32 param, s32 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_SOUND_SE_FRAME_REQ_H */
