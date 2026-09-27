/*
 * The per-player move-work update - `move_work_update`, `.text` 0x803250B0-0x803253BC (0x30C B), plus the
 * range's own extab 0x80016224-0x8001622C and extabindex 0x8003519C-0x800351A8 (one framed function).
 *
 * It walks the 0xB20-byte move-work records `get_move_work_adrs(2)` hands back: for each one it resolves
 * the record's 6-byte HUD key through the 0x130-stride table `lbl_806BE340` (`hud_key_lookup`, 0xFF for "not
 * present"), asks `draw_lsp_element` for the record's on-screen position and hands it to `move_work_draw` as a
 * 4-byte `_mh_ivec2_`.  It then walks the 0x12 lobby NPC records (`lb_npc`, stride 0x268) and, when the
 * lobby is idle, lets `lb_npc_move_work_draw` tick the live ones; the lobby's own screen state (`lobby_w`) and
 * `system_w`'s +0x20 byte pick between the plain and the expanded layouts, and the closing
 * `menu_slot_panel_draw(0)`/`move_work_name_draw(work)` pair runs unconditionally.
 *
 * Language and module.  Module `hud` from the naming scheme of the band: the range below is the landed
 * `hud/fn_80324F7C.c`, which this unit calls with exactly such a move-work record and which reads the
 * same `.bss` keys; no `__FILE__` string covers the range (the target object owns no `.data`/`.rodata` at
 * all).  The extension is `.cpp` because the range's whole callee surface is C++: the target relocates
 * against `get_move_work_adrs__FUc`, `get_move_work_max__FUc`, `set_zmode__FbUcb`,
 * `set_blendmode__FUcUcUc` and `get_option_cfg__FUc`, and rule 9 forbids spelling those manglings as
 * identifiers - written at C++ scope with their real signatures the front-end reproduces them byte by byte.
 *
 * Names.  No name this file defines or calls comes from the shared runtime dump: `DumpSymbols.zip`
 * answers `zz_` for every one of those 18 addresses, and the two non-`zz_` rows it does carry in the band
 * contradict the code at their addresses - `SaveLoad::DidGameIDChange(void)` at 0x800CF384 and
 * `BTM_IsDeviceUp` at 0x800D0708 are both one-byte `system_w` accessors (`+0x27`, and `+0x7D3 == 1`), so
 * neither is evidence for its address.  Every name below is therefore derived from the callee's own
 * body - the load/call surface quoted next to each declaration - and is a guess a later pass may refine.
 *
 * Residuals and recorded choices:
 *   * The callee declarations are this unit's own view, in the shape `include/menu/fn_802E4978.h` and
 *     `include/hud/layout.h` record for their bands: the C-linkage names sit in one `extern "C"` block,
 *     and the mangled ones at C++ scope.  `menu_slot_panel_draw`'s own header declares it `void` while its body takes the
 *     u8 index this call site passes (`clrlwi r3,r3,24`), and `menu_busy_ck`'s header says `u32` while
 *     every call site here compares with a signed `cmpwi`, so both are declared to this call site.
 *   * `lbl_806BE340` keeps the map's stem: three units read the same 0xBE0 `.bss` block with three
 *     different views (`menu/menu_item.cpp`'s option table, `menu/fn_802A6624.cpp`'s item records, this
 *     range's keyed HUD table), so naming it is a shared-file decision, and rule 7 does not cover `lbl_`.
 *   * `HudMoveWork` / `HudKeyTableEntry` / `HudLobbyWork` are per-unit views: `lobby_w` already has four
 *     differently-named views in `include/` (each unit reads different offsets) and this range reads four
 *     more, and `src/hud/fn_80324F7C.c` owns a `_mh_move_work_` that names only +0xB01 - the one shared
 *     home for the pair is a config_request, recorded rather than guessed.
 *
 * This object reproduces the target exactly: `.text` (0x30C), `extab` (8) and `extabindex` (0xC) are
 * byte-identical and all 50 relocations agree on offset, type, addend and target section - so the unit is
 * `Matching`.  Two source shapes are load-bearing and are recorded here rather than in code comments:
 *   * the local declaration order (work/base first, `j` before `p`/`i`) is what decides MWCC's web list
 *     and therefore which callee-saved register each value gets; every other instruction is unaffected;
 *   * `pos.x = pos.y = 0;` is the chained assignment on purpose - MWCC emits the two halfword zero
 *     stores to the ivec in reverse offset order, which is what the target has; two separate statements
 *     emit them the other way round.
 * The residual that remains is only the compiler's own local name: the target's extabindex reloc names
 * `@etb_80016224`, ours emits the auto-generated `@937` for the same section-relative address.
 */

#include "types.h"
#include "hud/layout.h"      /* _mh_ivec2_, the map's own type name */
#include "lobby/lb_npc.h"    /* _LB_NPC, the map's own class name (`lb_npc_Get_motion_no__FP7_LB_NPC`) */
#include "unsplit/unknown.h" /* SystemWork / system_w */

