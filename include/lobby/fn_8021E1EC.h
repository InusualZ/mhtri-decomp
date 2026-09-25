/* This unit's own types and declarations (`src/lobby/fn_8021E1EC.cpp`).
 *
 * A header rather than the unit's own source because the lobby C++ ABI it declares is spelled with the
 * map's own type names - `_mh_ivec2_` must be a type with exactly that name for MWCC to re-emit
 * `get_lsp_data__FUsP10_mh_ivec2_` - and a name that more than one file needs belongs in one header
 * (docs/plan.md 6.5 rule 1).  NOTE for the next pass: `_mh_ivec2_` is also defined locally by
 * `src/hud/fn_80324F7C.c` (that file's own rule-1 backlog) and by `include/unsplit/lobby.h`; the three
 * copies are identical (`s16 x; s16 y;`) and should collapse to the unsplit lobby header - this unit
 * cannot include that header, because its `lobby_w`/`lbl_80794880` views are not the ones this range
 * reads (see the source's file header).  Recorded as a config_request.
 *
 * docs/plan.md 6.5 rules 3/4/5: every type states its size, every field its offset and a name.
 */
#ifndef MHTRI_LOBBY_FN_8021E1EC_H
#define MHTRI_LOBBY_FN_8021E1EC_H

#include "types.h"
#include "nw4r/math.h"

/* The `.bss` lobby work block (`lobby_w`, 0x806AAB44, 0x17C bytes in the map).  This range reads the
 * four bytes below; the rest belongs to the units that own those offsets. */
typedef struct LbMenuLobbyWork {
    /* +0x000 */ u8 mode_0x000;
    /* +0x001 */ u8 scene_0x001;
    /* +0x002 */ u8 unused_0x002[0x25];
    /* +0x027 */ u8 flag_0x027;
    /* +0x028 */ u8 unused_0x028[0x2A];
    /* +0x052 */ u8 page_0x052;
    /* +0x053 */ u8 unused_0x053[0x5E];
    /* +0x0B1 */ u8 sub_0x0B1;
    /* +0x0B2 */ u8 unused_0x0B2[0xCA];
} LbMenuLobbyWork; /* size: 0x17C (the map's own record) */

/* The block `lbl_80794880` (a 4-byte pointer in `.sbss`) points at.  This range reads the one byte the
 * item-list page mirrors. */
/* One 8-byte item-page row at `LbMenuBigBlock::rows_0x4654`: the row's open/ready bytes and its state
 * word (`fn_802208D4` reads the word, `fn_80220038` sets the two bytes of row 0). */
typedef struct LbMenuRowSlot {
    /* +0x00 */ u8 open_0x00;
    /* +0x01 */ u8 ready_0x01;
    /* +0x02 */ u8 unused_0x02[2];
    /* +0x04 */ u16 state_0x04;
    /* +0x06 */ u8 unused_0x06[2];
} LbMenuRowSlot; /* size: 0x8 */

typedef struct LbMenuBigBlock {
    /* +0x0000 */ u8 unused_0x0000[0x4654];
    /* +0x4654 */ LbMenuRowSlot rows_0x4654[3];  /* the three item rows the page lays out */
    /* +0x466C */ u8 fade_c_0x466C;      /* the third countdown byte `fn_80220114` ticks down */
    /* +0x466D */ u8 unused_0x466D[0x03];
    /* +0x4670 */ u8 count_0x4670;       /* how many item rows the page fills */
    /* +0x4671 */ u8 unused_0x4671[0x1DC];
    /* +0x484D */ u8 list_kind_0x484D;
    /* +0x484E */ u8 unused_0x484E[0x1832];
} LbMenuBigBlock; /* size: 0x6010+ (the neighbour unit's extent; only the bytes above are read) */

/* The item row `fn_802208D4`/`fn_8022097C`/`fn_80221BBC` operate on.  Only the fields this range
 * touches are named. size: 0x64 (approximate: the extent is the largest offset read here). */
typedef struct LbMenuItem {
    /* +0x00 */ u8 unused_0x00;
    /* +0x01 */ u8 active_0x01;
    /* +0x02 */ u8 unused_0x02[0x22];
    /* +0x24 */ u8 flag_a_0x24;
    /* +0x25 */ u8 flag_b_0x25;
    /* +0x26 */ u8 flag_c_0x26;
    /* +0x27 */ u8 flag_d_0x27;
    /* +0x28 */ u8 unused_0x28[0x04];
    /* +0x2C */ u16 icon_0x2C;
    /* +0x2E */ s16 row_0x2E;
    /* +0x30 */ u16 kind_0x30;
    /* +0x32 */ u8 unused_0x32[0x0A];
    /* +0x3C */ u16 value_0x3C;
    /* +0x3E */ u8 unused_0x3E[0x08];
    /* +0x46 */ u8 slot_0x46;
    /* +0x47 */ u8 unused_0x47[0x05];
    /* +0x4C */ u16 base_a_0x4C;
    /* +0x4E */ u16 base_b_0x4E;
    /* +0x50 */ u8 id_0x50;
    /* +0x51 */ u8 prev_0x51;
    /* +0x52 */ u8 color_0x52;
    /* +0x53 */ u8 unused_0x53[0x03];
    /* +0x56 */ u8 flag_e_0x56;
    /* +0x57 */ u8 flag_f_0x57;
    /* +0x58 */ u8 unused_0x58[0x08];
    /* +0x60 */ u16 ids_0x60[];
} LbMenuItem; /* size: 0x64 */

