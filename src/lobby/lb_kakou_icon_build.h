/* `lb_kakou_icon_build`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_KAKOU_ICON_BUILD_H
#define MHTRI_LOBBY_LB_KAKOU_ICON_BUILD_H

#include "types.h"

struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* Builds the crafting icon record of `rec` into `icon`; returns its crafting state. */
u8 lb_kakou_icon_build(struct _EQUIP* rec, s32 flag, u8* icon, u8* table);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_KAKOU_ICON_BUILD_H */
