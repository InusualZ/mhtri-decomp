/*
 * include/hud/cockpit_icon_data.h - the quest cockpit's map icon tables and the flash animation id list, owned by
 * the data-only unit `hud/cockpit_icon_data.cpp`.
 *
 * The tables are declared as incomplete arrays on purpose: the readers (`hud/cockpit_quest.cpp`) reach them with
 * `lis`/`addi` pairs, which is what an extern array of unknown size compiles to.
 */
#ifndef MHTRI_HUD_COCKPIT_ICON_DATA_H
#define MHTRI_HUD_COCKPIT_ICON_DATA_H

#include "types.h"

/* 0x805E6CA0 - map number -> that map's per-area icon id list table (NULL = none). */
extern u16** quest_area_icon_lists_by_map[];
/* 0x805E6CF4 - quest id 1..3 -> the per-area icon id list table that replaces the map's while that quest is selected. */
extern u16** quest_area_icon_lists_by_quest[];
/* 0x805E6DCC - map number -> that map's icon id list (0xFFFF-terminated). */
extern u16* quest_map_icon_list_by_map[];
/* 0x805E6E20 - quest id 1..3 -> the icon id list that replaces the map's while that quest is selected. */
extern u16* quest_map_icon_list_by_quest[];
/* 0x805E6EA4 - map number -> that map's sprite id per area (indexed by area). */
extern u16* quest_area_sprite_list_by_map[];
/* 0x80792F68 - the pit map's area -> its two icon ids (0xFFFF-terminated). */
extern u16* quest_pit_area_icon_lists[];
/* 0x805E70C8 - the sprite id of each of the five slot rows (slot index >> 3); the row's anchor is added to the slot's. */
extern u16 cockpit_slot_row_sprite_ids[];
/* 0x805E70D4 - the sprite id of each of the eight slot columns (slot index & 7), 0xFFFF-terminated. */
extern u16 cockpit_slot_column_sprite_ids[];
/* 0x805E70E8 - the flash animation's sprite ids, 0xFFFF-terminated (read by the quest cockpit and the menu cockpit). */
extern u16 cockpit_flash_anim_ids[];

#endif /* MHTRI_HUD_COCKPIT_ICON_DATA_H */
