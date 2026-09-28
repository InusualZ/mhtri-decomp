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

#ifdef __cplusplus
}
#endif

/* C++-linkage callees the band reads whose owning header is not this band's. */
#ifdef __cplusplus
struct _mh_tex_uv_;
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

#endif /* MHTRI_UNSPLIT_MENU_H */
