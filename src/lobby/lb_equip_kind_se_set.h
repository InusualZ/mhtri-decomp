/* `lb_equip_kind_se_set`, defined by `lobby/fn_80219260.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_EQUIP_KIND_SE_SET_H
#define MHTRI_LOBBY_LB_EQUIP_KIND_SE_SET_H

#include "types.h"

struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* Plays the try-on sound of equipment `equip`'s kind. */
void lb_equip_kind_se_set(struct _EQUIP* equip);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_EQUIP_KIND_SE_SET_H */
