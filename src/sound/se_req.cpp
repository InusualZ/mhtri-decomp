/* sound/se_req.cpp - the SE (`se_w`) request cluster's middle: the `_PLW`-keyed request helpers, `map_se_req`, the held
 * shell-SE requests (`shell_se_req`, `shell_se_req2`) and the map's point-SE layer (the area's sound points, refreshed each
 * frame through the SE pool's point work).
 *
 * RANGE. `.text` 0x800DD40C..0x800E0504 (35 functions), `.data` 0x80597BA8..0x80597C80 (the two switch tables),
 *   `.sdata` 0x80791448..0x80791450 (the gun-shot SE tables), `.sdata2` 0x8079641C..0x80796438 (the float pool).
 * FLAGS. File-scope `#pragma peephole off` (retail keeps `extsb` before a compare and the `clrlwi` narrowing of byte
 *   arguments) and `#pragma optimization_level 4` (retail's register colouring in `shell_se_req` and `se_point_frame`;
 *   every other row is unchanged by it).
 * NAMES. Module `sound` from the neighbours (`sound/fn_800D7F54.cpp`, `sound/mhchar.cpp`); `se_req` is a GUESS (no
 *   `__FILE__` string). C++ linkage where the map row is mangled (`map_se_req`, `shell_se_req`, `shell_se_req2`,
 *   `ryugeki_*_se_req`), C elsewhere. GUESS (from each body and its callers): `pl_se_req_charge_end`,
 *   `pl_se_req_charge_start`, `pl_se_req_charge_level`, `pl_se_req_charge_7`, `pl_se_req_charge_end_b`,
 *   `pl_se_req_charge_2`, `pl_se_req_charge_burst`, `pl_se_req_charge_tip`, `pl_se_req_charge_hit`, `pl_se_req_flag`,
 *   `se_bank46_id13_req`, `se_slot_hold_start`, `se_slot_hold_start_owner`, `se_slot_hold_find`, `se_work_slots_clear`,
 *   `se_point_work_open`, `se_point_work_reset`, `se_point_handle_free_get`, `se_point_handle_release`,
 *   `se_point_handle_live_get`, `se_point_frame`, `se_point_area_scan`, `se_point_zone_level_get`, `gun_shot_se_tbl`,
 *   `gun_shot_echo_se_tbl`, the registry names `se_pool`, `se_disabled_ck`, `se_slot_level_calc`,
 *   `se_slot_kind_dispatch`, `snd_chunk_handle_get`, `snd_stream_dma_get`, `se_point_level_tbls`, and the
 *   `SePoolView`/`SePointRec`/`SePointTable`/`SePointZone` field names.
 * RESIDUALS. `shell_se_req`: retail joins the tails of cases 1->0, 34->35, 36->37 (the earlier case branches into the
 *   later one's held-slot code with the SE id in r31) and 19->18 (into the timer store); a `goto`-free source cannot
 *   join them, so cases 0/1, 34/35 and 36/37 share one label with a `kind` test (+0x34 B net) and 19 keeps its own
 *   copy. `se_point_frame`: retail carries a dead `b` to the epilogue after the no-table path's branch to the slot
 *   loop (one instruction short). `se_pool` (0x80794978, `.sbss`) has no registered owner (`src/unsplit/se_pool.h`).
 *   flipcheck: the seven registry names are undefined until they land; `pl_se_req_flag` is force-active in retail.
 * SHAPES. The held-slot refresh is one inline (`se_slot_hold_req`) in `shell_se_req2` and one macro pair
 *   (`SE_SLOT_HOLD_FIND`/`SE_SLOT_HOLD_REQ`) in `shell_se_req`; the inline form there scores lower.
 */

#include "types.h"
#include "nw4r/math.h"
#include "mh3_pad.h" /* VEC3_ctor / copyVec3 / setVec3 (rule 2) */
#include "pl.h"
#include "sound/se.h"
#include "sound/se_req.h"
#include "sound/fn_800D7F54.h" /* se_entry_request (rule 2) */
#include "sound/se_pop_spring_req.h"
#include "sound/pl_impact_se_req.h"
#include "sound/pl_gun_shot_se_req.h"
#include "sound/pl_gun_empty_se_req.h"
#include "sound/pl_ryugeki_se_req.h"
#include "sound/pl_se_charge_level_req.h"
#include "sound/se_disabled_ck.h"
#include "sound/se_slot_level_calc.h"
#include "sound/se_slot_kind_dispatch.h"
#include "sound/snd_chunk_handle_get.h"
#include "sound/snd_stream_dma_get.h"
#include "sound/se_point_level_tbls.h"
#include "sound/get_chacha_kamen_bank.h"
#include "sound/snd_stream_reloc_busy_ck.h"
#include "Pl/pl_segment_closest_point.h"
#include "ef/get_move_work_adrs.h"
#include "ef/GameMode_ck.h"
#include "ef/fn_800CDB2C.h" /* my_player_no */
#include "stage/stg_w.h" /* stage_map_kind_get / get_now_areano */
#include "stage/get_now_mapno.h"
#include "quest/quest_item_slot.h" /* quest_id_head_ck */
#include "lobby/fn_801E7530.h" /* lb_part_slot_open_ck */
#include "lobby/fn_802FA9A0.h" /* kujira_event_over_ck */
#include "lobby/lb_menu_pos_tbl.h" /* getItemListSelection */
#include "fn_8004CAD8.h" /* vec3_distance */
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/se_pool.h"

