/*
 * The `menu` band's unowned declarations (`include/unsplit/<module>.h`, rule 2's home when no
 * registered unit owns the symbol).
 *
 * Each name here is a `.text` row in `config/RMHE08/symbols.txt` with no owning registered unit: the
 * cockpit/HUD band around `menu/fn_802E4978.cpp` (0x802E4978-0x802E7408) and its neighbours.  A symbol
 * a registered unit owns is *not* declared here - the owner's header carries it and the consumer
 * includes that header: `menu/fn_802E4978.cpp` includes `fn_8004CAD8.h`, `ef/fn_800CDB2C.h`,
 * `Runtime.PPCEABI.H/memset.h`, `Pl/pl_act.h`, `menu/menu_item.h`, `ai/fn_802D44F4.h` and
 * `enemy/fn_80382310.h` for its callees, and the six owner headers that cannot be included from that
 * unit (with the clash that blocks each) are named in `include/menu/fn_802E4978.h`, which declares
 * those callees as the unit's own view.
 *
 * The C++-linkage callees (a mangled map name) are declared with their real signatures so the
 * front-end reproduces the map's mangling (rule 9); `get_lsp_data`, the `draw_*` family and
 * `font_set_size` come from `include/unsplit/lobby.h`, `set_blendmode` from `include/lobby/...`.
 */
#ifndef MHTRI_UNSPLIT_MENU_H
#define MHTRI_UNSPLIT_MENU_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the cockpit band's own callees (unregistered `.text`) ---- */
void fn_802DFCD4(void);                 /* 0x802DFCD4 */
void draw_lsp_parts(void);                 /* 0x802DFEBC */
void fn_802DFD38(void);                 /* 0x802DFD38 */
void draw_lsp_element(void*, u8, u8, u16);   /* 0x802E00A4 */
void fn_802E03D4(void*, s32, void*);    /* 0x802E03D4 */
void fn_802E0468(u8, s32);              /* 0x802E0468 */
s32 fn_802E06B0(u16, void*, void*);     /* 0x802E06B0 */
/* The five symbols below moved to their owner's header when proposal `803A3A50_fn_803A3A50`
 * registered `src/lobby/lb_quest_screen.cpp` over them (docs/plan.md 6.5 rule 2); this include
 * re-exports them for the units that already take this header. */
#include "lobby/lb_quest_screen.h"
/* `fn_803BECA0` (0x803BECA0) and `get_arena_cfg` (0x803BEBF0) are no longer declared here: the
 * range that defines them, `src/menu/get_pop_dat_ptr.cpp`, owns them and publishes both in
 * `include/menu/get_pop_dat_ptr.h`, which this header includes below (rule 2). */

/* The quest-work accessors this band used to declare moved to `include/lobby/lb_quest_screen.h`
 * for the same reason - that unit's registered range covers their addresses, and this header
 * includes it above. */

#ifdef __cplusplus
}
#endif

/* C++-linkage callees the band reads whose owning header is not this band's. */
#ifdef __cplusplus
struct _mh_tex_uv_;
struct _mh_ivec2_;
u8 get_arena_cfg(u8, u8);
u8 get_now_areano(void);
void set_zmode(u8, u8, u8);
void set_blendmode(u8, u8, u8);
void subTransSetPrio(u8, u32, u32, u32*);
u8** get_str_tbl(s32);

void flfntSetColor(u32);
void font_flush(void);
void font_locate(s16, s16);
void font_print(char*);
void font_print_ex(s16, s16, s16, char*, void*);
void drawshape_init(u8, u16);
void drawshape_set_vertex_array(const _mh_ivec2_*);
void drawshape_set_vertex_rect(s16, s16, s16, s16);
void drawshape_set_flat_color(u32);
void drawshape_set_texture_array(u16, const _mh_tex_uv_*, s32);
void drawshape_exec(void);
#endif

/* The `src/menu/get_pop_dat_ptr.cpp` band's own unowned data (its `.text` 0x803BE30C..0x803C4BA0 is
 * registered, so the symbols below resolve to no owning unit's data range and this is their rule-2
 * home).  Each is declared, never defined here. */
