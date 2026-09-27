/* This unit's own types and declarations (`src/lobby/fn_801F3294.cpp`).
 *
 * A header rather than the unit's own source because the lobby C++ ABI it declares is spelled with the
 * map's own type names - `_mh_ivec2_` must be a type with exactly that name for MWCC to re-emit
 * `get_lsp_data__FUsP10_mh_ivec2_` - and a name that more than one file needs belongs in one header
 * (docs/plan.md 6.5 rule 1).  `_mh_ivec2_` is also defined by `include/unsplit/lobby.h` and by
 * `src/hud/fn_80324F7C.c` (that file's own rule-1 backlog); the copies are identical (`s16 x; s16 y;`)
 * and this unit's one is here because it cannot include the unsplit header - that header spells
 * `lbl_80794880` as a byte array where this range loads the 4-byte pointer the map records, and it
 * declares `fn_8021213C` with two parameters where this range calls it with one.  Recorded as a
 * config_request.
 *
 * docs/plan.md 6.5 rules 3/4/5: every type states its size, every field its offset and a name.
 */
#ifndef MHTRI_LOBBY_FN_801F3294_H
#define MHTRI_LOBBY_FN_801F3294_H

#include "types.h"
/* The owner's header for `menu_cursor_step` (0x802A8EFC, the menu range `menu/fn_802A6624.cpp` owns): the
 * declaration lives there now and this header re-exports it (docs/plan.md 6.5 rule 2). */
#include "menu/fn_802A6624.h"

/* The 2D integer vector the lobby/HUD helpers exchange (`_mh_ivec2_` in the map's mangling).
 * size: 0x4 */
typedef struct _mh_ivec2_ {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
} _mh_ivec2_;

/* The `.bss` lobby work block (`lobby_w`, 0x806AAB44, 0x17C bytes in the map).  This range reads the
 * scene byte `fn_801F6168` switches on; the rest belongs to the units that own those offsets.
 * size: 0x17C */
typedef struct LbLobbyWork {
    /* +0x000 */ u8 unused_0x000[0x4D];
    /* +0x04D */ s16 scene_0x04D;
    /* +0x04F */ u8 unused_0x04F[0x12D];
} LbLobbyWork; /* size: 0x17C */

/* One 0xC-byte icon record: `fn_801F86FC` copies one from the world table into the caller's array and
 * writes its kind byte when the id is the "none" marker.  size: 0xC */
typedef struct LbIconRec {
    /* +0x00 */ u8 kind_0x00;
    /* +0x01 */ u8 unused_0x01[0xB];
} LbIconRec; /* size: 0xC */

/* The big lobby data block `lbl_80794880` points at (a 4-byte pointer in `.sbss`).  Only the fields
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

/* The sprite-data record the `draw_*` family takes by reference.  Only +0x1C - the colour word
 * `fn_801F60D4` overwrites - is named; `fn_802E0AD4` initialises the rest of the block.
 * size: 0x20 (approximate: the frame `fn_801F60D4` reserves for it) */
typedef struct _SPR_DATA_ {
    /* +0x00 */ u8 unused_0x00[0x1C];
    /* +0x1C */ u32 color_0x1C;
} _SPR_DATA_;

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

extern LbWorldBlock* lbl_80794880; /* .sbss 0x80794880, the 4-byte pointer the map records */
extern LbLobbyWork lobby_w;        /* .bss 0x806AAB44 */

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
u32 fn_8021213C(s32 mask);
s32 fn_802121F4(s32 what);
u16 fn_802122AC(void);
u32 fn_801EC9E0(LbPage* page);
void fn_801EC828(LbPageOwner* owner);
void fn_801EC7AC(LbPageOwner* owner);
void fn_802738E8(LbPage* page, LbStepArg* arg);
void fn_80273998(LbPage* page, s32 kind, u8 value);
u8 fn_802738B8(u8 index);
void fn_80223258(LbPage* page, u8 value);
u32 fn_8004D70C(s32 id);
s32 fn_8004AEC0(LbWorldBlock* world);
void fn_802DB140(u16* rows, s16 a, s16 b, u16 c, const _mh_ivec2_* pos);
void fn_802E0AD4(_SPR_DATA_* spr, u32 id, u8 flag, s32 arg);
void fn_8004A20C(LbIconRec* dst, const LbIconRec* src);
void fn_801F8318(LbPage* self, s16 x, s16 y);
void fn_801F6DBC(void* dst, s16 x, s16 y, u8* ptr);
void fn_801F353C(const s16* table, s16* out_max, s16* out_count);
void fn_801F6168(u8* slots, u8* rows, u32 row, u8 kind, s16 value, u8* extra, LbPage* page);
void fn_800DBC84(s32 id);
void fn_8004D0D8(LbWorldBlock* world, u8 id);
void fn_8004D0E0(LbWorldBlock* world, u8 id);
void fn_8004C038(LbPage* page, LbWorldBlock* world);
void fn_802DF7CC(s32 kind, const _mh_ivec2_* pos);
/* `menu_cursor_step` (0x802A8EFC) stood here as `s32 (s16, s16, u16, s32, s32)` - the call site's narrow
 * view - while its band had no registered unit, and the two spellings could not both be visible
 * ((10505) illegal overloading).  `menu/fn_802A6624.cpp` owns the address and its header
 * `include/menu/fn_802A6624.h` (included below) declares the definition's `s32`/`u16` spelling
 * (docs/plan.md 6.5 rule 2). */

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

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

#endif /* __cplusplus */

#endif /* MHTRI_LOBBY_FN_801F3294_H */
