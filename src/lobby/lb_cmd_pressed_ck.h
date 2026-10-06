/* The string tables and command-mask accessors of `lobby/lb_npc.cpp`'s player-character control band
 * (0x8020C588-0x80212810); its foreign callees are in `lobby/lb_npc_callees.h`. */
#ifndef MHTRI_LOBBY_LB_CMD_PRESSED_CK_H
#define MHTRI_LOBBY_LB_CMD_PRESSED_CK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The `lb_*_str` tables: four `.sbss` pointers to arrays of string pointers, indexed by species id -
 * `lb_chacha_skill_name_get` felyne-chaCha skill, `lb_chacha_mask_name_get` felyne mask, `lb_cat_name_get` felyne name,
 * `lb_pig_name_get` poogie name. */
extern s32* lb_chacha_skill_str;
extern s32* lb_chacha_mask_str;
extern s32* lb_cat_name;
extern s32* lb_pig_name;

/* The lobby string table: `lb_str_tbl` is a `.sbss` pointer to five arrays of string pointers, and
 * `lbl_805B9450` holds their counts.  `LbStr` returns group `kind`'s entry `idx`, or group 1's first
 * entry when either is out of range. */
extern u32** lb_str_tbl;
extern s32 lbl_805B9450[6];

/* The four command-mask testers and the four getters, all reading player 0 of
 * `LbLobbyWork::cmd_mask_0x084`. */
s32 lb_cmd_pressed_ck(u16 mask);
s32 lb_cmd_held_ck(u16 mask);
s32 lb_cmd_repeat_ck(u16 mask);
s32 lobby_cmd_release_ck(u16 mask);
u16 lb_cmd_repeat_get(void);
u16 lobby_cmd_trig_get(void);
u16 lobby_cmd_release_get(void);

/* `glplatTextureGetHeight` is the map's own name for the unguarded getter of `cmd_mask_0x084[0][0]`. */
u16 glplatTextureGetHeight(void);

s32 lb_yes_no_step(void* self);
s32 lobby_cmd_toggle_step_raw(void* self);
/* 0x80211DCC / 0x80211DE0 / 0x80211DF4 / 0x80211E08 - entry `id` of the four `lb_*_str` tables (GUESS names). */
s32 lb_chacha_skill_name_get(u32 id);
s32 lb_chacha_mask_name_get(u32 id);
s32 lb_cat_name_get(u32 id);
s32 lb_pig_name_get(u32 id);
s16 fn_802126E8(s16* table, s16 count, u32 value);
s16 fn_80212724(u8* table, s16 count, u32 value);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_CMD_PRESSED_CK_H */
