/* face_work_init.h - leaf header (docs/plan.md 6.5 rule 2): `fn_80047398.cpp`'s face work, for `menu/movie.cpp`.  GUESS names (the registry carries them). */
#ifndef MHTRI_FACE_WORK_INIT_H
#define MHTRI_FACE_WORK_INIT_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800474A8 - clears the face work. */
void face_work_init(void);
/* 0x80047634 / 0x80047780 - queue model `sex`'s face textures (face `face`) / its face-part textures. */
void face_tex_load(u8 sex, u16 face, s32 flag);
void face_parts_tex_load(u8 sex, s32 a, s32 b);
/* 0x800478F0 - builds model `sex`'s face texture objects once loaded. */
void face_tex_bind(u8 sex);
/* 0x80047AF0 / 0x80047B0C - show / hide model `sex`'s face. */
void face_draw_on(u8 sex);
void face_draw_off(u8 sex);
/* 0x80047B28 / 0x80047B40 - model `sex`'s eye and face type. */
u8 face_eye_get(u8 sex);
u8 face_type_get(u8 sex);
/* 0x80047B58 / 0x80047BE0 - step model `sex`'s face type forward / back. */
void face_type_next(u8 sex);
void face_type_prev(u8 sex);
/* 0x80047C68 - selects face slot `face` for model `sex`. */
void face_select_set(u8 sex, u8 face);
/* 0x80047C88 - sets model `sex`'s makeup and makeup colour. */
void face_makeup_set(u8 sex, u8 makeup, s8 color);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_FACE_WORK_INIT_H */
