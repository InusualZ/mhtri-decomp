/* `draw_monstericon_rec`, defined by `hud/layout.cpp` (leaf header, rule 2). */
#ifndef MHTRI_HUD_DRAW_MONSTERICON_REC_H
#define MHTRI_HUD_DRAW_MONSTERICON_REC_H

#include "types.h"

struct _SPR_DATA_;
struct _mh_ivec2_;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x802E198C - draws monster `id`'s icon in a copy of sprite record `rec` at `pos` (nothing for a monster with no icon). */
void draw_monstericon_rec(struct _SPR_DATA_* rec, u8 id, const struct _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_HUD_DRAW_MONSTERICON_REC_H */