/* The 3x3 float block `fn_802230C8` fills. */
/* The block `fn_80064080` hands out; only its +0x08 word is read here. size: 0xC (approximate). */
typedef struct LbGlobalBlock {
    /* +0x00 */ u8 unused_0x00[8];
    /* +0x08 */ s32 field_0x08;
} LbGlobalBlock; /* size: 0xC */

typedef struct LbMat3x3 {
    /* +0x00 */ f32 m_0x00;
    /* +0x04 */ f32 m_0x04;
    /* +0x08 */ f32 m_0x08;
    /* +0x0C */ f32 m_0x0C;
    /* +0x10 */ f32 m_0x10;
    /* +0x14 */ f32 m_0x14;
    /* +0x18 */ f32 m_0x18;
    /* +0x1C */ f32 m_0x1C;
    /* +0x20 */ f32 m_0x20;
} LbMat3x3; /* size: 0x24 */

/* The screen work block at `lbl_806AA8C8` (0x1C0 bytes in the map).  The range's table is the
 * 0x10-byte run at +0x0C (`index_0x14C` selects one of its rows), and the tail from +0x160 is the
 * 8 x 0xC vector run `fn_8021EFC8` clears. */
typedef struct LbMenuRow {
    /* +0x00 */ u32 table_0x00;    /* the row's table pointer `fn_8021EC20` hands out */
    /* +0x04 */ u32 model_0x04;    /* the row's model pointer `fn_8021EBF0` hands out */
    /* +0x08 */ u8 unused_0x08[8];
} LbMenuRow; /* size: 0x10 */

typedef struct LbMenuScratch {
    /* +0x000 */ u32 unused_0x000;
    /* +0x004 */ s16* fixed_a_0x004;  /* the -2 mode's string row */
    /* +0x008 */ u8* fixed_b_0x008;   /* the -2 mode's model row */
    /* +0x00C */ LbMenuRow rows_0x00C[20];
    /* +0x14C */ s16 index_0x14C;   /* -1 = none, -2 = the two special pointers, >= 0 = a row */
    /* +0x14E */ u8 unused_0x14E[0x02];
    /* +0x150 */ u32 timer_0x150;
    /* +0x154 */ u8 count_0x154;
    /* +0x155 */ u8 unused_0x155[0x0B];
    /* +0x160 */ VEC3 slots_0x160[8];
} LbMenuScratch; /* size: 0x1C0 (the map's own record) */

/* The fallback record `fn_8021E1EC` reads when the entry has no position of its own. */
typedef struct LbMenuFallback {
    /* +0x00 */ u8 unused_0x00[0x10];
    /* +0x10 */ VEC3 pos_0x10;
} LbMenuFallback; /* size: 0x1C */

/* The record `fn_8021E1EC` appends: the two candidate owners, the squared distance and the slot id. */
typedef struct LbMenuCandidate {
    /* +0x00 */ void* rec_0x00;
    /* +0x04 */ void* work_0x04;
    /* +0x08 */ f32 dist_sq_0x08;
    /* +0x0C */ s32 slot_0x0C;
} LbMenuCandidate; /* size: 0x10 */

/* The record `fn_8021E1EC` tracks and `fn_8021E304`/`fn_8021E340` classify: a position vector, the y
 * the range test runs against and the "busy" byte at +0xB03. size: 0xB04 (approximate: the extent is
 * the largest offset this range reads). */
typedef struct LbMenuActor {
    /* +0x000 */ u8 unused_0x000[0x3C];
    /* +0x03C */ VEC3 pos_0x03C;
    /* +0x048 */ u8 unused_0x048[0x88];
    /* +0x0D0 */ u8 unused_0x0D0[0xA33];
    /* +0xB03 */ u8 busy_0xB03;
} LbMenuActor; /* size: 0xB04 */

/* The entry `fn_8021E1EC` matches against: a kind word and two state bytes. size: 0x257 */
typedef struct LbMenuEntryRec {
    /* +0x000 */ u8 unused_0x000[2];
    /* +0x002 */ s8 kind_0x002;
    /* +0x003 */ u8 unused_0x003[0x253];
    /* +0x256 */ u8 mode_0x256;
} LbMenuEntryRec; /* size: 0x257 */

