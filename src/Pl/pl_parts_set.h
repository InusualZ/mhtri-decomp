/* Pl/pl_parts_set.h - leaf header (docs/plan.md 6.5 rule 2): `Pl/pl_act.cpp`'s model part setters, for `menu/movie.cpp`.  GUESS names (the registry carries them). */
#ifndef MHTRI_PL_PL_PARTS_SET_H
#define MHTRI_PL_PL_PARTS_SET_H

#include "types.h"

struct _PLW;
struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80269474 - sets model part `part` of `player` to `value`. */
u32 pl_parts_set(s32 player, u8 part, s32 value);
/* 0x8026954C - whether part `part` of `player` can take `value` (-1 no, 0 not yet, 1 yes). */
s32 pl_parts_set_ck(s32 player, u8 part, s32 value);
/* 0x8027EBA8 - the part model id of equipment record `equip` for `self`. */
s32 pl_equip_model_id_get(struct _PLW* self, struct _EQUIP* equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PL_PL_PARTS_SET_H */
