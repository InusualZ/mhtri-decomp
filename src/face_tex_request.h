/* face_tex_request.h - leaf header (docs/plan.md 6.5 rule 2): `fn_80047398.cpp`'s face texture request, for
 * `Pl/player_control.cpp`.  GUESS name (the registry carries it). */
#ifndef MHTRI_FACE_TEX_REQUEST_H
#define MHTRI_FACE_TEX_REQUEST_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80047CAC (0x8C): loads the slot's face and face-parts textures and arms its ready flags (`color` 0xFF: none). */
void face_tex_request(u8 slot, u8 sex, u8 face, u8 style, u8 color);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FACE_TEX_REQUEST_H */