/* The 8-byte row `fn_802208A8` copies whole. */
typedef struct LbMenuRow8 {
    /* +0x00 */ u16 id_0x00;
    /* +0x02 */ u16 value_0x02;
    /* +0x04 */ u8 kind_0x04;
    /* +0x05 */ u8 sub_0x05;
    /* +0x06 */ u16 rate_0x06;
} LbMenuRow8; /* size: 0x8 */

/* The 0x50-byte block at `lbl_806AAA88` (the item-list page's own state). */
typedef struct LbMenuItemState {
    /* +0x00 */ u32 active_0x00;
    /* +0x04 */ u32 stamp_0x04;      /* the tick the page last advanced on (`lbl_80794868`) */
    /* +0x08 */ u8 unused_0x08[0x0C];
    /* +0x14 */ f32 count_0x14;      /* the row count as a float (converted unsigned) */
    /* +0x18 */ f32 step_0x18;       /* how far one row advances per tick */
    /* +0x1C */ u8 unused_0x1C[0x14];
    /* +0x30 */ f32 scroll_0x30;
    /* +0x34 */ f32 depth_0x34;
    /* +0x38 */ s32 selected_0x38;
    /* +0x3C */ s32 changed_0x3C;
    /* +0x40 */ u8 unused_0x40[0x08];
    /* +0x48 */ s8 events_0x48;      /* `CalculateEvents`'s last answer */
    /* +0x49 */ u8 unused_0x49[0x07];
} LbMenuItemState; /* size: 0x50 (the map's own record) */

/* ---------------------------------------------------------------------------------------------------
 * Globals
 */
extern "C" {
extern LbMenuLobbyWork lobby_w;      /* .bss 0x806AAB44 */
extern LbMenuScratch lbl_806AA8C8;   /* .bss 0x806AA8C8 */
extern LbMenuItemState lbl_806AAA88; /* .bss 0x806AAA88 */
extern LbMenuBigBlock* lbl_80794880; /* .sbss 0x80794880, a 4-byte pointer (map: size:0x4 data:4byte) */
extern u8* lbl_80794B18;             /* .sbss 0x80794B18 */
extern const f32 lbl_80799C60;       /* .sdata2 0x80799C60 - the y-window this range tests against */
extern u8* lbl_807922B0[];           /* .sdata 0x807922B0 - the 8-byte row tables `fn_80223A18` indexes */
extern u8* lbl_807922B8[];           /* .sdata 0x807922B8 - the 2-byte row tables `fn_80223A44` indexes */
extern const u16 lbl_80791F30[];     /* .sdata 0x80791F30 */
extern const u16 lbl_805BA820[];     /* .data 0x805BA820 - the item page's sprite rows */
extern const u16 lbl_805BA968[];
extern const u16 lbl_805BA974[];
extern const u16 lbl_805BA980[];
extern const u16 lbl_805BA9E4[];
extern const u16 lbl_805BAA08[];
extern const u16 lbl_805BAA14[];
extern const f32 lbl_80799CD8;       /* .sdata2 0x80799CD8 */
extern const f32 lbl_80799C78;       /* .sdata2 0x80799C78 - the per-row step scale */
extern const f32 lbl_80799C7C;       /* .sdata2 0x80799C7C - the "scrolled to the end" mark */
extern u32 lbl_80794868;             /* .sbss 0x80794868 - the page's tick counter */
s32 CalculateEvents();
extern const f32 lbl_80799CDC;       /* .sdata2 0x80799CDC */
}

/* The 2D integer vector the lobby/HUD helpers exchange (`_mh_ivec2_` in the map's mangling). */
typedef struct _mh_ivec2_ {
    /* +0x00 */ s16 x;
    /* +0x02 */ s16 y;
} _mh_ivec2_; /* size: 0x4 */

/* The lobby UI ABI this range calls.  C++ linkage (the map's `__F...` manglings with C++ parameter
 * types), so they sit outside `extern "C"`; the front-end reproduces the map's names from these exact
 * signatures (rule 9).  They are declared here rather than included from `include/unsplit/lobby.h`
 * because that header's `lobby_w`/`lbl_80794880` views conflict with this unit's (see the file
 * header). */
void* get_lsp_data(u16 id, _mh_ivec2_* out);
void draw_sprite_ary(const u16* table, const _mh_ivec2_* pos);
void draw_sprite_anim_ary(const u16* table, u16 index, const _mh_ivec2_* pos);
void draw_sprite_anim_idx(u16 id, u16 index, const _mh_ivec2_* pos);
void draw_sprite_idx(u16 id, const _mh_ivec2_* pos);
s32 ck_WideMode(void);

/* The map's `calcDistanceSqXZ__FPQ34nw4r4math4VEC3PQ34nw4r4math4VEC3` is a FREE function (`__F`) whose
 * parameters carry the namespace type, so it is declared at global scope (the same shape
 * `include/nw4r/math.h` uses for `setVector3`). */
f32 calcDistanceSqXZ(VEC3* a, VEC3* b);


#endif /* MHTRI_LOBBY_FN_8021E1EC_H */