#pragma peephole off
#pragma optimization_level 4

/* One sound point of the area's point table: a looping or one-shot SE at a point or along a segment.
 * size: 0x48 */
struct SePointRec {
    /* +0x00 */ u8 state;            /* 0 off, 1 armed, 2 played once */
    /* +0x01 */ u8 shape;            /* 0 point, 1 segment */
    /* +0x02 */ u8 kind;             /* 0 timed SE, 1 level zone */
    /* +0x03 */ u8 pad_0x03;
    /* +0x04 */ nw4r::math::VEC3 pos;
    /* +0x10 */ u8 pad_0x10[0xC];
    /* +0x1C */ nw4r::math::VEC3 seg_start;
    /* +0x28 */ nw4r::math::VEC3 seg_end;
    /* +0x34 */ f32 radius;
    /* +0x38 */ u8 pad_0x38[4];
    /* +0x3C */ s32 interval;        /* frames between plays; 0 plays once */
    /* +0x40 */ u32 se_id;           /* the SE id, or the level-table index of a zone */
    /* +0x44 */ u8 handle;           /* the pool handle of a segment SE, 255 when none */
    /* +0x45 */ u8 area;
    /* +0x46 */ s16 timer;
};

/* The point table `snd_stream_dma_get` hands out once the area's stream is loaded.
 * size: 0x8 + 0x48 * count */
struct SePointTable {
    /* +0x00 */ s32 loaded;
    /* +0x04 */ s32 count;
    /* +0x08 */ SePointRec recs[1];
};

/* One level zone the area scan collects from the point table.
 * size: 0x14 */
struct SePointZone {
    /* +0x00 */ u8 active;
    /* +0x01 */ u8 area;
    /* +0x02 */ u8 pad_0x02;
    /* +0x03 */ u8 level_tbl;        /* index into `se_point_level_tbls` */
    /* +0x04 */ nw4r::math::VEC3 pos;
    /* +0x10 */ f32 radius;
};

/* The SE pool `se_pool` points at, as far as this unit reads it (`se_work_init` clears 0x2966C bytes of it; the
 * owner's own view is `sound/fn_800D7F54.cpp`'s `SePool`).
 * size: 0x2966C */
struct SePoolView {
    /* +0x00000 */ u8 pad_0x00000[4];
    /* +0x00004 */ nw4r::math::VEC3 cam_pos;
    /* +0x00010 */ u8 pad_0x00010[0xC];
    /* +0x0001C */ nw4r::math::VEC3 cam_target;
    /* +0x00028 */ u8 pad_0x00028[0x29256];
    /* +0x2927E */ u8 area;
    /* +0x2927F */ u8 prev_area;
    /* +0x29280 */ u8 pad_0x29280[0x60];
    /* +0x292E0 */ _se_w* point_work;
    /* +0x292E4 */ SeSlot* point_handles[32];
    /* +0x29364 */ SePointZone zones[32];
    /* +0x295E4 */ u8 pad_0x295E4[0x10];
    /* +0x295F4 */ s16 hold_timer[4];
    /* +0x295FC */ s16 wait_timer[16];
    /* +0x2961C */ u8 pad_0x2961C[0x50];
};

s8 gun_shot_se_tbl[4] = { 1, 2, 3, 0 };
s8 gun_shot_echo_se_tbl[4] = { 16, 17, 18, 0 };

/* Starts a claimed slot as a one-shot positional SE `id` at `pos`. */
inline void se_slot_start(SeSlot* slot, _se_w* work, s32 id, nw4r::math::VEC3* pos)
{
    slot->state = 3;
    slot->kind = 2;
    slot->owner = work->field_0x0C;
    slot->id = id;
    slot->field_0x30 = -1;
    slot->field_0x34 = -1;
    slot->param = 0;
    copyVec3(&slot->pos, pos);
    slot->field_0x10 = slot->pos;
}

extern "C" SeSlot* se_slot_hold_find(_se_w* work, u32 key, s32 id);
extern "C" void se_slot_hold_start(_se_w* work, nw4r::math::VEC3* pos, u32 key, SeSlot* slot, s32 id, s32 hold,
                                   u8 owner_ofs);
extern "C" void se_slot_hold_start_owner(_se_w* work, nw4r::math::VEC3* pos, u32 key, SeSlot* slot, s32 id, s32 hold,
                                         u8 unused, s32 owner);

/* Refreshes the held slot `key`/`id` of `work` at `pos`, or starts a new one held for `hold` frames. */
inline void se_slot_hold_req(_se_w* work, nw4r::math::VEC3* pos, u32 key, s32 id, s16 hold, u8 owner_ofs)
{
    SeSlot* slot = se_slot_hold_find(work, key, id);
    if (slot) {
        if (slot->field_0x48 <= -1) {
            return;
        }
        slot->field_0x48--;
        if (slot->field_0x48 == 0) {
            slot->in_use = 0;
        } else {
            copyVec3(&slot->pos, pos);
            slot->field_0x10 = slot->pos;
            return;
        }
    }
    SeSlot* fresh = fn_800D8E58(work, id);
    if (fresh) {
        se_slot_hold_start(work, pos, key, fresh, id, hold, owner_ofs);
    }
}

