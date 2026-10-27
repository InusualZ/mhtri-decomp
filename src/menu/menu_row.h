/*
 * menu/menu_row.h - the declarations of `menu/menu_row.cpp`'s symbols other units call (docs/plan.md 6.5 rule 2).
 */
#ifndef MHTRI_MENU_MENU_ROW_H
#define MHTRI_MENU_MENU_ROW_H

#include "types.h"

/* The 16-byte help-record body `place_rec_alloc` copies into a placement record (its +0x14): the kind and the five
 * halves the kind's filler stores. size: 0x10 */
typedef struct PlaceRec {
    /* +0x00 */ u32 kind;
    /* +0x04 */ u16 value_0x04;
    /* +0x06 */ u16 value_0x06;
    /* +0x08 */ u16 value_0x08;
    /* +0x0A */ u16 value_0x0A;
    /* +0x0C */ u16 value_0x0C;
    /* +0x0E */ u16 value_0x0E;
} PlaceRec;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8034C350 - the menu row of monster `monster` (`menu_row_table_get`'s index), or a negative value when the
 * monster has none: an 8-byte forward to the HUD layout's 0x802E1978.  GUESS name. */
s8 menu_row_monster_index_get(u8 monster);

/* 0x8034CD6C - files a help record `src` for the box of layout sprite `id` at `pos` (`place_rec_alloc_spr`).  Added with
 * `menu/menu_infomation.cpp`'s skill rows. */
void place_rec_alloc_idx(const PlaceRec* src, u16 id, const struct _mh_ivec2_* pos, s16 a, s16 b);

/* 0x8034CC2C - files a help record `src` for the screen rectangle (`x`, `y`, `width`, `height`). */
u8 place_rec_alloc(const PlaceRec* src, s16 x, s16 y, s16 width, s16 height, s16 a, s16 b);
/* 0x8034D200 / 0x8034D284 - fill a help record of kind 6 (a trade) / kind 9 (a course).  GUESS names. */
s32 place_rec_trade_set(PlaceRec* rec, u16 source, u16 item, u16 count, u16 cost, u16 cost_count);
s32 place_rec_course_set(PlaceRec* rec, u16 course);
/* 0x8034D258 - fill a help record of kind 8 (a skill; added with `menu/menu_infomation.cpp`'s skill panels).  GUESS
 * name. */
s32 place_rec_skill_set(PlaceRec* rec, u16 skill);
/* 0x8034D22C - fill a help record of kind 7 (a skill tree; added with `menu/menu_infomation.cpp`'s skill rows). */
s32 menu_row_rec_init(PlaceRec* rec, u16 tree);

/* The row and placement-list entries `hud/cockpit.cpp`'s pointer page calls (the owner's definitions' signatures). */
struct MenuRowData;
u32 menu_row_find(const struct MenuRowData* entry);
u16 menu_row_empty_count(void);
u32 menu_row_reserve(struct MenuRowData* entry);
void place_list_deactivate_all(void);
s32 place_list_overlap_find(s16 x, s16 y, s16 width, s16 height);
u32* place_rec_key_ptr(s8 index);
u8 place_list_key_count(u32 key);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_ROW_H */
