/*
 * menu/menu_infomation.h - the records and entry points `menu/menu_infomation.cpp` owns.  `StatusScreenWork` is the
 *   status screen's work record as `fn_8030BACC`/`equip_list_page_count`/`fn_8030A1D0` and `ef/fn_8030681C.cpp`'s
 *   `fn_80308EC0`/`fn_80308F1C` (below the 0x80308FB4 edge) read it: a partial view, not the complete layout.
 */
#ifndef MHTRI_MENU_MENU_INFOMATION_H
#define MHTRI_MENU_MENU_INFOMATION_H

#include "types.h"

/* The status screen's own work record (`fn_80308EC0`/`fn_80308F1C` below the seam, `fn_8030BACC`/
 * `equip_list_page_count`/`fn_8030A1D0` above it).  Only the fields those bodies reach are named, so the size
 * is the extent they prove, not the record's own.
 * size: 0x23C (the extent these bodies prove) */
typedef struct StatusScreenWork {
    u8    pad_0x000[0x4];          /* +0x000 */
    u16   field_0x04;              /* +0x004  the frame's flag word `fn_80308F1C` folds */
    u8    pad_0x006[0x2];          /* +0x006 */
    u16   field_0x08;              /* +0x008  its second flag word */
    u8    pad_0x00A[0x14 - 0x0A];  /* +0x00A */
    u8    field_0x14;              /* +0x014  the screen mode `fn_80308EC0`/`fn_8030BACC` set */
    u8    pad_0x015[0x1C - 0x15];  /* +0x015 */
    u8    field_0x1C;              /* +0x01C  the cursor row `fn_8030BACC` clears */
    u8    pad_0x01D[0x190 - 0x1D]; /* +0x01D */
    void* equip;                   /* +0x190  the record `fn_8030A1D0` walks */
    u8    pad_0x194[0x19E - 0x194];/* +0x194 */
    u16   field_0x19E;             /* +0x19E  the page count `equip_list_page_count` counts into */
    u8    field_0x1A0;             /* +0x1A0  one visible-row flag per row */
    u8    field_0x1A1;             /* +0x1A1 */
    u8    field_0x1A2;             /* +0x1A2 */
    u8    field_0x1A3;             /* +0x1A3 */
    u8    pad_0x1A4[0x1AE - 0x1A4];/* +0x1A4 */
    s8    field_0x1AE;             /* +0x1AE  the page index `menu_cursor_step` advances */
    s8    field_0x1AF;             /* +0x1AF  the last-page index (one past the count) */
    u8    pad_0x1B0[0x23A - 0x1B0];/* +0x1B0 */
    u8    field_0x23A;             /* +0x23A  the `fn_80308EC0` reset byte */
    u8    pad_0x23B[1];            /* +0x23B */
} StatusScreenWork;

/* The equip list record `equip_list_page_count` counts over: one 16-bit slot id per row and a matching per-row
 * flag byte, 0x5F2 into the record (a view of `_PLW`'s +0x5F2 run, which is how `quest/arenatask.cpp` calls it).
 * Only those two arrays are reached, so the size is their extent.
 * size: 0x60C (the extent `equip_list_page_count` proves) */
typedef struct EquipListWork {
    u8  pad_0x000[0x5F2];          /* +0x000 */
    u16 slot_id[8];                /* +0x5F2  one id per row (stride 2) */
    u8  row_flag[8];               /* +0x602  the per-row visible flag */
    u8  pad_0x60A[2];              /* +0x60A */
} EquipListWork;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8030A1D0 - the equip page count with the default column arrangement (the owner defines it).  The
 * owner's `fn_80308EC0` (the screen's entry, below the seam) arms the screen's page bytes from it. */
s32 fn_8030A1D0(void* equip, s32 mode);

/* 0x8030B790 - the number of six-row pages the equip list record fills (never below one). */
s32 equip_list_page_count(EquipListWork* list);

/* 0x8030CA50 / 0x8030D6A8 / 0x8030F374 / 0x8030F814 - the equipment detail page's per-kind stat blocks (the
 * blade weapons, the bowgun parts, kind 6) and its decoration-slot row, for the player work and the equipment
 * record the caller hands in.  GUESS names, from the kinds the quest-result screen picks them by. */
/* untyped: opaque handle passed through - the owner forwards the player work and the record untouched to its variant resolver */
s32 equip_detail_melee_draw(void* plw, void* equip, u32 row, u32 sel);
/* untyped: opaque handle passed through - the owner forwards the player work and the record untouched to its variant resolver */
void equip_detail_bowgun_draw(void* plw, void* equip, u16 row, u8 sel);
/* untyped: opaque handle passed through - the owner forwards the player work and the record untouched to its variant resolver */
void equip_detail_kind6_draw(void* plw, void* equip, u16 row, u8 sel);
/* untyped: opaque handle passed through - the owner forwards the player work and the record untouched to its variant resolver */
void equip_detail_slots_draw(void* plw, void* equip, u16 row, u8 sel);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_MENU_MENU_INFOMATION_H */
