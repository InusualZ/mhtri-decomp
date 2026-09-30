/*
 * hud/cockpit_quest.cpp - the quest cockpit HUD band: `.text` 0x802E7408..0x802EBED8 (64 functions), extab
 * 0x800150C4..0x8001529C, extabindex 0x8003378C..0x80033A50, and the claimed `.data` 0x805D5C74..0x805D6310.
 *
 * Home and name: `.data` 0x805D5D08 is the bare source name "cockpit_quest.cpp" and only `quest_bar_a_next_id` /
 * `quest_bar_b_next_id` (the `nw4r::db::Panic` asserts) reference it.
 *
 * Seam (unproven): `tudiscover` offers 0x802E932C as the strong boundary, and the `.sdata2` run 0x8079A8E0..0x8079A944 is
 * shared with the band below (`menu/fn_802E4978.cpp`), so the true TU is probably 0x802E4978..0x802EBBD0.  That is why
 * the object's `.data` starts 4 bytes late (the first 8-aligned table lands 4 B after the target's) - the claimed run
 * itself matches the target byte for byte - and why the pooled literals below are compiler-made (`.sdata2`, unclaimed).
 * The same sharing makes the owner of the icon tables (`hud/cockpit_icon_data.cpp`) an open question, together with
 * `hud/fn_802EBED8.cpp` and `ef/eft035.cpp`: whether one TU spans all four (and so owns the pools) is not decided here.
 *
 * Flags: `cflags_hud` plus a per-object `-pool off` (configure.py, with the four-row evidence table): with the unit's
 * own tables defined, MWCC shares one base register across 3+ `.data` objects per function where retail loads each with
 * its own `lis`/`addi`.  `-opt nopeephole` keeps retail's unfused `clrlwi`+`cmp` pairs.
 *
 * Source shapes that are load-bearing: locals are declared in the reverse of their target stack-offset order; `switch`
 * where retail compares with `cmpwi`; `while (count > 0) { ...; count--; p++; }` loops and `for (;;) { if (*p == 0xFFFF)
 * break; ... }` for the list walks; list counts kept `u32` and narrowed in the loop condition (`i < (u8)count`);
 * `-pool off`-dependent tables defined before their first use; `SprRecordWords` for the word-wise record copy; casts to
 * `u32` where retail compares with `cmplwi`; `#pragma fp_contract off` around the nine functions that keep retail's
 * unfused `fmul`+`fadd` (nine pairs, between the screen projection and `quest_pit_trap_marker_draw`); the results of
 * `get_worldworld_pos` (it returns the VEC3 by value) are bound to a `const VEC3&`, because a declared local copies the
 * hidden result temporary, and `quest_npc_marker_draw` passes the temporary to a `static inline` helper so the
 * reference does not pin a callee-saved register.
 *
 * Names: GUESSes throughout - no map name here comes from the runtime dump (`zz_`); they are read off call sites and
 * callee bodies (prefixes `quest_gauge_`, `quest_marker_`, `quest_pit_`, `quest_map_`, the callees renamed for this
 * unit, `Pl_*` helpers such as `Pl_motion_input_ck`, and the table names in `hud/cockpit_icon_data.h`).  The inventory is
 * `ledger.py unit hud/cockpit_quest.cpp` and the map.
 *
 * Residuals (per row; the report has the percentages):
 *  - `quest_targets_update_a`/`_b`, `quest_gauge_update`: one `li` in a scratch register after the call where MWCC
 *    keeps the byte in a callee-saved one (playbook 22; `if/else`, ternary and declaration-order spellings measured).
 *  - `quest_gauge_draw`: the `pos.x` narrowing pair (`extsh` placement) and scheduling of the stack reloads.
 *  - `quest_marker_ring_draw`, `quest_marker_icon_draw`, `quest_marker_draw_at`, `quest_marker_draw_faded`: the order
 *    the callee-saved float/int registers are assigned (r27/r28, f30/f31) and where the argument `fmr`/`frsp` sit.
 *  - `quest_mark_fade_value`: retail returns through one exit (`b end`), ours through `mr r3,r4; b`.
 *  - `quest_bar_b_next_id`: one argument register in the tail call; `quest_bar_a_next_id` and `quest_bar_b_next_id`
 *    also reference our compiler-local panic strings (`@NNNN`) where the target references `quest_source_file_name` /
 *    `quest_assert_fail_msg` (symbol identity, bytes equal).
 *  - `quest_gauge_bar_draw`, `quest_marker_pie_draw`, `quest_icon_rect_draw` (99.6-99.8 %): scheduling and the pooled
 *    `.sdata2` literal names.
 *  - `quest_npc_marker_draw`: one `fmr` position and one `fmuls` pair (the helper split, 98.5 %); the map icon draws
 *    `quest_map_icon_sprites_draw` and the others read `.sdata2`/list symbols whose names differ from the target's.
 *  - Others at 95-99 %: instruction scheduling only (`quest_player_marker_draw`, `quest_marker_draw_with_master`,
 *    `quest_marker_draw_all_non_master`, `quest_target_marker_draw`, `quest_mark_draw`).
 *
 * Data: `.data` is claimed and emitted (24 objects + the jump table the `quest_target_icon_get` switch emits).  The map
 * icon tables and the flash id list are `hud/cockpit_icon_data.cpp`'s.  Left out, with the census class: `.sdata`
 * `quest_gauge_shake_scale`/`quest_view_hold_ids*` (isolated-run, defined here, unclaimed) and `.sdata2` 0x8079A928..0x8079A984
 * (pool-synth: the compiler emits the literals).
 */

#include "types.h"
#include "pl.h"
#include "nw4r/math.h"
#include "Pl/pl_skill.h"
#include "Pl/pl_act.h"
#include "Pl/pl_master.h"
#include "hud/layout.h"
#include "hud/cockpit_quest.h"
#include "main.h"
#include "Network/network_pat_control.h"
#include "menu/menu_item.h"
#include "menu/get_pop_dat_ptr.h"
#include "unsplit/unknown.h"
#include "ai/fn_802D44F4.h"
#include "ef/fn_800CDB2C.h"
#include "lobby/lb_quest_screen.h"
#include "enemy/fn_8012EC74.h"
#include "mh3_pad.h"
#include "fn_8004CAD8.h"
#include "enemy/ENEMY_WORK.h"
#include "copyVec2.h"
#include "hud/quest_marker_draw.h"
#include "camera/fn_802B5C58.h"
#include "stage/stg_w.h"
#include "Pl/fn_80288CEC.h"
#include "Pl/fn_8027D684.h"
#include "Pl/fn_80273B14.h"
#include "ai/ainpc.h"
#include "unsplit/NetworkStream.h"
#include "unsplit/menu.h"
#include "sound/fn_800D7F54.h"
#include "menu/cockpit_hud_hidden_ck.h"
#include "hud/cockpit_icon_data.h"

/* nw4r's debug panic - the map's mangling is `Panic__Q24nw4r2dbFPCciPCce` (rule 9: the owner is
 * `nw4r::db`, so the declaration is the real one and the front-end reproduces the map name). */
namespace nw4r {
namespace db {
void Panic(const char* file, int line, const char* fmt, ...);
}  // namespace db
}  // namespace db

/* The quest bar's tables (0x805D5C74.., this unit's own `.data`).  The step table is two rows of blend-id offsets (row 0
 * for kinds 0/1/2 of `quest_bar_a_next_id`/`quest_bar_b_next_id`, row 1 for kinds 3/4); the colour tables are read as
 * `HudBlend` (three colours plus the copied tag word), so a three-word table is followed in memory by the next
 * table's first word, which is what the tag copy picks up. */
u16 quest_bar_id_step_table[2][5] = {
    { 0x0000, 0x0100, 0x01C0, 0x02C0, 0x0300 },
    { 0x0100, 0x01C0, 0x02C0, 0x0380, 0x03C0 },
};

u32 quest_bar_a_blend_from[4] = {
    0x13A013FF, 0x57CC33FF, 0x96E55CFF, 0xC11836FF,
};

u32 quest_bar_a_blend_to[3] = {
    0xC759BBFF, 0xCC5DEEFF, 0xCC7CFBFF,
};

u32 quest_bar_b_blend_base_from[3] = {
    0xE6BA00FF, 0xF2D200FF, 0xF7EA4CFF,
};

u32 quest_bar_b_blend_low_from[3] = {
    0xE63800FF, 0xE63800FF, 0xE63800FF,
};

u32 quest_bar_b_blend_low_to[3] = {
    0xE67373FF, 0xE67373FF, 0xE67373FF,
};

u32 quest_bar_b_blend_normal_to[4] = {
    0xCD7330FF, 0xDF8235FF, 0xE6AD6FFF, 0x00000000,
};

u32 quest_bar_b_blend_flash[6] = {
    0xF9F9C7FF, 0xFCFCE3FF, 0xFFFFFFFF, 0xF5F473FF,
    0xF7FF76FF, 0xF7FFC4FF,
};

u32 quest_bar_b_blend_dm_to[3] = {
    0x0AD7D2FF, 0x1EE1F0FF, 0x88E5F8FF,
};

u32 quest_bar_b_blend_dm_c0_to[3] = {
    0x8CD2E5FF, 0x9BD4F5FF, 0xBEE6FAFF,
};


/* 0x802E7408 (0x140).  The first bar's blend parameter: the player's slot 45 state picks a row of
 * the 2x5 `u16` table at 0x805D5C74 and the argument picks the column pair, and a zero result means
 * "no change" - the caller's previous value is kept by `quest_bar_id_keep`.  The `li r4,0 / cmplwi /
 * bne / li r4,1` sequence is the target's shape for the "slot is 0x7D" test, so the cancel flag is a
 * materialised local, not a folded comparison. */
u16 quest_bar_a_next_id(u16 id, u8 kind, _PLW* plw) {
    u16 off = 0;

    if (plw->field_0x464 == 0) {
        s32 row = 0;
        u8 slot = Pl_Skill_slot_item_get(plw, 45);
        s32 cancel = 0;

        if (slot == 0x7D) {
            cancel = 1;
        }
        if (cancel == 0) {
            switch (slot) {
            case 0x7C:
                off = 0;
                break;
            case 0x7E:
                off = 3;
                break;
            case 0x7F:
                off = 4;
                break;
            default:
                off = 2;
                break;
            }
            switch (kind) {
            case 0:
                row = 0;
                off = 0;
                break;
            case 1:
                row = 0;
                break;
            case 3:
                row = 1;
                break;
            default:
                nw4r::db::Panic("cockpit_quest.cpp", 0x7F8, "NW4R:Failed assertion 0");
                break;
            }
            off = quest_bar_id_step_table[row][off];
        }
    }
    if (off != 0) {
        return (u16)(id + off);
    }
    return quest_bar_id_keep(id, plw);
}

/* 0x802E7548 (0x148).  The second bar's blend parameter - `quest_bar_a_next_id`'s twin one slot family
 * over (slot 46, table values 0x80/0x82/0x83, kinds 0/2/4, line 2088), and its "off is zero" case
 * falls out as the target's mask `(-off | off) >> 31` rather than a branch (the caller then keeps
 * the previous value by adding it).  Its gate is `_PLW`'s +0x466 (`field_0x466`; `Pl/pl_act.cpp`
 * clamps the same field to 0x2328). */
u16 quest_bar_b_next_id(s32 id, u8 kind, _PLW* plw) {
    u16 off = 0;

    if (plw->field_0x466 == 0) {
        s32 row = 0;
        u8 slot = Pl_Skill_slot_item_get(plw, 46);
        s32 cancel = 0;

        if (slot == 0x81) {
            cancel = 1;
        }
        if (cancel == 0) {
            switch (slot) {
            case 0x80:
                off = 0;
                break;
            case 0x82:
                off = 3;
                break;
            case 0x83:
                off = 4;
                break;
            default:
                off = 2;
                break;
            }
            switch (kind) {
            case 0:
                row = 0;
                off = 0;
                break;
            case 2:
                row = 0;
                break;
            case 4:
                row = 1;
                break;
            default:
                nw4r::db::Panic("cockpit_quest.cpp", 0x828, "NW4R:Failed assertion 0");
                break;
            }
            off = quest_bar_id_step_table[row][off];
        }
    }
    return off != 0 ? (u16)(id + off) : 0;
}