/* 0x800DD40C (0xF8): starts the pool's point voice `kind` (0..2 pick SE 9..11) at `pos`. */
extern "C" void se_point_req(u8 kind, nw4r::math::VEC3* pos)
{
    _se_w* work;
    s32 id = 9;
    work = se_pool->point_work;
    if (work) {
        switch (kind) {
        case 0:
            id = 9;
            break;
        case 1:
            id = 10;
            break;
        case 2:
            id = 11;
            break;
        }
        SeSlot* slot = fn_800D8E58(work, id);
        if (slot) {
            se_slot_start(slot, work, id, pos);
        }
    }
}

/* 0x800DD504 (0x10): requests the spring point's bubbling sound at `pos`. */
extern "C" void se_pop_spring_req(nw4r::math::VEC3* pos)
{
    se_slot_req_pos(46, 64, pos);
}

/* 0x800DD514 (0x10): requests bank 46's SE 13 at `pos`. */
extern "C" void se_bank46_id13_req(nw4r::math::VEC3* pos)
{
    se_slot_req_pos(46, 13, pos);
}

extern "C" void pl_se_req_charge_end(_PLW* self)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        return se_frame_req(work, 0, 3, 3);
    }
}

extern "C" void pl_se_req_charge_start(_PLW* self, u8 flag)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return;
    }
    s32 id = (flag == 0) ? 1 : 2;
    return se_frame_req(work, id, 3, 3);
}

extern "C" void pl_se_req_charge_level(_PLW* self, u8 arg)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return;
    }
    s32 id = 0;
    switch (arg) {
    case 0:
        id = 7;
        break;
    case 1:
        id = 8;
        break;
    case 2:
        id = 9;
        break;
    }
    return se_frame_req(work, id, 3, 3);
}

extern "C" void pl_se_req_charge_7(_PLW* self)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        return se_frame_req(work, 7, 3, 3);
    }
}

extern "C" void pl_se_req_charge_end_b(_PLW* self)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        return se_frame_req(work, 0, 3, 3);
    }
}

extern "C" void pl_se_req_charge_2(_PLW* self)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        return se_frame_req(work, 2, 3, 3);
    }
}

/* 0x800DD62C (0x70): requests SE 8 (18 when the position test holds) for the actor at `pos`. */
extern "C" void pl_se_req_charge_hit(_PLW* self, nw4r::math::VEC3* pos)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        s32 id = (fn_800D8D8C(pos) == 1) ? 18 : 8;
        se_req_pos_ps(work, id, ((work->field_0x0C + 1) << 24) | 2, pos);
    }
}

extern "C" void pl_se_req_charge_burst(_PLW* self, nw4r::math::VEC3* pos)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        s32 id = (fn_800D8D8C(pos) == 1) ? 14 : 13;
        se_req_pos_ps(work, id, 2, pos);
    }
}

extern "C" void pl_se_req_charge_tip(_PLW* self, nw4r::math::VEC3* pos)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        s32 id = (fn_800D8D8C(pos) == 1) ? 17 : 7;
        se_req_pos_ps(work, id, ((work->field_0x0C + 1) << 24) | 2, pos);
    }
}

extern "C" void pl_se_charge_level_req(_PLW* self, nw4r::math::VEC3* pos)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        s32 id = (fn_800D8D8C(pos) == 1) ? 17 : 7;
        se_req_pos_ps(work, id, ((work->field_0x0C + 1) << 24) | 2, pos);
    }
}

/* 0x800DD7E0 (0xB8): requests the step SE `flag` picks (0 and 1 the two ids, -1 SE 0) at `pos`. */
extern "C" void pl_se_req_flag(_PLW* self, nw4r::math::VEC3* pos, s8 flag)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return;
    }
    s32 id;
    switch (flag) {
    case 0:
        if (fn_800D8D8C(pos) == 1) {
            id = 12;
        } else {
            id = 10;
        }
        break;
    case 1:
        if (fn_800D8D8C(pos) == 1) {
            id = 13;
        } else {
            id = 11;
        }
        break;
    case -1:
        id = 0;
        break;
    default:
        return;
    }
    se_req_pos_ps(work, id, 2, pos);
}

/* 0x800DD898 (0x70): requests the impact SE (33 when the position test holds, else 13) at `pos`. */
extern "C" void pl_impact_se_req(_PLW* self, nw4r::math::VEC3* pos)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        s32 id = (fn_800D8D8C(pos) == 1) ? 33 : 13;
        se_req_pos_ps(work, id, ((work->field_0x0C + 1) << 24) | 2, pos);
    }
}

/* 0x800DD908 (0xC4): requests the gun shot SE of `shot` at `pos`; a `kind` shot adds the echo SE. */
extern "C" void pl_gun_shot_se_req(_PLW* self, nw4r::math::VEC3* pos, u8 kind, u32 shot)
{
    _se_w* work = self->field_0xAFC;
    if (work) {
        s32 id = gun_shot_se_tbl[(u8)shot];
        if (kind == 0) {
            se_req_pos_ps(work, id, ((work->field_0x0C + 2) << 24) | 2, pos);
        } else {
            se_req_pos_ps(work, id + 10, ((work->field_0x0C + 2) << 24) | 2, pos);
            se_req_pos_ps(work, gun_shot_echo_se_tbl[(u8)shot], 2, pos);
        }
    }
}

