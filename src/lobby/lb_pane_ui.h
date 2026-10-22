/* The types and declarations of `lobby/lb_pane_ui.cpp`'s page/panel band, with the map's own type names its manglings
 * need (`_mh_ivec2_`); `unsplit/lobby.h` cannot be included beside it (a byte-array `lobby_world_block`, a two-argument
 * `lb_cmd_pressed_ck`). */
#ifndef MHTRI_LOBBY_LB_PANE_UI_H
#define MHTRI_LOBBY_LB_PANE_UI_H

#include "types.h"
/* `menu_cursor_step`'s owner header (`menu/menu_message.cpp`), re-exported. */
#include "menu/menu_message.h"
#include "fn_80047398/userdata_gunner_ck.h"   /* userdata_gunner_ck (rule 2) */

/* `_mh_ivec2_` (the map's mangling `P10_mh_ivec2_`) and `_SPR_DATA_` are global-scope types: the manglings of the callees below name
 * them.  A TU that includes this header inside a namespace (`LOBBY_VIEW_IN_NAMESPACE`) already has `_mh_ivec2_` from
 * `unsplit/lobby.h` and `_SPR_DATA_` from `hud/spr_data.h`. */
#ifndef LOBBY_VIEW_IN_NAMESPACE
/* The 2D integer vector the lobby/HUD helpers exchange (`_mh_ivec2_` in the map's mangling).
 * size: 0x4 */
typedef struct _mh_ivec2_ {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
} _mh_ivec2_;
#endif
#include "hud/spr_data.h"
#include "lobby/lobby_w.h"       /* `LbLobbyWork` / `lobby_w`, owned by lobby/lb_menu_pos_tbl.cpp (rule 2) */
#include "lobby/lb_cmd_repeat_ck.h"    /* the command-mask accessors of lobby/lb_npc.cpp, in this unit's view (rule 2) */
#include "lobby/lb_equip_page.h"  /* fn_80223258, owned by lobby/lb_equip_page.cpp (rule 2) */
#include "Pl/pl_act.h"            /* fn_802738E8 / fn_80273998 / fn_802738B8, owned by Pl/pl_act.cpp (rule 2) */

/* One 0xC-byte icon record: `fn_801F86FC` copies one from the world table into the caller's array and
 * writes its kind byte when the id is the "none" marker.  size: 0xC */
typedef struct LbIconRec {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 unused_0x01[0xB];
} LbIconRec; /* size: 0xC */

/* The big lobby data block `lobby_world_block` points at (a 4-byte pointer in `.sbss`).  Only the fields
 * this unit touches are named; the rest carries its offset.  The highest offset read here is 0x9E, so
 * the tail is left open and the stated size is that lower bound. */
typedef struct LbWorldBlock {
    /* +0x0000 */ u8 unused_0x0000[2];
    /* +0x0002 */ u8 count_0x0002;    /* the per-page entry count `fn_801F3294` compares an item id against */
    /* +0x0003 */ u8 unused_0x0003[0x11];
    /* +0x0014 */ u32 hair_color_0x0014; /* the hair colour word the colour pane edits (mode 1) */
    /* +0x0018 */ u8 unused_0x0018[0x86];
    /* +0x009E */ u8 count_0x009E;    /* the same byte for the page the range scrolls */
    /* +0x009F */ u8 unused_0x009F[0xD61];
    /* +0x0E00 */ LbIconRec entries_0x0E00[1]; /* the icon table `fn_801F86FC` indexes by entry id */
    /* +0x0E0C */ u8 unused_0x0E0C[0x2BAE];
    /* +0x39BA */ u8 field_0x39BA;     /* the byte `fn_801F3DEC` copies into the page's mode */
    /* +0x39BB */ u8 unused_0x39BB;
    /* +0x39BC */ u32 inner_color_0x39BC; /* the inner-wear colour word the colour pane edits (mode 0) */
    /* +0x39C0 */ u8 tail_0x39C0[];
} LbWorldBlock; /* size: 0x39C0+ */

/* The per-page record this unit's page functions work on (the object `fn_801F3DEC` initialises and
 * `fn_801F3294` drives as its request).  Every offset below is one the disassembly reads or writes; the
 * intermediate ranges are padding and carry theirs.
 * size: 0x263+ (approximate: 0x262 is the highest offset this unit reaches) */
