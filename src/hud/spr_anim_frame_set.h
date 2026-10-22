/* `spr_anim_frame_set`, defined by `hud/layout.cpp` (leaf header, rule 2). */
#ifndef MHTRI_HUD_SPR_ANIM_FRAME_SET_H
#define MHTRI_HUD_SPR_ANIM_FRAME_SET_H

#include "types.h"

struct _SPR_DATA_;
struct _SPR_ANIM_;
struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* Applies frame `frame` of animation `anim` to sprite record `rec`. */
u32 spr_anim_frame_set(struct _SPR_DATA_* rec, const struct _SPR_ANIM_* anim, u16 frame, struct _mh_ivec2_* out);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_SPR_ANIM_FRAME_SET_H */