extern u8 option_w[];           /* .bss 0x80659090, 0x24 B - the option table `get_option_cfg` reads */
extern u8 option_value_max[];   /* .data 0x805F8528 - the per-index ceiling `ck_option_cfg` clamps to */
extern u8 option_default_tbl[]; /* .data 0x805F8548 - the 31-byte default table */
extern u8 option_value_tbl[];   /* .sdata 0x807936B8 - option id -> (value, flag) byte pairs */
extern const char pop_file_name_c[]; /* .data 0x805F84E0 - "m%03d_%06d_c_pop.dat" */
extern const char pop_file_name_f[]; /* .data 0x805F84F8 - "06/m%03d_%06d_f_pop.dat" */
extern const char pop_file_name_r[]; /* .data 0x805F8510 - "06/m%03d_%06d_r_pop.dat" */
extern u32 demo_init_word_0;   /* .sdata 0x80790E7C */
extern u32 demo_init_word_1;   /* .sdata 0x80790E80 */
extern u32 demo_saved_words[2]; /* .sbss 0x80794C88, two words */

/* The band's `.bss` work blocks.  dtk's split objects carry no `.bss` content, so no unit claims
 * these and this header is their rule-2 home; both are referenced only from inside the band. */
typedef struct PopData PopData;

/* One pop-list entry.  The stride is 0x1A4; only the bytes the band's accessors touch are named.
 * size: 0x1A4 */
typedef struct PopEntry {
    /* +0x000 */ u8 pad_0x000[0x1];
    /* +0x001 */ u8 field_0x001;
    /* +0x002 */ u8 field_0x002;
    /* +0x003 */ u8 pad_0x003[0x2];
    /* +0x005 */ u8 count_0x005;  /* the byte `counter_inc` advances */
    /* +0x006 */ u8 pad_0x006[0xE2];
    /* +0x0E8 */ u32 field_0x0E8;
    /* +0x0EC */ u16 field_0x0EC;
    /* +0x0EE */ u8 pad_0x0EE[0x5E];
    /* +0x14C */ u16 field_0x14C;
    /* +0x14E */ u8 pad_0x14E[0x56];
} PopEntry; /* size: 0x1A4 */

/* One 0x14-byte demo sub-record - the block at `demo_work` +0x14 the demo accessors test. size: 0x14 */
typedef struct DemoEntry {
    /* +0x00 */ u8 flag_0x00;
    /* +0x01 */ u8 pad_0x01[0x7];
    /* +0x08 */ u8 flag_0x08;
    /* +0x09 */ u8 pad_0x09[0xB];
} DemoEntry; /* size: 0x14 */

/* The demo/event work `demo_work` (.bss 0x806D2B20, 0x28 B).  size: 0x28 */
typedef struct DemoWork {
    /* +0x00 */ u8 state_0x00;   /* 1 = running, 4 = the demo the game asks about */
    /* +0x01 */ u8 pad_0x01;
    /* +0x02 */ u8 demo_no_0x02; /* `get_demo_no`, 0xFF = none */
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ u32 field_0x04;
    /* +0x08 */ u8 field_0x08;
    /* +0x09 */ u8 pad_0x09[0x3];
    /* +0x0C */ u8 field_0x0C;
    /* +0x0D */ u8 field_0x0D;
    /* +0x0E */ u8 pad_0x0E[0x6];
    /* +0x14 */ DemoEntry entry_0x14;
} DemoWork; /* size: 0x28 */

extern PopData* pop_dat_ptrs[10]; /* .bss 0x806D2AF8, ten pointers */
extern DemoWork demo_work;        /* .bss 0x806D2B20 */

/* `get_option_cfg`, `get_arena_cfg`, `get_cfg` and `event_demo_ck` are this band's own - their
 * declarations come from the owner's header (rule 2). */
#include "menu/get_pop_dat_ptr.h"

/* ---- the 0x803A/0x803B arena/quest-result band (proposal `803B0F98`) ----
 *
 * The records and globals `src/menu/arena_result.cpp` reads.  All of them belong to no registered
 * unit: `quest_work` is the 0x6AB8-byte block `quest_init` (0x803AD47C) memsets and stores into
 * `quest_work_ptr`, and the band's string tables sit in `.data`/`.sdata` next to it.
 */
/* One item slot of the result screen's per-player item list. size: 0x4 */
typedef struct QuestItemSlot {
    /* +0x000 */ u16 id;      /* the item id the count belongs to */
    /* +0x002 */ s16 count;
} QuestItemSlot;
/* One 0x6-byte entry of the arena item table at `quest_work.arena_items_0x6A2C`: the item `id`, the
 * `count` the caller matches on, and the `remaining` count `quest_element_item_apply` spends.
 * size: 0x6 */