typedef struct LbPage {
    /* +0x000 */ u8 state_0x000;           /* `fn_801F3294`'s step, 0..2 */
    /* +0x001 */ u8 done_0x001;            /* set when the page has finished, read at the step change */
    /* +0x002 */ u8 mode_0x002;            /* 0 = the scrolling page, 1 = the fixed one */
    /* +0x003 */ u8 kind_0x003;            /* the entry kind, 0..2 */
    /* +0x004 */ union {
        /* +0x004 */ s16 value_0x004;      /* the cursor `fn_801F3294` advances */
        /* +0x004 */ u16 part_0x004;       /* the dye pane's armour piece (0 = all) */
    };
    /* +0x006 */ s16 count_0x006;          /* its bound */
    /* +0x008 */ s16 field_0x008;          /* zeroed by the initialiser */
    /* +0x00A */ s16 max_0x00A;            /* `fn_801F353C`'s out_max, read signed by `fn_801F5534` */
    /* +0x00C */ u16 num_0x00C;            /* its out_count, and `fn_801F3EDC`'s answer for kind 0 */
    /* +0x00E */ union {
        /* +0x00E */ u16 field_0x00E;
        /* +0x00E */ s16 anim_0x00E;       /* the value `fn_801EC804` animates */
    };
    /* +0x010 */ union {
        /* +0x010 */ u16 field_0x010;
        /* +0x010 */ s16 cursor_x_0x010;   /* the free colour cursor */
    };
    /* +0x012 */ union {
        /* +0x012 */ u16 field_0x012;
        /* +0x012 */ s16 cursor_y_0x012;
    };
    /* +0x014 */ union {
        struct {
            /* +0x014 */ u32 id_0x014;
            /* +0x018 */ u32 slot_0x018;
            /* +0x01C */ u8 flag_0x01C;    /* mirrored from the world block by `fn_801F3294` */
            /* +0x01D */ u8 unused_0x01D[3];
        }; /* size: 0xC */
        struct {
            /* +0x014 */ s16 sel_id_0x014; /* the colour `fn_801F4444` takes for its "current" row */
            /* +0x016 */ u16 part_colors_0x016[5]; /* the five armour pieces' loaded dye colours */
        }; /* size: 0xC */
    };
    /* +0x020 */ u16 sel_0x020;            /* `fn_801F4444`'s decoded selection */
    /* +0x022 */ u8 sel_flag_0x022;        /* set while the selection is the open one */
    /* +0x023 */ u8 field_0x023;           /* cleared by the initialiser */
    /* +0x024 */ s32 stepper_0x024;       /* the confirm stepper */
    /* +0x028 */ struct LbPage* next_0x028; /* the panel `fn_801F3F78` forwards its byte to */
    /* +0x02C */ u8 unused_0x02C[0x22C];
    /* +0x258 */ u8 rgb_a_0x258[3];        /* the three colour bytes `fn_801F3998` writes (which 1) */
    /* +0x25B */ u8 unused_0x25B;
    /* +0x25C */ u8 rgb_b_0x25C[3];        /* the same three bytes for which 0 */
    /* +0x25F */ u8 unused_0x25F;
    /* +0x260 */ u8 kind_0x260;            /* the kind byte the entry walk switches on */
    /* +0x261 */ u8 unused_0x261;
    /* +0x262 */ u8 sub_0x262;             /* the byte `fn_801F3F78` writes and `fn_801F3F14` matches */
    /* +0x263 */ u8 tail_0x263[];
} LbPage; /* size: 0x263+ (approximate) */

/* The owner object `fn_801F3294`/`fn_801F6A9C` take: it holds the page pointer at +0x10 and the two
 * runs `lb_box_grid_draw` is handed.  Only the offsets those two functions reach are named.
 * size: 0x3B8+ (approximate: 0x3B8 is the highest offset an argument points at) */
typedef struct LbPageOwner {
    /* +0x000 */ u8 unused_0x000[0x10];
    /* +0x010 */ LbPage* page_0x010;       /* the per-page record this unit's other functions work on */
    /* +0x014 */ u8 unused_0x014[0x1C];
    /* +0x030 */ u8 rows_0x030[0x24];      /* the row run `lb_box_grid_draw` reads (second argument) */
    /* +0x054 */ u8 slots_0x054[0x364];    /* the run it fills (first argument) */
    /* +0x3B8 */ u8 tail_0x3B8[];          /* the run it reads (sixth argument) */
} LbPageOwner; /* size: 0x3B8+ (approximate) */

/* One 0xA-byte entry of the lobby selection table at `lbl_805B8674`: the three packed bytes are read as
 * bitfields by `fn_801F4444`/`fn_801F37C8`.  size: 0xA */
