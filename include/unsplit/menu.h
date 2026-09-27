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
s32 fn_803A87E0(void);                  /* 0x803A87E0 */
s32 fn_803A881C(void);                  /* 0x803A881C */
s32 fn_803A8858(void);                  /* 0x803A8858 */
s32 fn_803A9690(void);                  /* 0x803A9690 */
s32 fn_803AA41C(s32, f32);              /* 0x803AA41C */
u32 fn_803AB190(s32);                   /* 0x803AB190 */
u32 fn_803B521C(s32);                   /* 0x803B521C */
void fn_803B6078(u16, u16*);            /* 0x803B6078 */
s32 fn_803B8E1C(void);                  /* 0x803B8E1C */
u8 fn_803BECA0(u8, u8);                 /* 0x803BECA0 */

#ifdef __cplusplus
}
#endif

/* C++-linkage callees the band reads whose owning header is not this band's. */
#ifdef __cplusplus
struct _mh_tex_uv_;
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

#endif /* MHTRI_UNSPLIT_MENU_H */