/* One 0x130-byte entry of the keyed table `lbl_806BE340` (10 entries, the map's own 0xBE0 bytes).
 * `hud_key_lookup` identifies an entry by memcmp-ing this range's 6-byte key against the entry's +0x03, and
 * the two payload bytes `draw_lsp_element` takes come out of it.  The rest of the entry is the table's own
 * data, untouched here (rule 5's padding exception).  size: 0x130 */
typedef struct HudKeyTableEntry {
    /* +0x000 */ u8 active_0x000;     /* non-zero while the entry is loaded */
    /* +0x001 */ u8 pad_0x001[0x2];
    /* +0x003 */ u8 key_0x003[0x6];   /* the id `hud_key_lookup` matches */
    /* +0x009 */ u8 pad_0x009[0x1B];
    /* +0x024 */ u16 id_0x024;        /* draw_lsp_element's fourth argument */
    /* +0x026 */ u8 pad_0x026[0xF1];
    /* +0x117 */ u8 index_0x117;      /* draw_lsp_element's second argument */
    /* +0x118 */ u8 pad_0x118[0x18];
} HudKeyTableEntry; /* size: 0x130 */

/* One player's 0xB20-byte move-work record - the element `get_move_work_adrs(2)` indexes and
 * `src/hud/fn_80324F7C.c` takes (its own view names the +0xB01 byte, this one names the three this
 * range reads).  The stride is the `addi r26,r26,2848` below.  size: 0xB20 */
typedef struct HudMoveWork {
    /* +0x000 */ u8 pad_0x000[0x2];
    /* +0x002 */ u8 index_0x002;      /* draw_lsp_element's third argument */
    /* +0x003 */ u8 pad_0x003[0x7];
    /* +0x00A */ u8 field_0x00A;
    /* +0x00B */ u8 pad_0x00B[0x1];
    /* +0x00C */ u16 field_0x00C;     /* == 4 selects the plain indicator layout */
    /* +0x00E */ u8 pad_0x00E[0x5CD];
    /* +0x5DB */ u8 key_0x5DB[0x6];   /* the key `hud_key_lookup` matches against the HUD table */
    /* +0x5E1 */ u8 pad_0x5E1[0x520];
    /* +0xB01 */ u8 flags_0xB01;      /* non-zero while the record has a live indicator to draw */
    /* +0xB02 */ u8 pad_0xB02[0x1E];
} HudMoveWork; /* size: 0xB20 */

/* The lobby work block (`lobby_w`, .bss 0x806AAB44, 0x17C B) as this range sees it: the scene/mode byte
 * and the three gate bytes below are the only ones it touches.  size: 0x17C */
typedef struct HudLobbyWork {
    /* +0x000 */ u8 mode_0x000;
    /* +0x001 */ u8 pad_0x001[0x3];
    /* +0x004 */ u8 field_0x004;
    /* +0x005 */ u8 field_0x005;
    /* +0x006 */ u8 pad_0x006[0x20];
    /* +0x026 */ u8 field_0x026;
    /* +0x027 */ u8 pad_0x027[0x155];
} HudLobbyWork; /* size: 0x17C */

extern HudKeyTableEntry lbl_806BE340[]; /* .bss 0x806BE340, 0xBE0 B = 10 entries */
extern HudMoveWork* get_move_work_adrs(u8 kind);
extern u16 get_move_work_max(u8 kind);
extern HudLobbyWork lobby_w;            /* .bss 0x806AAB44 */
extern _LB_NPC lb_npc[0x12];            /* .bss 0x806A7BA0, 0x2B50 B = 0x12 x 0x268 */
extern u8 get_option_cfg(u8 index);     /* 0x803BEC70 -> get_option_cfg__FUc */
extern void set_zmode(bool first, u8 mode, bool second); /* 0x800E4688 -> set_zmode__FbUcb */
extern void set_blendmode(u8 a, u8 b, u8 c);              /* 0x800E4574 -> set_blendmode__FUcUcUc */

/* The C-linkage half of the call surface (the map's `fn_` stems, unregistered addresses): this unit's
 * own view until an owner header carries them. */