typedef struct LbSelRec {
    /* +0x00 */ u16 name_id_0x00; /* the colour's name string */
    /* +0x02 */ u8 match_0x02;   /* the kind byte `fn_801F3F14` matches */
    /* +0x03 */ u8 bits_0x03;    /* the top nibble is a field of the packed value */
    /* +0x04 */ u8 bits_0x04;
    /* +0x05 */ u8 bits_0x05;
    /* +0x06 */ s16 cursor_x_0x06; /* the free colour cursor's spot for the entry */
    /* +0x08 */ s16 cursor_y_0x08;
} LbSelRec; /* size: 0xA */

/* The 0xC-byte per-step argument `fn_801F3294` fills for `pl_equip_model_set`: the first byte is the kind and
 * the rest is left zeroed.  size: 0xC */
typedef struct LbStepArg {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 unused_0x01[0xB];
} LbStepArg; /* size: 0xC */

#ifdef __cplusplus
extern "C" {
#endif

extern LbWorldBlock* lobby_world_block; /* .sbss 0x80794880, the 4-byte pointer the map records */

extern LbSelRec lbl_805B8674[];    /* .data 0x805B8674, 0xA-byte stride selection table */
extern const s16 lbl_805B875C[];   /* .data 0x805B875C, `-2`-terminated index table */
extern const s16 lbl_805B8768[];   /* .data 0x805B8768, `-1`-terminated value table */
extern const s16 lbl_805B8798[];   /* .data 0x805B8798, idem for the other kind */
extern const s16 lb_dye_grid_cells_a[16]; /* .data 0x805B8778, the dye grid's cells (kind 1) */
extern const s16 lb_dye_grid_cells_b[16]; /* .data 0x805B87A8, the dye grid's cells (kind 2) */
extern const s16 lb_color_grid_widths[8]; /* .data 0x805B87C8, the colour grid's row widths */
extern const s16 lb_box_sub_str_bases[8];  /* .data 0x805B85E8, each category's first sub-row string */
extern const u16 lb_box_sub_frame_kinds[8]; /* .data 0x805B8608, the sub frame's animation frame per row count */
extern const u16 lb_box_top_frame[18];     /* .data 0x805B8824, the top list's frame sprites */
extern const u16 lb_box_top_frame_alt[16]; /* .data 0x805B8848, idem off the first page */
extern const u16 lb_box_top_row_panels[8]; /* .data 0x805B8868, the category rows' panels */
extern const u16 lb_box_sub_frame[14];     /* .data 0x805B8878, the sub list's frame sprites */
extern const u16 lb_box_sub_row_panels[8]; /* .data 0x805B8894, the sub rows' panels */
extern const u16 lb_box_guide_frame[24];   /* .data 0x805B88A4, the guide frame's sprites */
extern const u16 lb_box_pouch_tabs[10];    /* .data 0x805B88F0, the pouch tabs' sprites */
extern const u16 lb_count_box_frame[8];    /* .data 0x805B8978, the count box's sprites */
extern const u16 lb_combine_result_rows[8]; /* .data 0x805B8988, the combine result row's sprites */
extern const u16 lb_model_pane_frame[16];  /* .data 0x805B8998, the model pane's frame sprites */
extern const u16 lb_model_pane_rows[12];   /* .data 0x805B89B8, the model pane rows' layout sprites */
extern u16 lb_color_slot_cursor_inner[6];  /* .data 0x805B89F8, the inner colour slot's cursor sprites */
extern const u16 lb_color_pane_cell_lsp[14]; /* .data 0x805B8A80, the colour grid cells' layout sprites */
extern const u16 lb_color_pane_grid_frame[8]; /* .data 0x805B8A9C, the colour grid's frame */
extern const u16 lb_color_pane_free_frame[12]; /* .data 0x805B8AAC, the free colour cell's frame */
extern const u16 lb_color_pane_grid_mark[6]; /* .data 0x805B8AC4, the colour grid's mark */
extern const u16 lb_color_pane_frame[20];  /* .data 0x805B8AD0, the colour pane's frame */
extern const u16 lb_dye_pane_frame[18];    /* .data 0x805B8A10, the dye pane's frame */
extern const u16 lb_dye_part_lsp[10];      /* .data 0x805B8A34, the armour piece rows' layout sprites */
extern const u16 lb_dye_grid_frame[20];    /* .data 0x805B8A48, the dye grid's frame */
extern const u16 lb_dye_part_icon[8];      /* .data 0x805B8A70, the armour piece icons */
extern const u16 lb_my_set_detail_frame[8]; /* .data 0x805B8AF8, the saved set detail's frame */
extern const u16 lb_my_set_status_frame[8]; /* .data 0x805B8B08, the saved set status page's frame */
extern const u16 lb_gun_set_frame[6];      /* .data 0x805B8B18, the gunner set pane's frame */
extern const u16 lb_gun_set_panel[24];     /* .data 0x805B8B24, the gunner set pane's panel */
extern const u16 lb_gun_set_title[10];     /* .data 0x805B8B54, the gunner set pane's title */
extern const u16 lb_box_grid_frame[18];    /* .data 0x805B8904, the item box grid's frame */
extern const u16 lb_box_grid_overlay[16];  /* .data 0x805B8928, the grid's overlay */
extern const u16 lb_box_grid_rows[12];     /* .data 0x805B8948, the grid rows' layout sprites */
extern const u16 lb_box_grid_cols[12];     /* .data 0x805B8960, the grid columns' layout sprites (+8: the cell) */
extern u16 lb_sort_blade_classes[8];       /* .data 0x805B8618, the blade sort menu's row -> weapon class table */
extern const u16 lb_sort_blade_titles[8];  /* .data 0x805B8628, the blade class sort's titles */
extern u16 lb_sort_blade_classes_late[6];  /* .data 0x805B8638, the same once the late weapon classes are unlocked */
extern const u16 lb_sort_blade_titles_late[6]; /* .data 0x805B8644, idem once the late classes are unlocked */
extern const u16 lb_sort_order_titles[6];  /* .data 0x805B8650, the order sort's titles */
extern const u16 lb_sort_gun_titles[6];    /* .data 0x805B865C, the bowgun sort's titles */
extern const u16 lb_sort_gun_order_titles[6]; /* .data 0x805B8668, the bowgun order sort's titles */
extern u16 lbl_805B8810[];         /* .data 0x805B8810, `PutPageArrow`'s sprite rows */
extern const s16 lbl_805B85F8[];   /* .data 0x805B85F8 */
extern u16 lbl_805B88D4[];         /* .data 0x805B88D4, the arrow's sprite rows */
extern u16 lbl_805B8914[];         /* .data 0x805B8914 */

/* The unsplit plain-C callees.  Their map names are bare `fn_XXXXXXXX` stems, so they take C linkage
 * (a C++ declaration would mangle the name objdiff pairs on); each is declared with the argument count
 * and widths the target's call site shows. */
u32 lb_page_record_empty_ck(LbPage* page);
void fn_801EC828(LbPageOwner* owner);
void fn_801EC7AC(LbPageOwner* owner);
u32 userdata_flag_ck(s32 id);
void put_page_count(u16* rows, s16 a, s16 b, u16 c, const _mh_ivec2_* pos);
void sprite_frame_apply(_SPR_DATA_* spr, u32 id, u32 frame, _mh_ivec2_* pos);
void equip_record_copy(LbIconRec* dst, const LbIconRec* src);
void fn_801F353C(const s16* table, s16* out_max, s16* out_count);
void sysSE_stop(s32 id);
void fn_8004D0D8(LbWorldBlock* world, u8 id);
void fn_8004D0E0(LbWorldBlock* world, u8 id);
void fn_8004C038(LbPage* page, LbWorldBlock* world);
void put_button_guide(s32 kind, const _mh_ivec2_* pos);

#ifdef __cplusplus
}
#endif