/* 0x802E7690 (0xF8).  One 0x10-byte blend record: with `t == 0` the three components are copied,
 * otherwise they are interpolated between `a` and `b` by `0.5f * (1.0f - anim_tick_cos(t))`
 * clamped to [0.0f, 1.0f]; the fourth word is always copied.  `anim_tick_cos` maps the blend clock to
 * a fraction (`1.0f` is 1.0f, `0.0f` 0.0f). */
void quest_blend_lerp(HudBlend* out, const HudBlend* a, const HudBlend* b, u16 t) {
    if (t == 0) {
        out->x = a->x;
        out->y = a->y;
        out->z = a->z;
    } else {
        f32 f = 0.5f * (1.0f - anim_tick_cos(t));

        if (f < 0.0f) {
            f = 0.0f;
        }
        if (f > 1.0f) {
            f = 1.0f;
        }
        out->x = color_lerp(a->x, b->x, f);
        out->y = color_lerp(a->y, b->y, f);
        out->z = color_lerp(a->z, b->z, f);
    }
    out->tag = a->tag;
}

/* 0x802E7788.  Steps both gauge colour blends: the first follows the act kind's bar id, the second the
 * player's health status (stagger, the 0xCC/0xC0/0x88 damage conditions, low health). */
void quest_gauge_blend_update(CockpitWork* work) {
    _PLW* plw = work->plw;
    u8 kind = pl_act_kind_get(plw->area_0x16);
    s32 step;
    const u32* to;

    switch (kind) {
    case 1:
    case 3:
        work->blend_0x13E = quest_bar_a_next_id(work->blend_0x13E, kind, plw);
        break;
    default:
        work->blend_0x13E = quest_bar_id_keep(work->blend_0x13E, plw);
        break;
    }
    quest_blend_lerp(&work->bar_a_0x144, (const HudBlend*)quest_bar_a_blend_from,
                     (const HudBlend*)quest_bar_a_blend_to, work->blend_0x13E);
    if (Pl_act_state_ck(plw) == 0) {
        if (Pl_dm_condition_ck(plw, 0xCC) == 0) {
            if (plw->field_0x378 > 0x96) {
                switch (kind) {
                default:
                    work->blend_0x140 = 0;
                    break;
                case 2:
                case 4:
                    work->blend_0x140 = quest_bar_b_next_id(work->blend_0x140, kind, plw);
                    break;
                }
                quest_blend_lerp(&work->bar_b_0x154, (const HudBlend*)quest_bar_b_blend_base_from,
                                 (const HudBlend*)quest_bar_b_blend_normal_to, work->blend_0x140);
                return;
            }
            work->blend_0x140 += 0x5B0;
            quest_blend_lerp(&work->bar_b_0x154, (const HudBlend*)quest_bar_b_blend_low_from,
                             (const HudBlend*)quest_bar_b_blend_low_to, work->blend_0x140);
            return;
        }
        step = 0x200;
        to = quest_bar_b_blend_dm_to;
        if (Pl_dm_condition_ck(plw, 0xC0) != 0) {
            to = quest_bar_b_blend_dm_c0_to;
        }
        if (Pl_dm_condition_ck(plw, 0x88) != 0) {
            step = 0x300;
        }
        work->blend_0x140 += step;
        quest_blend_lerp(&work->bar_b_0x154, (const HudBlend*)quest_bar_b_blend_base_from, (const HudBlend*)to,
                         work->blend_0x140);
        return;
    }
    work->blend_0x140 += 0x400;
    quest_blend_lerp(&work->bar_b_0x154, (const HudBlend*)quest_bar_b_blend_flash,
                     (const HudBlend*)(quest_bar_b_blend_flash + 3), work->blend_0x140);
}

/* 0x802E796C.  Per-frame update of the health and stagger gauges: the shake clock, the eased stagger value, the
 * low-stagger notice and the flash effect's frame counter. */
void quest_gauge_update(CockpitWork* work, u8 mode) {
    _PLW* plw = work->plw;
    _mh_ivec2_ pos;
    HudNotice* notice;
    _SPR_DATA_* anchor;
    u32 limit_shake;
    u32 limit_notice;

    quest_gauge_blend_update(work);
    if (work->health_hit_clock_0x13A != 0) {
        work->health_hit_clock_0x13A--;
    }
    if ((s16)(plw->health + ((plw->health_max * 25) / 100)) < work->health_prev_0x13C) {
        work->health_hit_clock_0x13A = 9;
    }
    work->health_prev_0x13C = plw->health;
    if (plw->kind_0x09 != 3) {
        if (work->stagger_flash_0x136 != 0) {
            if (work->stagger_flash_0x136 > 8) {
                work->stagger_flash_0x136 -= 8;
                work->stagger_shown_0x134 = plw->field_0x37E;
            } else {
                work->stagger_flash_0x136 = 0;
                work->stagger_shown_0x134 = 0;
            }
        }
    } else if (plw->field_0x00A != 8) {
        work->stagger_flash_0x136 = 0xFF;
        work->stagger_shown_0x134 += 4;
        if (work->stagger_shown_0x134 > plw->field_0x37E) {
            work->stagger_shown_0x134 = plw->field_0x37E;
        }
    }
    if (plw->field_0x37E <= 0x19) {
        if (plw->field_0x00A != 8) {
            if (plw->field_0x37E != 0) {
                limit_shake = 0x32;
                limit_notice = 0x3C;
            } else {
                limit_shake = 0x19;
                limit_notice = 0x1E;
            }
            work->stagger_shake_idx_0x137++;
            if (work->stagger_shake_idx_0x137 > limit_shake) {
                work->stagger_shake_idx_0x137 = 1;
            }
            work->stagger_notice_clock_0x138++;
            if (work->stagger_notice_clock_0x138 > limit_notice) {
                if (mode == 0xFF) {
                    get_lsp_data(0xBAC, &pos);
                    anchor = get_lsp_data(0xBAE, NULL);
                    pos.y = pos.y + (s16)((((plw->field_0x380 * 0xB4) / 150) - anchor->height) - 0x11);
                    notice = hud_notice_spawn(0, 1, pos.x, pos.y, 1, 1, 0);
                } else {
                    switch (mode) {
                    case 0:
                        get_lsp_data(0xCCA, &pos);
                        break;
                    case 1:
                        get_lsp_data(0xCCB, &pos);
                        break;
                    default:
                        pos.y = 0;
                        pos.x = 0;
                        break;
                    }
                    anchor = get_lsp_data(0xCF7, NULL);
                    pos.x += anchor->pos.x;
                    pos.y += anchor->pos.y;
                    anchor = get_lsp_data(0xCF9, NULL);
                    pos.y = pos.y + (s16)((((plw->field_0x380 * 0x84) / 150) - anchor->height) - 0x11);
                    notice = hud_notice_spawn(0, 9, pos.x, pos.y, 1, 1, 0);
                }
                if (notice != 0) {
                    hud_notice_set_flag_ptr(notice, &work->notice_flag_0x139);
                    sysSE_req(0x1E);
                }
                work->stagger_notice_clock_0x138 = 0;
            }
        }
    } else {
        work->stagger_shake_idx_0x137 = 0;
        work->stagger_notice_clock_0x138 = 0x3C;
    }
    s8 flag = 4;
    if (cockpit_hud_hidden_ck(plw, mode) == 1) {
        flag = 0;
    }
    work->notice_flag_0x139 = flag;
    if (work->flash_hold_0x17F != 0) {
        work->flash_frame_0x180++;
        if ((s32)sprite_ary_last_frame(cockpit_flash_anim_ids) < (s32)work->flash_frame_0x180) {
            work->flash_frame_0x180 = 0;
        }
    } else {
        work->flash_frame_0x180 = 0;
    }
    if (work->flash_hold_0x17F != 0) {
        work->flash_hold_0x17F--;
    }
}

/* 0x802E7CFC.  Draws one gauge bar of sprite `id`: the filled part up to `shown` (blending from the first to the
 * second colour, or on to the third past the half), and a tail quad from `shown` to `ghost` in the tag colour. */
void quest_gauge_bar_draw(u16 id, s32 shown, s32 ghost, s32 capacity, s32 units, const HudBlend* colors,
                          const _mh_ivec2_* offset, s16 width) {
    _SPR_DATA_* rec = get_lsp_data(id, NULL);
    f32 bar_width;
    _mh_ivec2_ origin;
    f32 fill;
    f32 scale[2];
    _mh_ivec2_ verts[12];
    u32 cols[12];
    s16 bottom;
    u8 count;

    if (width <= 0) {
        bar_width = (f32)rec->width;
    } else {
        bar_width = (f32)width;
    }
    uv_pair_copy(&origin, &rec->pos);
    origin.x += offset->x;
    origin.y += offset->y;
    fill = (2.0f * (f32)shown) / (f32)capacity;
    scale[0] = bar_width / (f32)units;
    scale[1] = 1.0f;
    _mh_ivec2_* tail = verts;
    u32* tail_cols = cols;
    verts[0].x = verts[3].x = origin.x;
    verts[1].x = verts[2].x = origin.x + shown;
    verts[0].y = verts[1].y = origin.y;
    bottom = origin.y + rec->height;
    verts[2].y = verts[3].y = bottom;
    cols[0] = cols[3] = colors->x;
    if (fill <= 1.0f) {
        count = 4;
        cols[1] = cols[2] = color_lerp(colors->x, colors->y, fill);
    } else {
        count = 8;
        verts[5].x = verts[6].x = verts[2].x;
        verts[1].x = verts[2].x = verts[4].x = verts[7].x = origin.x + capacity / 2;
        verts[4].y = verts[5].y = origin.y;
        verts[6].y = verts[7].y = bottom;
        cols[1] = cols[2] = cols[4] = cols[7] = colors->y;
        cols[5] = cols[6] = color_lerp(colors->y, colors->z, fill - 1.0f);
    }
    if (ghost > shown) {
        tail += count;
        tail_cols += count;
        tail[0].x = tail[3].x = origin.x + shown;
        tail[1].x = tail[2].x = origin.x + ghost;
        tail[0].y = tail[1].y = verts[0].y;
        tail[2].y = tail[3].y = verts[2].y;
        tail_cols[0] = tail_cols[1] = tail_cols[2] = tail_cols[3] = colors->tag;
        count += 4;
    }
    drawshape_init(3, count);
    drawshape_set_vertex_array(verts);
    drawshape_set_color_array(cols);
    drawshape_set_scale_ivec2(&origin.x, scale);
    drawshape_exec();
}

s16 quest_gauge_shake_scale = 2;

s16 quest_gauge_shake_table[12] = {
    0, 1, 0, 2, 0, 3, 0, 3, 0, 3, 0, 3,
};

_mh_ivec2_ quest_health_tick_verts[64] = {
    { 71, 9 }, { 71, 31 }, { 86, 31 }, { 86, 9 },
    { 86, 9 }, { 86, 31 }, { 120, 31 }, { 120, 9 },
    { 120, 9 }, { 120, 31 }, { 154, 31 }, { 154, 9 },
    { 154, 9 }, { 154, 31 }, { 188, 31 }, { 188, 9 },
    { 188, 9 }, { 188, 31 }, { 222, 31 }, { 222, 9 },
    { 222, 9 }, { 222, 31 }, { 256, 31 }, { 256, 9 },
    { 256, 9 }, { 256, 31 }, { 290, 31 }, { 290, 9 },
    { 290, 9 }, { 290, 31 }, { 324, 31 }, { 324, 9 },
    { 324, 9 }, { 324, 31 }, { 358, 31 }, { 358, 9 },
    { 358, 9 }, { 358, 31 }, { 392, 31 }, { 392, 9 },
    { 392, 9 }, { 392, 31 }, { 426, 31 }, { 426, 9 },
    { 426, 9 }, { 426, 31 }, { 460, 31 }, { 460, 9 },
    { 460, 9 }, { 460, 31 }, { 494, 31 }, { 494, 9 },
    { 494, 9 }, { 494, 31 }, { 528, 31 }, { 528, 9 },
    { 528, 9 }, { 528, 31 }, { 562, 31 }, { 562, 9 },
    { 562, 9 }, { 562, 31 }, { 596, 31 }, { 596, 9 },
};