typedef struct QuestArenaItem {
    /* +0x000 */ u16 id;
    /* +0x002 */ u16 count;
    /* +0x004 */ u16 remaining;
} QuestArenaItem;
/* The record `quest_work.pad_0x03C` points at: one player's quest/arena result row.  Only the
 * offsets this band reads are named, and the whole tail below +0x372 is not evidenced.
 * size: 0x714 (lower bound) */
typedef struct QuestRecord {
    /* +0x000 */ u8 unused_0x000[0x02C];
    /* +0x02C */ u16 field_0x02C;     /* the clear time in frames (compare against 0x2710/0x2328) */
    /* +0x02E */ u8 unused_0x02E[0x05D];
    /* +0x08B */ u8 field_0x08B;      /* index into string table 5 */
    /* +0x08C */ u8 unused_0x08C[0x0AE];
    /* +0x13A */ s16 field_0x13A;     /* formatted through string table 6's format 0 */
    /* +0x13C */ u8 unused_0x13C[0x05C];
    /* +0x198 */ u8 field_0x198;      /* index into string table 41 */
    /* +0x199 */ u8 unused_0x199[0x173];
    /* +0x30C */ s32 field_0x30C;
    /* +0x310 */ u8 unused_0x310[0x038];
    /* +0x348 */ s32 field_0x348;
    /* +0x34C */ s32 field_0x34C;
    /* +0x350 */ s32 field_0x350;
    /* +0x354 */ s32 field_0x354;
    /* +0x358 */ u8 unused_0x358[0x01A];
    /* +0x372 */ u16 field_0x372;
    /* +0x374 */ u8 unused_0x374[0x3A0];
} QuestRecord;
/* One 0x60-byte entry of the quest work block's element array at `quest_work` +0x94: `flags` gates
 * the entry, `id` is the u16 the callers match on and `value` is the count/target they compare.
 * `key_bytes` is the same +0x04..+0x06 pair as three bytes - `quest_players_state_get` reads the
 * three bytes of entry 6. size: 0x60 */
typedef struct QuestElement {
    /* +0x00 */ u32 flags;
    union {
        struct {
            /* +0x04 */ u16 id;
            /* +0x06 */ s16 value;
        };
        /* +0x04 */ u8 key_bytes[3];
    };
    /* +0x07 */ u8 unused_0x07[0x59];
} QuestElement;
/* The quest/arena work block (`quest_work`, .bss 0x806C5858, 0x6AB8 B - the size `quest_init`
 * memsets).  `quest_work_ptr` (.sbss 0x80794C40) is the same object's address.  Only the offsets
 * the band reads are named, in ascending order. size: 0x6AB8 */
typedef struct QuestWork {
    /* +0x0000 */ u8 unused_0x0000[0x01C];
    /* +0x001C */ s32 field_0x01C;              /* a clear-time-like pair with +0x020 */
    /* +0x0020 */ s32 field_0x020;
    /* +0x0024 */ s32 field_0x024;
    /* +0x0028 */ u8 unused_0x0028[0x014];
    /* +0x003C */ QuestRecord* record_0x03C;    /* the current result row, 0 when there is none */
    /* +0x0040 */ u8 unused_0x0040[0x050];
    /* +0x0090 */ s8 field_0x090;               /* the two bytes `sprintf` formats as the score */
    /* +0x0091 */ s8 field_0x091;
    /* +0x0092 */ u8 unused_0x0092[0x002];
    /* +0x0094 */ QuestElement elements_0x0094[3];
    /* +0x01B4 */ u8 unused_0x01B4[0x120];      /* entries 3.. of the same 0x60-stride array: the
                                                 * callers that reach them use their absolute offsets
                                                 * below (entry 6's +0x04..+0x06 is +0x2D4) */
    /* +0x02D4 */ u8 player_state_0x2D4[3];     /* entry 6's three key bytes */
    /* +0x02D7 */ u8 unused_0x02D7[0x011];
    /* +0x02E8 */ s32 field_0x2E8;              /* clamped to 0 before it is formatted */
    /* +0x02EC */ u8 unused_0x02EC[0x2EC];
    /* +0x05D8 */ u16 field_0x5D8;              /* the run's point score */
    /* +0x05DA */ u8 unused_0x05DA[0x60BE];
    /* +0x6698 */ u16* slot_values_0x6698[8];   /* the eight u16 stacks `quest_slot_items_get` copies */
    /* +0x66B8 */ s32 slot_counts_0x66B8[8];
    /* +0x66D8 */ u8 unused_0x66D8[0x080];
    /* +0x6758 */ s32 field_0x6758;             /* quest_init stores 0x19 here */
    /* +0x675C */ u8 unused_0x675C[0x004];
    /* +0x6760 */ u8 field_0x6760[8];
    /* +0x6768 */ u8 unused_0x6768[0x20D];
    /* +0x6975 */ u8 field_0x6975;             /* the screen's active flag */
    /* +0x6976 */ u8 unused_0x6976[0x002];
    /* +0x6978 */ u8 field_0x6978;             /* the screen phase the dispatchers switch on */
    /* +0x6979 */ u8 unused_0x6979[0x003];
    /* +0x697C */ s32 field_0x697C;
    /* +0x6980 */ u8 unused_0x6980[0x0AA];
    /* +0x6A2A */ s8 count_0x6A2A;             /* entries in the arena item table below */
    /* +0x6A2B */ u8 unused_0x6A2B;
    /* +0x6A2C */ QuestArenaItem arena_items_0x6A2C[3];
    /* +0x6A3E */ u8 unused_0x6A3E[0x02C];
    /* +0x6A6A */ QuestItemSlot player_items_0x6A6A[4][3];
    /* +0x6A9A */ u8 unused_0x6A9A[0x01E];
} QuestWork;
extern QuestWork quest_work;              /* .bss 0x806C5858 */
extern QuestWork* quest_work_ptr;         /* .sbss 0x80794C40, set by `quest_init` */
extern char quest_text_buffer[0x100];     /* .bss 0x806CC310, the 100-byte text scratch */
/* The band's three `quest_list_*` globals: an item array, its u16 key/value array and the count
 * (`quest_work_word_get` walks them). */
