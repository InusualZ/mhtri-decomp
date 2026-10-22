/* Leaf header (docs/plan.md 6.5 rule 2): symbols `lobby/lb_companion_ui.cpp` defines, for `hud/cockpit_quest.cpp`. */
#ifndef MHTRI_LOBBY_COCKPIT_ITEM_RING_FRAME_IDS_H
#define MHTRI_LOBBY_COCKPIT_ITEM_RING_FRAME_IDS_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x805E6EF8: the cockpit's sprite id runs (0xFFFF-terminated) in `lobby/lb_companion_ui`'s `.data`/`.sdata`. */
extern u16 cockpit_item_ring_frame_ids[];
extern u16 cockpit_item_ring_shell_next_ids[];
extern u16 cockpit_item_ring_shell_prev_ids[];
extern u16 cockpit_item_ring_shell_prev2_ids[];
extern u16 cockpit_item_ring_shell_next2_ids[];
extern u16 cockpit_item_ring_act_ids[];
extern u16 cockpit_split_item_ring_act_ids[];
extern u16 cockpit_scope_frame_ids[];
extern u16 cockpit_scope_anim_ids[];
extern u16 cockpit_scope_frame_ids_wide[];
extern u16 cockpit_scope_anim_ids_wide[];
extern u16 cockpit_split_scope_frame_ids[];
extern u16 cockpit_split_scope_anim_ids[];
extern u16 cockpit_split_scope_frame_ids_wide[];
extern u16 cockpit_split_scope_anim_ids_wide[];
extern u16 cockpit_supply_box_frame_ids[];
extern u16 cockpit_item_tag_digit_ids[];
extern u16 cockpit_item_ring_open_ids[];
extern u16 cockpit_item_ring_level_ids[];
extern u16 cockpit_item_ring_digit_ids[];
extern u16 cockpit_split_item_ring_open_ids[];
extern u16 cockpit_split_item_ring_level_ids[];
extern u16 cockpit_split_item_ring_frame_ids[];
extern u16 cockpit_split_item_ring_shell_next_ids[];
extern u16 cockpit_split_item_ring_shell_prev_ids[];
extern u16 cockpit_split_item_ring_digit_ids[];
extern u16 cockpit_zoom_ring_ids[];
extern u16 cockpit_zoom_ring_ids_wide[];
extern u16 cockpit_ammo_fire_ids[];
extern u16 cockpit_split_zoom_ring_ids[];
extern u16 cockpit_split_zoom_ring_ids_wide[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_COCKPIT_ITEM_RING_FRAME_IDS_H */
