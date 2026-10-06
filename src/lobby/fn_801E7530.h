/* Declarations owned by `lobby/fn_801E7530.cpp`. */
#ifndef MHTRI_LOBBY_FN_801E7530_H
#define MHTRI_LOBBY_FN_801E7530_H

#include "types.h"
#include "lobby/lb_npc.h"
#include "camera/camera.h"
#include "ef/eft052.h"
#include "menu/menu_message.h"

#ifdef __cplusplus
extern "C" {
#endif

extern u8 jumptable_805B7CD8[36];

/* 0x801E9B74 - whether lobby part slot `slot` is open (its progress flags set and the slot not locked).  GUESS. */
u32 lb_part_slot_open_ck(u8 slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_FN_801E7530_H */
