/* lobby/lb_menu_scratch.cpp - the lobby target marker: the scratch block `lb_menu_scratch` and its constructor, the
 *   candidate recorder and filters, the marker's per-frame update and draw, and the lobby spot lists.
 * RANGE. .text 0x8021E1EC-0x8021F020 (14 functions); .ctors 0x8056F360-0x8056F364, .data 0x805BA0C0-0x805BA540, .bss
 *   0x806AA8C8-0x806AAA88 (`lb_menu_scratch`), .sdata 0x80791EE0-0x80791EF0, .sdata2 0x80799C60-0x80799C78, extab,
 *   extabindex.  `tudiscover` reports the left edge as a strong cut (the `.sdata2` run ending at 0x80799C60), and
 *   the extabindex run starts with `lb_menu_candidate_add` (`lb_menu_actor_angle_get`'s record ends there).
 *   `lobby/lb_menu_pos_tbl.cpp` and `lobby/lb_equip_page.cpp` follow; the three share `lobby/fn_8021E1EC.h`.
 *   `lb_spot_net_tbl[8]`/`lb_spot_tbl[0]` point at `lb_page_state_0/1`, `.bss` records `lobby/lb_menu_pos_tbl.cpp` owns.
 * FLAGS. `cflags_lobby` and file-scope `#pragma peephole off` (retail keeps the narrowing `clrlwi`s), back on around
 *   `lb_menu_npc_target_ck`, whose retail body has none to keep (docs/lobby.md); `#pragma fp_contract off` around
 *   `lb_menu_marker_update` (retail keeps `fmuls`/`fmuls`/`fadds` for the spot distance).
 * NAMES. `__sinit_\lb_menu_scratch_cpp`/`__ct__13LbMenuScratchFv` are the compiler's names for the `.ctors` entry and
 *   the block's constructor.  GUESS (from each body and its callers): `lb_fabsf`, `lb_menu_candidate_add`,
 *   `lb_menu_npc_target_ck`, `lb_spot_open_ck`, `lb_menu_idle_ck`, `lb_zone_list_kind_ck`, `lb_menu_marker_update`,
 *   `lb_menu_marker_draw`, `lb_spot_net_area0..7`/`lb_spot_area1..2` (the spot lists the two area tables index),
 *   `lb_marker_frame_ids`/`lb_marker_more_ids` (the sprite rows the draw passes).
 * RESIDUALS. `lb_menu_marker_update`: the callee-saved colouring of the two candidate loops (retail: NPC counter r27,
 *   move-work count r27, cursor r28, index r29; ours r28/r29/r29/r27) and of the selection search (retail: index r6,
 *   row r3, key r4/r5; ours r3/r4/r5/r6) - local order, pointer/index loops, scoping and a by-value key do not move it.
 *   `fn_80208AC0` (`lobby/lb_npc.cpp`) keeps its map name.
 * SHAPES. The `.bss` object is defined at the foot of the file, after every use, so each access keeps its own
 *   `lis`/`addi`; the `.data` is defined after the functions so the switch table leads it, as in retail.  The
 *   draw reads the block through one local pointer (retail materialises it once), and the selection search is an
 *   inline helper whose `return` gives retail's per-hit store.
 */
#include "types.h"
#include "nw4r/math.h"

#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