/* 0x800DD9CC (0x30): requests the ryugeki shot SE (31 for a `kind` shot, else 11) at `pos`. */
void ryugeki_shot_se_req(_PLW* self, nw4r::math::VEC3* pos, u8 kind)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return;
    }
    s32 id = (kind == 0) ? 11 : 31;
    se_req_pos_ps(work, id, 2, pos);
}

/* 0x800DD9FC (0xCC): claims a slot for the ryugeki positional SE (id 10/30 by kind) at `pos`. */
extern "C" void pl_ryugeki_se_req(_PLW* self, nw4r::math::VEC3* pos, u8 kind)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return;
    }
    s32 id = (kind == 0) ? 10 : 30;
    SeSlot* slot = fn_800D8E58(work, id);
    if (!slot) {
        return;
    }
    se_slot_start(slot, work, id, pos);
}

/* 0x800DDAC8 (0x13C): claims a slot for the ryugeki fire SE `level` (12..14, or 32..34 for a `kind` shot). */
void ryugeki_fire_se_req(_PLW* self, nw4r::math::VEC3* pos, u8 kind, u8 level)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return;
    }
    s32 id = 0;
    if (kind == 0) {
        switch (level) {
        case 0:
            id = 12;
            break;
        case 1:
            id = 13;
            break;
        case 2:
            id = 14;
            break;
        }
    } else {
        switch (level) {
        case 0:
            id = 32;
            break;
        case 1:
            id = 33;
            break;
        case 2:
            id = 34;
            break;
        }
    }
    if (id == 0) {
        return;
    }
    SeSlot* slot = fn_800D8E58(work, id);
    if (slot) {
        se_slot_start(slot, work, id, pos);
    }
}

/* 0x800DDC04 (0xB4): claims a slot for the empty-magazine SE 20 at `pos`. */
extern "C" void pl_gun_empty_se_req(_PLW* self, nw4r::math::VEC3* pos)
{
    _se_w* work = self->field_0xAFC;
    if (!work) {
        return;
    }
    SeSlot* slot = fn_800D8E58(work, 20);
    if (slot) {
        se_slot_start(slot, work, 20, pos);
    }
}

/* 0x800DDCB8 (0x158): plays the map SE `kind` at `pos` through the pool's point work. */
void map_se_req(u8 kind, nw4r::math::VEC3* pos)
{
    _se_w* work;
    s32 id;
    switch (kind) {
    case 0:
        id = 40;
        break;
    case 1:
        id = 41;
        break;
    case 2:
        id = 59;
        break;
    case 3:
        id = 60;
        break;
    case 4:
        id = 42;
        break;
    case 5:
        id = 43;
        break;
    case 6:
        id = 44;
        break;
    case 7:
        id = 49;
        break;
    case 8:
        id = 50;
        break;
    case 9:
        id = 55;
        break;
    case 10:
        id = 19;
        break;
    case 11:
        id = 16;
        break;
    case 12:
        id = 47;
        break;
    case 13:
        id = 45;
        break;
    case 14:
        id = 58;
        break;
    }
    work = se_pool->point_work;
    if (work) {
        SeSlot* slot = fn_800D8E58(work, id);
        if (slot) {
            se_slot_start(slot, work, id, pos);
        }
    }
}

/* 0x800DDE10 (0xA0): starts `slot` as SE `id` held for `hold` frames under `key`, owned `owner_ofs` past the work. */
extern "C" void se_slot_hold_start(_se_w* work, nw4r::math::VEC3* pos, u32 key, SeSlot* slot, s32 id, s32 hold,
                                   u8 owner_ofs)
{
    slot->state = 3;
    slot->kind = 2;
    slot->owner = work->field_0x0C + owner_ofs;
    slot->id = id;
    slot->field_0x30 = -1;
    slot->field_0x34 = -2;
    slot->param = 0;
    slot->field_0x4A = 0;
    slot->field_0x4B = 0;
    slot->field_0x03 = 1;
    slot->field_0x44 = key;
    slot->field_0x48 = (s16)hold;
    copyVec3(&slot->pos, pos);
    slot->field_0x10 = slot->pos;
}

/* 0x800DDEB0 (0x94): starts `slot` as SE `id` held for `hold` frames under `key`, owned by `owner`. */
extern "C" void se_slot_hold_start_owner(_se_w* work, nw4r::math::VEC3* pos, u32 key, SeSlot* slot, s32 id, s32 hold,
                                         u8 unused, s32 owner)
{
    slot->state = 3;
    slot->kind = 2;
    slot->owner = owner;
    slot->id = id;
    slot->field_0x30 = -1;
    slot->field_0x34 = -2;
    slot->param = 0;
    slot->field_0x4A = 0;
    slot->field_0x4B = 0;
    slot->field_0x03 = 1;
    slot->field_0x44 = key;
    slot->field_0x48 = (s16)hold;
    copyVec3(&slot->pos, pos);
    slot->field_0x10 = slot->pos;
}

/* 0x800DDF44 (0xD0): finds the live held slot of `work` under `key` playing `id` whose hold is still running. */
extern "C" SeSlot* se_slot_hold_find(_se_w* work, u32 key, s32 id)
{
    int i;
    SeSlot* slot = &work->slots[0];
    for (i = 0; i < 32; i++, slot++) {
        if (slot->in_use == 0) {
            continue;
        }
        if (slot->field_0x03 == 0) {
            continue;
        }
        if (slot->field_0x44 != key) {
            continue;
        }
        if ((s32)slot->owner != work->field_0x0C) {
            continue;
        }
        if ((s32)slot->id != id) {
            continue;
        }
        if (slot->field_0x48 <= -1) {
            continue;
        }
        return slot;
    }
    return 0;
}