#if defined(__cplusplus) && !defined(LOBBY_VIEW_IN_NAMESPACE)
/* A C++-linkage declaration must sit at global scope (a namespace would change the mangled name), so a TU that includes this header
 * inside a namespace defines `LOBBY_VIEW_IN_NAMESPACE` and takes these from `unsplit/lobby.h` instead. */

/* The mangled map names are the compiler's spelling of these declarations (rule 9): the front-end
 * reproduces each map name exactly, and the call site writes the plain function. */
void PutPageArrow(u16* table, s16 a, s16 b, u16 c, const _mh_ivec2_* pos, u8 flags);
u32 chk_pointer(void);
void get_lsp_data(u16 id, _mh_ivec2_* out);
u8 get_option_cfg(u8 index);
void sysSE_req(s32 id);
void set_blendmode(u8 a, u8 b, u8 c);
void draw_sprite_idx(u16 id, const _mh_ivec2_* pos);
void draw_sprite_anim_idx(u16 id, u16 kind, const _mh_ivec2_* pos);
void draw_itemicon_item_id(const _SPR_DATA_& spr, u16 id, const _mh_ivec2_* pos);

#endif /* __cplusplus && !LOBBY_VIEW_IN_NAMESPACE */

/* Declarations moved here from `unsplit/lobby.h` (docs/plan.md 6.5 rule 2: the owner declares). */
#ifdef __cplusplus
extern "C" {
#endif

extern u8 jumptable_805B8C74[104];

extern u8 jumptable_805B8CF0[84];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_LOBBY_LB_PANE_UI_H */