#include "lobby/fn_8021E1EC.h"
#include "lobby/fn_80208AC0.h" /* fn_80208AC0, owned by lobby/lb_npc.cpp (rule 2) */
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "Pl/plw.h" /* `_PLW` (rule 1) */
#include "Pl/Pl_master_ck.h" /* `Pl_act_ck`, owned by Pl/pl_act.cpp (rule 2) */
#include "lobby/lb_npc.h" /* `_LB_NPC`/`lb_npc`/`LbSpot`, owned by lobby/lb_npc.cpp (rule 1/2) */
#include "lobby/lb_spot_tbl.h" /* this unit's spot tables (rule 2) */
#include "lobby/lb_spot_cur_get.h"
#include "lobby/lb_menu_marker_update.h"
#include "lobby/lb_menu_marker_draw.h"
#include "lobby/lb_menu_scratch_init.h" /* owned by lobby/fn_80219260.cpp (rule 2) */
#include "lobby/lb_menu_row16_copy.h" /* owned by lobby/fn_80219260.cpp (rule 2) */
#include "lobby/lb_cmd_repeat_ready_ck.h" /* owned by lobby/fn_80219260.cpp (rule 2) */
#include "lobby/lb_cmd_pressed_ck.h" /* owned by lobby/lb_npc.cpp (rule 2) */
#include "lobby/LbStr.h" /* owned by lobby/lb_npc.cpp (rule 2) */
#include "lobby/lb_speaker_name_get.h" /* owned by lobby/fn_80212810.cpp (rule 2) */
#include "ef/get_move_work_adrs.h" /* owned by ef/system_core.cpp (rule 2) */
#include "stage/stage_zone_hit_ck.h" /* owned by stage/stg_w.cpp (rule 2) */
#include "sqrtf_thunk.h"
#include "sound/fn_800D7F54.h" /* `sysSE_stop` (rule 2) */
#include "sound/fn_800E3CBC.h" /* `set_blendmode` (rule 2) */
#include "sound/set_zmode.h"
#include "hud/draw_font_idx.h" /* owned by hud/layout.cpp (rule 2) */
#include "font/flfnt.h" /* `flfntStrLen` (rule 2) */
#include "menu/get_pop_dat_ptr.h" /* `get_option_cfg` (rule 2) */

/* This range's record view of the user-data block; the owner's `lobby_world_block` is a plain `u8*`. */
static inline LbMenuBigBlock* lb_menu_block(void)
{
    return (LbMenuBigBlock*)lobby_world_block;
}

/* Retail keeps the narrowing `clrlwi` the peephole pass folds away; `lb_menu_npc_target_ck` (none to keep) keeps the pass on. */
#pragma peephole off

/* ---------------------------------------------------------------------------------------------------
 * Foreign callees.
 *
 * The plain `fn_XXXXXXXX` names are this unit's own spelling of unsplit addresses whose bracketing
 * registered units name different modules (rule 2's named gap), the same way
 * `src/lobby/fn_80212810.cpp` records them; the C++-linkage lobby ABI lives in this unit's header.
 */
extern "C" {
u32 lb_menu_actor_angle_get(LbMenuActor* self, VEC3* pos);
f32 fabsf(f32 v);

u32 lb_menu_angle_rank_get(u32 idx, u32 limit);
void lb_menu_row_insert(LbMenuCandidate* candidate, LbMenuRow* row);
f32 lb_fabsf(f32 v);
u32 game_ready_ck(void);
u8 userdata_rank_band_get(void);









}