_mh_ivec2_ quest_health_tick_verts_wide[64] = {
    { 71, 9 }, { 71, 31 }, { 86, 31 }, { 86, 9 },
    { 86, 9 }, { 86, 31 }, { 130, 31 }, { 130, 9 },
    { 130, 9 }, { 130, 31 }, { 174, 31 }, { 174, 9 },
    { 174, 9 }, { 174, 31 }, { 218, 31 }, { 218, 9 },
    { 218, 9 }, { 218, 31 }, { 262, 31 }, { 262, 9 },
    { 262, 9 }, { 262, 31 }, { 306, 31 }, { 306, 9 },
    { 306, 9 }, { 306, 31 }, { 350, 31 }, { 350, 9 },
    { 350, 9 }, { 350, 31 }, { 394, 31 }, { 394, 9 },
    { 394, 9 }, { 394, 31 }, { 438, 31 }, { 438, 9 },
    { 438, 9 }, { 438, 31 }, { 482, 31 }, { 482, 9 },
    { 482, 9 }, { 482, 31 }, { 526, 31 }, { 526, 9 },
    { 526, 9 }, { 526, 31 }, { 570, 31 }, { 570, 9 },
    { 570, 9 }, { 570, 31 }, { 614, 31 }, { 614, 9 },
    { 614, 9 }, { 614, 31 }, { 658, 31 }, { 658, 9 },
    { 658, 9 }, { 658, 31 }, { 702, 31 }, { 702, 9 },
    { 702, 9 }, { 702, 31 }, { 746, 31 }, { 746, 9 },
};