typedef struct QuestListItem {
    /* +0x02C */ u16 field_0x02C;
    /* +0x02E */ u8 unused_0x02E[0x002];
} QuestListItem; /* size: 0x30 (lower bound) */
extern QuestListItem** quest_list_items;   /* .sbss 0x80794C3C */
extern u16* quest_list_values;             /* .sbss 0x80794C24 */
extern s32 quest_list_count;               /* .sbss 0x80794C44 */
/* The shared screen block `Screen_w` (.bss 0x8065903C, 0x54 B): +0x14 is the frame duration the
 * clear-time formatters divide by and the frame scale the band's `draw_*` calls use.  Moved here
 * from `src/menu/fn_802E4978.cpp`, which defined the same view locally - the second user is when a
 * type moves into a header (rule 1). size: 0x54 */
typedef struct ScreenGeomView {
    /* +0x00 */ u8 unused_0x00[0x14];
    /* +0x14 */ f32 field_0x14;
    /* +0x18 */ u8 unused_0x18[0x54 - 0x18];
} ScreenGeomView;
extern "C" ScreenGeomView Screen_w;
/* The band's own data tables. */
extern u8 quest_pair_table[];      /* .data 0x805F7898, read [index * 2 + sub] */
extern u8 quest_byte_table[];      /* .data 0x805F78B4 */
extern u8* arena_time_table[];     /* .data 0x805F7AF8, 12 pointers to u16 time tables */
extern char* quest_grade_none_text_table[];   /* .data 0x8060DAD8, indexed by `system_w`'s map index */
extern u16 multi_arena_clr_time[];     /* .data 0x805F7B28, 10 u16 pairs */
/* Pooled float constants the target objects address as globals (playbook 29: declare, never
 * define - a definition would make MWCC emit a second copy in `.sdata2`). */
extern const f32 frames_per_second_60f;   /* .sdata2 0x8079C524 */
extern const f32 percent_scale_100f;      /* .sdata2 0x8079C558 */
extern const f32 quest_grade_ratio_10f;   /* .sdata2 0x8079C540 */
extern const f32 quest_grade_ratio_30f;   /* .sdata2 0x8079C55C */
extern const f32 quest_grade_ratio_50f;   /* .sdata2 0x8079C520 */
/* The band's unowned callees (C++ linkage: the declaration reproduces the map's mangling, rule 9). */
/* The band's unowned callees (C linkage: the map rows are plain names, so a fixed name here has to
 * mangle to the same spelling).  Each is one small accessor of the quest work area `quest_element_*`
 * (0x60-byte entries at `quest_work` +0x94) or of the result record's +0x310 flag word. */
extern "C" {
}
#endif /* MHTRI_UNSPLIT_MENU_H */
