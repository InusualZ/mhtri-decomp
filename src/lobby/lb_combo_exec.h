/* Leaf header (docs/plan.md 6.5 rule 2): a symbol `lobby/lb_companion_ui.cpp` defines, for a consumer that cannot include
 * the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_LOBBY_LB_COMBO_EXEC_H
#define MHTRI_LOBBY_LB_COMBO_EXEC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8033B380 - combines `item_a` with `item_b`: the result item (-1 on failure), the count made in `yield`. */
s16 lb_combo_exec(u8* yield, u16 item_a, u16 item_b, s32 mode, u16 slot_a, u16 slot_b);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_COMBO_EXEC_H */