/* 0x800DE014 (0x2E8): holds the shell SE `kind` at the point of `capsule` nearest the camera. */
void shell_se_req2(_se_w* work, nw4r::math::CAPSULE* capsule, u8 kind, u32 key)
{
    nw4r::math::VEC3 listener;
    VEC3_ctor(&listener);
    nw4r::math::VEC3 near;
    VEC3_ctor(&near);
    if (se_pool != 0 && work != 0 && se_disabled_ck() != 1) {
        copyVec3(&listener, &se_pool->cam_pos);
        switch (kind) {
        case 7:
            key += kind;
            pl_segment_closest_point(&capsule->start, &capsule->end, &listener, &near);
            se_slot_hold_req(work, &near, key, 32, 33, 0);
            break;
        case 23:
            key += kind;
            pl_segment_closest_point(&capsule->start, &capsule->end, &listener, &near);
            se_slot_hold_req(work, &near, key, 2, 33, 0);
            break;
        case 27:
            key += kind;
            pl_segment_closest_point(&capsule->start, &capsule->end, &listener, &near);
            se_slot_hold_req(work, &near, key, 32, 10, 0);
            break;
        }
    }
}

/* The held-slot refresh `shell_se_req` expands per case: refreshes the live slot `k`/`i` of `w` at `pos` and leaves
 * the case, or frees a slot whose hold ran out so that the case starts a new one. */
#define SE_SLOT_HOLD_FIND(w, k, i)                                                                                          slot = se_slot_hold_find(w, k, i);                                                                                     if (slot) {                                                                                                                if (slot->field_0x48 <= -1) {                                                                                              break;                                                                                                             }                                                                                                                      slot->field_0x48--;                                                                                                    if (slot->field_0x48 == 0) {                                                                                               slot->in_use = 0;                                                                                                  } else {                                                                                                                   copyVec3(&slot->pos, pos);                                                                                             slot->field_0x10 = slot->pos;                                                                                          break;                                                                                                             }                                                                                                                  }

/* `SE_SLOT_HOLD_FIND`, then a new slot for SE `i` held `h` frames (owner `o` past the work) when none is live. */
#define SE_SLOT_HOLD_REQ(w, i, h, o)                                                                                        SE_SLOT_HOLD_FIND(w, key, i)                                                                                           slot = fn_800D8E58(w, i);                                                                                              if (slot) {                                                                                                                se_slot_hold_start(w, pos, key, slot, i, h, o);                                                                    }