extern "C" {

/* The marker's two sprite rows, defined with the unit's data after the functions. */
extern u16 lb_marker_frame_ids[3];
extern u16 lb_marker_more_ids[4];

/* ---------------------------------------------------------------------------------------------------
 * Definitions, in address order.
 */

/* The candidate recorder: copies the entry's position (or the fallback work record's), rejects it when
 * it is outside the y window or its slot is, and otherwise appends it to the scratch row. */
s32 lb_menu_candidate_add(LbMenuActor* self, LbMenuActor* rec, LbMenuFallback* work, f32 radius, u32 limit)
{
    LbMenuCandidate candidate;
    u32 idx;
    f32 dist_sq;
    VEC3 pos;

    VEC3_ctor(&pos);
    if (rec != 0) {
        copyVec3(&pos, &rec->pos_0x03C);
    } else {
        copyVec3(&pos, &work->pos_0x10);
    }
    if (lb_fabsf(pos.y - self->pos_0x03C.y) < 200.0f) {
        idx = lb_menu_actor_angle_get(self, &pos);
        dist_sq = calcDistanceSqXZ(&self->pos_0x03C, &pos);
        if ((idx < limit || idx > (u32)(-1 - (s32)limit)) && dist_sq < radius) {
            candidate.rec_0x00 = rec;
            candidate.work_0x04 = work;
            candidate.dist_sq_0x08 = dist_sq;
            candidate.slot_0x0C = (s16)lb_menu_angle_rank_get(idx, limit);
            lb_menu_row_insert(&candidate, &lb_menu_scratch.rows_0x00C[0]);
            return 1;
        }
    }
    return 0;
}

/* The y-window filter's absolute value helper (retail: `b <fabsf>`). */
f32 lb_fabsf(f32 v)
{
    return fabsf(v);
}

/* The entry filter: the entry's kind byte must not be 0x49 on a busy actor, and its mode must not be
 * the "special" one.  (peephole on: this body has no narrowing store the pass could fold.) */
#pragma peephole on
u32 lb_menu_npc_target_ck(LbMenuActor* self, LbMenuEntryRec* entry)
{
    if (entry->kind_0x002 == 0x49 && self->busy_0xB03) {
        return 0;
    }
    if (entry->mode_0x256 == 1) {
        return 0;
    }
    return 1;
}

#pragma peephole off

/* The page's per-kind gate: whether the entry kind may be opened from the current lobby state. */
u32 lb_spot_open_ck(LbMenuActor* self, s16* kind)
{
    s32 ok = 1;

    if (fn_80208AC0((_PLW*)self) != 0) {
        ok = 0;
    }
    switch (*kind) {
    case 10:
    case 11:
    case 14:
    case 15:
        if (game_ready_ck() == 0) {
            ok = 0;
        }
        if (lobby_w.state_0x000) {
            ok = 0;
        }
        break;
    case 1:
    case 6:
        if (self->busy_0xB03) {
            ok = 0;
        }
        break;
    case 3:
        if (lobby_w.state_0x000) {
            ok = 0;
        }
        if (!lobby_w.page_0x052) {
            ok = 0;
        }
        break;
    case 4:
        if (lobby_w.page_0x052 != 6) {
            ok = 0;
        }
        break;
    default:
        if (lobby_w.state_0x000) {
            ok = 0;
        }
        break;
    }
    return ok;
}

/* The item-list page's "still idle" gate. */
u32 lb_menu_idle_ck(void)
{
    if (lobby_w.state_0x000 == 0 && lobby_w.flag_0x027 == 0) {
        return 1;
    }
    return 0;
}

/* The page's availability gate: the item list must be open and its kind must match. */
s32 lb_zone_list_kind_ck(s32 kind)
{
    if (userdata_rank_band_get() == 0) {
        return 0;
    }
    switch ((u8)kind) {
    case 0:
        if (lb_menu_block()->list_kind_0x484D) {
            return 0;
        }
        break;
    case 1:
        if (lb_menu_block()->list_kind_0x484D != 1) {
            return 0;
        }
        break;
    case 2:
        if (lb_menu_block()->list_kind_0x484D != 2) {
            return 0;
        }
        break;
    default:
        return 0;
    }
    return 1;
}

/* Selects the row of `scratch` whose table and model pointers equal `key`'s, or -1 when none does. */
static inline void lb_menu_row_find(LbMenuScratch* scratch, LbMenuRow* key)
{
    s32 i;

    for (i = 0; i < 20; i++) {
        if (key->table_0x00 == scratch->rows_0x00C[i].table_0x00 && key->model_0x04 == scratch->rows_0x00C[i].model_0x04) {
            scratch->index_0x14C = i;
            return;
        }
    }
    scratch->index_0x14C = -1;
}

/* Retail keeps the distance's `fmuls`/`fmuls`/`fadds`. */
#pragma fp_contract off
/* 0x8021E538 (0x6B8): rebuilds the target marker's rows - the lobby NPCs and the other players in range, the stage zone
 * and the lobby spot the player stands on - then keeps the selection on the same row and steps it on input. */
void lb_menu_marker_update(LbMenuScratch* scratch)
{
    LbMenuRow saved;
    _PLW* plw = scratch->plw_0x000;
    s32 busy;

    if (scratch->index_0x14C >= 0) {
        lb_menu_row16_copy(&saved, &scratch->rows_0x00C[scratch->index_0x14C]);
    }
    lb_menu_scratch_init(0);
    scratch->count_0x154 = 0;
    if (lobby_w.state_0x000 == 0 && Pl_act_ck(plw, 0, 25) == 0 && plw->field_0x313 == 0) {
        busy = 0;
    } else {
        busy = 1;
    }
    scratch->fixed_b_0x008 = 0;
    scratch->fixed_a_0x004 = 0;
    if (busy == 0) {
        if (fn_80208AC0(plw) == 0 && plw->field_0x00A != 12 && lobby_w.state_0x000 == 0) {
            f32 range = 160000.0f;
            s32 i;

            for (i = 0; i < 18; i++) {
                if (lb_npc[i].field_0x000 != 0 && lb_npc[i].field_0x001 != 0
                    && lb_menu_npc_target_ck((LbMenuActor*)plw, (LbMenuEntryRec*)&lb_npc[i]) == 1) {
                    scratch->count_0x154 += lb_menu_candidate_add((LbMenuActor*)scratch->plw_0x000, 0, (LbMenuFallback*)&lb_npc[i], range, 0x2AAB);
                }
            }
        }
        if (lobby_w.state_0x000 == 0) {
            f32 range = 90000.0f;
            _PLW* other = (_PLW*)get_move_work_adrs(2);
            u16 count = get_move_work_max(2);
            u32 i;

            for (i = 0; i < count; i++, other++) {
                if (other->slot_active != 0 && other->field_0x001 != 0 && plw->chunk_ofs != other->chunk_ofs
                    && other->field_0xB04 != 0) {
                    scratch->count_0x154 += lb_menu_candidate_add((LbMenuActor*)scratch->plw_0x000, (LbMenuActor*)other, 0, range, 0x238E);
                }
            }
        }
        {
            u32* group;

            if (lobby_w.page_0x052 == 0) {
                group = stage_group_word_get(lobby_w.field_0x001, lobby_w.field_0x002);
            } else {
                group = 0;
            }
            if (stage_zone_hit_ck(group, (u32*)&scratch->fixed_b_0x008, &scratch->plw_0x000->vec_0x03C) != 0) {
                if (game_ready_ck() == 1) {
                    s32 reject = 0;

                    switch (*scratch->fixed_b_0x008) {
                    case 5:
                        if (lb_zone_list_kind_ck(0) == 0) {
                            reject = 1;
                        }
                        break;
                    case 6:
                        if (lb_zone_list_kind_ck(1) == 0) {
                            reject = 1;
                        }
                        break;
                    case 7:
                        if (lb_zone_list_kind_ck(2) == 0) {
                            reject = 1;
                        }
                        break;
                    }
                    if (reject != 0) {
                        scratch->fixed_b_0x008 = 0;
                    } else {
                        scratch->count_0x154++;
                    }
                } else {
                    scratch->count_0x154++;
                }
            }
        }
    }
    if (scratch->fixed_b_0x008 == 0) {
        LbSpot* spot;

        if (lobby_wp->field_0x001 == 21) {
            spot = lb_spot_net_tbl[lobby_wp->field_0x002];
        } else {
            spot = lb_spot_tbl[lobby_wp->field_0x002];
        }
        for (; spot->kind_0x00 != 0; spot++) {
            if (plw->motion_pos_0x40 >= spot->pos_0x04.y - 50.0f && plw->motion_pos_0x40 < 100.0f + spot->pos_0x04.y
                && lb_spot_open_ck((LbMenuActor*)plw, &spot->kind_0x00) == 1) {
                f32 dx = plw->motion_pos_0x3C - spot->pos_0x04.x;
                f32 dz = plw->motion_pos_0x44 - spot->pos_0x04.z;

                if (sqrtf_thunk(dx * dx + dz * dz) <= spot->radius_0x14) {
                    scratch->fixed_a_0x004 = spot;
                    scratch->count_0x154++;
                    break;
                }
            }
        }
    }
    if (scratch->rows_0x00C[0].rank_0x0C == -1) {
        if (scratch->fixed_a_0x004 == 0 && scratch->fixed_b_0x008 == 0) {
            scratch->index_0x14C = -1;
        } else if (scratch->index_0x14C != -2) {
            scratch->index_0x14C = -2;
            sysSE_stop(2);
        }
    } else {
        if (scratch->index_0x14C >= 0) {
            lb_menu_row_find(scratch, &saved);
        }
        if (scratch->index_0x14C != -1) {
            if (lb_cmd_pressed_ck(0x20) != 0 && lb_menu_idle_ck() == 1
                && (scratch->fixed_a_0x004 != 0 || scratch->fixed_b_0x008 != 0)) {
                if (scratch->index_0x14C != -2) {
                    scratch->index_0x14C = -2;
                    sysSE_stop(2);
                }
            } else if (lb_cmd_repeat_ready_ck(0xC) != 0 && lb_menu_idle_ck() == 1) {
                s16 prev = scratch->index_0x14C;

                if (prev == -2) {
                    scratch->index_0x14C = 0;
                } else {
                    s32 next = prev + 1;
                    s32 rank = scratch->rows_0x00C[next].rank_0x0C;

                    if ((rank == -1 || next >= 20) && scratch->fixed_a_0x004 == 0 && scratch->fixed_b_0x008 == 0) {
                        scratch->index_0x14C = 0;
                    } else if ((rank == -1 || next >= 20) && (scratch->fixed_a_0x004 != 0 || scratch->fixed_b_0x008 != 0)) {
                        scratch->index_0x14C = -2;
                    } else {
                        scratch->index_0x14C++;
                    }
                }
                if (scratch->index_0x14C != prev) {
                    sysSE_stop(2);
                }
            } else if (scratch->fixed_a_0x004 == 0 && scratch->fixed_b_0x008 == 0 && scratch->index_0x14C == -2) {
                scratch->index_0x14C = 0;
                sysSE_stop(2);
            }
        } else {
            scratch->index_0x14C = 0;
            sysSE_stop(2);
        }
    }
}

#pragma fp_contract reset

/* The current row's model pointer, or 0 when there is none. */
s32 lb_target_npc_get(void)
{
    if (lb_menu_scratch.index_0x14C >= 0 && lb_menu_scratch.rows_0x00C[lb_menu_scratch.index_0x14C].model_0x04 != 0) {
        return lb_menu_scratch.rows_0x00C[lb_menu_scratch.index_0x14C].model_0x04;
    }
    return 0;
}

/* The current row's table pointer, or 0 when there is none. */
s32 lb_plsearch_target_get(void)
{
    if (lb_menu_scratch.index_0x14C >= 0 && lb_menu_scratch.rows_0x00C[lb_menu_scratch.index_0x14C].table_0x00 != 0) {
        return lb_menu_scratch.rows_0x00C[lb_menu_scratch.index_0x14C].table_0x00;
    }
    return 0;
}

/* The special (fixed) entry's two pointers, one per accessor; only valid in the -2 mode. */
LbSpot* lb_spot_cur_get(void)
{
    if (lb_menu_scratch.index_0x14C == -2) {
        return lb_menu_scratch.fixed_a_0x004;
    }
    return 0;
}

u8* lb_place_rec_get(void)
{
    if (lb_menu_scratch.index_0x14C == -2) {
        return lb_menu_scratch.fixed_b_0x008;
    }
    return 0;
}

/* 0x8021EC98 (0x324): draws the target marker - the selected row's name (an NPC's speaker name, the spot's or the
 * place's label) in a frame sized to the name, the "more targets" hint and the animated cursor. */
void lb_menu_marker_draw(void)
{
    s8* name = 0;
    LbMenuScratch* scratch = &lb_menu_scratch;
    u32 frame;
    _mh_ivec2_ pos;

    set_zmode(0, 0, 0);
    set_blendmode(4, 5, 1);
    switch (scratch->index_0x14C) {
    case -1:
        return;
    case -2: {
        LbSpot* spot = lb_spot_cur_get();

        if (spot != 0) {
            if (game_ready_ck() == 1 && spot->kind_0x00 == 3 && lobby_w.page_0x052 == 2) {
                name = (s8*)LbStr(4, 16);
            } else {
                name = (s8*)LbStr(4, spot->kind_0x00);
            }
        } else {
            u8* place = lb_place_rec_get();

            if (place != 0) {
                if (lobby_w.field_0x001 == 21) {
                    name = (s8*)LbStr(1, *place + 30);
                } else {
                    name = (s8*)LbStr(1, *place + 39);
                }
            } else if (lobby_w.field_0x001 == 22) {
                switch (lobby_w.sub_0x0B1) {
                case 1:
                case 3:
                    name = (s8*)LbStr(1, 50);
                    break;
                case 2:
                    if (lobby_w.page_0x052 != 0) {
                        name = (s8*)LbStr(1, 51);
                    }
                    break;
                }
            }
        }
        break;
    }
    default: {
        LbMenuRow* row = &scratch->rows_0x00C[scratch->index_0x14C];

        if (row->table_0x00 == 0 && row->model_0x04 != 0) {
            name = lb_speaker_name_get(((_LB_NPC*)row->model_0x04)->field_0x002);
        }
        break;
    }
    }
    if (name != 0) {
        s32 len = flfntStrLen((char*)name);

        if (len > 18) {
            frame = 3;
        } else if (len > 14) {
            frame = 2;
        } else {
            frame = len > 10;
        }
        get_lsp_data(0x18CD, &pos);
        if (game_ready_ck() == 0) {
            pos.y = 30;
        } else {
            pos.y = 120;
        }
        draw_sprite_anim_ary(lb_marker_frame_ids, frame, &pos);
        if (get_option_cfg(7) == 0) {
            draw_sprite_anim_idx(0x18D2, frame, &pos);
        } else {
            draw_sprite_anim_idx(0x18D3, frame, &pos);
        }
        draw_font_idx(0x18D5, name, 4, &pos);
        if (scratch->count_0x154 > 1) {
            draw_sprite_ary(lb_marker_more_ids, &pos);
            draw_font_idx(0x18D6, (s8*)LbStr(0, 430), 0, &pos);
        }
        set_blendmode(4, 1, 1);
        draw_sprite_anim_idx(0x18D7, scratch->timer_0x150 % 30, &pos);
        set_blendmode(4, 5, 1);
    }
}

/* ---------------------------------------------------------------------------------------------------
 * Data, in address order: the lobby spots (one zero-kind-terminated list per area) and the two per-mode area tables
 * `lb_menu_marker_update` and `lobby/lb_npc.cpp`'s spot search index, then the marker's sprite rows.
 */
LbSpot lb_spot_net_area0[4] = {
    { 10, 0, { 0.0f, 0.0f, 1100.0f }, 0x0, 300.0f },
    { 11, 0, { 20.0f, 0.0f, -1570.0f }, 0x0, 400.0f },
    { 15, 0, { 535.0f, 0.0f, -1022.0f }, 0x7000, 190.0f },
    { 0 },
};
LbSpot lb_spot_net_area1[2] = {
    { 14, 0, { 147.47f, 196.76f, 1217.34f }, 0x0, 300.0f },
    { 0 },
};
LbSpot lb_spot_net_area2[19] = {
    { 5, 0, { -630.0f, 140.0f, -380.0f }, 0x58E4, 80.0f },
    { 5, 1, { -735.0f, 140.0f, -525.0f }, 0x58E4, 80.0f },
    { 5, 2, { -500.0f, 140.0f, -695.0f }, 0xDA51, 80.0f },
    { 5, 3, { -395.0f, 140.0f, -550.0f }, 0xDA51, 80.0f },
    { 5, 4, { 765.0f, 140.0f, -425.0f }, 0x8AAB, 80.0f },
    { 5, 5, { 595.0f, 140.0f, -370.0f }, 0x8AAB, 80.0f },
    { 5, 6, { 510.0f, 140.0f, -650.0f }, 0xCCD, 80.0f },
    { 5, 7, { 680.0f, 140.0f, -705.0f }, 0xCCD, 80.0f },
    { 5, 8, { -30.0f, 470.0f, -1875.0f }, 0xB3E9, 80.0f },
    { 5, 9, { 15.0f, 470.0f, -2045.0f }, 0xB3E9, 80.0f },
    { 5, 10, { -265.0f, 470.0f, -2125.0f }, 0x3555, 80.0f },
    { 5, 11, { -310.0f, 470.0f, -1955.0f }, 0x3555, 80.0f },
    { 7, 0, { -395.0f, 140.0f, -1215.0f }, 0x71C7, 350.0f },
    { 9, 0, { -1240.0f, 500.0f, -1700.0f }, 0x0, 150.0f },
    { 3, 0, { -900.0f, 474.0f, -1872.0f }, 0x0, 320.0f },
    { 4, 0, { -900.0f, 474.0f, -1872.0f }, 0x0, 420.0f },
    { 8, 0, { -122.5f, 170.0f, -281.5f }, 0x4000, 80.0f },
    { 8, 1, { 9.5f, 170.0f, -281.5f }, 0xC001, 80.0f },
    { 0 },
};
LbSpot lb_spot_net_area3[2] = {
    { 12, 0, { -698.0f, 0.0f, -420.0f }, 0x0, 150.0f },
    { 0 },
};
LbSpot lb_spot_net_area4[2] = {
    { 1, 0, { -490.0f, 0.0f, -410.0f }, 0x0, 150.0f },
    { 0 },
};
LbSpot lb_spot_net_area5[3] = {
    { 1, 0, { 261.0f, 0.0f, 62.0f }, 0x0, 145.0f },
    { 6, 0, { -12.0f, 0.0f, 29.0f }, 0x85B0, 100.0f },
    { 0 },
};
LbSpot lb_spot_net_area6[3] = {
    { 1, 0, { 441.0f, 0.0f, -36.0f }, 0x0, 149.0f },
    { 6, 0, { 220.0f, 0.0f, -128.0f }, 0x8000, 100.0f },
    { 0 },
};
LbSpot lb_spot_net_area7[3] = {
    { 1, 0, { 516.0f, 0.0f, 356.0f }, 0x0, 180.0f },
    { 6, 0, { 58.0f, 0.0f, -202.0f }, 0x8000, 121.0f },
    { 0 },
};
/* The online lobby's spots, one list per area (`lobby_wp`'s area byte); area 8 has none. */
LbSpot* lb_spot_net_tbl[10] = {
    lb_spot_net_area0, lb_spot_net_area1, lb_spot_net_area2, lb_spot_net_area3, lb_spot_net_area4,
    lb_spot_net_area5, lb_spot_net_area6, lb_spot_net_area7, (LbSpot*)&lb_page_state_0, 0,
};
LbSpot lb_spot_area1[3] = {
    { 1, 0, { -42.0f, 0.0f, -700.0f }, 0x0, 187.0f },
    { 6, 0, { -75.0f, 0.0f, -265.0f }, 0xA222, 100.0f },
    { 0 },
};
LbSpot lb_spot_area2[2] = {
    { 13, 0, { 1660.0f, 400.0f, 218.0f }, 0x0, 180.0f },
    { 0 },
};
/* The offline lobby's spots, one list per area; area 0 has none. */
LbSpot* lb_spot_tbl[4] = {
    (LbSpot*)&lb_page_state_1, lb_spot_area1, lb_spot_area2, 0,
};

/* The marker's frame sprites (one per name length) and the "more targets" sprite row. */
u16 lb_marker_frame_ids[3] = { 0x18CE, 0x18CF, 0xFFFF };
u16 lb_marker_more_ids[4] = { 0x18D0, 0x18D1, 0x18D4, 0xFFFF };

/* This unit's `.bss`, defined after every use so each access keeps its own `lis`/`addi` like the target; its
 * constructor runs from the unit's `__sinit`. */
LbMenuScratch lb_menu_scratch;             /* .bss 0x806AA8C8 */

}  /* extern "C" */

/* 0x8021EFC8 (0x58): constructs the eight slot vectors. */
__declspec(noinline) inline LbMenuScratch::LbMenuScratch()
{
    VEC3* p = slots_0x160;

    do {
        VEC3_ctor(&p[0]);
        p++;
    } while (p < slots_0x160 + 8);
}
