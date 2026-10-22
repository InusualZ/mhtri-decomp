/* `lb_equip_flags_ck` and `lb_item_flags_ck`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_EQUIP_FLAGS_CK_H
#define MHTRI_LOBBY_LB_EQUIP_FLAGS_CK_H

#include "types.h"

struct _EQUIP;

#ifdef __cplusplus
extern "C" {
#endif

/* Whether the equipment record of kind `kind` and id `id` carries every bit of `bits` in its wear flags. */
u32 lb_equip_flags_ck(u8 kind, u16 id, u8 bits);
/* Whether the equipment box entry `equip` carries every bit of `bits` in its wear flags. */
u32 lb_item_flags_ck(struct _EQUIP* equip, u8 bits);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_EQUIP_FLAGS_CK_H */