/* 0x800DE2FC (0x164C): holds the shell SE `kind` (the projectile and environment loops) at `pos` under `key`. */
void shell_se_req(_se_w* work, nw4r::math::VEC3* pos, u8 kind, u32 key)
{
    SeSlot* slot;
    SeWork* move;
    s32 id;
    if ((u32)(kind - 15) <= 5) {
        work = se_pool->point_work;
    }
    if (work != 0 && se_disabled_ck() != 1) {
        switch (kind) {
        case 0:
        case 1:
            key += kind;
            id = 37;
            if (kind == 1 && fn_800D8D8C(pos) == 1) {
                id = 237;
            }
            SE_SLOT_HOLD_REQ(work, id, 15, 0)
            break;
        case 2:
            key += kind;
            id = (fn_800D8D8C(pos) == 1) + 30;
            slot = fn_800D8E58(work, id);
            if (slot) {
                se_slot_hold_start(work, pos, key, slot, id, 15, 0);
            }
            break;
        case 3:
            key += kind;
            move = (SeWork*)get_move_work_adrs(0);
            if (move == 0) {
                break;
            }
            SE_SLOT_HOLD_REQ(move->se[0], 25, 12, 0)
            break;
        case 4:
            key += kind;
            move = (SeWork*)get_move_work_adrs(0);
            if (move == 0) {
                break;
            }
            SE_SLOT_HOLD_REQ(move->se[0], 66, 15, 0)
            break;
        case 5:
            key += kind;
            SE_SLOT_HOLD_REQ(work, 40, 30, 0)
            break;
        case 6:
            SE_SLOT_HOLD_FIND(work, key, key)
            slot = fn_800D8E58(work, key);
            if (slot) {
                if (key == 0) {
                    se_slot_hold_start(work, pos, key, slot, key, 81, 0);
                } else {
                    se_slot_hold_start(work, pos, key, slot, key, 80, 0);
                }
            }
            break;
        case 8:
            key += kind;
            SE_SLOT_HOLD_REQ(work, 38, 12, 0)
            break;
        case 9:
            key += kind;
            SE_SLOT_HOLD_REQ(work, 44, 14, 0)
            break;
        case 10:
            key += kind;
            SE_SLOT_HOLD_REQ(work, 61, 9, 0)
            break;
        case 11:
            key += kind;
            SE_SLOT_HOLD_REQ(work, 35, 15, 0)
            break;
        case 12:
            key += kind;
            slot = se_slot_hold_find(work, key, 19);
            if (slot) {
                if (slot->field_0x48 <= -1) {
                    break;
                }
                slot->field_0x48--;
                if (slot->field_0x48 == 0) {
                    slot->in_use = 0;
                } else {
                    copyVec3(&slot->pos, pos);
                    slot->field_0x10 = slot->pos;
                }
                break;
            }
            slot = fn_800D8E58(work, 19);
            if (slot) {
                se_slot_hold_start(work, pos, key, slot, 19, 150, 0);
            }
            break;
        case 13:
            id = (fn_800D8D8C(pos) == 1) ? 18 : 8;
            if (se_pool->hold_timer[key] > 0) {
                se_pool->hold_timer[key]--;
                if (se_pool->hold_timer[key] != 0) {
                    break;
                }
            }
            se_pool->hold_timer[key] = 18;
            key += kind;
            slot = fn_800D8E58(work, id);
            if (slot) {
                se_slot_hold_start(work, pos, key, slot, id, 18, 1);
            }
            break;
        case 14:
            id = (fn_800D8D8C(pos) == 1) ? 19 : 9;
            if (se_pool->hold_timer[key] > 0) {
                se_pool->hold_timer[key]--;
                if (se_pool->hold_timer[key] != 0) {
                    break;
                }
            }
            se_pool->hold_timer[key] = 12;
            key += kind;
            slot = fn_800D8E58(work, id);
            if (slot) {
                se_slot_hold_start(work, pos, key, slot, id, 12, 1);
            }
            break;
        case 15:
            id = (key & 3) + 20;
            key += kind;
            SE_SLOT_HOLD_REQ(work, id, 70, 0)
            break;
        case 16:
            id = (key == 0) + 3;
            if (se_pool->wait_timer[key & 15] > 0) {
                se_pool->wait_timer[key & 15]--;
                if (se_pool->wait_timer[key & 15] != 0) {
                    break;
                }
            }
            se_pool->wait_timer[key & 15] = 30;
            slot = fn_800D8E58(work, id);
            if (slot) {
                se_slot_hold_start(work, pos, key, slot, id, 30, 0);
            }
            break;
        case 17:
            if (se_pool->wait_timer[key & 15] > 0) {
                se_pool->wait_timer[key & 15]--;
                if (se_pool->wait_timer[key & 15] != 0) {
                    break;
                }
            }
            se_pool->wait_timer[key & 15] = 63;
            slot = fn_800D8E58(work, 18);
            if (slot) {
                se_slot_hold_start(work, pos, key, slot, 18, 63, 0);
            }
            break;
        case 18:
            id = 46;
            if (se_pool->wait_timer[key & 15] > 0) {
                se_pool->wait_timer[key & 15]--;
                if (se_pool->wait_timer[key & 15] != 0) {
                    break;
                }
            }
            se_pool->wait_timer[key & 15] = 9;
            slot = fn_800D8E58(work, id);
            if (slot) {
                se_slot_hold_start(work, pos, key, slot, id, 9, 0);
            }
            break;
        case 19:
            id = 48;
            if (se_pool->wait_timer[key & 15] > 0) {
                se_pool->wait_timer[key & 15]--;
                if (se_pool->wait_timer[key & 15] != 0) {
                    break;
                }
            }
            se_pool->wait_timer[key & 15] = 9;
            slot = fn_800D8E58(work, id);
            if (slot) {
                se_slot_hold_start(work, pos, key, slot, id, 9, 0);
            }
            break;
        case 20:
            if (se_pool->wait_timer[key & 15] > 0) {
                se_pool->wait_timer[key & 15]--;
                if (se_pool->wait_timer[key & 15] != 0) {
                    break;
                }
            }
            se_pool->wait_timer[key & 15] = 10;
            slot = fn_800D8E58(work, 17);
            if (slot) {
                se_slot_hold_start(work, pos, key, slot, 17, 10, 0);
            }
            break;
        case 21:
            id = (fn_800D8D8C(pos) == 1) ? 17 : 7;
            if (se_pool->hold_timer[key] > 0) {
                se_pool->hold_timer[key]--;
                if (se_pool->hold_timer[key] != 0) {
                    break;
                }
            }
            se_pool->hold_timer[key] = 17;
            key += kind;
            slot = fn_800D8E58(work, id);
            if (slot) {
                se_slot_hold_start(work, pos, key, slot, id, 17, 1);
            }
            break;
        case 22:
            key += kind;
            SE_SLOT_HOLD_REQ(work, 63, 15, 0)
            break;
        case 24:
            key += kind;
            SE_SLOT_HOLD_REQ(work, 3, 15, 0)
            break;
        case 25:
            id = (key != 0);
            SE_SLOT_HOLD_FIND(work, 0, id)
            slot = fn_800D8E58(work, id);
            if (slot) {
                se_slot_hold_start_owner(work, pos, key, slot, id, 15, 0, get_chacha_kamen_bank());
            }
            break;
        case 26:
            SE_SLOT_HOLD_FIND(work, 0, 0)
            slot = fn_800D8E58(work, 0);
            if (slot) {
                se_slot_hold_start_owner(work, pos, key, slot, 0, 27, 0, get_chacha_kamen_bank());
            }
            break;
        case 28:
            key += kind;
            SE_SLOT_HOLD_REQ(work, 142, 36, 0)
            break;
        case 29:
            key += kind;
            SE_SLOT_HOLD_REQ(work, 144, 25, 0)
            break;
        case 30:
            key += kind;
            SE_SLOT_HOLD_REQ(work, 146, 23, 0)
            break;
        case 31:
            key += kind;
            SE_SLOT_HOLD_REQ(work, 158, 42, 0)
            break;
        case 32:
            key += kind;
            SE_SLOT_HOLD_REQ(work, 163, 16, 0)
            break;
        case 33:
            key += kind;
            SE_SLOT_HOLD_REQ(work, 184, 30, 0)
            break;
        case 34:
        case 35:
            key += kind;
            if (kind == 34) {
                id = 38;
            } else {
                id = 39;
            }
            SE_SLOT_HOLD_REQ(work, id, 15, 0)
            break;
        case 36:
        case 37:
            key += kind;
            if (kind == 36) {
                id = 50;
            } else {
                id = 52;
            }
            SE_SLOT_HOLD_REQ(work, id, 10, 0)
            break;
        case 38:
            key += kind;
            move = (SeWork*)get_move_work_adrs(0);
            if (move == 0) {
                break;
            }
            id = (fn_800D8D8C(pos) == 1) ? 129 : 29;
            SE_SLOT_HOLD_REQ(move->se[0], id, 12, 0)
            break;
        }
    }
}

