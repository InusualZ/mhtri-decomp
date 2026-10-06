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
    /* +0x0003 */ u8 unused_0x0003[0x9B];
    /* +0x009E */ u8 count_0x009E;    /* the same byte for the page the range scrolls */
    /* +0x009F */ u8 unused_0x009F[0xD61];
    /* +0x0E00 */ LbIconRec entries_0x0E00[1]; /* the icon table `fn_801F86FC` indexes by entry id */
    /* +0x0E0C */ u8 unused_0x0E0C[0x2BAE];
    /* +0x39BA */ u8 field_0x39BA;     /* the byte `fn_801F3DEC` copies into the page's mode */
    /* +0x39BB */ u8 tail_0x39BB[];
} LbWorldBlock; /* size: 0x39BB+ */

/* The per-page record this unit's page functions work on (the object `fn_801F3DEC` initialises and
 * `fn_801F3294` drives as its request).  Every offset below is one the disassembly reads or writes; the
 * intermediate ranges are padding and carry theirs.
 * size: 0x263+ (approximate: 0x262 is the highest offset this unit reaches) */
typedef struct LbPage {
    /* +0x000 */ u8 state_0x000;           /* `fn_801F3294`'s step, 0..2 */
    /* +0x001 */ u8 done_0x001;            /* set when the page has finished, read at the step change */
    /* +0x002 */ u8 mode_0x002;            /* 0 = the scrolling page, 1 = the fixed one */
    /* +0x003 */ u8 kind_0x003;            /* the entry kind, 0..2 */
    /* +0x004 */ s16 value_0x004;          /* the cursor `fn_801F3294` advances */
    /* +0x006 */ s16 count_0x006;          /* its bound */
    /* +0x008 */ s16 field_0x008;          /* zeroed by the initialiser */
    /* +0x00A */ s16 max_0x00A;            /* `fn_801F353C`'s out_max, read signed by `fn_801F5534` */
    /* +0x00C */ u16 num_0x00C;            /* its out_count, and `fn_801F3EDC`'s answer for kind 0 */
    /* +0x00E */ u16 field_0x00E;
    /* +0x010 */ u16 field_0x010;
    /* +0x012 */ u16 field_0x012;
    /* +0x014 */ union {
        /* +0x014 */ u32 id_0x014;         /* read 32-bit by `fn_801F37C8` for its "current" row */
        /* +0x014 */ s16 sel_id_0x014;     /* ... and 16-bit by `fn_801F4444` (narrow load) */
    };
    /* +0x018 */ u32 slot_0x018;           /* `fn_801F37C8` packs the selected row's bytes into it */
    /* +0x01C */ u8 flag_0x01C;            /* mirrored from the world block by `fn_801F3294` */
    /* +0x01D */ u8 unused_0x01D[3];
    /* +0x020 */ u16 sel_0x020;            /* `fn_801F4444`'s decoded selection */
    /* +0x022 */ u8 sel_flag_0x022;        /* set while the selection is the open one */
    /* +0x023 */ u8 field_0x023;           /* cleared by the initialiser */
    /* +0x024 */ u8 unused_0x024[4];
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
 * runs `fn_801F6168` is handed.  Only the offsets those two functions reach are named.
 * size: 0x3B8+ (approximate: 0x3B8 is the highest offset an argument points at) */
typedef struct LbPageOwner {
    /* +0x000 */ u8 unused_0x000[0x10];
    /* +0x010 */ LbPage* page_0x010;       /* the per-page record this unit's other functions work on */
    /* +0x014 */ u8 unused_0x014[0x1C];
    /* +0x030 */ u8 rows_0x030[0x24];      /* the row run `fn_801F6168` reads (second argument) */
    /* +0x054 */ u8 slots_0x054[0x364];    /* the run it fills (first argument) */
    /* +0x3B8 */ u8 tail_0x3B8[];          /* the run it reads (sixth argument) */
} LbPageOwner; /* size: 0x3B8+ (approximate) */

/* One 0xA-byte entry of the lobby selection table at `lbl_805B8674`: the three packed bytes are read as
 * bitfields by `fn_801F4444`/`fn_801F37C8`.  size: 0xA */
typedef struct LbSelRec {
    /* +0x00 */ u8 unused_0x00[2];
    /* +0x02 */ u8 match_0x02;   /* the kind byte `fn_801F3F14` matches */
    /* +0x03 */ u8 bits_0x03;    /* the top nibble is a field of the packed value */
    /* +0x04 */ u8 bits_0x04;
    /* +0x05 */ u8 bits_0x05;
    /* +0x06 */ u8 unused_0x06[4];
} LbSelRec; /* size: 0xA */

/* The 0xC-byte per-step argument `fn_801F3294` fills for `fn_802738E8`: the first byte is the kind and
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
extern u16 lbl_805B8810[];         /* .data 0x805B8810, `PutPageArrow`'s sprite rows */
extern const s16 lbl_805B85F8[];   /* .data 0x805B85F8 */
extern u16 lbl_805B88D4[];         /* .data 0x805B88D4, the arrow's sprite rows */
extern u16 lbl_805B8914[];         /* .data 0x805B8914 */

/* The unsplit plain-C callees.  Their map names are bare `fn_XXXXXXXX` stems, so they take C linkage
 * (a C++ declaration would mangle the name objdiff pairs on); each is declared with the argument count
 * and widths the target's call site shows. */
u32 fn_801EC9E0(LbPage* page);
void fn_801EC828(LbPageOwner* owner);
void fn_801EC7AC(LbPageOwner* owner);
u32 userdata_flag_ck(s32 id);
void fn_802DB140(u16* rows, s16 a, s16 b, u16 c, const _mh_ivec2_* pos);
void sprite_frame_apply(_SPR_DATA_* spr, u32 id, u8 flag, s32 arg);
void equip_record_copy(LbIconRec* dst, const LbIconRec* src);
void fn_801F8318(LbPage* self, s16 x, s16 y);
void fn_801F6DBC(void* dst, s16 x, s16 y, u8* ptr);
void fn_801F353C(const s16* table, s16* out_max, s16* out_count);
void fn_801F6168(u8* slots, u8* rows, u32 row, u8 kind, s16 value, u8* extra, LbPage* page);
void sysSE_stop(s32 id);
void fn_8004D0D8(LbWorldBlock* world, u8 id);
void fn_8004D0E0(LbWorldBlock* world, u8 id);
void fn_8004C038(LbPage* page, LbWorldBlock* world);
void fn_802DF7CC(s32 kind, const _mh_ivec2_* pos);

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
