/* face_tex_image_get.h - leaf header (docs/plan.md 6.5 rule 2): `fn_80047398.cpp`'s face texture image accessor, for
 * `Pl/player_control.cpp`.  GUESS name (the registry carries it). */
#ifndef MHTRI_FACE_TEX_IMAGE_GET_H
#define MHTRI_FACE_TEX_IMAGE_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80047470 (0x38): the slot's face (`which` 0) or makeup (`which` 1) texture image. */
u8* face_tex_image_get(u8 slot, s32 which);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FACE_TEX_IMAGE_GET_H */