/* 0x800DF948 (0x88): clears the in-use flag of all 32 SE slots. */
extern "C" void se_work_slots_clear(_se_w* work)
{
    for (int i = 0; i < 32; i++) {
        work->slots[i].in_use = 0;
    }
}

extern "C" void se_point_area_scan(SePoolView* pool);

/* 0x800DF9D0 (0x124): opens the pool's point work for `owner`, frees every point handle and rescans the area. */
extern "C" void se_point_work_open(_ENEMY_WORK* owner)
{
    se_pool->point_work = (_se_w*)se_entry_request(8, owner, 0);
    for (int i = 0; i < 32; i++) {
        se_pool->point_handles[i] = 0;
    }
    se_point_area_scan(se_pool);
}

/* 0x800DFAF4 (0x118): stops the point work's slots, frees every point handle and rescans the area. */
extern "C" void se_point_work_reset(void)
{
    se_work_slots_clear(se_pool->point_work);
    for (int i = 0; i < 32; i++) {
        se_pool->point_handles[i] = 0;
    }
    se_point_area_scan(se_pool);
}

/* 0x800DFC0C (0x11C): returns the first free point handle, 255 when all 32 are taken. */
extern "C" u8 se_point_handle_free_get(void)
{
    for (int i = 0; i < 32; i++) {
        if (se_pool->point_handles[i] == 0) {
            return i;
        }
    }
    return 255;
}

/* 0x800DFD28 (0x28): frees the point handle `handle` (255 is no handle). */
extern "C" void se_point_handle_release(u8 handle)
{
    if (handle == 255) {
        return;
    }
    se_pool->point_handles[handle] = 0;
}

/* 0x800DFD50 (0x7C): returns the slot behind point handle `handle` while its stream still plays, else 0. */
extern "C" SeSlot* se_point_handle_live_get(_se_w* work, u8 handle)
{
    SeSlot* slot = se_pool->point_handles[handle];
    if (slot == 0) {
        return 0;
    }
    if (slot->in_use == 0) {
        return 0;
    }
    if (snd_stream_reloc_busy_ck(slot->field_0x3C)) {
        return slot;
    }
    return 0;
}

