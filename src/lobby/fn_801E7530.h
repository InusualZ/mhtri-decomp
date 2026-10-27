/* Declarations owned by `lobby/fn_801E7530.cpp`. */
#ifndef MHTRI_LOBBY_FN_801E7530_H
#define MHTRI_LOBBY_FN_801E7530_H

#include "types.h"
#include "lobby/lb_npc.h"
#include "camera/camera.h"
#include "ef/eft052.h"
#include "menu/menu_message.h"

/* One row of the farm's seed table (`lb_farm_seed_rows`, 12 rows, three tiers of four): the plot level it needs, the days
 * it grows and its price.  size: 0x4 */
typedef struct LbFarmSeedRow {
    /* +0x0 */ u8 level_0x0;
    /* +0x1 */ u8 days_0x1;
    /* +0x2 */ u16 price_0x2;
} LbFarmSeedRow;

/* One row of a seed's harvest table (`lb_farm_harvest_tables` points at one per seed): the item, its count and the per-cent
 * chance of the row; the table ends with item 0.  size: 0x6 */
typedef struct LbFarmHarvestRow {
    /* +0x0 */ u16 item_0x0;
    /* +0x2 */ u16 count_0x2;
    /* +0x4 */ u8 rate_0x4;
    /* +0x5 */ u8 pad_0x5;
} LbFarmHarvestRow;

/* One row of the plot-grade weight table (`lb_farm_grade_rows`; the second row serves three plots): the weight count and
 * the weights `weight_table_index` draws from.  size: 0x8 */
typedef struct LbFarmGradeRow {
    /* +0x0 */ u8 count_0x0;
    /* +0x1 */ u8 pad_0x1[3];
    /* +0x4 */ s16* weights_0x4;
} LbFarmGradeRow;

/* One plot tab of the farm screen (`lb_farm_tab_rows`, three rows): its layout, cursor and frame sprites, its frame
 * animation and its page arrows.  size: 0x10 */
typedef struct LbFarmTabRow {
    /* +0x0 */ u16 lsp_0x0;
    /* +0x2 */ u16 cursor_0x2;
    /* +0x4 */ u16 frame_0x4;
    /* +0x6 */ u8 pad_0x6[2];
    /* +0x8 */ const u16* frames_0x8;
    /* +0xC */ const u16* arrows_0xC;
} LbFarmTabRow;

#ifdef __cplusplus
extern "C" {
#endif

extern u8 jumptable_805B7CD8[36];

/* The farm's tables (this unit's `.data`/`.sdata`). */
extern u16 lb_farm_fertilizer_items[8];               /* the fertilizers the picker lists, 0-padded */
extern LbFarmSeedRow lb_farm_seed_rows[12];
extern LbFarmHarvestRow* lb_farm_harvest_tables[12];   /* one harvest table per seed */
extern LbFarmGradeRow lb_farm_grade_rows[2];           /* one and two plots, then three plots */
extern u16 lb_farm_box_page_frame[10];
extern u16 lb_farm_box_frame[36];
extern LbFarmTabRow lb_farm_tab_rows[3];
extern u8 lb_farm_tab_order[16];                       /* the tab order per picked plot, three each */
extern u16 lb_farm_info_frame[16];
extern u16 lb_farm_window_frame[6];
extern u16 lb_farm_plot_window_body[6];
extern u16 lb_farm_edit_window_body[6];
extern u16 lb_farm_plot_window_rows[8];
extern u16 lb_farm_edit_rows[22];
extern u16 lb_farm_edit_seed_hilite[6];
extern u16 lb_farm_edit_fert_hilite[6];
extern u16 lb_farm_confirm_frame[10];
extern u16 lb_farm_edit_cursor[6];
extern u16 lb_farm_seed_info_frame[28];
extern u16 lb_farm_seed_list_frame[16];
extern u16 lb_farm_box_page_lsp[4];
extern u16 lb_farm_box_cell_lsp[4];
extern u16 lb_farm_box_page_tabs[4];
extern u16 lb_farm_grade_star_lsp[4];
extern u16 lb_farm_plot_title_ids[4];
extern u16 lb_farm_plot_title_ids_pick[4];
extern u16 lb_farm_plot_growing_frame[4];
extern u16 lb_farm_plot_empty_frame[4];
extern u16 lb_farm_plot_prompt_frame[4];
extern u16 lb_farm_confirm_blink_frame[4];
extern char lb_farm_price_fmt[3];                      /* "%d" */
extern u16 lb_farm_tier_icon_ids[4];
extern u16 lb_farm_seed_item_lsp[2];
extern char lb_farm_seed_price_fmt[5];                 /* "%d%s" */
extern u16 lb_farm_seed_cell_lsp[4];

/* 0x801E9B74 - whether lobby part slot `slot` is open (its progress flags set and the slot not locked).  GUESS. */
u32 lb_part_slot_open_ck(u8 slot);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_FN_801E7530_H */
