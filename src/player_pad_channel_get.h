/* Leaf header (docs/plan.md 6.5 rule 2): the `pad_connect` unit's symbol `menu/menu_item.cpp` calls, for a
 * consumer that cannot include the owner's full header.  C linkage (the map row is a plain name). */
#ifndef MHTRI_PLAYER_PAD_CHANNEL_GET_H
#define MHTRI_PLAYER_PAD_CHANNEL_GET_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x800472DC - returns idx plus twice the pad-connect mode byte: the controller channel a split-screen player's menu reads */
u8 player_pad_channel_get(u8 idx);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_PLAYER_PAD_CHANNEL_GET_H */