/* 0x800DFDCC (0x520): runs the area's sound points for one frame and steps the point work's slots. */
extern "C" void se_point_frame(SePool* self, s32 arg)
{
    SePoolView* pool = (SePoolView*)self;
    SePointRec* rec;
    s32 i;
    SeSlot* slot;
    u8 area;
    _se_w* work;
    SePointTable* table;
    nw4r::math::VEC3 pos;
    _PLW* player;
    VEC3_ctor(&pos);
    if (se_disabled_ck() == 1) {
        pool->prev_area = pool->area;
        pool->area = get_now_areano();
        if (se_pool->point_work == 0) {
            se_point_work_open(0);
        } else if (pool->prev_area != pool->area) {
            se_point_work_reset();
        }
        return;
    }
    player = (_PLW*)get_move_work_adrs(2);
    player += (s8)my_player_no();
    table = (SePointTable*)snd_stream_dma_get();
    if (table == 0) {
        work = se_pool->point_work;
        if (work == 0) {
            se_point_work_open(0);
        }
        if (work == 0) {
            return;
        }
        copyVec3(&work->listener_pos, &pool->cam_pos);
        copyVec3(&work->listener_target, &pool->cam_target);
    } else {
        rec = table->recs;
        if (table->loaded == 0 || table->count == 0) {
            return;
        }
        work = se_pool->point_work;
        if (work == 0) {
            return;
        }
        if (snd_chunk_handle_get(work->field_0x0C) == -1) {
            return;
        }
        copyVec3(&work->listener_pos, &pool->cam_pos);
        copyVec3(&work->listener_target, &pool->cam_target);
        area = pool->area;
        if (area == 0 && stage_map_kind_get(get_now_mapno()) == 1 && quest_id_head_ck() == 0) {
            nw4r::math::VEC3 left;
            copyVec3(&pos, setVec3(&left, -389.0f, 420.0f, 485.0f));
            shell_se_req(work, &pos, 6, 0);
            nw4r::math::VEC3 right;
            copyVec3(&pos, setVec3(&right, -353.0f, 420.0f, -474.0f));
            shell_se_req(work, &pos, 6, 1);
        }
        for (i = 0; i < table->count; i++, rec++) {
            if (rec->state == 0) {
                continue;
            }
            if (rec->area != area) {
                rec->state = 1;
                rec->timer = 0;
                continue;
            }
            if (rec->state == 2) {
                continue;
            }
            if (rec->timer > 0) {
                rec->timer--;
                if (rec->kind != 0) {
                    continue;
                }
                if (rec->shape != 1) {
                    continue;
                }
                if (rec->handle != 255) {
                    SeSlot* live = se_point_handle_live_get(work, rec->handle);
                    if (live && live->field_0x3C && snd_stream_reloc_busy_ck(live->field_0x3C)) {
                        if (GameMode_ck() == 2) {
                            if (player) {
                                pl_segment_closest_point(&rec->seg_start, &rec->seg_end, &player->vec_0x03C, &pos);
                            } else {
                                pl_segment_closest_point(&rec->seg_start, &rec->seg_end, &work->listener_pos, &pos);
                            }
                        } else {
                            pl_segment_closest_point(&rec->seg_start, &rec->seg_end, &work->listener_pos, &pos);
                        }
                        copyVec3(&live->pos, &pos);
                        live->field_0x10 = live->pos;
                        continue;
                    }
                }
                se_point_handle_release(rec->handle);
                rec->handle = 255;
                continue;
            }
            rec->timer = rec->interval;
            if (rec->kind != 0) {
                continue;
            }
            if (stage_map_kind_get(get_now_mapno()) == 22) {
                if (kujira_event_over_ck() == 0 && rec->se_id == 4) {
                    continue;
                }
                if (lb_part_slot_open_ck(0) == 0 && lb_part_slot_open_ck(1) == 0 && lb_part_slot_open_ck(2) == 0 &&
                    rec->se_id == 5) {
                    continue;
                }
                if (getItemListSelection() == 0 && rec->se_id == 13) {
                    continue;
                }
            }
            slot = fn_800D8E58(work, rec->se_id);
            if (slot) {
                slot->state = 3;
                slot->kind = 2;
                slot->owner = work->field_0x0C;
                slot->id = rec->se_id;
                slot->field_0x30 = -1;
                slot->field_0x34 = -1;
                slot->param = 0;
                if (rec->shape == 0) {
                    copyVec3(&slot->pos, &rec->pos);
                    slot->field_0x10 = slot->pos;
                } else {
                    pl_segment_closest_point(&rec->seg_start, &rec->seg_end, &work->listener_pos, &pos);
                    copyVec3(&slot->pos, &pos);
                    slot->field_0x10 = slot->pos;
                    rec->handle = 255;
                    u8 handle = se_point_handle_free_get();
                    if (handle != 255) {
                        se_pool->point_handles[handle] = slot;
                        rec->handle = handle;
                    }
                }
            }
            if (rec->interval == 0) {
                rec->state = 2;
            }
        }
    }
    slot = &work->slots[0];
    for (i = 0; i < 32; i++, slot++) {
        if (slot->in_use == 0) {
            continue;
        }
        switch (slot->state) {
        case 3:
        case 4:
            se_slot_level_calc(work, slot);
            se_slot_kind_dispatch(slot);
            break;
        }
    }
}

/* 0x800E02EC (0x13C): collects the current area's level zones from the point table into the pool. */
extern "C" void se_point_area_scan(SePoolView* pool)
{
    s32 i;
    s32 j;
    SePointRec* rec;
    s32 n;
    SePointTable* table = (SePointTable*)snd_stream_dma_get();
    if (table == 0) {
        return;
    }
    rec = table->recs;
    if (table->loaded == 0 || table->count == 0) {
        return;
    }
    for (i = 0; i < 32; i++) {
        memset(&pool->zones[i], 0, sizeof(SePointZone));
    }
    n = 0;
    for (j = 0; j < table->count; j++, rec++) {
        if (rec->state == 0) {
            continue;
        }
        if (rec->area != pool->area) {
            continue;
        }
        if (rec->kind != 1) {
            continue;
        }
        if (rec->shape != 0) {
            return;
        }
        pool->zones[n].active = 1;
        pool->zones[n].area = rec->area;
        copyVec3(&pool->zones[n].pos, &rec->pos);
        pool->zones[n].radius = rec->radius;
        pool->zones[n].level_tbl = rec->se_id;
        n++;
        if (n >= 32) {
            return;
        }
    }
}

/* 0x800E0428 (0xDC): returns the level the zone around `pos` gives the current area, 0 outside every zone. */
extern "C" u8 se_point_zone_level_get(_se_w* work, nw4r::math::VEC3* pos)
{
    SePoolView* pool = (SePoolView*)work;
    s32 i;
    SePointZone* zone;
    u8 area;
    u8 level = 0;
    area = pool->area;
    zone = pool->zones;
    for (i = 0; i < 32; i++, zone++) {
        if (zone->active == 0) {
            continue;
        }
        if (zone->area != area) {
            continue;
        }
        f32 dist = vec3_distance(&zone->pos, pos);
        if (dist > zone->radius) {
            continue;
        }
        u8* tbl = se_point_level_tbls[zone->level_tbl];
        u8 step = (s32)(dist / (zone->radius / 8.0f));
        if (step < 8) {
            level = tbl[step];
            break;
        }
    }
    return level;
}