_mh_tex_uv_ quest_health_tick_uvs[64] = {
    { 84, 1 }, { 84, 23 }, { 99, 23 }, { 99, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
    { 100, 1 }, { 100, 23 }, { 134, 23 }, { 134, 1 },
};

s16 quest_gauge_flicker_ofs_hit[52] = {
    0, 2, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

s16 quest_gauge_flicker_ofs_idle[28] = {
    0, 2, -2, 1, -1, 1, -1, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

/* 0x802E7FA0.  Draws the quest cockpit's three gauges for the first player: the stamina bar (0xB9E group), the
 * health bar with its tick marks (0xB99 group) and the stagger gauge with its flash (0xBAC group). */
void quest_gauge_draw(void) {
    CockpitWork* work = cockpit_work;
    _PLW* plw = cockpit_work[0].plw;
    s32 wide = ck_WideMode();
    _DRAW_RECT_2D_TEX rect;
    _mh_ivec2_ quad[6];
    _mh_tex_uv_ uvs[6];
    u32 colors[6];
    _mh_ivec2_ spot;
    _mh_ivec2_ flash_pos;
    _mh_ivec2_ corner;
    _mh_ivec2_ pos;
    s32 copy;
    _mh_tex_uv_ uv_a;
    s32 anchor_a;
    s32 anchor_b;
    _SPR_DATA_* rec;
    _mh_ivec2_* ticks;
    s16 cell;
    u32 cells;
    u32 tick_count;
    s32 tick_end;
    u16 scaled_max;
    u16 shown;
    u16 top;
    s16 shake;
    f32 fill;
    u16 count;

    set_blendmode(4, 5, 1);
    get_lsp_data(0xB9E, &spot);
    pos.y = 0;
    pos.x = 0;
    rec = get_lsp_data(0xB9F, NULL);
    if (wide == 0) {
        cell = rec->width;
    } else {
        cell = 0x6E;
    }
    cells = plw->field_0x37A / 150;
    corner.x = rec->pos.x + spot.x;
    corner.y = rec->pos.y + spot.y;
    draw_rect_2d_tex_init(&rect, &corner.x, (u16)(cell * cells), rec->height, rec->color, &rec->uv0.u, &rec->uv1.u);
    draw_rect_2d_tex_by_id(&rect, rec->tex_flag);
    pos.x = (s16)(cell * (cells - 1)) + spot.x;
    pos.y += spot.y;
    if ((u32)wide == 1) {
        pos.x += 0x19;
    }
    draw_sprite_idx(0xBA0, &pos);
    uv_pair_copy(&copy, &pos);
    if (wide == 0) {
        quest_gauge_bar_draw(0xBA1, plw->field_0x378, 0, plw->field_0x37A, 0x384, &work->bar_b_0x154, &spot, 0);
    } else {
        quest_gauge_bar_draw(0xBA1, plw->field_0x378, 0, plw->field_0x37A, 0x384, &work->bar_b_0x154, &spot,
                             0x294);
    }
    if (plw->field_0x37A >= 0x384) {
        if (work->sparkle_b_frame_0x18E < 0x60) {
            work->sparkle_b_frame_0x18E++;
            anchor_a = copy;
            put_lsp_anchor_offset(0xBA4, &spot, &anchor_a);
            draw_sprite_anim_idx(0xBA5, work->sparkle_b_frame_0x18E, &spot);
        }
    } else {
        work->sparkle_b_frame_0x18E = 0;
    }
    get_lsp_data(0xB99, &spot);
    pos.y = 0;
    pos.x = 0;
    if (work->health_hit_clock_0x13A != 0) {
        pos.y = quest_gauge_shake_scale * quest_gauge_shake_table[work->health_hit_clock_0x13A];
    }
    pos.x += spot.x;
    pos.y += spot.y;
    tick_count = plw->health_max / 10;
    if ((s32)tick_count > 0xF) {
        tick_count = 0xF;
    }
    tick_end = tick_count * 4;
    drawshape_init(3, (u16)(tick_end + 4));
    ticks = quest_health_tick_verts;
    if (wide != 0) {
        ticks = quest_health_tick_verts_wide;
    }
    drawshape_set_vertex_array(ticks);
    drawshape_set_flat_color(-1);
    drawshape_set_texture_array(0xD, quest_health_tick_uvs);
    drawshape_set_offset_ivec2(&pos.x);
    drawshape_exec();
    if (wide == 0) {
        pos.x += (s16)(quest_health_tick_verts[tick_end].x - quest_health_tick_verts[4].x);
    } else {
        pos.x = (s16)(pos.x + (s16)(quest_health_tick_verts_wide[tick_end].x - quest_health_tick_verts_wide[4].x)) + 0xA;
    }
    draw_sprite_idx(0xB9C, &pos);
    uv_pair_copy(&copy, &pos);
    pos = spot;
    if (wide == 0) {
        quest_gauge_bar_draw(0xB9D, plw->health, plw->field_0x376, plw->health_max, 0x96, &work->bar_a_0x144,
                             &pos, 0);
    } else {
        quest_gauge_bar_draw(0xB9D, plw->health, plw->field_0x376, plw->health_max, 0x96, &work->bar_a_0x144,
                             &pos, 0x294);
    }
    if (plw->health_max >= 0x96) {
        if (work->sparkle_a_frame_0x18C < 0x60) {
            work->sparkle_a_frame_0x18C++;
            anchor_b = copy;
            put_lsp_anchor_offset(0xBA2, &spot, &anchor_b);
            draw_sprite_anim_idx(0xBA3, work->sparkle_a_frame_0x18C, &spot);
        }
    } else {
        work->sparkle_a_frame_0x18C = 0;
    }
    get_lsp_data(0xBAC, &spot);
    scaled_max = (plw->field_0x380 * 0xB4) / 150;
    shown = (work->stagger_shown_0x134 * 0xB4) / 150;
    top = scaled_max - 0x11;
    if (plw->kind_0x09 != 3) {
        draw_sprite_idx(0xBB0, &spot);
        rec = get_lsp_data(0xBB1, NULL);
        corner.x = rec->pos.x + spot.x;
        corner.y = rec->pos.y + spot.y;
        draw_rect_2d_tex_init(&rect, &corner.x, rec->width, top, rec->color, &rec->uv0.u, &rec->uv1.u);
        draw_rect_2d_tex_by_id(&rect, rec->tex_flag);
        pos.x = spot.x;
        pos.y = spot.y + (top - rec->height);
        draw_sprite_idx(0xBB2, &pos);
    } else {
        if (plw->field_0x37E != 0) {
            shake = quest_gauge_flicker_ofs_hit[work->stagger_shake_idx_0x137];
        } else {
            shake = quest_gauge_flicker_ofs_idle[work->stagger_shake_idx_0x137];
        }
        pos.x = shake + spot.x;
        pos.y = spot.y;
        draw_sprite_idx(0xBAD, &pos);
        rec = get_lsp_data(0xBAF, NULL);
        corner.x = rec->pos.x + pos.x;
        corner.y = rec->pos.y + pos.y;
        draw_rect_2d_tex_init(&rect, &corner.x, rec->width, rec->height, rec->color, &rec->uv0.u, &rec->uv1.u);
        rec = get_lsp_data(0xBAE, NULL);
        rect.y = rec->pos.y;
        rect.end_y += (s16)(top - rec->height);
        rect.v0 -= top;
        draw_rect_2d_tex_by_id(&rect, rec->tex_flag);
    }
    if (work->stagger_flash_0x136 != 0) {
        rec = get_lsp_data(0xBB3, NULL);
        flash_pos.x = rec->pos.x;
        flash_pos.y = (rec->pos.y + scaled_max) - shown;
        uv_a.u = rec->uv0.u;
        uv_a.v = rec->uv1.v - shown;
        if (work->stagger_shake_idx_0x137 != 0) {
            draw_rect_2d_tex_init(&rect, &flash_pos.x, rec->width, shown,
                                  work->stagger_flash_0x136 | 0xC1183600, &uv_a.u, &rec->uv1.u);
            rect.x += pos.x;
            rect.end_x += pos.x;
            draw_rect_2d_tex_by_id(&rect, rec->tex_flag);
            return;
        }
        flash_pos.x += spot.x;
        flash_pos.y += spot.y;
        fill = (2.0f * (f32)work->stagger_shown_0x134) / (f32)plw->field_0x380;
        quad[0].x = quad[2].x = flash_pos.x;
        quad[1].x = quad[3].x = flash_pos.x + rec->width;
        quad[0].y = quad[1].y = flash_pos.y;
        quad[2].y = quad[3].y = flash_pos.y + shown;
        uvs[0].u = uvs[2].u = rec->uv0.u;
        uvs[1].u = uvs[3].u = rec->uv1.u;
        uvs[0].v = uvs[1].v = uv_a.v;
        if (fill <= 1.0f) {
            colors[0] = colors[1] = work->stagger_flash_0x136 | color_lerp(0x93DDF500, 0x16D9D000, fill);
            colors[2] = colors[3] = work->stagger_flash_0x136 | 0x93DDF500;
            uvs[2].v = uvs[3].v = rec->uv1.v;
            count = 4;
        } else {
            s16 half;
            quad[4].x = flash_pos.x;
            quad[5].x = quad[1].x;
            quad[4].y = quad[5].y = quad[3].y;
            half = (plw->field_0x380 * 0xB4) / 300;
            quad[2].y = quad[3].y = quad[3].y - half;
            colors[0] = colors[1] = work->stagger_flash_0x136 | color_lerp(0x16D9D000, 0xB79200, fill - 1.0f);
            colors[2] = colors[3] = work->stagger_flash_0x136 | 0x16D9D000;
            colors[4] = colors[5] = work->stagger_flash_0x136 | 0x93DDF500;
            uvs[4].u = uvs[0].u;
            uvs[5].u = uvs[1].u;
            uvs[2].v = uvs[3].v = rec->uv1.v - half;
            uvs[4].v = uvs[5].v = rec->uv1.v;
            count = 6;
        }
        drawshape_init(5, count);
        drawshape_set_vertex_array(quad);
        drawshape_set_color_array(colors);
        drawshape_set_texture_array(rec->tex_flag, uvs);
        drawshape_exec();
    }
}

/* 0x802E884C (0x70).  "Is this quest target usable by the player?": `quest_target_usable_ck` is asked twice -
 * a global gate first (the same arguments with flags 0), then the per-target test when the gate
 * returns 1.  The result is the boolean, materialised as 1/0 by the branch. */
s32 quest_target_visible_ck(CockpitWork* work, _ENEMY_WORK* target) {
    u8 ok = quest_target_usable_ck(work, target, 0);

    if (ok == 1) {
        ok = quest_target_usable_ck(work, target, 1);
    }
    if (ok != 0) {
        return 1;
    }
    return 0;
}

/* A sprite record copied as nine words (the target copies the layout record with `lwz`/`stw` pairs, never with the
 * field-wise copy `_SPR_DATA_` assignment produces); `window` views the copy as the record.  size: 0x24 */
typedef struct SprRecordWords {
    /* +0x00 */ s32 word[9];
} SprRecordWords;

/* 0x802E88BC.  Spawns the quest window's notice for marker `idx` when the cockpit allows it: the window spot is taken
 * off the layout record, then the marker's screen position decides where the notice appears. */
void quest_notice_spawn(s32 unused_ctx, u8 idx) {
    CockpitWork* work;
    HudNotice* notice;
    SprRecordWords rec;
    f32 scale[2];
    _mh_ivec2_ spot;
    _mh_ivec2_ pos;
    _SPR_DATA_* window = (_SPR_DATA_*)&rec;

    if ((u32)screen_split_mode_ck() != 1 && Pl_motion_input_ck(0) != 1) {
        work = cockpit_work;
        if (get_option_cfg(14) != 1 && work->mark_gate_0x0CE != 1) {
            get_lsp_data(0xBE3, &spot);
            rec = *(const SprRecordWords*)get_lsp_data(0xBE4, NULL);
            window->pos.x += spot.x;
            work->window_x_0x084 = window->pos.x;
            work->window_y_0x086 = window->pos.y;
            work->window_scale_0x088 = (f32)window->width;
            if (quest_marker_screen_pos(work, &pos, scale, idx) != 0) {
                notice = hud_notice_spawn(0, 0, pos.x, pos.y, 1, 1, 0);
                if (notice != 0) {
                    hud_notice_set_flag_ptr(notice, &work->notice_flag_0x0CF);
                }
            }
        }
    }
}

/* 0x802E8A20.  `quest_notice_spawn` for one quest target: only when the target is visible and the player's motion
 * is not taking input. */
void quest_target_notice_spawn(_ENEMY_WORK* target, u8 idx) {
    CockpitWork* work;
    HudNotice* notice;
    SprRecordWords rec;
    f32 scale[2];
    _mh_ivec2_ spot;
    _mh_ivec2_ pos;
    _SPR_DATA_* window = (_SPR_DATA_*)&rec;

    if ((u32)screen_split_mode_ck() != 1) {
        work = cockpit_work;
        if (get_option_cfg(14) != 1 && work->mark_gate_0x0CE != 1 && quest_target_visible_ck(work, target) != 0) {
            get_lsp_data(0xBE3, &spot);
            rec = *(const SprRecordWords*)get_lsp_data(0xBE4, NULL);
            window->pos.x += spot.x;
            work->window_x_0x084 = window->pos.x;
            work->window_y_0x086 = window->pos.y;
            work->window_scale_0x088 = (f32)window->width;
            if (Pl_motion_input_ck(0) == 0 && quest_marker_screen_pos(work, &pos, scale, idx) != 0) {
                notice = hud_notice_spawn(0, 0, pos.x, pos.y, 1, 1, 0);
                if (notice != 0) {
                    hud_notice_set_flag_ptr(notice, &work->notice_flag_0x0CF);
                }
            }
        }
    }
}

/* 0x802E8BA4 (0xE8).  Fill the quest-target bitmask: every live record of the kind-3 move table
 * whose `quest_target_visible_ck` predicate holds raises the bit at its own index, and the two quest-view
 * fields are reset before the count.  The target's `loop_5` is a `for` over the table
 * (`get_move_work_max(3)` entries, stride 0xB18); the residual is in the file header. */
void quest_targets_update_a(CockpitWork* work) {
    _ENEMY_WORK* target;
    s32 count;
    s8 state;

    work->target_rank_0x0D0 = quest_target_rank_get(work, work->plw);
    work->target_mask_0x0BC = 0;
    target = (_ENEMY_WORK*)get_move_work_adrs(3);
    count = get_move_work_max(3);
    while (count > 0) {
        if ((target->field_0x1C8 & 1) != 0 && quest_target_visible_ck(work, target) != 0) {
            work->target_mask_0x0BC |= (u16)(1 << target->area_no);
        }
        count--;
        target++;
    }
    state = 4;
    if (quest_mark_visible_ck(work) != 0) {
        state = 0;
    }
    work->notice_flag_0x0CF = state;
    quest_view_frame_task(work, 0);
}

/* 0x802E8C8C (0xF0).  `quest_targets_update_a`'s twin for the second cockpit view: the same target scan and
 * countdown, with the per-frame task argument 1, and the player state's pending flag latched into
 * its live one at the end. */
void quest_targets_update_b(CockpitWork* work) {
    CockpitState* state = &cockpit_state;
    _ENEMY_WORK* target;
    s32 count;
    s8 view_state;

    work->target_rank_0x0D0 = quest_target_rank_get(work, work->plw);
    work->target_mask_0x0BC = 0;
    target = (_ENEMY_WORK*)get_move_work_adrs(3);
    count = get_move_work_max(3);
    while (count > 0) {
        if ((target->field_0x1C8 & 1) != 0 && quest_target_visible_ck(work, target) != 0) {
            work->target_mask_0x0BC |= (u16)(1 << target->area_no);
        }
        count--;
        target++;
    }
    view_state = 4;
    if (quest_mark_visible_ck(work) != 0) {
        view_state = 0;
    }
    work->notice_flag_0x0CF = view_state;
    quest_view_frame_task(work, 1);
    state->pit_trap_shown_0x059 = state->pit_trap_count_0x058;
    state->pit_trap_count_0x058 = 0;
}

/* 0x802E8D7C (0xB8).  "May this view's quest mark be shown?": the view gate first, then the option
 * configuration (`get_option_cfg(14)` in normal play, `get_arena_cfg(player chunk, 14)` in VS
 * mode), then the item menu's frame, and finally the player work record's own +0x5BC byte. */
s32 quest_mark_visible_ck(CockpitWork* work) {
    if (work->mark_gate_0x0CE != 0) {
        return 0;
    }
    if (system_w.vs_mode_0x8b0 == 0) {
        if (get_option_cfg(14) == 1) {
            return 0;
        }
    } else if (get_arena_cfg(work->plw->chunk_ofs, 14) == 1) {
        return 0;
    }
    /* `menu/menu_item.h` names the record `menu_item_frame_update` walks `MenuFrameWork`; it is the same
     * player work record this header calls `_PLW` (both views name +0x008 and +0x5BC), and the
     * target's call site passes the `CockpitWork::plw` pointer unchanged. */
    if (menu_item_frame_update((MenuFrameWork*)work->plw) == 1) {
        return 0;
    }
    return work->plw->field_0x5BC == 0;
}

/* 0x802E8E34 (0x28).  Toggle the view's +0xCE gate and report its new value. */
u8 quest_mark_toggle(CockpitWork* work) {
    if (work->mark_gate_0x0CE == 0) {
        work->mark_gate_0x0CE = 1;
    } else {
        work->mark_gate_0x0CE = 0;
    }
    return work->mark_gate_0x0CE;
}

/* 0x802E8E5C (0x8).  The view's +0xCE gate. */
u8 quest_mark_flag_get(CockpitWork* work) {
    return work->mark_gate_0x0CE;
}

/* 0x802E8E64.  Arms the quest window's marker draw for the first player: the window spot is slid to the framed
 * position (and the ring's scale trio armed) when the view gate is set, then the marker drawer runs. */
void quest_marker_arm(void) {
    CockpitWork* work = cockpit_work;
    _mh_ivec2_ pos;

    if (get_cfg(cockpit_work[0].plw->chunk_ofs, 14) == 1) {
        return;
    }
    set_blendmode(4, 5, 1);
    get_lsp_data(0xBE3, &pos);
    pos.y = 0;
    if (work->mark_gate_0x0CE == 0) {
        work->draw_id_0x0BA = 0xBE5;
        {
            _mh_ivec2_ mark = pos;
            quest_marker_draw(work, 0xBE4, 0, &mark);
        }
    } else {
        pos.x += 0x20C;
        pos.y = 0xC6;
        work->ring_scale_0x0A0 = 2.3499999f;
        work->ring_extent_x_0x0A4 = 0.110815607f;
        work->ring_extent_y_0x0A8 = 0.166223407f;
        work->draw_id_0x0BA = 0xBE5;
        {
            _mh_ivec2_ mark = pos;
            quest_marker_draw_faded(work, 0xBE4, 0, &mark);
        }
    }
}

/* 0x802E8F54 (0x68).  Project a world position into the screen space `screen_projection_get` describes:
 * x and z are divided by the two scales after the offsets are taken out. */
void quest_screen_project(f32* out, const f32* pos) {
    const QuestScreen* scr = screen_projection_get();

    out[0] = (pos[0] - scr->ofs_x) / scr->scale_x;
    out[1] = (pos[2] - scr->ofs_y) / scr->scale_y;
}

/* 0x802E8FBC (0xB4).  The squared screen distance of the marker: the offset is rotated by the
 * view's rotation clock and carried by the two scales of +0xAC/+0xB0, and the two squares are
 * added. */
#pragma fp_contract off
f32 quest_mark_dist_sq(CockpitWork* work, const f32* pos) {
    f32 rot[2];
    f32 dx = pos[0] - work->marker_ref_x_0x098;
    f32 dy = pos[1] - work->marker_ref_y_0x09C;
    f32 x;
    f32 y;

    quest_rot_pair(&rot[1], &rot[0], work->ring_angle_0x0B8);
    x = dx * rot[0] - dy * rot[1];
    y = dx * rot[1] + dy * rot[0];
    x *= work->ring_inv_x_0x0AC;
    y *= work->ring_inv_y_0x0B0;
    return x * x + y * y;
}
#pragma fp_contract on

/* 0x802E9070 (0x50).  Sine and cosine of the ring's rotation clock `t`: `anim_tick_angle` maps the ticks to an angle and
 * `math_sincos_idx` splits it (`0.00390625f` is the index gain). */
void quest_rot_pair(f32* a, f32* b, u16 t) {
    math_sincos_idx(a, b, 0.00390625f * anim_tick_angle(t));
}

/* 0x802E90C0 (0x3C).  Project and hand the result to `quest_mark_dist_sq`, which turns it into the
 * squared screen distance of the marker. */
f32 quest_mark_dist(CockpitWork* work, const f32* pos) {
    f32 sp8[2];

    quest_screen_project(sp8, pos);
    return quest_mark_dist_sq(work, sp8);
}

/* 0x802E90FC (0x34).  "Is the marker inside the screen?" - the projection is compared against the
 * 1.0f at `1.0f`, and the target's `cror eq,lt,eq` + `mfcr`/`rlwinm` is MWCC's `<=` in an
 * integer context. */
s32 quest_mark_on_screen_ck(CockpitWork* work, const f32* pos) {
    return quest_mark_dist(work, pos) <= 1.0f;
}

/* 0x802E9130 (0x6C).  Fades the marker colour's alpha byte by how far the target is: inside `work->ring_radius_0x0B4`
 * squared the colour stands, outside it the alpha is raised by the remaining fraction of 1.0f. */
u32 quest_mark_fade_value(CockpitWork* work, u32 color, f32 dist) {
    f32 fade = work->ring_radius_0x0B4;
    f32 near = fade * fade;
    f32 t;

    if (dist < near) {
        return color;
    }
    t = (1.0f - dist) / (1.0f - near);
    color = (color & 0xFFFFFF00) + (u8)(t * (f32)(u8)color);
    return color;
}

/* 0x802E919C.  Blits one cell of the marker sheet (`index` picks the column and row of a ten-wide sheet) as a
 * 4096-unit rectangle centred on `pos`, with an optional second texture pass at `pos2`. */
void quest_marker_icon_draw(CockpitWork* work, const f32* pos, u8 index, u16 tex_no, u32 color, const f32* pos2,
                            u16 tex_no2, f32 scale, f32 scale2) {
    _SPR_DATA_* rec = get_lsp_data(work->draw_id_0x0BA, NULL);
    s16 cell_u = rec->uv1.u - rec->uv0.u;
    s16 cell_v = rec->uv1.v - rec->uv0.v;
    _mh_tex_uv_ lo;
    _mh_tex_uv_ hi;

    lo.u = rec->uv0.u + cell_u * (index % 10);
    lo.v = rec->uv0.v + cell_v * (index / 10);
    hi.u = lo.u + cell_u;
    hi.v = lo.v + cell_v;
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(pos[0] - 2048.0f, pos[1] - 2048.0f, 4096.0f, 4096.0f);
    drawshape_set_flat_color(color);
    drawshape_set_texture_rect(rec->tex_flag, &lo, &hi);
    drawshape_set_tex_scale_uniform(pos, tex_no, 0.000244140625f * scale);
    if (pos2 != 0) {
        drawshape_set_tex_scale_uniform(pos2, tex_no2, scale2);
    }
    drawshape_exec();
}

/* 0x802E932C.  Projects the world position `pos` and draws the marker icon for it: at the window spot when `clip` is
 * 0, otherwise at the marker ring's position when it lies inside the screen; returns the squared screen distance. */
#pragma fp_contract off
f32 quest_marker_draw_at(CockpitWork* work, const f32* pos, u8 index, u16 tex_no, u32 color, s32 clip, f32 scale) {
    f32 proj[2];
    f32 dist;
    f32 dx;
    f32 dy;

    quest_screen_project(proj, pos);
    if (clip != 0) {
        dist = quest_mark_dist_sq(work, proj);
        if (dist <= 1.0f) {
            f32 out[2];
            dx = proj[0] - work->marker_ref_x_0x098;
            dy = proj[1] - work->marker_ref_y_0x09C;
            out[0] = work->marker_ofs_x_0x090 + 384.0f * dx;
            out[1] = work->marker_ofs_y_0x094 + 384.0f * dy;
            scale = scale / work->ring_scale_0x0A0;
            quest_marker_icon_draw(work, out, index, tex_no, quest_mark_fade_value(work, color, dist),
                                   &work->marker_ofs_x_0x090, work->ring_angle_0x0B8, scale, work->ring_scale_0x0A0);
        }
    } else {
        f32 out[2];
        out[0] = (f32)work->window_x_0x084 + work->window_scale_0x088 * proj[0];
        out[1] = (f32)work->window_y_0x086 + work->window_scale_0x088 * proj[1];
        quest_marker_icon_draw(work, out, index, tex_no, color, 0, 0, scale, 1.0f);
        dist = 0.0f;
    }
    return dist;
}
#pragma fp_contract on

f32 quest_marker_ring_outline_verts[20] = {
    0.0f, 0.0f, 0.0f, -0.75f,
    0.530250013f, -0.530250013f, 0.75f, 0.0f,
    0.530250013f, 0.530250013f, 0.0f, 0.75f,
    -0.530250013f, 0.530250013f, -0.75f, 0.0f,
    -0.530250013f, -0.530250013f, 0.0f, -0.75f,
};

f32 quest_marker_ring_fill_verts[36] = {
    0.0f, -1.0f, 0.0f, -0.75f,
    0.707000017f, -0.707000017f, 0.530250013f, -0.530250013f,
    1.0f, 0.0f, 0.75f, 0.0f,
    0.707000017f, 0.707000017f, 0.530250013f, 0.530250013f,
    0.0f, 1.0f, 0.0f, 0.75f,
    -0.707000017f, 0.707000017f, -0.530250013f, 0.530250013f,
    -1.0f, 0.0f, -0.75f, 0.0f,
    -0.707000017f, -0.707000017f, -0.530250013f, -0.530250013f,
    0.0f, -1.0f, 0.0f, -0.75f,
};

f32 quest_marker_pie_verts[52] = {
    0.0f, 0.0f, 1.0f, 0.0f,
    0.965924978f, 0.258819014f, 0.866024971f, 0.5f,
    0.707105994f, 0.707105994f, 0.5f, 0.866024971f,
    0.258819014f, 0.965924978f, -0.0f, 1.0f,
    -0.258819014f, 0.965924978f, -0.5f, 0.866024971f,
    -0.707105994f, 0.707105994f, -0.866024971f, 0.5f,
    -0.965924978f, 0.258819014f, -1.0f, -0.0f,
    -0.965924978f, -0.258819014f, -0.866024971f, -0.5f,
    -0.707105994f, -0.707105994f, -0.5f, -0.866024971f,
    -0.258819014f, -0.965924978f, 0.0f, -1.0f,
    0.258819014f, -0.965924978f, 0.5f, -0.866024971f,
    0.707105994f, -0.707105994f, 0.866024971f, -0.5f,
    0.965924978f, -0.258819014f, 1.0f, 0.0f,
};

/* 0x802E94D0.  Draws the marker ring: an outline pass in the flat colour, then the filled pass whose colours
 * alternate between the colour with and without its alpha byte. */
void quest_marker_ring_draw(const f32* offset, const f32* tex_offset, u32 color, f32 scale, f32 tex_scale) {
    u32 cols[18];
    f32 size;

    cols[0] = color & 0xFFFFFF00;
    cols[1] = color;
    cols[2] = color & 0xFFFFFF00;
    cols[3] = color;
    cols[4] = color & 0xFFFFFF00;
    cols[5] = color;
    cols[6] = color & 0xFFFFFF00;
    cols[7] = color;
    cols[8] = color & 0xFFFFFF00;
    cols[9] = color;
    cols[10] = color & 0xFFFFFF00;
    cols[11] = color;
    cols[12] = color & 0xFFFFFF00;
    cols[13] = color;
    cols[14] = color & 0xFFFFFF00;
    cols[15] = color;
    cols[16] = color & 0xFFFFFF00;
    cols[17] = color;
    size = scale * (384.0f * tex_scale);
    drawshape_init(6, 10);
    drawshape_set_vertex_array_f32(quest_marker_ring_outline_verts);
    drawshape_set_flat_color(color);
    drawshape_set_texture_array_f32(0x13, quest_marker_ring_outline_verts);
    drawshape_set_offset_uniform(offset, size);
    drawshape_set_tex_offset_uniform(tex_offset, tex_scale);
    drawshape_exec();
    drawshape_init(5, 18);
    drawshape_set_vertex_array_f32(quest_marker_ring_fill_verts);
    drawshape_set_color_array(cols);
    drawshape_set_texture_array_f32(0x13, quest_marker_ring_fill_verts);
    drawshape_set_offset_uniform(offset, size);
    drawshape_set_tex_offset_uniform(tex_offset, tex_scale);
    drawshape_exec();
}

/* 0x802E962C.  Draws the marker's pie: one flat-coloured fan, then the same 25 rim points scaled by the inverse of
 * `tex_scale` as a textured strip. */
void quest_marker_pie_draw(const f32* offset, const f32* tex_offset, const f32* extent, u32 color, u16 tex_no,
                           f32 scale, f32 tex_scale) {
    f32 strip[100];
    u32 cols[50];
    f32 tex_ext[2];
    f32 ofs[2];
    f32 inverse = 1.0f / tex_scale;
    const f32* src;
    f32* dst;
    u32* col;
    s32 n;
    u32 faded;

    tex_ext[0] = tex_scale * extent[0];
    tex_ext[1] = tex_scale * extent[1];
    ofs[0] = scale * (384.0f * tex_ext[0]);
    ofs[1] = scale * (384.0f * tex_ext[1]);
    faded = color & 0xFFFFFF00;
    drawshape_init(6, 26);
    drawshape_set_vertex_array_f32(quest_marker_pie_verts);
    drawshape_set_flat_color(color);
    drawshape_set_texture_array_f32(0x13, quest_marker_pie_verts);
    drawshape_set_offset_f32(ofs, offset);
    drawshape_set_tex_offset_f32(tex_ext, tex_no, tex_offset);
    drawshape_exec();
    src = quest_marker_pie_verts + 2;
    dst = strip;
    col = cols;
    n = 25;
    do {
        dst[0] = src[0] * inverse;
        dst[1] = src[1] * inverse;
        copyVec2(dst + 2, src);
        col[0] = faded;
        col[1] = color;
        n--;
        src += 2;
        dst += 4;
        col += 2;
    } while (n > 0);
    drawshape_init(5, 50);
    drawshape_set_vertex_array_f32(strip);
    drawshape_set_color_array(cols);
    drawshape_set_texture_array_f32(0x13, strip);
    drawshape_set_offset_f32(ofs, offset);
    drawshape_set_tex_offset_f32(tex_ext, tex_no, tex_offset);
    drawshape_exec();
}

f32 quest_pit_area_offsets[2][3] = {
    { -9500.0f, 0.0f, 5500.0f },
    { 5500.0f, 0.0f, 7700.0f },
};

/* 0x802E97A8.  The player's marker position: the actor position, moved by the pit area's entry offset when the
 * current map is the pit map and the player is not flagged. */
void quest_player_marker_pos(_PLW* plw, VEC3* out) {
    VEC3 to;
    VEC3 from;
    u32 extra_to;
    u32 extra_from;

    copyVec3(out, &plw->vec_0x03C);
    if (stage_map_kind_get(get_now_mapno()) == 9 && Pl_area_flag_get(plw->chunk_ofs) == 0) {
        VEC3_ctor(&to);
        VEC3_ctor(&from);
        stage_area_point_get(plw->area_0x16, 0, &from, &extra_from);
        stage_area_point_get(plw->area_0x16, 1, &to, &extra_to);
        out->x = out->x + (quest_pit_area_offsets[plw->area_0x16][0] + (to.x - from.x));
        out->z = out->z + (quest_pit_area_offsets[plw->area_0x16][2] + (to.z - from.z));
    }
}

/* 0x802E98A8.  Draws one player's marker: the world position is projected and the marker icon is drawn, and the
 * player's slot is appended to the mark table with its colour (faded by distance when `clip`). */
#pragma fp_contract off
void quest_player_marker_draw(CockpitWork* work, _PLW* plw, s32 clip) {
    VEC3 pos;
    VEC3 local;
    f32 proj[2];
    _mh_ivec2_ mark;
    u32 icon;
    f32 scale;
    f32 dist;
    u32 color;
    u32 slot_color;
    u16 slot_val;

    VEC3_ctor(&pos);
    VEC3_ctor(&local);
    quest_player_marker_pos(plw, &local);
    const VEC3& world = get_worldworld_pos(&local, plw->area_0x16);
    copyVec3(&pos, &world);
    icon = Pl_master_ck(plw) == 1 ? 0 : 10;
    if (clip != 0) {
        scale = 25.0f;
    } else {
        scale = 0.846153855f * (25.0f * work->window_icon_scale_0x08C);
    }
    slot_color = player_color_get(plw->chunk_ofs);
    dist = quest_marker_draw_at(work, &pos.x, icon, (u16)(f32)(u16)(0x18000 - plw->field_0x058), slot_color, clip, scale);
    if (work->slot_timer_0x0C0[plw->chunk_ofs] >= 0) {
        quest_screen_project(proj, &pos.x);
        slot_val = work->slot_timer_0x0C0[plw->chunk_ofs];
        if (work->slot_state_0x0C9[plw->chunk_ofs] == 0) {
            color = slot_color;
        } else {
            color = 0xF0F0F0FF;
        }
        if (clip != 0) {
            if (dist <= 1.0f) {
                mark.x = work->marker_ofs_x_0x090 + 384.0f * (proj[0] - work->marker_ref_x_0x098);
                mark.y = work->marker_ofs_y_0x094 + 384.0f * (proj[1] - work->marker_ref_y_0x09C);
                quest_mark_append(work, &mark, 1, quest_mark_fade_value(work, color, dist), slot_val);
            }
        } else {
            mark.x = (f32)work->window_x_0x084 + work->window_scale_0x088 * proj[0];
            mark.y = (f32)work->window_y_0x086 + work->window_scale_0x088 * proj[1];
            quest_mark_append(work, &mark, 1, color, slot_val);
        }
    }
}

#pragma fp_contract on

/* 0x802E9B48.  Draws the markers of every live non-master player, then the local player's. */
void quest_marker_draw_all_non_master(CockpitWork* work, s32 clip) {
    _PLW* plw = (_PLW*)get_move_work_adrs(2);
    s32 count = get_move_work_max(2);

    while (count > 0) {
        if (plw->slot_active != 0 && Pl_master_ck(plw) == 0) {
            quest_player_marker_draw(work, plw, clip);
        }
        count--;
        plw++;
    }
    plw = (_PLW*)get_move_work_adrs(2);
    quest_player_marker_draw(work, plw + (s8)my_player_no(), clip);
}

/* 0x802E9C0C.  Draws the markers of every live player but the cockpit's own, then the cockpit's own. */
void quest_marker_draw_others_then_self(CockpitWork* work, s32 clip) {
    _PLW* plw = (_PLW*)get_move_work_adrs(2);
    s32 count = get_move_work_max(2);

    while (count > 0) {
        if (plw->slot_active != 0 && plw != work->plw) {
            quest_player_marker_draw(work, plw, clip);
        }
        count--;
        plw++;
    }
    quest_player_marker_draw(work, work->plw, clip);
}

/* 0x802E9CB4.  Draws every live non-master player's marker, then the cockpit player's own (dimmed, unless the map
 * is kind 6) and the master's at fixed scales. */
void quest_marker_draw_with_master(CockpitWork* work, _PLW* plw) {
    VEC3 pos;
    _PLW* other;
    s32 count;
    f32 scale;
    _PL_ROOT* root;

    VEC3_ctor(&pos);
    other = (_PLW*)get_move_work_adrs(2);
    count = get_move_work_max(2);
    while (count > 0) {
        if (other->slot_active != 0 && Pl_master_ck(other) == 0) {
            quest_player_marker_draw(work, other, 0);
        }
        count--;
        other++;
    }
    if ((u8)stage_map_kind_get(get_now_mapno()) != 6) {
        const VEC3& world = get_worldworld_pos(&plw->vec_0x03C, plw->area_0x16);
        copyVec3(&pos, &world);
        scale = 25.0769234f;
        quest_marker_draw_at(work, &pos.x, 0, (u16)(f32)(u16)(0x18000 - plw->field_0x058),
                             color_mult(player_color_get(plw->chunk_ofs), 0x999999FF), 0, scale);
    }
    root = (_PL_ROOT*)get_move_work_adrs(0);
    {
        const VEC3& world = get_worldworld_pos(&root->pos_0x128, root->area_no_0xEB);
        copyVec3(&pos, &world);
        scale = 31.3461533f;
        quest_marker_draw_at(work, &pos.x, 0, (u16)(f32)(u16)(0x18000 - root->rot_y_0x134),
                             player_color_get(plw->chunk_ofs), 0, scale);
    }
}

u16 quest_view_hold_ids[4] = { 0x0BEA, 0x0BEB, 0xFFFF, 0 };
u16 quest_view_hold_ids_b[4] = { 0x0BED, 0x0BEE, 0xFFFF, 0 };

u16 quest_view_icon_ids[6] = {
    0x0BF0, 0x0BF1, 0x0BF2, 0x0BF3, 0xFFFF, 0x0000,
};

/* 0x802E9E9C.  Steps the four slot timers (and, for view 1, the view timer): a timer at or past the icon animation's
 * last frame is cleared, the slot's flag drops when the hold animation ends, and the timers ring the SEs at frames
 * 0, 30 and 60. */
void quest_view_frame_task(CockpitWork* work, u32 view) {
    s32 i = 0;
    s16 view_timer;

    do {
        if (work->slot_timer_0x0C0[i] >= 0) {
            if ((s32)sprite_ary_last_frame(quest_view_icon_ids) <= work->slot_timer_0x0C0[i]) {
                work->slot_timer_0x0C0[i] = -1;
            } else {
                u8 bit = 1 << i;

                if ((work->slot_flags_0x0C8 & bit) != 0 && (s32)sprite_ary_last_frame(quest_view_hold_ids) <= work->slot_timer_0x0C0[i]) {
                    work->slot_flags_0x0C8 &= (u8)~bit;
                }
                switch (work->slot_timer_0x0C0[i]) {
                case 0:
                    sysSE_req(0x11);
                    break;
                case 0x1E:
                    sysSE_req(0x12);
                    break;
                case 0x3C:
                    sysSE_req(0x13);
                    break;
                }
                work->slot_timer_0x0C0[i]++;
            }
        }
        i++;
    } while (i < 4);
    if (view == 1 && (view_timer = work->view_timer_0x0BE, view_timer >= 0)) {
        if ((s32)sprite_ary_last_frame(quest_view_icon_ids) <= view_timer) {
            work->view_timer_0x0BE = -1;
            return;
        }
        if ((s32)sprite_ary_last_frame(quest_view_hold_ids) <= view_timer) {
            work->view_timer_0x0BE = -1;
            return;
        }
        switch (view_timer) {
        case 0:
            sysSE_req(0x11);
            break;
        case 0x1E:
            sysSE_req(0x12);
            break;
        case 0x3C:
            sysSE_req(0x13);
            break;
        }
        work->view_timer_0x0BE++;
    }
}

/* 0x802EA050 (0x7C).  Every live player's `quest_slot_arm_a` arm must have taken for the caller's
 * action to pass; the result is the AND over the players `cockpit_state` counts. */
s32 quest_slot_arm_all_a(s8 value) {
    s32 ok = 1;
    CockpitState* state = &cockpit_state;
    s32 i;

    for (i = 0; i < state->field_0x000; i++) {
        if (quest_slot_arm_a(&cockpit_work[i], value) == 0) {
            ok = 0;
        }
    }
    return ok;
}

/* 0x802EA0CC (0x6C).  Arm one player's slot: an armed (negative) slot timer is cleared, the
 * player's bit goes into the shared flag byte and the slot's own byte is cleared. */
s32 quest_slot_arm_a(CockpitWork* work, s8 slot) {
    if (work->slot_timer_0x0C0[slot] >= 0) {
        return 0;
    }
    work->slot_timer_0x0C0[slot] = 0;
    work->slot_flags_0x0C8 |= (u8)(1 << slot);
    work->slot_state_0x0C9[slot] = 0;
    ai_npc_hold_item_arm();
    return 1;
}

/* 0x802EA138 (0x7C).  `quest_slot_arm_all_a`'s twin over `quest_slot_arm_b`. */
s32 quest_slot_arm_all_b(s8 value) {
    s32 ok = 1;
    CockpitState* state = &cockpit_state;
    s32 i;

    for (i = 0; i < state->field_0x000; i++) {
        if (quest_slot_arm_b(&cockpit_work[i], value) == 0) {
            ok = 0;
        }
    }
    return ok;
}

/* 0x802EA1B4 (0x50).  `quest_slot_arm_a` without the follow-up call: the slot's own byte is set
 * instead of cleared. */
s32 quest_slot_arm_b(CockpitWork* work, s8 slot) {
    if (work->slot_timer_0x0C0[slot] >= 0) {
        return 0;
    }
    work->slot_timer_0x0C0[slot] = 0;
    work->slot_flags_0x0C8 |= (u8)(1 << slot);
    work->slot_state_0x0C9[slot] = 1;
    return 1;
}

/* 0x802EA204 (0x74).  Append one mark to the player's 8-entry table: the texture coordinate pair
 * is copied and the three values ride behind it, and the count only advances while there is room. */
void quest_mark_append(CockpitWork* work, const _mh_ivec2_* pos, u8 tex_idx, u32 coord, u16 size) {
    QuestMark* mark;

    if (work->mark_count_0x0CD >= 8) {
        return;
    }
    mark = &work->marks_0x0D4[work->mark_count_0x0CD];
    uv_pair_copy(&mark->pos, pos);
    mark->field_0x04 = coord;
    mark->field_0x08 = size;
    mark->field_0x0A = tex_idx;
    work->mark_count_0x0CD++;
}

/* 0x802EA278.  Draws the mark's sprites: every id of the icon list (the hold list when `mode` is 0) at `pos`,
 * animated to `frame`, with the colour's alpha scaled by the sprite's own. */
void quest_mark_draw(const _mh_ivec2_* pos, u8 mode, u32 color, u16 frame) {
    const u16* ids;
    _SPR_DATA_ rec;

    if (mode == 0) {
        ids = quest_view_hold_ids;
    } else {
        ids = quest_view_icon_ids;
    }
    while (*ids != 0xFFFF) {
        spr_data_copy(&rec, get_lsp_data(*ids, 0));
        sprite_frame_apply(&rec, *ids, frame, 0);
        rec.color = (color & 0xFFFFFF00) + (u8)(((u32)(u8)rec.color * (u8)color) / 255);
        draw_sprite(rec, pos);
        ids++;
    }
}

/* 0x802EA33C.  Draws every mark of the table and empties it. */
void quest_marks_flush(CockpitWork* work) {
    QuestMark* mark;
    s32 count = work->mark_count_0x0CD;

    if (count != 0) {
        mark = work->marks_0x0D4;
        while (count > 0) {
            quest_mark_draw(&mark->pos, mark->field_0x0A, mark->field_0x04, mark->field_0x08);
            count--;
            mark++;
        }
        work->mark_count_0x0CD = 0;
    }
}

#pragma fp_contract off
/* Draws the AI NPC's marker at `world` and appends its mark-table entry. */
static inline void quest_npc_marker_draw_at(const VEC3& world, CockpitWork* work, _AINPC_W* npc, s32 clip) {
    f32 proj[2];
    _mh_ivec2_ mark;
    f32 scale;
    u32 color;
    f32 dist;

    if (clip != 0) {
        scale = 25.0f;
    } else {
        scale = 0.846153855f * (25.0f * work->window_icon_scale_0x08C);
    }
    if ((u32)ai_npc_arrived_ck(npc) == 1) {
        color = 0xF1A86DFF;
    } else {
        color = 0xC8E52EFF;
    }
    dist = quest_marker_draw_at(work, &world.x, 0xB, (u16)(0x18000 - npc->field_0x194), color, clip, scale);
    if (work->view_timer_0x0BE >= 0) {
        quest_screen_project(proj, &world.x);
        if (clip != 0) {
            if (dist <= 1.0f) {
                f32 fx = 384.0f * (proj[0] - work->marker_ref_x_0x098);
                f32 fy = 384.0f * (proj[1] - work->marker_ref_y_0x09C);

                mark.x = work->marker_ofs_x_0x090 + work->ring_scale_0x0A0 * fx;
                mark.y = work->marker_ofs_y_0x094 + work->ring_scale_0x0A0 * fy;
                quest_mark_append(work, &mark, 1, quest_mark_fade_value(work, 0xC8E52EFF, dist),
                                  work->view_timer_0x0BE);
            }
        } else {
            mark.x = (f32)work->window_x_0x084 + work->window_scale_0x088 * proj[0];
            mark.y = (f32)work->window_y_0x086 + work->window_scale_0x088 * proj[1];
            quest_mark_append(work, &mark, 1, 0xC8E52EFF, work->view_timer_0x0BE);
        }
    }
}

/* 0x802EA3B4.  Draws the AI NPC's marker (and its mark-table entry) while the player root says the NPC is present. */
void quest_npc_marker_draw(s32 clip) {
    CockpitWork* work;
    _AINPC_W* npc;

    if (((_PL_ROOT*)get_move_work_adrs(0))->npc_present_0xEF != 0) {
        work = cockpit_work;
        npc = work->ai_npc_0x048;
        quest_npc_marker_draw_at(get_worldworld_pos(&npc->vec_0x178, npc->field_0x1A4), work, npc, clip);
    }
}
#pragma fp_contract on

/* 0x802EA5F4.  The cockpit's quest-target rank bits: 4 while the AI NPC holds (or skill 0x97), 2 while skill 0x96 is
 * active - which also runs the two blink clocks - and 1 while the player's +0x460 timer runs or skill 0x31's quest has
 * under thirty seconds left. */
u8 quest_target_rank_get(CockpitWork* work, _PLW* plw) {
    u8 rank = 0;

    if (Pl_Skill_ck(plw, 0x97) == 1) {
        rank = 4;
    } else if ((u32)ai_npc_hold_ck(work->ai_npc_0x048) == 1) {
        rank = 4;
    } else if (Pl_Skill_ck(plw, 0x96) == 1) {
        rank = 2;
        if (work->quest_timer_0x188 > 0) {
            work->quest_timer_0x188--;
        }
        if (work->blink_clock_0x184 <= 0) {
            work->blink_clock_0x184 = (s32)(5.0f * (60.0f * Screen_w.frame_scale));
        } else {
            work->blink_clock_0x184--;
            if (work->blink_clock_0x184 <= 0) {
                work->blink_clock_0x184 = (s32)(5.0f * (60.0f * Screen_w.frame_scale));
                if (rand() % 3 != 0) {
                    work->quest_timer_0x188 = (s32)(30.0f * Screen_w.frame_scale);
                }
            }
        }
    }
    if ((u32)Pl_timer_0x460_ck(plw) == 1) {
        rank |= 1;
    } else if (Pl_cat_skill_ck(plw, 0x31) == 1) {
        s32 limit = quest_time_limit_get();

        if ((f32)(limit - quest_time_elapsed_get()) < 30.0f * Screen_w.frame_scale) {
            rank |= 1;
        }
    }
    return rank;
}

/* 0x802EA7D4 (0x8).  `quest_target_usable_ck` with no flags. */
u8 quest_target_usable0_ck(CockpitWork* work, _ENEMY_WORK* target) {
    return quest_target_usable_ck(work, target, 0);
}
/* 0x802EA7DC.  How a quest target may be shown: 0 hidden, 1 not markable, 2 plain, 3 highlighted - from the target's
 * live flag, the cockpit's rank bits (+0xD0) and the target's mark timer. */
u8 quest_target_usable_ck(CockpitWork* work, _ENEMY_WORK* target, s32 flags) {
    u8 rank;

    if (target->active == 0) {
        return 0;
    }
    if ((target->field_0x1C8 & 1) != 0) {
        if (flags == 0 && em_markable_ck(target) == 0) {
            return 1;
        }
        rank = work->target_rank_0x0D0;
        if ((rank & 4) != 0) {
            return 3;
        }
        if ((rank & 2) != 0) {
            if (work->quest_timer_0x188 > 0) {
                return 3;
            }
            if ((rank & 1) != 0) {
                return 3;
            }
            return em_mark_timer_ck(target) == 1 ? 3 : 0;
        }
        if ((rank & 1) != 0) {
            return 2;
        }
        if (em_mark_timer_ck(target) == 1) {
            return 2;
        }
    } else if (em_mark_timer_ck(target) != 0) {
        return 2;
    }
    return 0;
}

/* 0x802EA908.  The target's marker colour: grey for a dead action, per-kind for a live one (blinking towards light grey
 * while the skill 0xBE window is open), plain for an idle one; the alpha byte is full while the target is shown. */
#pragma fp_contract off
u32 quest_target_color(CockpitWork* work, _ENEMY_WORK* target, u8 kind) {
    _PLW* plw = work->plw;
    u32 color;
    f32 fraction;
    s32 step;
    s32 alpha;

    if (target->action == 0xB) {
        color = 0x80808000;
    } else if ((target->field_0x1C8 & 1) != 0) {
        if (kind == 3) {
            u8 variant = target->field_0x43D;

            if (variant == 1) {
                color = 0xED1A4000;
            } else if (variant == 2) {
                color = 0xFAE40800;
            } else {
                color = 0x4CE3E300;
            }
        } else {
            color = 0xEF679700;
        }
        if (Pl_Skill_ck(plw, 0xBE) == 1 && em_motion_window_ck(target) == 1) {
            step = work->frame_0x190 % 24;
            if (step < 0xC) {
                fraction = (f32)step / 12.0f;
                color = color_lerp(color, 0xE0E0E000, fraction);
            } else {
                fraction = (f32)(step - 0xC) / 12.0f;
                color = color_lerp(0xE0E0E000, color, fraction);
            }
        }
    } else {
        color = 0xE4C05B00;
    }
    if (target->visible_0x001 == 0 || target->field_0x1D4 <= 0.0f) {
        alpha = 0;
    } else {
        alpha = 0xFF;
    }
    return color | alpha;
}
#pragma fp_contract on

/* 0x802EAAB4.  Picks the target's marker icon, scale and colour; returns 1 when it is to be drawn. */
s32 quest_target_icon_get(CockpitWork* work, _ENEMY_WORK* target, u8* icon, f32* scale, u32* color) {
    u8 kind;

    *icon = 1;
    if ((target->field_0x1C8 & 1) != 0) {
        *scale = 26.0f;
    } else {
        *scale = 18.0f;
    }
    kind = quest_target_usable0_ck(work, target);
    *color = quest_target_color(work, target, kind);
    if ((u8)*color == 0) {
        return 0;
    }
    if (kind == 2) {
        return 1;
    }
    if (kind != 3) {
        return 0;
    }
    if ((target->field_0x1C8 & 1) != 0) {
        switch (target->team) {
        case 7:
        case 8:
        case 9:
        case 12:
        case 14:
            *icon = 5;
            break;
        default:
            if ((s32)target->field_0x1E2 == 1 || (s32)target->field_0x1E2 == 3) {
                *icon = 3;
            } else {
                *icon = 4;
            }
            break;
        case 15:
        case 16:
        case 18:
        case 19:
            *icon = 6;
            *scale = 30.0f;
            break;
        case 20:
            *icon = 7;
            break;
        case 25:
            if ((s32)target->field_0x1E2 == 1 || (s32)target->field_0x1E2 == 3) {
                *icon = 9;
            } else {
                *icon = 8;
            }
            break;
        }
    }
    return 1;
}

/* 0x802EAC38.  Draws one quest target's marker (when it is on screen, for the clipped view). */
#pragma fp_contract off
void quest_target_marker_draw(CockpitWork* work, _ENEMY_WORK* target, s32 clip) {
    VEC3 pos;
    u8 icon;
    f32 scale;
    u32 color;

    VEC3_ctor(&pos);
    const VEC3& world = get_worldworld_pos(&target->pos, target->area_no);
    copyVec3(&pos, &world);
    if ((clip == 0 || quest_mark_on_screen_ck(work, &pos.x) != 0) &&
        quest_target_icon_get(work, target, &icon, &scale, &color) != 0) {
        if (clip == 0) {
            scale *= 0.846153855f * work->window_icon_scale_0x08C;
        }
        quest_marker_draw_at(work, &pos.x, icon, (u16)(0x18000 - target->field_0x1C0), color, clip, scale);
    }
}
#pragma fp_contract on

/* 0x802EAD28.  Draws the markers of every quest target that may be shown as plain or highlighted. */
void quest_target_markers_draw(CockpitWork* work, s32 clip) {
    _ENEMY_WORK* target = (_ENEMY_WORK*)get_move_work_adrs(3);
    s32 count = get_move_work_max(3);

    while (count > 0) {
        if ((u8)(quest_target_usable0_ck(work, target) + 0xFE) <= 1) {
            quest_target_marker_draw(work, target, clip);
        }
        count--;
        target++;
    }
}

/* 0x802EADC4.  Draws the unclipped markers of the live quest targets that may be shown. */
void quest_target_markers_draw_live(CockpitWork* work) {
    _ENEMY_WORK* target = (_ENEMY_WORK*)get_move_work_adrs(3);
    s32 count = get_move_work_max(3);

    while (count > 0) {
        if ((target->field_0x1C8 & 1) != 0 && (u8)(quest_target_usable0_ck(work, target) + 0xFE) <= 1) {
            quest_target_marker_draw(work, target, 0);
        }
        count--;
        target++;
    }
}

/* 0x802EAE64.  Draws the hold-animation sprites of quest target `idx` at its marker position, their texture scales
 * stretched by the window scale and the marker's own. */
void quest_target_icon_anim_draw(CockpitWork* work, u8 idx, u16 frame) {
    _SPR_DATA_ rec;
    f32 scale[2];
    _mh_ivec2_ pos;
    f32 factor;
    const u16* ids;

    if (quest_marker_screen_pos(work, &pos, scale, idx) != 0) {
        factor = 0.00260416674f * work->window_scale_0x088;
        scale[0] *= factor;
        scale[1] *= factor;
        ids = quest_view_hold_ids_b;
        while (*ids != 0xFFFF) {
            sprite_frame_apply(&rec, *ids, frame, 0);
            rec.u_scale = (u16)((f32)rec.u_scale * scale[0]);
            rec.v_scale = (u16)((f32)rec.v_scale * scale[1]);
            draw_sprite(rec, &pos);
            ids++;
        }
    }
}

/* 0x802EAF88.  Runs the hold animation for every quest target of the bitmask. */
void quest_target_icons_anim_draw(void) {
    CockpitWork* work = cockpit_work;
    u16 frame;
    s32 i;

    if (work->target_mask_0x0BC != 0) {
        frame = work->frame_0x190 % (sprite_ary_last_frame(quest_view_hold_ids_b) + 1);
        i = 0;
        do {
            if ((work->target_mask_0x0BC & (1 << i)) != 0) {
                quest_target_icon_anim_draw(work, i, frame);
            }
            i++;
        } while (i < 13);
    }
}

/* 0x802EB034.  Draws one square icon of sprite `id` at the layout record's position: in the window frame when
 * `clip` is 0, otherwise on the marker ring (rotated by the ring's clock, and dropped when it lies off the ring). */
#pragma fp_contract off
void quest_icon_rect_draw(CockpitWork* work, u16 id, u16 size, u32 color, s32 clip) {
    _SPR_DATA_* rec = get_lsp_data(id, NULL);
    f32 pos[2];
    f32 x;
    f32 y;
    f32 rot[2];
    f32 dx;
    f32 dy;
    f32 u;
    f32 v;
    f32 scaled_u;
    f32 scaled_v;
    f32 dist;
    f32 side;
    f32 half;
    f32 ring;

    pos[0] = 0.00260416674f * (f32)rec->pos.x;
    pos[1] = 0.00260416674f * (f32)rec->pos.y;
    if (clip != 0) {
        dx = pos[0] - work->marker_ref_x_0x098;
        dy = pos[1] - work->marker_ref_y_0x09C;
        quest_rot_pair(&rot[1], &rot[0], work->ring_angle_0x0B8);
        u = dx * rot[0] - dy * rot[1];
        v = dx * rot[1] + dy * rot[0];
        scaled_u = u * work->ring_inv_x_0x0AC;
        scaled_v = v * work->ring_inv_y_0x0B0;
        dist = scaled_u * scaled_u + scaled_v * scaled_v;
        if (dist > 1.0f) {
            return;
        }
        ring = 384.0f * work->ring_scale_0x0A0;
        x = work->marker_ofs_x_0x090 + ring * u;
        y = work->marker_ofs_y_0x094 + ring * v;
        color = quest_mark_fade_value(work, color, dist);
    } else {
        x = (f32)work->window_x_0x084 + pos[0] * work->window_scale_0x088;
        y = (f32)work->window_y_0x086 + pos[1] * work->window_scale_0x088;
    }
    side = (f32)size;
    half = 0.5f * side;
    drawshape_init(3, 0xFFFF);
    drawshape_set_vertex_rect(x - half, y - half, side, side);
    drawshape_set_flat_color(color);
    drawshape_set_texture_rect(rec->tex_flag, &rec->uv0, &rec->uv1);
    drawshape_exec();
}
#pragma fp_contract on

/* 0x802EB268.  Draws the icons of the current area's list at the full size - the selected quest's list table replaces
 * the map's while a quest is selected. */
void quest_map_area_icons_draw(CockpitWork* work) {
    u16** lists;
    u16* ids;

    lists = quest_area_icon_lists_by_map[get_now_mapno()];
    if (lists != NULL) {
        if (quest_select_ready_ck() != 0) {
            switch ((u16)quest_id_get()) {
            case 1:
                lists = quest_area_icon_lists_by_quest[0];
                break;
            case 2:
                lists = quest_area_icon_lists_by_quest[1];
                break;
            case 3:
                lists = quest_area_icon_lists_by_quest[2];
                break;
            }
        }
        ids = lists[get_now_areano()];
        if (ids != NULL) {
            for (;;) {
                if (*ids == 0xFFFF) {
                    break;
                }
                quest_icon_rect_draw(work, *ids, 0x18, 0xACACACAA, 1);
                ids++;
            }
        }
    }
}

/* 0x802EB35C.  The icon id list of map `mapno`: the selected quest's list replaces the map's while a quest is selected. */
const u16* quest_map_icon_list_get(u8 mapno) {
    u16* list = quest_map_icon_list_by_map[mapno];

    if (list == NULL) {
        return NULL;
    }
    if (quest_select_ready_ck() != 0) {
        switch ((u16)quest_id_get()) {
        case 1:
            list = quest_map_icon_list_by_quest[0];
            break;
        case 2:
            list = quest_map_icon_list_by_quest[1];
            break;
        case 3:
            list = quest_map_icon_list_by_quest[2];
            break;
        }
    }
    return list;
}

/* 0x802EB400.  Draws the current map's icon id list (the pit map's area list on map kind 9) at `size`, skipping the
 * first entry on the second level of a kind 6 map, then the area's own icons. */
void quest_map_icons_draw(CockpitWork* work, u16 size) {
    u8 mapno;
    const u16* ids;
    u32 skip_first;
    s32 i;
    u32 count;

    mapno = get_now_mapno();
    if ((u8)stage_map_kind_get(get_now_mapno()) != 9) {
        ids = quest_map_icon_list_get(mapno);
    } else {
        ids = quest_pit_area_icon_lists[get_now_areano()];
    }
    if (ids != NULL) {
        skip_first = 0;
        if ((u8)stage_map_kind_get(get_now_mapno()) == 6 && (u8)get_now_areano() != 0) {
            skip_first = 1;
        }
        count = stage_map_area_count_get(mapno);
        for (i = 0; i < (u8)count; i++, ids++) {
            if (*ids == 0xFFFF) {
                break;
            }
            if (*ids != 0) {
                if (i != 0 || skip_first != 1) {
                    quest_icon_rect_draw(work, *ids, size, 0xF0F0F0AA, 1);
                }
            }
        }
        quest_map_area_icons_draw(work);
    }
}

/* 0x802EB51C.  On the pit map, draws the area's two icons at `size`. */
void quest_pit_area_icons_draw(CockpitWork* work, u16 size) {
    const u16* ids;

    if ((u8)stage_map_kind_get(get_now_mapno()) == 9) {
        ids = quest_pit_area_icon_lists[get_now_areano()];
        quest_icon_rect_draw(work, ids[0], size, 0xF0F0F0AA, 1);
        quest_icon_rect_draw(work, ids[1], size, 0xF0F0F0AA, 1);
    }
}

/* 0x802EB5C4.  Draws the map's icon id list at `size` with the marked ones (the quest-target bitmask) highlighted. */
void quest_map_icon_marks_draw(CockpitWork* work, u16 size, s32 clip) {
    u8 mapno;
    const u16* ids;
    s32 skip_first;
    s32 i;
    u32 count;
    u32 color;

    mapno = get_now_mapno();
    if ((u8)stage_map_kind_get(get_now_mapno()) != 9) {
        ids = quest_map_icon_list_get(mapno);
    } else {
        ids = quest_pit_area_icon_lists[get_now_areano()];
    }
    if (ids != NULL) {
        skip_first = 0;
        if ((u8)stage_map_kind_get(get_now_mapno()) == 6 && (u8)get_now_areano() != 0) {
            skip_first = 1;
        }
        count = stage_map_area_count_get(mapno);
        for (i = 0; i < (u8)count; i++, ids++) {
            if (*ids == 0xFFFF) {
                break;
            }
            if (*ids != 0) {
                if (i == 0) {
                    if (skip_first == 0) {
                        quest_icon_rect_draw(work, *ids, size, 0xF0F0F0C8, 0);
                    }
                } else {
                    color = 0xF0F0F0C8;
                    if ((work->target_mask_0x0BC & (1 << i)) != 0) {
                        color = 0xF1CC3CAA;
                    }
                    quest_icon_rect_draw(work, *ids, size, color, clip);
                }
            }
        }
    }
}

/* 0x802EB714.  On the pit map, draws the area's two icons at `size`, the second highlighted. */
void quest_pit_area_icon_marks_draw(CockpitWork* work, u16 size, s32 clip) {
    const u16* ids;

    if ((u8)stage_map_kind_get(get_now_mapno()) == 9) {
        ids = quest_pit_area_icon_lists[get_now_areano()];
        quest_icon_rect_draw(work, ids[0], size, 0xF0F0F0C8, 0);
        quest_icon_rect_draw(work, ids[1], size, 0xF1CC3CAA, clip);
    }
}

/* 0x802EB7C4.  Draws the map's icon id list at `size`, the icon of the player's own area in the colour of sprite 0x11D2
 * frame `frame`. */
void quest_map_icon_sprites_draw(CockpitWork* work, u16 size, u8 frame, s32 clip) {
    u8 mapno;
    const u16* ids;
    _PL_ROOT* root;
    _SPR_DATA_ rec;
    u32 own_color;
    u8 area;
    s32 skip_first;
    s32 i;
    u32 count;
    u32 color;

    mapno = get_now_mapno();
    ids = quest_map_icon_list_get(mapno);
    if (ids != NULL) {
        root = (_PL_ROOT*)get_move_work_adrs(0);
        area = root->area_no_0xEB;
        sprite_frame_apply(&rec, 0x11D2, frame, 0);
        own_color = rec.color;
        skip_first = 0;
        if ((u8)stage_map_kind_get(get_now_mapno()) == 6 && area != 0) {
            skip_first = 1;
        }
        count = stage_map_area_count_get(mapno);
        for (i = 0; i < (u8)count; i++, ids++) {
            if (*ids == 0xFFFF) {
                break;
            }
            if (*ids != 0) {
                color = 0xF0F0F0C8;
                if (i == area) {
                    color = own_color;
                }
                if (i == 0) {
                    if (skip_first == 0) {
                        quest_icon_rect_draw(work, *ids, size, color, clip);
                    }
                } else {
                    quest_icon_rect_draw(work, *ids, size, color, clip);
                }
            }
        }
    }
}

/* 0x802EB8E8.  The window position and texture scales of the current area's marker sprite of quest target `idx`;
 * 0 (with everything zeroed) when the map has none. */
#pragma fp_contract off
s32 quest_marker_screen_pos(CockpitWork* work, _mh_ivec2_* pos, f32* scale, u8 idx) {
    u16* ids = quest_area_sprite_list_by_map[get_now_mapno()];
    _SPR_DATA_* rec;
    f32 v[2];

    if (ids == NULL) {
        pos->y = 0;
        pos->x = 0;
        scale[1] = 0.0f;
        scale[0] = 0.0f;
        return 0;
    }
    rec = get_lsp_data(ids[idx], NULL);
    v[0] = 0.00260416674f * (f32)rec->pos.x;
    v[1] = 0.00260416674f * (f32)rec->pos.y;
    pos->x = (s16)((f32)work->window_x_0x084 + v[0] * work->window_scale_0x088);
    pos->y = (s16)((f32)work->window_y_0x086 + v[1] * work->window_scale_0x088);
    scale[0] = (f32)rec->u_scale / 100.0f;
    scale[1] = (f32)rec->v_scale / 100.0f;
    return 1;
}
#pragma fp_contract on

/* 0x802EBA74.  Draws one pit trap's marker: the clipped view rotates with the marker ring's clock. */
#pragma fp_contract off
f32 quest_pit_trap_marker_draw(CockpitWork* work, PitTrap* trap, s32 clip) {
    f32 scale;
    u32 color;
    s32 tex_no;

    if (clip != 0) {
        scale = 18.0f;
    } else {
        scale = 0.846153855f * (18.0f * work->window_icon_scale_0x08C);
    }
    if (trap->kind_0x0C == 0) {
        color = 0x53E37EFF;
    } else {
        color = 0xE069FFFF;
    }
    if (clip != 0) {
        tex_no = 0x10000 - work->ring_angle_0x0B8;
    } else {
        tex_no = 0;
    }
    return quest_marker_draw_at(work, &trap->pos.x, 0xE, tex_no, color, clip, scale);
}
#pragma fp_contract on

/* 0x802EBAE0.  Draws every registered pit trap's marker. */
void quest_pit_traps_draw(CockpitWork* work, s32 clip) {
    CockpitState* state = &cockpit_state;
    PitTrap* trap = state->pit_traps_0x018;
    s32 count = state->pit_trap_shown_0x059;

    while (count > 0) {
        quest_pit_trap_marker_draw(work, trap, clip);
        count--;
        trap++;
    }
}

/* 0x802EBB58.  Registers a pit trap (position and kind) for this frame's markers, up to four. */
void PitTrapSet(const VEC3* pos, u8 kind) {
    CockpitState* state = &cockpit_state;
    PitTrap* trap;

    if (state->pit_trap_count_0x058 < 4) {
        trap = &state->pit_traps_0x018[state->pit_trap_count_0x058];
        copyVec3(&trap->pos, pos);
        trap->kind_0x0C = kind;
        state->pit_trap_count_0x058++;
    }
}

/* 0x802EBBD0.  Draws the quest cockpit's marker ring and everything on it: the ring's pie at the window spot (rotated
 * by the camera when the option is on), then the map icons, the quest targets, the players and the pit traps. */
void quest_marker_draw_faded(CockpitWork* work, u16 id, u32 mode, const _mh_ivec2_* pos) {
    _PLW* plw = work->plw;
    u32 angle_on;
    u16 angle;
    _SPR_DATA_* rec;
    VEC3 local;
    f32 proj[2];
    f32 scale;
    u32 prev_player;

    work->window_x_0x084 = pos->x;
    work->window_y_0x086 = pos->y;
    angle_on = 0;
    if (get_cfg(plw->chunk_ofs, 0xF) == 1) {
        angle_on = 1;
    }
    if (angle_on == 1) {
        if (mode == 1) {
            prev_player = my_player_no();
            my_player_no_set(plw->chunk_ofs);
            angle = camera_angle_y_get();
            my_player_no_set(prev_player);
        } else {
            angle = camera_angle_y_get();
        }
    } else {
        angle = 0;
    }
    rec = get_lsp_data(id, NULL);
    VEC3_ctor(&local);
    quest_player_marker_pos(plw, &local);
    const VEC3& world = get_worldworld_pos(&local, plw->area_0x16);
    quest_screen_project(proj, &world.x);
    if (stage_map_kind_get(get_now_mapno()) == 6) {
        if (get_now_areano() == 1) {
            scale = 0.800000012f;
        } else {
            work->ring_extent_x_0x0A4 = 0.0831117034f;
            work->ring_extent_y_0x0A8 = 0.124667555f;
            scale = 0.649999976f;
        }
    } else {
        scale = 0.800000012f;
    }
    work->marker_ofs_x_0x090 = (f32)work->window_x_0x084;
    work->marker_ofs_y_0x094 = (f32)work->window_y_0x086;
    drawshape_copy_vec2(&work->marker_ref_x_0x098, proj);
    work->ring_inv_x_0x0AC = 1.0f / work->ring_extent_x_0x0A4;
    work->ring_inv_y_0x0B0 = 1.0f / work->ring_extent_y_0x0A8;
    work->ring_angle_0x0B8 = angle;
    work->ring_radius_0x0B4 = scale;
    quest_marker_pie_draw(&work->marker_ofs_x_0x090, &work->marker_ref_x_0x098, &work->ring_extent_x_0x0A4, rec->color,
                          0x10000 - angle, work->ring_scale_0x0A0, scale);
    if (mode == 0) {
        quest_map_icons_draw(work, 0x28);
    } else {
        quest_pit_area_icons_draw(work, 0x28);
    }
    quest_target_markers_draw(work, 1);
    if (mode == 0) {
        quest_npc_marker_draw(1);
    }
    if (mode == 0) {
        quest_marker_draw_all_non_master(work, 1);
    } else {
        quest_marker_draw_others_then_self(work, 1);
    }
    quest_pit_traps_draw(work, 1);
}

/* 0x802EBE2C.  Whether the quest marker may show for the player: never on map kind 6, always while the move work
 * state holds or skill 0x86 is on, never with skill 0x87, else by the item timer of id 0x2A. */
s32 quest_mark_allowed_ck(_PLW* plw) {
    if (stage_map_kind_get(get_now_mapno()) == 6) {
        return 0;
    }
    if (move_work_state_ck() != 0) {
        return 1;
    }
    if (Pl_Skill_ck(plw, 0x86) == 1) {
        return 1;
    }
    if (Pl_Skill_ck(plw, 0x87) == 1) {
        return 0;
    }
    return Pl_item_timer_get(plw, 0x2A) != 0;
}
