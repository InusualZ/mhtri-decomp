/* `lb_equip_kind_ck`, defined by `lobby/fn_80212810.cpp` (leaf header, rule 2). */
#ifndef MHTRI_LOBBY_LB_EQUIP_KIND_CK_H
#define MHTRI_LOBBY_LB_EQUIP_KIND_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Returns 1 when equipment `id` of kind `kind` passes the filter `sel`. */
u32 lb_equip_kind_ck(u8 sel, s32 kind, s32 id);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_EQUIP_KIND_CK_H */
