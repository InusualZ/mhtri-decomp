/* face_tex_ready_ck.h - leaf header (docs/plan.md 6.5 rule 2): `fn_80047398.cpp`'s face texture ready check, for
 * `Pl/player_control.cpp`.  GUESS name (the registry carries it). */
#ifndef MHTRI_FACE_TEX_READY_CK_H
#define MHTRI_FACE_TEX_READY_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80047D38 (0x34): 1 once the slot's face texture request has finished (or none is pending). */
s32 face_tex_ready_ck(u8 slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FACE_TEX_READY_CK_H */