extern "C" {
bool camera_work_ck(void);                        /* 0x802BE3EC - `camera work +0x284 == 1` */
u32 my_player_no(void);                          /* 0x800CF384 - `system_w +0x27`, the local player's index */
u32 game_ready_ck(void);                          /* 0x800D0708 - `system_w +0x7D3 == 1`, the field's only writer clears it */
u8 hud_key_lookup(const void* key);                /* 0x8033A0BC - memcmps 6 bytes over the 10x0x130 table, 0xFF = absent */
s16 draw_lsp_element(void* work, u8 index, u8 kind, u16 id); /* 0x802E00A4 - draws the element's sprites/text, returns its -0x12 y offset */
void move_work_draw(void* work, const void* pos, u32 draw); /* 0x80324304 - projects the position and draws the move-work element */
u32 map22_camera_ck(void);                        /* 0x803253BC - 0 only in map 0x16/area 2 with the camera >= 1300.0f away */
void lb_npc_move_work_draw(_LB_NPC* npc);         /* 0x80324544 - a character joint's world position, projected and drawn */
void hud_key_table_draw(void);                    /* 0x803248D4 - draws the 10 entries of the keyed table, via move_work_draw */
void draw_lsp_anim_ary(void);                     /* 0x80324CC4 - the animated sprite runs (draw_sprite_anim_ary/draw_sprite) */
void move_indicator_draw(void* work);             /* 0x80324F7C - the sibling unit's own indicator draw */
s32 menu_busy_ck(void);                           /* 0x802A02CC - the 0-argument wrapper of the menu slot's own busy check at 0x802A02D4 */
void draw_lsp_sprite_variant(u32 lsp, u32 index); /* 0x802DF758 - get_lsp_data(lsp) + two draw_sprite_ary runs */
void note_box_draw(void);                         /* 0x80383AE4 - the note layout state's box and its get_str_tbl(0x58) string */
void draw_lsp_parts(void);                        /* 0x802DFEBC - set_blendmode(4,5,1) + three state-flag-gated sub-blits */
void menu_slot_panel_draw(u8 index);              /* 0x802A26F4 - draws one menu slot's entries (ItemName, fonts, sprites) */
void move_work_name_draw(void* work);             /* 0x803C3098 - the record's name plate (LbStr + draw_font_idx) */
}

/* Updates every live player's move-work indicator, then the lobby NPCs and the HUD frame. */
extern "C" void move_work_update(void)
{
    HudMoveWork* base;
    HudMoveWork* work;
    u16 max;
    u32 state;
    u16 j;
    HudMoveWork* p;
    u16 i;
    u32 mode;
    u32 busy;
    _mh_ivec2_ pos;

    max = get_move_work_max(2);
    state = 0;

    if (camera_work_ck() == 1) {
        return;
    }

    base = get_move_work_adrs(2);
    work = (HudMoveWork*)((u8*)base + (s8)my_player_no() * 0xB20);

    set_zmode(false, 0, false);
    set_blendmode(4, 5, 1);

    p = get_move_work_adrs(2);
    for (i = 0; i < max; i++, p++) {
        u8 found;

        pos.x = pos.y = 0;
        if (game_ready_ck() == 1) {
            found = hud_key_lookup(p->key_0x5DB);
            if (found != 0xFF && lbl_806BE340[found].active_0x000 != 0) {
                pos.y = draw_lsp_element(p, lbl_806BE340[found].index_0x117, p->index_0x002,
                                    lbl_806BE340[found].id_0x024);
            }
        }
        if (p->flags_0xB01 != 0) {
            _mh_ivec2_ draw = pos;
            move_work_draw(p, &draw, 1);
        }
    }

    if (map22_camera_ck() == 1) {
        state = 1;
    }

    if (lobby_w.mode_0x000 == 0) {
        for (j = 0; j < 0x12; j++) {
            _LB_NPC* npc = &lb_npc[j];

            if (npc->field_0x000 == 0) {
                continue;
            }
            if (npc->field_0x001 == 0) {
                continue;
            }
            if (state == 1) {
                lb_npc_move_work_draw(npc);
            } else if (npc->field_0x002 == 1) {
                lb_npc_move_work_draw(npc);
            }
        }
    }

    if (game_ready_ck() == 1) {
        busy = 0;
        if (lobby_w.field_0x026 == 1) {
            busy = 1;
        } else if (system_w.field_0x20 == 1) {
            busy = 1;
        }
        mode = 1;
        if (busy == 0) {
            if (lobby_w.field_0x004 == 0 && lobby_w.mode_0x000 == 0 && menu_busy_ck() == 0) {
                hud_key_table_draw();
                mode = 0;
            }
            if (lobby_w.mode_0x000 == 0) {
                draw_lsp_anim_ary();
                if (work->field_0x00A == 0 && work->field_0x00C == 4) {
                    if (lobby_w.field_0x005 == 0 && menu_busy_ck() == 0) {
                        move_indicator_draw(work);
                    }
                } else if (menu_busy_ck() == 0) {
                    if (get_option_cfg(7) == 0) {
                        if (mode == 1) {
                            draw_lsp_sprite_variant(0x18B0, 3);
                        } else {
                            draw_lsp_sprite_variant(0x18B0, 2);
                        }
                    } else if (mode == 1) {
                        draw_lsp_sprite_variant(0x18B0, 0x11);
                    } else {
                        draw_lsp_sprite_variant(0x18B0, 0x10);
                    }
                }
            }
        }

        note_box_draw();
        draw_lsp_parts();
        menu_slot_panel_draw(0);
        move_work_name_draw(work);
    }
}
