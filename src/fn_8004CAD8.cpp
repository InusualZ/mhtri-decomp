/*
 * The game-root save-block / math-helper translation unit (`.text` 0x8004CAD8..0x8005270C).
 * RANGE. extab 0x80006CF8..0x80007000, extabindex 0x8001ECE4..0x8001F170, .text 0x8004CAD8..0x8005270C,
 *   .rodata 0x8056F4A0..0x8056F4C0, .data 0x80581584..0x80581770, .bss 0x80669F68..0x8066A920,
 *   .sdata 0x80790EF0..0x80790EF8, .sbss 0x80794884..0x807948A0, .sdata2 0x80795BC0..0x80795BF8.
 *   One TU: its .sdata2 pool holds no duplicate value (fn_80047398.cpp and userdata_item.cpp each pool their own
 *   0x4330000000000000/1.0f), and every section tiles with its neighbours.
 * FLAGS. `#pragma peephole off` (retail re-extends narrow parameters with clrlwi/extsh and keeps not+and; 30 -> 53 of
 *   86 rows identical when measured) and `#pragma fp_contract off` (retail keeps fmuls+fadds).  The nw4r/SDK inline
 *   helpers retail emits out of line (MTX34/VEC3 conversions, VEC3Sub/Add/Scale/Dot/LenSq, FSqrt, MTX34RotXYZRad)
 *   are written as the functions they compile to; the fast casts come from OS/OSFastCast.h under `dont_inline`.
 * NAMES. Class 4: no `__FILE__` string (only nw4r's "arithmetic.h") and the dump has `zz_` placeholders.  Module
 *   GUESS: the save-block (user data) helper and game math library; proposed home `src/userdata/userdata_util.cpp`.
 *   GUESS (from each body and its callers): every renamed own row of this range (userdata_*, my_set/gun_set_*,
 *   hunter_rank_*, gallery_*, vs_user_*, wpad_*, mtx34_*, vec3_*, ivec2_*, hermite_interp, calcAngXZ, calcMtxAngX,
 *   rotMatrixXYZ, cpSetRotMatrixYXZ, math_sqrtf, monster_size_table_get (foreign), the three unowned tables.
 * RESIDUALS. write_wpad_memory 70 % / read_wpad_memory 83 %: retail shares the read and verify-retry tails between
 *   steps 3 and 4 (and 0 and 1) - goto-shaped; the goto-free source duplicates them (+204 B / -8 B).
 *   getDirVec 65 %: retail returns the VEC3 by NRVO (VEC3 had a constructor there); nw4r::math::VEC3 here has none.
 *   lobby_friend_list_sort 84 %, item_se_ck 83 %, hunter_rank_tier_get 67 %, the *_count_add pair 78 %:
 *   register/compare-shape residuals not reached by source variants tried.
 *   Data: .data/.sdata/.sdata2 not yet emitted in retail order (string literals and pool entries compile to @N);
 *   .data 0x80581180..0x805813E8 (event_flag_id_table, hunter_rank_points_table, gallery_unlock_table) is read only
 *   here but lies before userdata_item.cpp's .data, so it stays unclaimed (src/unsplit/userdata_tables.h).
 */

#pragma peephole off
#pragma fp_contract off

#include "types.h"
#include "nw4r/math.h"               /* nw4r::math::VEC3 / MTX34 + the free-function declarations (rule 9) */
#include "Runtime.PPCEABI.H/memset.h" /* owned by Runtime.PPCEABI.H/memset.c (rule 2) */
#include "Runtime.PPCEABI.H/memcpy.h" /* memcpy */
#include "quest/quest_types.h"        /* Q_UserData (the save block), Q_MoveWork */
#include "quest/quest_result_work.h"  /* Q_ResultWork, Q_QuestStat, Q_SizeRecord */
#include "mh3_pad/system_w.h"         /* system_w (mh3_pad.cpp) */
#include "fn_80047398/lobby_world_block.h" /* lobby_world_block, the cached save-block pointer */
#include "ef/fn_800CDB2C.h"           /* my_player_no (ef/system_core.cpp) */
#include "ef/get_move_work_adrs.h"    /* get_move_work_adrs (ef/system_core.cpp) */
#include "Pl/equip_slot_rec_get.h"    /* equip_slot_rec_get (Pl/pl_act.cpp) */
#include "equip_record_copy.h"        /* equip_record_copy (userdata_item.cpp) */
#include "ef/monster_size_table_get.h" /* monster_size_table_get (ef/system_core.cpp) */
#include "unsplit/userdata_tables.h"  /* the unowned save-block tables this unit reads */
#include "Pl/fn_8027D684.h"           /* PlSlotRec (the armour row equip_slot_rec_get hands back) */
#include "ef/cp_vector.h"             /* _CP_VECTOR */
#include "mh3_pad/vec3.h"             /* VEC3_ctor, copyVec3 (mh3_pad.cpp) */
#include "nw4r/math_types.h"          /* nw4r::math::MTX34Scale / MTX34RotXYZFIdx */
#include "nw4r/math_triangular.h"     /* nw4r::math::SinFIdx / CosFIdx */
#include "MTX/vec.h"                  /* PSVECNormalize, Vec */
#include "MTX/mtxvec.h"               /* PSMTXMultVec */
#include "MSL/mathf.h"                /* sqrtf */
#include "MSL_C/strstr.h"             /* strstr */
#include "OS/OSFastCast.h"            /* OSu16tof32 */
#include "nw4r/math_arithmetic.h"     /* nw4r::math::FrSqrt */
#include "nw4r/db_assert.h"           /* nw4r::db::Warning */
#include "draw_rect.h"    /* DrawRect, _DRAW_RECT_2D_TEX */
#include "enemy/lobby_state_block.h"  /* LobbyFriendList, LobbyTimerRow (enemy/em020_prog.cpp) */
#include "MSL_C/string.h"             /* strcmp, strcpy */
#include "MSL_C/strcat.h"             /* strcat */
#include "MSL_C/printf.h"             /* sprintf */
#include "MSL_C/alloc.h"              /* memcmp */
#include "vs_user_data.h" /* _vs_user_data */
#include "WPAD/WPADMem.h"             /* WPADReadGameData, WPADWriteGameData */
#include "OS/OSTime.h"                /* OSGetTick */
#include "mh3_pad/Psw.h"              /* Psw (mh3_pad.cpp) */
#include "mh3_pad/option_w.h"         /* option_w (mh3_pad.cpp) */
#include "sound/fn_800D7F54.h"        /* sysSE_req */
#include "enemy/lobby_net_busy_ck.h"  /* lobby_net_busy_ck (enemy/em020_prog.cpp) */
#include "menu/data_file_options_get.h" /* data_file_options_get (menu/menu_plsearch.cpp) */
#include "menu/menu_plsearch.h"       /* save_data_slot_max */
#include "menu/wii_sysmsg_table.h"    /* wii_sysmsg_table (menu/menu_sysmsg.cpp) */
#include "font/flfnt.h"               /* flfntStrLen */

/* --- the unit's own work blocks (declared, never defined - the split object defines them) ---------- */

/* .bss 0x80669F68 / 0x8066A1E8: the 0x27C B `FqResult` buffer and the 0x438 B `qResult` buffer the accessors hand
 * out. */
u8 fq_result_work[0x27C];
u8 q_result_work[0x438];
/* .bss 0x8066A620: the two 0x100 B VS user blocks `get_vsUser_work` indexes (0/1). */
u8 vs_user_work[0x200];
/* .bss 0x8066A820: the remote-memory transfer buffer. */
u8 wpad_mem_buf[0x100];

/* .sbss 0x80794884..: the remote-memory access state (`write_wpad_memory` / `read_wpad_memory`). */
u8 wpad_mem_step;          /* the access step */
u8 wpad_mem_player;        /* the player the access is for */
s32 wpad_mem_result;       /* the last transfer result (or the overwrite question's code) */
s32 wpad_mem_verify_retry; /* the write-and-verify attempts */
s32 wpad_mem_read_retry;   /* the read attempts */
u8 wpad_mem_done;          /* raised by `wpad_memory_callback` */
u8 wpad_mem_sel;           /* the overwrite question's cursor (1 = no) */
u8 wpad_mem_chan;          /* the remote the access goes to */
u8 wpad_mem_cb_chan;       /* the remote the last callback came from */
s32 wii_sysmsg_slot_count; /* the save slot count `wii_sysmsg_gen` substitutes */

/* .rodata 0x8056F4A0: the game title the remotes' game data is stamped with (UTF-16). */
const u16 wpad_game_title[16] = {'M', 'o', 'n', 's', 't', 'e', 'r', 'H', 'u', 'n', 't', 'e', 'r', '3', 0, 0};

/* One step of the hunter-rank table past rank 50: the last rank of the step and the points each of its ranks
 * costs.  size: 0x8 */
typedef struct HunterRankStep {
    /* +0x0 */ u32 rank_limit;
    /* +0x4 */ u32 points_per_rank;
} HunterRankStep;

/* The hunter-rank steps, ended by a rank limit of 0xFFFF (the end mark is one word, not a whole step).
 * size: 0x9C */
typedef struct HunterRankStepTable {
    /* +0x00 */ HunterRankStep steps[19];
    /* +0x98 */ u32 end_mark;
} HunterRankStepTable;

/* .data 0x805815C8: each equipment kind's first index in the save block's equipment flags. */
s16 equip_flag_base_table[16] = {
    0x000, 0x000, 0x08C, 0x10E, 0x190, 0x212, 0x2A8, 0x2B2, 0x320, 0x38E, 0x3FC, 0x46A, 0x4A6, 0x4E2, 0x51E, 0x55A,
};

/* .data 0x805815E8: the points per rank past rank 50, in steps of 50 ranks. */
HunterRankStepTable hunter_rank_step_table = {
    {
        {100, 7850}, {150, 8000}, {200, 8150}, {250, 8300}, {300, 8450}, {350, 8600}, {400, 8750},
        {450, 8900}, {500, 9050}, {550, 9200}, {600, 9350}, {650, 9500}, {700, 9650}, {750, 9800},
        {800, 9950}, {850, 10100}, {900, 10350}, {950, 10500}, {1000, 10650},
    },
    0xFFFF,
};

/* The save block the lobby band caches in `lobby_world_block`. */
static inline Q_UserData* userdata_block(void)
{
    return (Q_UserData*)lobby_world_block;
}

/* --- worn equipment ------------------------------------------------------------------------------------ */

/* 0x8004CAD8 (0x9C): clears the worn-record index of armour kind `kind` (1..6); returns 0 for any other kind. */
extern "C" s32 set_slot_none(Q_UserData* user, u8 kind)
{
    switch (kind) {
    case 1:
        user->box_armour_0x0092[0] = 0xFFFF;
        break;
    case 2:
        user->box_armour_0x0092[1] = 0xFFFF;
        break;
    case 3:
        user->box_armour_0x0092[2] = 0xFFFF;
        break;
    case 4:
        user->box_armour_0x0092[3] = 0xFFFF;
        break;
    case 5:
        user->box_armour_0x0092[4] = 0xFFFF;
        break;
    case 6:
        user->box_armour_0x0092[5] = 0xFFFF;
        break;
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
        return 0;
    default:
        return 0;
    }
    return 1;
}

extern "C" u32 equip_wearable_ck(Q_UserData* user, _EQUIP* rec);

/* 0x8004CB74 (0x124): drops every worn armour or bowgun-part record the hunter can no longer wear. */
extern "C" void userdata_worn_unwearable_clear(Q_UserData* user)
{
    if (equip_wearable_ck(user, &user->armour_0x0044[4]) == 0) {
        user->box_armour_0x0092[4] = 0xFFFF;
    }
    if (equip_wearable_ck(user, &user->armour_0x0044[0]) == 0) {
        user->box_armour_0x0092[0] = 0xFFFF;
    }
    if (equip_wearable_ck(user, &user->armour_0x0044[2]) == 0) {
        user->box_armour_0x0092[2] = 0xFFFF;
    }
    if (equip_wearable_ck(user, &user->armour_0x0044[1]) == 0) {
        user->box_armour_0x0092[1] = 0xFFFF;
    }
    if (equip_wearable_ck(user, &user->armour_0x0044[3]) == 0) {
        user->box_armour_0x0092[3] = 0xFFFF;
    }
    if (equip_wearable_ck(user, &user->armour_0x0044[5]) == 0) {
        user->box_armour_0x0092[5] = 0xFFFF;
    }
    if (equip_wearable_ck(user, &user->parts_0x002C[0]) == 0) {
        user->box_parts_0x008E[0] = 0xFFFF;
    }
    if (equip_wearable_ck(user, &user->parts_0x002C[1]) == 0) {
        user->box_parts_0x008E[1] = 0xFFFF;
    }
}

/* 0x8004CC98 (0x38): the wearer mask of the hunter: 1 male / 2 female, plus 4 blademaster or 8 gunner. */
extern "C" u8 userdata_equip_mask_get(Q_UserData* user)
{
    u8 mask = 0;
    if (user->hunter_sex_0x0000 == 0) {
        mask |= 1;
    } else {
        mask |= 2;
    }
    if (user->weapon_0x0020.kind == 11) {
        mask |= 8;
    } else {
        mask |= 4;
    }
    return mask;
}

/* 0x8004CCD0 (0xA0): whether the hunter can wear `rec` (bowgun parts need a bowgun, armour the wearer mask). */
extern "C" u32 equip_wearable_ck(Q_UserData* user, _EQUIP* rec)
{
    u8 mask = userdata_equip_mask_get(user);
    PlSlotRec* slot;
    switch (rec->kind) {
    case 12:
    case 13:
        if (user->weapon_0x0020.kind != 11) {
            return 0;
        }
        break;
    default:
        slot = equip_slot_rec_get(rec);
        if (slot == NULL) {
            return 0;
        }
        if (mask != (mask & slot->wear_flags)) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x8004CD70 (0x34): whether box record `index` exists and is of kind `kind`. */
extern "C" u32 userdata_box_kind_ck(Q_UserData* user, u16 index, u8 kind)
{
    if (index == 0xFFFF) {
        return 0;
    }
    return user->equip_box_0x0E00[index].kind == kind;
}

/* 0x8004CDA4 (0x318): rebuilds the worn records from the equipment box indices. */
extern "C" void userdata_worn_refresh(Q_UserData* user)
{
    int i;

    memset(&user->weapon_0x0020, 0, sizeof(_EQUIP));
    memset(&user->parts_0x002C[0], 0, sizeof(_EQUIP) * 2);
    if ((u8)equip_kind_table_class(user->equip_box_0x0E00[user->box_weapon_0x008C].kind) == 1) {
        equip_record_copy(&user->weapon_0x0020, &user->equip_box_0x0E00[user->box_weapon_0x008C]);
    } else {
        user->weapon_0x0020.kind = 8;
        user->weapon_0x0020.item_id = 1;
    }
    if (user->weapon_0x0020.kind == 11 && Gunner_opt_ok_ck(&user->weapon_0x0020) == 0) {
        user->parts_0x002C[0].kind = 12;
        user->parts_0x002C[0].item_id = user->weapon_0x0020.item_id;
        user->parts_0x002C[1].kind = 13;
        user->parts_0x002C[1].item_id = user->weapon_0x0020.item_id;
    } else {
        _EQUIP* part = user->parts_0x002C;
        for (i = 0; i < 2; i++, part++) {
            if (user->box_parts_0x008E[i] != 0xFFFF &&
                (u8)equip_kind_table_class(user->equip_box_0x0E00[user->box_parts_0x008E[i]].kind) == 2) {
                equip_record_copy(part, &user->equip_box_0x0E00[user->box_parts_0x008E[i]]);
            }
        }
    }
    userdata_worn_unwearable_clear(user);
    if (userdata_box_kind_ck(user, user->box_armour_0x0092[3], 4) == 1) {
        equip_record_copy(&user->armour_0x0044[3], &user->equip_box_0x0E00[user->box_armour_0x0092[3]]);
    } else {
        memset(&user->armour_0x0044[3], 0, sizeof(_EQUIP));
        user->armour_0x0044[3].kind = 4;
    }
    if (userdata_box_kind_ck(user, user->box_armour_0x0092[4], 5) == 1) {
        equip_record_copy(&user->armour_0x0044[4], &user->equip_box_0x0E00[user->box_armour_0x0092[4]]);
    } else {
        memset(&user->armour_0x0044[4], 0, sizeof(_EQUIP));
        user->armour_0x0044[4].kind = 5;
    }
    if (userdata_box_kind_ck(user, user->box_armour_0x0092[0], 1) == 1) {
        equip_record_copy(&user->armour_0x0044[0], &user->equip_box_0x0E00[user->box_armour_0x0092[0]]);
    } else {
        memset(&user->armour_0x0044[0], 0, sizeof(_EQUIP));
        user->armour_0x0044[0].kind = 1;
    }
    if (userdata_box_kind_ck(user, user->box_armour_0x0092[1], 2) == 1) {
        equip_record_copy(&user->armour_0x0044[1], &user->equip_box_0x0E00[user->box_armour_0x0092[1]]);
    } else {
        memset(&user->armour_0x0044[1], 0, sizeof(_EQUIP));
        user->armour_0x0044[1].kind = 2;
    }
    if (userdata_box_kind_ck(user, user->box_armour_0x0092[2], 3) == 1) {
        equip_record_copy(&user->armour_0x0044[2], &user->equip_box_0x0E00[user->box_armour_0x0092[2]]);
    } else {
        memset(&user->armour_0x0044[2], 0, sizeof(_EQUIP));
        user->armour_0x0044[2].kind = 3;
    }
    if (userdata_box_kind_ck(user, user->box_armour_0x0092[5], 6) == 1) {
        equip_record_copy(&user->armour_0x0044[5], &user->equip_box_0x0E00[user->box_armour_0x0092[5]]);
    } else {
        memset(&user->armour_0x0044[5], 0, sizeof(_EQUIP));
        user->armour_0x0044[5].kind = 6;
    }
}

/* 0x8004D0BC (0x1C): whether equipment box record `index` is empty. */
extern "C" u32 userdata_box_empty_ck(Q_UserData* user, u16 index)
{
    return user->equip_box_0x0E00[index].kind == 0;
}

/* 0x8004D0D8 (0x8): stores the character make's demo equipment set. */
extern "C" void userdata_hunter_equip_set(Q_UserData* user, u8 equip)
{
    user->hunter_equip_0x009E = equip;
}

/* 0x8004D0E0 (0x8): stores the character make's hair style. */
extern "C" void userdata_hunter_hair_set(Q_UserData* user, u8 hair)
{
    user->hunter_hair_0x0002 = hair;
}

/* 0x8004D0E8 (0x38): clamps `*value += delta` into [0, 9999999]. */
extern "C" void score_add_clamped(s32 delta, s32* value)
{
    *value += delta;
    if (*value > 9999999) {
        *value = 9999999;
    } else if (*value < 0) {
        *value = 0;
    }
}

/* 0x8004D120 (0x14): caches the save block in `lobby_world_block` and returns it. */
Q_UserData* get_userdata(void)
{
    return (Q_UserData*)(lobby_world_block = (u8*)system_w.layout_0x95c);
}

/* 0x8004D134 (0xC): the 0x27C B FqResult buffer. */
extern "C" void* get_FqResult_work(void)
{
    return fq_result_work;
}

/* 0x8004D140 (0xC): the 0x438 B qResult buffer. */
Q_ResultWork* get_qResult_work(void)
{
    return (Q_ResultWork*)q_result_work;
}

/* 0x8004D14C (0x30): one of the two 0x100 B VS user slots, or null when the index is out of range. */
void* get_vsUser_work(s32 index)
{
    if (index < 0) {
        return 0;
    }
    if (index >= 2) {
        return 0;
    }
    return vs_user_work + index * 0x100;
}

/* 0x8004D17C (0x14): clears the FqResult buffer. */
void clear_FqResult_work(void)
{
    memset(fq_result_work, 0, sizeof(fq_result_work));
}

/* 0x8004D190 (0x14): clears the qResult buffer. */
void clear_qResult_work(void)
{
    memset(q_result_work, 0, sizeof(q_result_work));
}

/* 0x8004D1A4 (0x4C): clears both VS user slots. */
extern "C" void vs_user_work_clear_all(void)
{
    memset(vs_user_work, 0, 0x100);
    memset(vs_user_work + 0x100, 0, 0x100);
}

/* 0x8004D1F0 (0x20): clears VS user slot `index`. */
extern "C" void vs_user_work_clear(u8 index)
{
    memset(vs_user_work + (index << 8), 0, 0x100);
}

/* --- progress, item and equipment flags ------------------------------------------------------------- */

/* 0x8004D210 (0x34): raises progress flag `id`. */
extern "C" void userdata_progress_flag_set(s32 id)
{
    userdata_block()->progress_flags_0x47DA[id / 16] |= (u16)(1 << (id & 15));
}

/* 0x8004D244 (0x38): drops progress flag `id`. */
extern "C" void userdata_progress_flag_clear(s32 id)
{
    userdata_block()->progress_flags_0x47DA[id / 16] &= (u16)~(1 << (id & 15));
}

/* 0x8004D27C (0x38): whether progress flag `id` is raised. */
extern "C" u32 userdata_progress_flag_ck(s32 id)
{
    return ((1 << (id & 15)) & userdata_block()->progress_flags_0x47DA[id / 16]) != 0;
}

/* 0x8004D2B4 (0x34): marks item `item` as seen. */
extern "C" void userdata_item_seen_set(Q_UserData* user, u16 item)
{
    if (item > 746) {
        return;
    }
    user->item_seen_0x4AD8[item / 32U] |= 1 << (item & 31);
}

/* 0x8004D2E8 (0x40): whether bit `item` of the item bit run `bits` is raised. */
extern "C" u32 item_bit_ck(const u32* bits, u16 item)
{
    if (item > 746) {
        return 0;
    }
    return ((1 << (item & 31)) & bits[item / 32U]) != 0;
}

/* 0x8004D328 (0xC): whether item `id` was seen. */
extern "C" u32 userdata_box_item_ck(Q_UserData* user, u16 id)
{
    return item_bit_ck(user->item_seen_0x4AD8, id);
}

/* 0x8004D334 (0x44): credits resource points, clamped into [0, 9999999]. */
extern "C" void userdata_zenny_add(s32 amount)
{
    userdata_block()->points_0x3F04 += amount;
    s32 value = userdata_block()->points_0x3F04;
    if (value > 9999999) {
        userdata_block()->points_0x3F04 = 9999999;
    } else if (value < 0) {
        userdata_block()->points_0x3F04 = 0;
    }
}

/* 0x8004D378 (0x58): raises flag bit `bit` of equipment item `item` of kind `kind`. */
extern "C" void userdata_equip_flag_set(u8 kind, u16 item, u16 bit)
{
    if (kind == 0 || kind >= 16) {
        return;
    }
    userdata_block()->equip_flags_0x4B50[(s16)(item + equip_flag_base_table[kind])] |= (u8)(1 << bit);
}

/* 0x8004D3D0 (0x18): `userdata_equip_flag_set` for an equipment record. */
extern "C" void userdata_equip_rec_flag_set(_EQUIP* rec, u16 bit)
{
    userdata_equip_flag_set(rec->kind, rec->item_id, bit);
}

/* 0x8004D3E8 (0x5C): drops flag bit `bit` of equipment item `item` of kind `kind`. */
extern "C" void userdata_equip_flag_clear(u8 kind, u16 item, u16 bit)
{
    if (kind == 0 || kind >= 16) {
        return;
    }
    userdata_block()->equip_flags_0x4B50[(s16)(item + equip_flag_base_table[kind])] &= (u8)~(1 << bit);
}

/* 0x8004D444 (0x18): `userdata_equip_flag_clear` for an equipment record. */
extern "C" void userdata_equip_rec_flag_clear(_EQUIP* rec, u16 bit)
{
    userdata_equip_flag_clear(rec->kind, rec->item_id, bit);
}

/* 0x8004D45C (0x60): whether flag bit `bit` of equipment item `item` of kind `kind` is raised. */
extern "C" u32 userdata_equip_flag_ck(u8 kind, u16 item, u16 bit)
{
    if (kind == 0 || kind >= 16) {
        return 0;
    }
    return ((1 << bit) & userdata_block()->equip_flags_0x4B50[(s16)(item + equip_flag_base_table[kind])]) != 0;
}

/* 0x8004D4BC (0x18): `userdata_equip_flag_ck` for an equipment record. */
extern "C" u32 userdata_equip_rec_flag_ck(_EQUIP* rec, u16 bit)
{
    return userdata_equip_flag_ck(rec->kind, rec->item_id, bit);
}

/* 0x8004D4D4 (0x128): drops the "new" mark (bit 2) of every equipment item. */
extern "C" void userdata_item_new_marks_clear(void)
{
    s16 i;
    for (i = 0; i < 1500; i++) {
        userdata_block()->equip_flags_0x4B50[i] &= ~4;
    }
}

/* 0x8004D5FC (0x44): the bit index of event `event` in the event-id table, 0xFFFF when it has none. */
extern "C" u16 event_flag_index_get(u16 event)
{
    u16 index = 0;
    const u16* id;
    for (id = event_flag_id_table; *id != 0xFFFF; id++, index++) {
        if (*id == event) {
            return index;
        }
    }
    return 0xFFFF;
}

/* 0x8004D640 (0x58): raises the flag of event `event`. */
extern "C" void userdata_event_flag_set(u16 event)
{
    u16 index = event_flag_index_get(event);
    if (index == 0xFFFF) {
        return;
    }
    userdata_block()->event_flags_0x512C[index / 32U] |= 1 << (index & 31);
}

/* 0x8004D698 (0x74): raises the cleared quest's event flag (the special quests only for player 0). */
extern "C" void quest_clear_event_flag_set(void)
{
    Q_ResultWork* result = get_qResult_work();
    if (((Q_MoveWork*)get_move_work_adrs(0))->field_0xFD != 4) {
        return;
    }
    if ((u32)(result->progress_0x1E0 - 14000) <= 2 || (u32)(result->progress_0x1E0 - 19000) <= 1) {
        if ((s8)my_player_no() != 0) {
            return;
        }
    }
    userdata_event_flag_set(result->progress_0x1E0);
}

/* 0x8004D70C (0x68): whether the flag of event `event` is raised. */
extern "C" u32 userdata_flag_ck(u16 event)
{
    u16 index = event_flag_index_get(event);
    if (index != 0xFFFF && ((1 << (index & 31)) & userdata_block()->event_flags_0x512C[index / 32U]) != 0) {
        return 1;
    }
    return 0;
}

/* 0x8004D774 (0x28): raises the result-screen event bit of event `event` (9000 + bit). */
extern "C" void userdata_event_bit_set(u16 event)
{
    userdata_block()->event_bits_0x52E2 |= (u16)(1 << (s8)(event - 9000));
}

/* 0x8004D79C (0x2C): whether the result-screen event bit of event `event` is raised. */
extern "C" u32 userdata_event_bit_ck(u16 event)
{
    u16 bits = userdata_block()->event_bits_0x52E2;
    return (bits & (1 << (s8)(event - 9000))) != 0;
}

/* --- saved equipment sets --------------------------------------------------------------------------- */

/* 0x8004D7C8 (0x44): empties equipment set `index`. */
extern "C" void my_set_clear(Q_UserData* user, u8 index)
{
    Q_MySet* set = &user->my_set_0x3380[index];
    set->weapon_0x00 = 0xFFFF;
    set->parts_0x02[0] = 0xFFFF;
    set->parts_0x02[1] = 0xFFFF;
    set->armour_0x06[3] = 0xFFFF;
    set->armour_0x06[4] = 0xFFFF;
    set->armour_0x06[0] = 0xFFFF;
    set->armour_0x06[1] = 0xFFFF;
    set->armour_0x06[2] = 0xFFFF;
    set->armour_0x06[5] = 0xFFFF;
    set->flag_0x12 = 0;
}

/* 0x8004D80C (0x60): stores the worn records' box indices as equipment set `index`. */
extern "C" void my_set_store(Q_UserData* user, u8 index)
{
    Q_MySet* set = &user->my_set_0x3380[index];
    set->weapon_0x00 = user->box_weapon_0x008C;
    set->parts_0x02[0] = user->box_parts_0x008E[0];
    set->parts_0x02[1] = user->box_parts_0x008E[1];
    set->armour_0x06[3] = user->box_armour_0x0092[3];
    set->armour_0x06[4] = user->box_armour_0x0092[4];
    set->armour_0x06[0] = user->box_armour_0x0092[0];
    set->armour_0x06[1] = user->box_armour_0x0092[1];
    set->armour_0x06[2] = user->box_armour_0x0092[2];
    set->armour_0x06[5] = user->box_armour_0x0092[5];
    set->flag_0x12 = 0;
}

/* 0x8004D86C (0x30): whether equipment set `index` is in use. */
extern "C" u32 my_set_used_ck(Q_UserData* user, u8 index)
{
    return user->my_set_0x3380[index].weapon_0x00 != 0xFFFF;
}

/* 0x8004D89C (0x60): whether any equipment set is in use. */
extern "C" u32 my_set_any_ck(Q_UserData* user)
{
    u8 i;
    for (i = 0; i < 20; i++) {
        if (my_set_used_ck(user, i) == 1) {
            return 1;
        }
    }
    return 0;
}

/* 0x8004D8FC (0xC4): whether equipment set `index` uses box record `box`. */
extern "C" u32 my_set_box_used_ck(Q_UserData* user, u16 box, u8 index)
{
    Q_MySet* set = &user->my_set_0x3380[index];
    if (set->weapon_0x00 == box) {
        return 1;
    }
    if (box == set->parts_0x02[0]) {
        return 1;
    }
    if (box == set->parts_0x02[1]) {
        return 1;
    }
    if (set->armour_0x06[3] == box) {
        return 1;
    }
    if (set->armour_0x06[4] == box) {
        return 1;
    }
    if (set->armour_0x06[0] == box) {
        return 1;
    }
    if (set->armour_0x06[1] == box) {
        return 1;
    }
    if (set->armour_0x06[2] == box) {
        return 1;
    }
    return set->armour_0x06[5] == box;
}

/* 0x8004D9C0 (0x70): whether any equipment set uses box record `box`. */
extern "C" u32 my_set_record_used_ck(Q_UserData* user, u16 box)
{
    u8 i;
    for (i = 0; i < 20; i++) {
        if (my_set_box_used_ck(user, box, i) == 1) {
            return 1;
        }
    }
    return 0;
}

/* 0x8004DA30 (0x2C): swaps box index `a` for `b` (and `b` for `a`) in `*slot`. */
extern "C" void box_index_swap(u16* slot, u16 a, u16 b)
{
    if (*slot == a) {
        *slot = b;
    } else if (*slot == b) {
        *slot = a;
    }
}

/* 0x8004DA5C (0xE4): swaps box indices `a` and `b` in every equipment set. */
extern "C" void my_set_equip_id_swap(Q_UserData* user, u16 a, u16 b)
{
    int i;
    int j;
    Q_MySet* set = user->my_set_0x3380;
    for (i = 0; i < 20; i++) {
        box_index_swap(&set->weapon_0x00, a, b);
        for (j = 0; j < 2; j++) {
            box_index_swap(&set->parts_0x02[j], a, b);
        }
        box_index_swap(&set->armour_0x06[3], a, b);
        box_index_swap(&set->armour_0x06[4], a, b);
        box_index_swap(&set->armour_0x06[0], a, b);
        box_index_swap(&set->armour_0x06[1], a, b);
        box_index_swap(&set->armour_0x06[2], a, b);
        box_index_swap(&set->armour_0x06[5], a, b);
        set++;
    }
}

/* 0x8004DB40 (0x2C): empties bowgun set `index`. */
extern "C" void gun_set_clear(Q_UserData* user, u8 index)
{
    Q_GunSet* set = &user->gun_set_0x36C8[index];
    set->weapon_0x00 = 0xFFFF;
    set->parts_0x02[0] = 0xFFFF;
    set->parts_0x02[1] = 0xFFFF;
    set->flag_0x06 = 0;
}

/* 0x8004DB6C (0x30): stores the worn bowgun and parts as bowgun set `index`. */
extern "C" void gun_set_store(Q_UserData* user, u8 index)
{
    Q_GunSet* set = &user->gun_set_0x36C8[index];
    set->weapon_0x00 = user->box_weapon_0x008C;
    set->parts_0x02[0] = user->box_parts_0x008E[0];
    set->parts_0x02[1] = user->box_parts_0x008E[1];
    set->flag_0x06 = 0;
}

/* 0x8004DB9C (0x30): whether bowgun set `index` is in use. */
extern "C" u32 gun_set_used_ck(Q_UserData* user, u8 index)
{
    return user->gun_set_0x36C8[index].weapon_0x00 != 0xFFFF;
}

/* 0x8004DBCC (0x60): whether any bowgun set is in use. */
extern "C" u32 gun_set_any_ck(Q_UserData* user)
{
    u8 i;
    for (i = 0; i < 20; i++) {
        if (gun_set_used_ck(user, i) == 1) {
            return 1;
        }
    }
    return 0;
}

/* 0x8004DC2C (0x54): whether bowgun set `index` uses box record `box`. */
extern "C" u32 gun_set_box_used_ck(Q_UserData* user, u16 box, u8 index)
{
    Q_GunSet* set = &user->gun_set_0x36C8[index];
    if (set->weapon_0x00 == box) {
        return 1;
    }
    if (box == set->parts_0x02[0]) {
        return 1;
    }
    if (box == set->parts_0x02[1]) {
        return 1;
    }
    return 0;
}

/* 0x8004DC80 (0x70): whether any bowgun set uses box record `box`. */
extern "C" u32 gun_set_record_used_ck(Q_UserData* user, u16 box)
{
    u8 i;
    for (i = 0; i < 20; i++) {
        if (gun_set_box_used_ck(user, box, i) == 1) {
            return 1;
        }
    }
    return 0;
}

/* 0x8004DCF0 (0x84): swaps box indices `a` and `b` in every bowgun set. */
extern "C" void my_set_gun_id_swap(Q_UserData* user, u16 a, u16 b)
{
    int i;
    int j;
    Q_GunSet* set = user->gun_set_0x36C8;
    for (i = 0; i < 20; i++) {
        box_index_swap(&set->weapon_0x00, a, b);
        for (j = 0; j < 2; j++) {
            box_index_swap(&set->parts_0x02[j], a, b);
        }
        set++;
    }
}

/* --- hunter rank ------------------------------------------------------------------------------------ */

/* 0x8004DD74 (0x30): the hunter-rank tier: 0 before the rank starts, 1 up to rank 30, 2 above. */
extern "C" u32 userdata_rank_band_get(void)
{
    u16 rank = userdata_block()->hunter_rank_0x3DE4;
    if (rank == 0) {
        return 0;
    }
    return 2 - (rank <= 30);
}

/* 0x8004DDA4 (0x28): starts the hunter rank at 1; returns 0 when it had already started. */
extern "C" u32 userdata_online_first_set(void)
{
    Q_UserData* user = userdata_block();
    if (user->hunter_rank_0x3DE4 != 0) {
        return 0;
    }
    user->hunter_rank_0x3DE4 = 1;
    return 1;
}

/* 0x8004DDCC (0x8): always 1. */
extern "C" u32 hunter_rank_enabled_ck(void)
{
    return 1;
}

/* 0x8004DDD4 (0xA8): the hunter points rank `rank` starts at. */
extern "C" u32 hunter_rank_points_get(u16 rank)
{
    u16 base_rank;
    u32 points;
    const HunterRankStep* step;

    if (rank == 0) {
        return 0;
    }
    if (rank > 999) {
        return 10000000;
    }
    points = hunter_rank_points_table[49];
    step = hunter_rank_step_table.steps;
    if (rank <= 50) {
        return hunter_rank_points_table[rank - 1];
    }
    for (base_rank = 50; step->rank_limit != 0xFFFF; step++) {
        if (rank <= step->rank_limit) {
            return points + step->points_per_rank * (rank - base_rank);
        }
        points += step->points_per_rank * 50;
        base_rank += 50;
    }
    return 10000000;
}

/* 0x8004DE7C (0x94): the hunter rank the save block's points reach. */
extern "C" u16 get_hunter_rank_max(Q_UserData* user)
{
    int rank;
    if (user->hunter_rank_0x3DE4 == 0) {
        return 0;
    }
    if (user->hunter_points_0x3DE0 >= hunter_rank_points_get(999)) {
        return 999;
    }
    for (rank = 0; rank < 1000; rank++) {
        if (user->hunter_points_0x3DE0 < hunter_rank_points_get(rank)) {
            return rank - 1;
        }
    }
    return 0;
}

/* 0x8004DF10 (0xEC): the hunter rank, its points and the points the next rank starts at. */
extern "C" void userdata_hunter_rank_info_get(u16* rank, u32* points, u32* next)
{
    int i;
    if (userdata_block()->hunter_rank_0x3DE4 == 0) {
        *rank = 1;
        *points = userdata_block()->hunter_points_0x3DE0;
        *next = 0;
        return;
    }
    *points = userdata_block()->hunter_points_0x3DE0;
    if (userdata_block()->hunter_points_0x3DE0 >= hunter_rank_points_get(999)) {
        *rank = 999;
        *next = 0;
        return;
    }
    for (i = 0; i < 1000; i++) {
        if (*points < hunter_rank_points_get(i)) {
            *rank = i - 1;
            *next = hunter_rank_points_get(i);
            return;
        }
    }
}

/* 0x8004DFFC (0x190): raises the progress flags rank `rank` unlocks. */
extern "C" void hunter_rank_progress_flags_set(u16 rank)
{
    if (userdata_progress_flag_ck(51) == 0 && rank >= 8) {
        userdata_progress_flag_set(51);
    }
    if (userdata_progress_flag_ck(460) == 0 && rank >= 9) {
        userdata_progress_flag_set(460);
    }
    if (userdata_progress_flag_ck(52) == 0 && rank >= 17) {
        userdata_progress_flag_set(52);
    }
    if (userdata_progress_flag_ck(461) == 0 && rank >= 18) {
        userdata_progress_flag_set(461);
    }
    if (userdata_progress_flag_ck(53) == 0 && rank >= 30) {
        userdata_progress_flag_set(53);
    }
    if (userdata_progress_flag_ck(462) == 0 && rank >= 31) {
        userdata_progress_flag_set(462);
    }
    if (userdata_progress_flag_ck(54) == 0 && rank >= 39) {
        userdata_progress_flag_set(54);
    }
    if (userdata_progress_flag_ck(463) == 0 && rank >= 40) {
        userdata_progress_flag_set(463);
    }
    if (userdata_progress_flag_ck(55) == 0 && rank >= 50) {
        userdata_progress_flag_set(55);
    }
    if (userdata_progress_flag_ck(464) == 0 && rank >= 51) {
        userdata_progress_flag_set(464);
    }
}

/* 0x8004E18C (0x38): holds the points just below the start of rank `rank` (an urgent quest gates it). */
extern "C" void userdata_hunter_points_hold(Q_UserData* user, u16 rank)
{
    user->hunter_points_0x3DE0 = hunter_rank_points_get(rank) - 1;
}

/* 0x8004E1C4 (0x260): credits `points` hunter points and steps the rank, stopping at an urgent-quest gate;
 * returns 1 when the points fell below the rank, 2 at the cap, 3..7 at a gate, else 0. */
extern "C" s32 userdata_hunter_points_apply(Q_UserData* user, s32 points)
{
    u16 rank = user->hunter_rank_0x3DE4;
    s32 result = 0;
    u32 magnitude = points;
    u32 start;
    u16 next;

    if (points < 0) {
        magnitude = -points;
    }
    if (points < 0 && user->hunter_points_0x3DE0 < magnitude) {
        user->hunter_points_0x3DE0 = 0;
        result = 1;
    } else {
        user->hunter_points_0x3DE0 += points;
    }
    if (user->hunter_points_0x3DE0 > 9999999) {
        user->hunter_points_0x3DE0 = 9999999;
    }
    start = hunter_rank_points_get(rank);
    if (user->hunter_points_0x3DE0 < start) {
        user->hunter_points_0x3DE0 = start;
        hunter_rank_progress_flags_set(user->hunter_rank_0x3DE4 = get_hunter_rank_max(user));
        return 1;
    }
    if (rank >= 999) {
        hunter_rank_progress_flags_set(user->hunter_rank_0x3DE4);
        return 2;
    }
    do {
        next = rank + 1;
        if (user->hunter_points_0x3DE0 < hunter_rank_points_get(next)) {
            hunter_rank_progress_flags_set(user->hunter_rank_0x3DE4 = get_hunter_rank_max(user));
            return result;
        }
        switch (next) {
        case 8:
            rank++;
            break;
        case 9:
            if (userdata_flag_ck(14000) == 1) {
                rank++;
            } else {
                userdata_hunter_points_hold(user, 9);
                result = 3;
            }
            break;
        case 17:
            rank++;
            break;
        case 18:
            if (userdata_flag_ck(14001) == 1) {
                rank++;
            } else {
                userdata_hunter_points_hold(user, 18);
                result = 4;
            }
            break;
        case 30:
            rank++;
            break;
        case 31:
            if (userdata_flag_ck(14002) == 1) {
                rank++;
            } else {
                userdata_hunter_points_hold(user, 31);
                result = 5;
            }
            break;
        case 39:
            rank++;
            break;
        case 40:
            if (userdata_flag_ck(19000) == 1) {
                rank++;
            } else {
                userdata_hunter_points_hold(user, 40);
                result = 6;
            }
            break;
        case 50:
            rank++;
            break;
        case 51:
            if (userdata_flag_ck(19001) == 1) {
                rank++;
            } else {
                userdata_hunter_points_hold(user, 51);
                result = 7;
            }
            break;
        default:
            rank++;
            break;
        }
    } while (result == 0);
    hunter_rank_progress_flags_set(user->hunter_rank_0x3DE4 = get_hunter_rank_max(user));
    return result;
}

/* 0x8004E424 (0xC): credits `points` hunter points to the cached save block. */
extern "C" s32 userdata_hunter_points_add(s32 points)
{
    return userdata_hunter_points_apply(userdata_block(), points);
}

/* 0x8004E430 (0x4): credits `points` hunter points to `user`. */
extern "C" s32 userdata_hunter_points_add_to(Q_UserData* user, s32 points)
{
    return userdata_hunter_points_apply(user, points);
}

/* --- hunt records ----------------------------------------------------------------------------------- */

/* 0x8004E434 (0x50): adds 41 hunt counts into record set a, each clamped to 9999. */
extern "C" void userdata_record_a_count_add(const u16* counts)
{
    int i;
    for (i = 0; i < 41; i++) {
        userdata_block()->set_a.count[i] += *counts++;
        if (userdata_block()->set_a.count[i] > 9999) {
            userdata_block()->set_a.count[i] = 9999;
        }
    }
}

/* 0x8004E484 (0x50): adds 41 capture counts into record set b, each clamped to 9999. */
extern "C" void userdata_record_b_count_add(const u16* counts)
{
    int i;
    for (i = 0; i < 41; i++) {
        userdata_block()->set_b.count[i] += *counts++;
        if (userdata_block()->set_b.count[i] > 9999) {
            userdata_block()->set_b.count[i] = 9999;
        }
    }
}

/* 0x8004E4D4 (0x94): folds 41 measured sizes into the size records: `which` 0 keeps the smallest, else the largest. */
extern "C" void userdata_size_records_update(const Q_SizeRecord* sizes, u8 which)
{
    int i;
    u16 size;
    if (which == 0) {
        for (i = 0; i < 41; i++, sizes++) {
            size = sizes->size_min;
            if (size != 0) {
                Q_SizeRecord* rec = &userdata_block()->sizes_0x3BC0[i];
                if (rec->size_min > size || rec->size_min == 0) {
                    rec->size_min = size;
                }
            }
        }
    } else {
        for (i = 0; i < 41; i++, sizes++) {
            size = sizes->size_min;
            if (size != 0) {
                Q_SizeRecord* rec = &userdata_block()->sizes_0x3BC0[i];
                if (rec->size_max < size) {
                    rec->size_max = size;
                }
            }
        }
    }
}

/* 0x8004E568 (0x3C): adds `count` hunts of monster `monster`, clamped to 9999. */
extern "C" void userdata_hunt_count_add(u8 monster, s32 count)
{
    userdata_block()->set_a.count[monster] += count;
    if (userdata_block()->set_a.count[monster] > 9999) {
        userdata_block()->set_a.count[monster] = 9999;
    }
}

/* 0x8004E5A4 (0x3C): adds `count` captures of monster `monster`, clamped to 9999. */
extern "C" void userdata_capture_count_add(u8 monster, s32 count)
{
    userdata_block()->set_b.count[monster] += count;
    if (userdata_block()->set_b.count[monster] > 9999) {
        userdata_block()->set_b.count[monster] = 9999;
    }
}

/* 0x8004E5E0 (0x54): folds one measured size of monster `monster` into its smallest and largest records. */
extern "C" void userdata_size_record_set(u8 monster, u16 size)
{
    if (size == 0) {
        return;
    }
    if (userdata_block()->sizes_0x3BC0[monster].size_min > size || userdata_block()->sizes_0x3BC0[monster].size_min == 0) {
        userdata_block()->sizes_0x3BC0[monster].size_min = size;
    }
    if (userdata_block()->sizes_0x3BC0[monster].size_max < size) {
        userdata_block()->sizes_0x3BC0[monster].size_max = size;
    }
}

/* 0x8004E634 (0xCC): the crown class of size `size` of monster `monster`: 0xFF without a size table, 1 the gold
 * crown, 2 the silver crown, 3 the small crown, else 0. */
extern "C" u8 monster_size_crown_get(u8 monster, u16 size)
{
    const u16* table;
    u16 silver;
    u16 gold;
    u16 small;

    monster_size_table_get(monster, &table);
    silver = table[0];
    gold = table[1];
    small = table[2];
    if (silver == 0 && gold == 0 && small == 0) {
        return 0xFF;
    }
    if (size == 0) {
        return 0;
    }
    if (gold != 0 && size >= gold) {
        return 1;
    }
    if (silver != 0 && size >= silver) {
        return 2;
    }
    if (small != 0 && size <= small) {
        return 3;
    }
    return 0;
}

extern "C" void quest_stat_copy(Q_QuestStat* dst, const Q_QuestStat* src);

/* 0x8004E700 (0x10): stores a quest stat into the save block's own record. */
extern "C" void userdata_quest_stat_set(const Q_QuestStat* src)
{
    quest_stat_copy(&userdata_block()->quest_stat, src);
}

/* 0x8004E710 (0x34): copies one 16-byte quest stat. */
extern "C" void quest_stat_copy(Q_QuestStat* dst, const Q_QuestStat* src)
{
    dst->valid_0x00 = src->valid_0x00;
    dst->byte_0x01 = src->byte_0x01;
    dst->half_0x02 = src->half_0x02;
    dst->word_0x04 = src->word_0x04;
    dst->flags_0x08 = src->flags_0x08;
    dst->word_0x0C = src->word_0x0C;
}

/* --- gallery and unlock flags ----------------------------------------------------------------------- */

/* 0x8004E744 (0x18): forgets every announced unlock. */
extern "C" void unlock_seen_clear(void)
{
    system_w.unlock_seen_0x888[0] = 0;
    system_w.unlock_seen_0x888[1] = 0;
}

/* 0x8004E75C (0x48): drops gallery bit `bit`. */
void gallery_bit_off(long bit)
{
    system_w.unlock_seen_0x888[bit / 32] &= ~(1 << (bit % 32));
}

/* 0x8004E7A4 (0x80): closes gallery entry `id`. */
extern "C" void gallery_close(s32 id)
{
    int bit = -1;
    const GalleryUnlock* entry;
    for (entry = gallery_unlock_table; entry->id != 0; entry++) {
        if (entry->id == id) {
            bit = entry->bit;
            break;
        }
    }
    if (bit < 0) {
        return;
    }
    system_w.unlock_seen_0x888[bit / 32] &= ~(1 << (bit % 32));
}

/* 0x8004E824 (0x80): opens gallery entry `id`. */
void gallery_open(u8 id)
{
    int bit = -1;
    const GalleryUnlock* entry;
    for (entry = gallery_unlock_table; entry->id != 0; entry++) {
        if (entry->id == id) {
            bit = entry->bit;
            break;
        }
    }
    if (bit < 0) {
        return;
    }
    system_w.unlock_seen_0x888[bit / 32] |= 1 << (bit % 32);
}

/* 0x8004E8A4 (0x18): opens the default gallery entry (bit 30). */
void gallery_def_set(void)
{
    system_w.unlock_seen_0x888[0] |= 0x40000000;
}

/* 0x8004E8BC (0x2C): ORs the two unlock words into the announced set. */
extern "C" void unlock_seen_mark(const u32* bits)
{
    system_w.unlock_seen_0x888[0] |= bits[0];
    system_w.unlock_seen_0x888[1] |= bits[1];
}

/* 0x8004E8E8 (0xAC): raises the unlock bit event `event` maps to in the quest move work. */
extern "C" void unlock_bit_raise(u8 event)
{
    Q_MoveWork* move = (Q_MoveWork*)get_move_work_adrs(0);
    if (move == NULL) {
        return;
    }
    int bit = -1;
    const GalleryUnlock* entry;
    for (entry = gallery_unlock_table; entry->id != 0; entry++) {
        if (entry->id == event) {
            bit = entry->bit;
            break;
        }
    }
    if (bit < 0) {
        return;
    }
    move->unlock_bits_0x148[bit / 32] |= 1 << (bit % 32);
}

/* 0x8004E994 (0x90): whether gallery entry `id` is open. */
extern "C" u32 gallery_open_ck(u8 id)
{
    int bit = -1;
    const GalleryUnlock* entry;
    for (entry = gallery_unlock_table; entry->id != 0; entry++) {
        if (entry->id == id) {
            bit = entry->bit;
            break;
        }
    }
    if (bit < 0) {
        return 0;
    }
    return ((1 << (bit % 32)) & system_w.unlock_seen_0x888[bit / 32]) != 0;
}

/* --- the item menu's row records -------------------------------------------------------------------- */

extern "C" void place_rec_copy(Q_ItemRow* dst, const Q_ItemRow* src);

/* 0x8004EA24 (0x34): the item menu's eight row records, or null without a save block. */
extern "C" Q_ItemRow* userdata_item_rows_get(void)
{
    Q_UserData* user = get_userdata();
    if (user == NULL) {
        return NULL;
    }
    return user->item_rows_0x4078;
}

/* 0x8004EA58 (0x9C): files `row` in the first free row record, or shifts the rows up and files it last. */
extern "C" void userdata_item_row_push(const Q_ItemRow* row)
{
    int i;
    Q_ItemRow* rows = get_userdata()->item_rows_0x4078;
    Q_ItemRow* rec = rows;
    for (i = 0; i < 8; i++, rec++) {
        if (rec->kind_0x00 == 0) {
            place_rec_copy(rec, row);
            return;
        }
    }
    for (i = 0; i < 7; i++, rows++) {
        place_rec_copy(rows, rows + 1);
    }
    place_rec_copy(rows, row);
}

/* 0x8004EAF4 (0x24): copies one row record. */
extern "C" void place_rec_copy(Q_ItemRow* dst, const Q_ItemRow* src)
{
    *dst = *src;
}

/* 0x8004EB18 (0x12C): whether `item` is the item or equipment one of the item menu's row records names. */
extern "C" u32 item_se_ck(u16 item)
{
    int i;
    Q_ItemRow* row = get_userdata()->item_rows_0x4078;
    for (i = 0; i < 8; i++, row++) {
        switch (row->kind_0x00) {
        case 1:
        case 2:
            if (row->item_id_0x08 == item) {
                return 1;
            }
            break;
        case 3:
        case 4:
            if (row->equip_id_0x06 == item) {
                return 1;
            }
            break;
        case 6:
            if (row->id_0x0A == item) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

/* --- arena rows and the friend list ------------------------------------------------------------------ */

/* 0x8004EC44 (0x68): copies the first 0x1F bytes of each arena row out to `dst` (12 packed rows). */
extern "C" void userdata_arena_rows_export(u8* dst)
{
    int i;
    Q_ArenaRow* row = get_userdata()->arena_rows_0x4114;
    for (i = 0; i < 12; i++) {
        memcpy(dst, row->data_0x00, sizeof(row->data_0x00));
        dst += sizeof(row->data_0x00);
        row++;
    }
}

/* 0x8004ECAC (0x68): copies 12 packed 0x1F-byte rows from `src` into the arena rows. */
extern "C" void userdata_arena_rows_import(const u8* src)
{
    int i;
    Q_ArenaRow* row = get_userdata()->arena_rows_0x4114;
    for (i = 0; i < 12; i++) {
        memcpy(row->data_0x00, src, sizeof(row->data_0x00));
        src += sizeof(row->data_0x00);
        row++;
    }
}

/* 0x8004ED14 (0x38): arena row `index`. */
extern "C" Q_ArenaRow* userdata_arena_row_get(s32 index)
{
    return &get_userdata()->arena_rows_0x4114[index];
}

extern "C" void lobby_friend_row_copy(LobbyTimerRow* dst, const LobbyTimerRow* src);
extern "C" void userdata_friend_row_copy(Q_FriendRow* dst, const Q_FriendRow* src);

/* 0x8004ED4C (0x250): brings the saved friend list in step with the lobby's `list`: drops the saved friends the
 * list no longer has, saves the new ones (up to 50), then orders the list like the saved one. */
extern "C" void lobby_friend_list_sort(LobbyFriendList* list)
{
    Q_FriendBook* book = &get_userdata()->friends_0x53AC;
    s32 i;
    s32 j;
    s32 k;
    s32 found;
    s32 index;
    Q_FriendRow* row;

    for (i = 0; i < book->count_0x000;) {
        found = 0;
        for (j = 0; j < list->count_0x000 && found == 0; j++) {
            if (list->rows_0x004[j].id_0x14 != NULL &&
                strcmp(list->rows_0x004[j].id_0x14, book->rows_0x004[i].id_0x14.text_0x00) == 0) {
                found = 1;
            }
        }
        if (found == 0 && book->count_0x000 > 0) {
            book->rows_0x004[i].name_0x00.text_0x00[0] = 0;
            book->rows_0x004[i].id_0x14.text_0x00[0] = 0;
            for (k = i; k + 1 < book->count_0x000; k++) {
                userdata_friend_row_copy(&book->rows_0x004[k], &book->rows_0x004[k + 1]);
            }
            book->count_0x000--;
        } else {
            i++;
        }
    }
    for (j = 0; j < list->count_0x000; j++) {
        index = -1;
        for (k = 0; k < book->count_0x000; k++) {
            if (strcmp(list->rows_0x004[j].id_0x14, book->rows_0x004[k].id_0x14.text_0x00) == 0) {
                index = k;
            }
        }
        if (index == -1 && book->count_0x000 < 50) {
            book->count_0x000++;
            row = &book->rows_0x004[book->count_0x000 - 1];
            strcpy(row->name_0x00.text_0x00, list->rows_0x004[j].name_0x00);
            strcpy(row->id_0x14.text_0x00, list->rows_0x004[j].id_0x14);
        }
    }
    for (i = 0; i < book->count_0x000; i++) {
        for (j = 0; j < list->count_0x000; j++) {
            if (strcmp(list->rows_0x004[j].id_0x14, book->rows_0x004[i].id_0x14.text_0x00) == 0 && j != i) {
                LobbyTimerRow tmp;
                lobby_friend_row_copy(&tmp, &list->rows_0x004[i]);
                lobby_friend_row_copy(&list->rows_0x004[i], &list->rows_0x004[j]);
                lobby_friend_row_copy(&list->rows_0x004[j], &tmp);
            }
        }
    }
}

/* 0x8004EF9C (0x5C): copies one lobby friend row (the name, the id text with its state byte, the flag and the
 * timer). */
extern "C" void lobby_friend_row_copy(LobbyTimerRow* dst, const LobbyTimerRow* src)
{
    *(LobbyNameText*)dst->name_0x00 = *(const LobbyNameText*)src->name_0x00;
    *(LobbyMailIdText*)dst->id_0x14 = *(const LobbyMailIdText*)src->id_0x14;
    dst->flag_0x1F = src->flag_0x1F;
    dst->timer_0x20 = src->timer_0x20;
}

/* 0x8004EFF8 (0x7C): copies one saved friend row. */
extern "C" void userdata_friend_row_copy(Q_FriendRow* dst, const Q_FriendRow* src)
{
    dst->name_0x00 = src->name_0x00;
    dst->id_0x14 = src->id_0x14;
    dst->profile_0x1F = src->profile_0x1F;
}

/* --- the VS user blocks and the Wii remote memory -------------------------------------------------- */

/* 0x8004F074 (0x18): VS user block `index` (unchecked). */
extern "C" _vs_user_data* vs_user_slot_get(u8 index)
{
    return (_vs_user_data*)(vs_user_work + (index << 8));
}

/* 0x8004F08C (0x270): fills VS user block `index` from the save block (moving the VS credit and the VS items into it);
 * `new_seed` stamps a fresh seed into both, else the block takes the save's seed. */
void set_mydata2vs(u8 index, u8 new_seed)
{
    _vs_user_data* vs;
    u32 sum = 0;
    Q_UserData* user = get_userdata();
    int i;
    u8* byte;

    vs_user_work_clear(index);
    vs = (_vs_user_data*)(vs_user_work + (index << 8));
    vs->hunter_sex_0x00 = user->hunter_sex_0x0000;
    vs->hunter_face_0x01 = user->hunter_face_0x0001;
    vs->hunter_hair_0x02 = user->hunter_hair_0x0002;
    memcpy(&vs->name_0x03, user->hunter_name_0x0003, sizeof(vs->name_0x03));
    vs->hair_color_0x14 = user->hair_color_0x0014;
    vs->point_0x18 = user->vs_point_0x53A4;
    user->vs_point_0x53A4 = 0;
    vs->reserved_0x1C = 0;
    vs->hunter_equip_0x20 = user->hunter_equip_0x009E;
    vs->hunter_voice_0x21 = user->hunter_voice_0x009F;
    vs->face_type_0x22 = user->face_type_0x39A0;
    vs->face_eye_0x23 = user->face_eye_0x39A1;
    vs->appearance_0x24 = user->appearance_0x39BA;
    vs->makeup_0x25 = user->makeup_0x3DE6;
    vs->makeup_color_0x26 = user->makeup_color_0x3DE7;
    vs->inner_color_0x28 = user->inner_color_0x39BC;
    memcpy(vs->slot_a_0x6C, user->vs_slot_a_0x52E4, sizeof(vs->slot_a_0x6C));
    memcpy(vs->slot_b_0x94, user->vs_slot_b_0x530C, sizeof(vs->slot_b_0x94));
    vs->ready_mask_0xBC = user->event_bits_0x52E2;
    memcpy(vs->save_name_0xBE, system_w.save_name_0x7ea[0], sizeof(vs->save_name_0xBE));
    vs->player_0xCF = system_w.vs_load_player_0x7e8;
    vs->save_slot_0x27 = system_w.vs_load_slot_0x7e9;
    if (index == 0) {
        memcpy(vs->option_0xD0, option_w, sizeof(vs->option_0xD0));
    } else {
        data_file_options_get(vs->option_0xD0);
    }
    if (new_seed != 0) {
        u8 tag = vs->save_slot_0x27 + vs->player_0xCF * 3;
        u8 count = (user->seed_0x53A8 & 0xFF0000) >> 16;
        count++;
        if (count == 0) {
            count = 1;
        }
        vs->seed_0xF0 = (u16)OSGetTick() | ((tag << 24) | (count << 16));
        user->seed_0x53A8 = vs->seed_0xF0;
    } else {
        vs->seed_0xF0 = user->seed_0x53A8;
    }
    memcpy(vs->item_0x2C, user->vs_items_0x5364, sizeof(vs->item_0x2C));
    memset(user->vs_items_0x5364, 0, sizeof(user->vs_items_0x5364));
    byte = (u8*)vs;
    for (i = 0; i < 0xFC; i++) {
        sum += *byte++;
    }
    vs->checksum_0xFC = sum;
}

/* 0x8004F2FC (0xB8): copies VS user block `index`'s hunter name into the player work. */
extern "C" void vs_user_name_copy(_PLW* plw, u8 index)
{
    _vs_user_data* vs = vs_user_slot_get(index);
    int i;
    for (i = 0; i < 0x11; i++) {
        plw->hunter_0x5CA.name_0x5CA[i] = vs->name_0x03.text_0x00[i];
    }
}

/* 0x8004F3B4 (0x94): stores the byte sum of VS user block `index`'s first 0xFC bytes in its checksum. */
extern "C" void vs_user_checksum_set(u8 index)
{
    u32 sum = 0;
    _vs_user_data* vs = (_vs_user_data*)(vs_user_work + (index << 8));
    u8* byte = (u8*)vs;
    int i;
    for (i = 0; i < 0xFC; i++) {
        sum += *byte++;
    }
    vs->checksum_0xFC = sum;
}

/* 0x8004F448 (0xC0): whether VS user block `vs` was filled from the save block `user` (same look, name, eyes and
 * a matching non-zero seed). */
extern "C" u32 userdata_vs_match_ck(Q_UserData* user, _vs_user_data* vs)
{
    if (user->hunter_sex_0x0000 != vs->hunter_sex_0x00) {
        return 0;
    }
    if (user->hunter_face_0x0001 != vs->hunter_face_0x01) {
        return 0;
    }
    if (memcmp(user->hunter_name_0x0003, &vs->name_0x03, 16) != 0) {
        return 0;
    }
    if (user->face_eye_0x39A1 != vs->face_eye_0x23) {
        return 0;
    }
    if (user->seed_0x53A8 == 0 || user->seed_0x53A8 != vs->seed_0xF0) {
        return 0;
    }
    return 1;
}

/* 0x8004F508 (0xB0): whether the two VS user blocks are the same hunter's (same look, name, eyes and seed). */
s32 ck_mydata_vs(_vs_user_data* mine, _vs_user_data* other)
{
    if (mine->hunter_sex_0x00 != other->hunter_sex_0x00) {
        return 0;
    }
    if (mine->hunter_face_0x01 != other->hunter_face_0x01) {
        return 0;
    }
    if (memcmp(&mine->name_0x03, &other->name_0x03, 16) != 0) {
        return 0;
    }
    if (mine->face_eye_0x23 != other->face_eye_0x23) {
        return 0;
    }
    return mine->seed_0xF0 == other->seed_0xF0;
}

/* 0x8004F5B8 (0xC): reads `size` bytes of remote `chan`'s game data at `addr`. */
extern "C" s32 wpad_game_data_read(s32 chan, u8* dst, u16 size, u16 addr, WPADCallback callback)
{
    return WPADReadGameData(chan, dst, size, addr, callback);
}

/* 0x8004F5C4 (0xC): writes `size` bytes of game data at `addr` to remote `chan`. */
extern "C" s32 wpad_game_data_write(s32 chan, const u8* src, u16 size, u16 addr, WPADCallback callback)
{
    return WPADWriteGameData(chan, src, size, addr, callback);
}

/* 0x8004F5D0 (0x4): sets the game title the remotes' game data is stamped with. */
extern "C" void wpad_game_title_set(const u16* title)
{
    wpadCopyTitleString(title);
}

/* 0x8004F5D4 (0x4): the game title remote `chan`'s game data carries. */
extern "C" s32 wpad_game_title_get(s32 chan, u16** title)
{
    return wpadGetDpdCalibration(chan, (u8**)title);
}

/* 0x8004F5D8 (0x30): starts a remote-memory access for player `player` on remote `chan`. */
void wpad_memory_access_init(u8 player, u8 chan)
{
    wpad_mem_step = 0;
    wpad_mem_player = player;
    wpad_mem_chan = chan;
    wpad_mem_result = 0;
    wpad_mem_verify_retry = 0;
    wpad_mem_read_retry = 0;
    memset(wpad_mem_buf, 0, sizeof(wpad_mem_buf));
}

/* 0x8004F608 (0x18): the game-data transfer callback: records the result and the remote. */
extern "C" void wpad_memory_callback(s32 chan, s32 result)
{
    wpad_mem_done = 1;
    wpad_mem_result = result;
    wpad_mem_cb_chan = chan;
}

/* 0x8004F620 (0x434): steps the write of player `player`'s VS user block to the remote: 0 busy, 1 done, 100/101 the
 * overwrite question is up (another game's data / this game's), -100 declined, else the failing result. */
/* free: retail C++ global function (map mangling write_wpad_memory__FUcUc), not a member */
s32 write_wpad_memory(u8 player, u8 mode)
{
    u16* title = NULL;
    u16 pressed;
    _vs_user_data* vs;
    s32 ret;
    int i;

    if (lobby_net_busy_ck() == 1) {
        pressed = 0;
    } else {
        pressed = Psw[player].button_0x2C0.pressed_0x04;
    }
    vs = vs_user_slot_get(player);
    switch (wpad_mem_step) {
    case 0:
        if (wpad_game_title_get(wpad_mem_chan, &title) == 0) {
            for (i = 0; i < 16; i++) {
                if (title[i] != wpad_game_title[i]) {
                    break;
                }
            }
            if (i >= 16) {
                if (mode == 1) {
                    wpad_mem_step = 2;
                    break;
                }
                wpad_mem_result = 100;
            } else {
                wpad_mem_result = 101;
            }
            wpad_mem_sel = 1;
            wpad_mem_step = 1;
            return wpad_mem_result;
        }
        wpad_mem_step = 2;
        break;
    case 1:
        if ((pressed & 8) && wpad_mem_sel == 0) {
            wpad_mem_sel = 1;
            sysSE_req(3);
            return wpad_mem_result;
        }
        if ((pressed & 4) && wpad_mem_sel == 1) {
            wpad_mem_sel = 0;
            sysSE_req(3);
            return wpad_mem_result;
        }
        if (pressed & 0x10) {
            sysSE_req(0);
            if (wpad_mem_sel == 1) {
                return -100;
            }
            wpad_mem_result = 0;
            wpad_mem_step = 2;
            break;
        }
        return wpad_mem_result;
    case 2:
        wpad_mem_done = 0;
        wpad_game_title_set(wpad_game_title);
        ret = wpad_game_data_write(wpad_mem_chan, (u8*)vs, 0x100, 0, wpad_memory_callback);
        switch (ret) {
        case -1:
        case -2:
            wpad_mem_result = ret;
            return ret;
        case 0:
            wpad_mem_step = 3;
            break;
        }
        break;
    case 3:
        if (wpad_mem_done == 0) {
            break;
        }
        switch (wpad_mem_result) {
        case -1:
        case -2:
        case -3:
            if (++wpad_mem_verify_retry > 3) {
                return wpad_mem_result;
            }
            wpad_mem_step = 0;
            break;
        case 0:
            wpad_mem_done = 0;
            ret = wpad_game_data_read(wpad_mem_chan, wpad_mem_buf, 0x100, 0, wpad_memory_callback);
            switch (ret) {
            case -1:
            case -2:
            case -5:
            case -6:
                if (++wpad_mem_read_retry > 3) {
                    return wpad_mem_result;
                }
                break;
            case 0:
                wpad_mem_step = 4;
                break;
            }
            break;
        }
        break;
    case 4:
        if (wpad_mem_done == 0) {
            break;
        }
        switch (wpad_mem_result) {
        case -1:
        case -2:
        case -3:
            wpad_mem_done = 0;
            ret = wpad_game_data_read(wpad_mem_chan, wpad_mem_buf, 0x100, 0, wpad_memory_callback);
            switch (ret) {
            case -1:
            case -2:
            case -5:
            case -6:
                if (++wpad_mem_read_retry > 3) {
                    return wpad_mem_result;
                }
                break;
            case 0:
                wpad_mem_step = 4;
                break;
            }
            break;
        case -5:
        case -6:
            if (++wpad_mem_verify_retry > 3) {
                return wpad_mem_result;
            }
            wpad_mem_step = 0;
            break;
        case 0:
            for (i = 0; i < 0x100; i++) {
                if (*(u8*)vs != *wpad_mem_buf) {
                    break;
                }
            }
            if (i < 0x100) {
                if (++wpad_mem_verify_retry > 3) {
                    return wpad_mem_result;
                }
                wpad_mem_step = 0;
                break;
            }
            return 1;
        }
        break;
    }
    return 0;
}

/* 0x8004FA54 (0x22C): steps the read of a VS user block from the remote into `dst`: 0 busy, 1 done, -100 when the
 * block is empty, damaged or (`mode` 1) not this save's, else the failing result. */
/* free: retail C++ global function (map mangling read_wpad_memory__FP13_vs_user_dataUc), not a member */
s32 read_wpad_memory(_vs_user_data* dst, u8 mode)
{
    Q_UserData* user = get_userdata();
    s32 ret;
    u32 sum;
    int i;
    _vs_user_data* vs;

    switch (wpad_mem_step) {
    case 0:
        for (;;) {
            wpad_game_title_set(wpad_game_title);
            wpad_mem_done = 0;
            ret = wpad_game_data_read(wpad_mem_chan, wpad_mem_buf, 0x100, 0, wpad_memory_callback);
            switch (ret) {
            case -1:
            case -2:
            case -5:
            case -6:
                if (++wpad_mem_read_retry > 3) {
                    return wpad_mem_result;
                }
                continue;
            case 0:
                wpad_mem_step = 1;
                break;
            }
            break;
        }
        break;
    case 1:
        if (wpad_mem_done == 0) {
            break;
        }
        switch (wpad_mem_result) {
        case 0:
            sum = 0;
            for (i = 0; i < 0xFC; i++) {
                sum += wpad_mem_buf[i];
            }
            vs = (_vs_user_data*)wpad_mem_buf;
            if (sum == 0) {
                return -100;
            }
            if (vs->checksum_0xFC != sum) {
                return -100;
            }
            if (mode == 1) {
                if (system_w.vs_load_player_0x7e8 != vs->player_0xCF) {
                    return -100;
                }
                if (system_w.vs_load_slot_0x7e9 != vs->save_slot_0x27) {
                    return -100;
                }
                if (userdata_vs_match_ck(user, vs) == 0) {
                    return -100;
                }
            }
            memcpy(dst, wpad_mem_buf, sizeof(wpad_mem_buf));
            return 1;
        }
        break;
    }
    return 0;
}

/* The item-box page record `exportItemBoxPage` packs for the network: one page of 50 equipment records, the item
 * bits, the profile word, the hunter points and rank, and two 'U' marks.  size: 0x2DC */
struct ItemBoxPageRecord {
    /* +0x000 */ _EQUIP equip_0x000[50];
    /* +0x258 */ u32 item_seen_0x258[30];
    /* +0x2D0 */ u32 profile_0x2D0;
    /* +0x2D4 */ u32 hunter_points_0x2D4;
    /* +0x2D8 */ u16 hunter_rank_0x2D8;
    /* +0x2DA */ u8 mark_0x2DA[2];
};

/* The equipment record `exportEquipRecord` packs for the network: the worn records, the item bits, the profile word,
 * the hunter points and rank, and two 'U' marks.  size: 0xF0 */
struct EquipExportRecord {
    /* +0x00 */ _EQUIP weapon_0x00;
    /* +0x0C */ _EQUIP parts_0x0C[2];
    /* +0x24 */ _EQUIP armour_0x24[6];
    /* +0x6C */ u32 item_seen_0x6C[30];
    /* +0xE4 */ u32 profile_0xE4;
    /* +0xE8 */ u32 hunter_points_0xE8;
    /* +0xEC */ u16 hunter_rank_0xEC;
    /* +0xEE */ u8 mark_0xEE[2];
};

/* 0x8004FC80 (0xEC): packs equipment-box page `page` of the cached save block into `rec`. */
extern "C" void exportItemBoxPage(ItemBoxPageRecord* rec, s32 page)
{
    Q_UserData* user = userdata_block();
    int i;
    _EQUIP* dst;
    _EQUIP* src;

    rec->profile_0x2D0 = user->profile_0x001C;
    rec->hunter_points_0x2D4 = user->hunter_points_0x3DE0;
    rec->hunter_rank_0x2D8 = user->hunter_rank_0x3DE4;
    for (i = 0; i < 30; i++) {
        rec->item_seen_0x258[i] = user->item_seen_0x4AD8[i];
    }
    src = &user->equip_box_0x0E00[page * 50];
    dst = rec->equip_0x000;
    for (i = 0; i < 50; i++) {
        equip_record_copy(dst, src);
        dst++;
        src++;
    }
    rec->mark_0x2DA[0] = 'U';
    rec->mark_0x2DA[1] = 'U';
}

/* 0x8004FD6C (0x11C): packs the worn equipment of the cached save block into `rec`. */
extern "C" void exportEquipRecord(EquipExportRecord* rec)
{
    Q_UserData* user = userdata_block();
    int i;

    equip_record_copy(&rec->weapon_0x00, &user->weapon_0x0020);
    for (i = 0; i < 2; i++) {
        equip_record_copy(&rec->parts_0x0C[i], &user->parts_0x002C[i]);
    }
    equip_record_copy(&rec->armour_0x24[0], &user->armour_0x0044[0]);
    equip_record_copy(&rec->armour_0x24[1], &user->armour_0x0044[1]);
    equip_record_copy(&rec->armour_0x24[2], &user->armour_0x0044[2]);
    equip_record_copy(&rec->armour_0x24[3], &user->armour_0x0044[3]);
    equip_record_copy(&rec->armour_0x24[4], &user->armour_0x0044[4]);
    equip_record_copy(&rec->armour_0x24[5], &user->armour_0x0044[5]);
    rec->profile_0xE4 = user->profile_0x001C;
    rec->hunter_points_0xE8 = user->hunter_points_0x3DE0;
    rec->hunter_rank_0xEC = user->hunter_rank_0x3DE4;
    for (i = 0; i < 30; i++) {
        rec->item_seen_0x6C[i] = user->item_seen_0x4AD8[i];
    }
    rec->mark_0xEE[0] = 'U';
    rec->mark_0xEE[1] = 'U';
}

extern "C" char* findSubstring(const char* haystack, const char* needle);

/* 0x8004FE88 (0x11C): builds Wii system message `id` in `out`, each "%b" replaced by the save slot count; returns
 * the message's display length (0 for a bad id or no buffer). */
s32 wii_sysmsg_gen(long id, char* out, long unused)
{
    char number[16];
    char text[512];
    char message[512];
    char* part;
    char* tag;
    s32 len;

    if (id >= 24) {
        return 0;
    }
    if (out == NULL) {
        return 0;
    }
    strcpy(text, wii_sysmsg_table[system_w.field_0x09][id]);
    message[0] = 0;
    part = text;
    for (;;) {
        tag = findSubstring(part, "%b");
        if (tag == NULL) {
            break;
        }
        *tag = 0;
        wii_sysmsg_slot_count = save_data_slot_max();
        sprintf(number, "%d", wii_sysmsg_slot_count);
        strcat(message, part);
        strcat(message, number);
        part = tag + 2;
    }
    strcat(message, part);
    len = flfntStrLen(message);
    strcpy(out, message);
    return len;
}

/* --- vector and matrix helpers ------------------------------------------------------------------- */

/* 0x8004FFA4 (0x4): the first occurrence of `needle` in `haystack`, or null. */
extern "C" char* findSubstring(const char* haystack, const char* needle)
{
    return strstr(haystack, needle);
}

/* 0x8004FFA8 (0x10): sets the three components. */
void setVector3(VEC3* v, f32 x, f32 y, f32 z)
{
    v->x = x;
    v->y = y;
    v->z = z;
}

/* 0x8004FFB8 (0x10): copies the SDK vector `src` into `dst`. */
void vec_to_mh_vec3(VEC3* dst, Vec* src)
{
    setVector3(dst, src->x, src->y, src->z);
}

/* 0x8004FFC8 (0x60): builds the capsule from its two end points and its radius. */
extern "C" void capsule_box_from_points(VEC3* start, VEC3* end, nw4r::math::CAPSULE* out, f32 radius)
{
    copyVec3(&out->start, start);
    copyVec3(&out->end, end);
    out->radius = radius;
}

/* 0x80050028 (0x1C): copies the three components. */
extern "C" void vec3_copy(VEC3* dst, const VEC3* src)
{
    dst->x = src->x;
    dst->y = src->y;
    dst->z = src->z;
}

/* 0x80050044 (0x88): converts the three 16-bit angles of `ang` to radians. */
extern "C" void cp_vector_to_rad(const _CP_VECTOR* ang, VEC3* out)
{
    out->x = (6.2831855f * (u16)ang->x) / 65536.0f;
    out->y = (6.2831855f * (u16)ang->y) / 65536.0f;
    out->z = (6.2831855f * (u16)ang->z) / 65536.0f;
}

extern "C" f32 sin_idx_get(u16 angle);
extern "C" f32 anim_tick_cos(u16 angle);

/* 0x800500CC (0xE8): rotates `mtx` about X by the 16-bit angle `angle`. */
void rotMatrixX(u32 angle, MTX34* mtx)
{
    f32 y = mtx->m[1][0];
    f32 s = sin_idx_get(angle);
    f32 c = anim_tick_cos(angle);

    mtx->m[1][0] = c * y - s * mtx->m[2][0];
    mtx->m[2][0] = s * y + c * mtx->m[2][0];
    y = mtx->m[1][1];
    mtx->m[1][1] = c * y - s * mtx->m[2][1];
    mtx->m[2][1] = s * y + c * mtx->m[2][1];
    y = mtx->m[1][2];
    mtx->m[1][2] = c * y - s * mtx->m[2][2];
    mtx->m[2][2] = s * y + c * mtx->m[2][2];
}

extern "C" f32 anim_tick_angle(u16 angle);

/* 0x800501B4 (0x30): the cosine of the 16-bit angle `angle`. */
extern "C" f32 anim_tick_cos(u16 angle)
{
    return nw4r::math::CosFIdx(anim_tick_angle(angle) * (1.0f / 256.0f));
}

/* 0x800501E4 (0x30): the 16-bit angle `angle` as a float, through the paired-single fast cast (the fast cast and
 * its load are not inlined: they follow as their own local functions). */
#pragma dont_inline on
extern "C" f32 anim_tick_angle(u16 angle)
{
    f32 value;
    OSu16tof32(&angle, &value);
    return value;
}

/* 0x8005024C (0x30): the sine of the 16-bit angle `angle`. */
extern "C" f32 sin_idx_get(u16 angle)
{
    return nw4r::math::SinFIdx(anim_tick_angle(angle) * (1.0f / 256.0f));
}
#pragma dont_inline reset

/* 0x8005027C (0xE8): rotates `mtx` about Y by the 16-bit angle `angle`. */
void rotMatrixY(u32 angle, MTX34* mtx)
{
    f32 x = mtx->m[0][0];
    f32 s = sin_idx_get(angle);
    f32 c = anim_tick_cos(angle);

    mtx->m[0][0] = s * mtx->m[2][0] + c * x;
    mtx->m[2][0] = c * mtx->m[2][0] - s * x;
    x = mtx->m[0][1];
    mtx->m[0][1] = s * mtx->m[2][1] + c * x;
    mtx->m[2][1] = c * mtx->m[2][1] - s * x;
    x = mtx->m[0][2];
    mtx->m[0][2] = s * mtx->m[2][2] + c * x;
    mtx->m[2][2] = c * mtx->m[2][2] - s * x;
}

/* 0x80050364 (0xE8): rotates `mtx` about Z by the 16-bit angle `angle`. */
void rotMatrixZ(u32 angle, MTX34* mtx)
{
    f32 x = mtx->m[0][0];
    f32 s = sin_idx_get(angle);
    f32 c = anim_tick_cos(angle);

    mtx->m[0][0] = c * x - s * mtx->m[1][0];
    mtx->m[1][0] = s * x + c * mtx->m[1][0];
    x = mtx->m[0][1];
    mtx->m[0][1] = c * x - s * mtx->m[1][1];
    mtx->m[1][1] = s * x + c * mtx->m[1][1];
    x = mtx->m[0][2];
    mtx->m[0][2] = c * x - s * mtx->m[1][2];
    mtx->m[1][2] = s * x + c * mtx->m[1][2];
}

extern "C" MTX34* mtx34_identity(MTX34* mtx);
/* free: retail C linkage evidenced (unmangled map name) */
extern "C" void MTX34_ctor(MTX34* mtx);
extern "C" void mtx34_concat_assign(MTX34* dst, const MTX34* src);

/* 0x8005044C (0x88): rotates `mtx` about its local X axis by the 16-bit angle `angle`. */
void rotLocalMatX(u32 angle, MTX34* mtx)
{
    MTX34 rot;
    MTX34 unused;
    MTX34_ctor(&rot);
    MTX34_ctor(&unused);
    mtx34_identity(&rot);
    f32 s = sin_idx_get(angle);
    f32 c = anim_tick_cos(angle);
    rot.m[1][1] = c;
    rot.m[2][1] = s;
    rot.m[1][2] = -s;
    rot.m[2][2] = c;
    mtx34_concat_assign(mtx, &rot);
}

extern "C" f32 (*mtx34_get_ptr(MTX34* mtx))[4];

/* 0x800504D4 (0x34): sets `mtx` to the identity and returns it. */
extern "C" MTX34* mtx34_identity(MTX34* mtx)
{
    PSMTXIdentity(mtx34_get_ptr(mtx));
    return mtx;
}

/* 0x80050508 (0x4): the matrix as the SDK's `Mtx` rows. */
extern "C" f32 (*mtx34_get_ptr(MTX34* mtx))[4]
{
    return mtx->m;
}

/* 0x8005050C (0x4): the matrix record's (empty) constructor. */
/* free: retail C linkage evidenced (unmangled map name; ef/g3d/Pl units call it as a C function) */
extern "C" void MTX34_ctor(MTX34* mtx)
{
}

/* 0x80050510 (0x88): rotates `mtx` about its local Y axis by the 16-bit angle `angle`. */
void rotLocalMatY(u32 angle, MTX34* mtx)
{
    MTX34 rot;
    MTX34 unused;
    MTX34_ctor(&rot);
    MTX34_ctor(&unused);
    mtx34_identity(&rot);
    f32 s = sin_idx_get(angle);
    f32 c = anim_tick_cos(angle);
    rot.m[0][0] = c;
    rot.m[2][0] = -s;
    rot.m[0][2] = s;
    rot.m[2][2] = c;
    mtx34_concat_assign(mtx, &rot);
}

/* 0x80050598 (0x88): rotates `mtx` about its local Z axis by the 16-bit angle `angle`. */
void rotLocalMatZ(u32 angle, MTX34* mtx)
{
    MTX34 rot;
    MTX34 unused;
    MTX34_ctor(&rot);
    MTX34_ctor(&unused);
    mtx34_identity(&rot);
    f32 s = sin_idx_get(angle);
    f32 c = anim_tick_cos(angle);
    rot.m[0][0] = c;
    rot.m[1][0] = s;
    rot.m[0][1] = -s;
    rot.m[1][1] = c;
    mtx34_concat_assign(mtx, &rot);
}

/* 0x80050620 (0x54): rotates `mtx` about X, then Y, then Z by the three angles of `ang`. */
extern "C" void rotMatrixXYZ(_CP_VECTOR* ang, MTX34* mtx)
{
    rotMatrixX(ang->x, mtx);
    rotMatrixY(ang->y, mtx);
    rotMatrixZ(ang->z, mtx);
}

/* 0x80050674 (0x94): scales the three rows of `mtx`'s rotation by the components of `scale`. */
void scaleMat34W(MTX34* mtx, VEC3* scale)
{
    mtx->m[0][0] *= scale->x;
    mtx->m[0][1] *= scale->x;
    mtx->m[0][2] *= scale->x;
    mtx->m[1][0] *= scale->y;
    mtx->m[1][1] *= scale->y;
    mtx->m[1][2] *= scale->y;
    mtx->m[2][0] *= scale->z;
    mtx->m[2][1] *= scale->z;
    mtx->m[2][2] *= scale->z;
}

/* 0x80050708 (0xC): scales `mtx` in place by `scale` (`MTX34Scale` with the matrix as both operands). */
extern "C" MTX34* mtx34_scale_in_place(MTX34* mtx, const VEC3* scale)
{
    return nw4r::math::MTX34Scale(mtx, mtx, scale);
}

/* 0x80050714 (0x4): the single-precision square root. */
extern "C" f32 sqrtf_thunk(f32 x)
{
    return sqrtf(x);
}

extern "C" VEC3* vec3_normalize_into(VEC3* dst, const VEC3* src);

/* 0x80050718 (0x138): normalises the three columns of `mtx`'s rotation and scales them by `scale`. */
void setScaleMat34W(MTX34* mtx, VEC3* scale)
{
    VEC3 axis;
    VEC3_ctor(&axis);
    axis.x = mtx->m[0][0];
    axis.y = mtx->m[1][0];
    axis.z = mtx->m[2][0];
    vec3_normalize_into(&axis, &axis);
    mtx->m[0][0] = axis.x * scale->x;
    mtx->m[1][0] = axis.y * scale->y;
    mtx->m[2][0] = axis.z * scale->z;
    axis.x = mtx->m[0][1];
    axis.y = mtx->m[1][1];
    axis.z = mtx->m[2][1];
    vec3_normalize_into(&axis, &axis);
    mtx->m[0][1] = axis.x * scale->x;
    mtx->m[1][1] = axis.y * scale->y;
    mtx->m[2][1] = axis.z * scale->z;
    axis.x = mtx->m[0][2];
    axis.y = mtx->m[1][2];
    axis.z = mtx->m[2][2];
    vec3_normalize_into(&axis, &axis);
    mtx->m[0][2] = axis.x * scale->x;
    mtx->m[1][2] = axis.y * scale->y;
    mtx->m[2][2] = axis.z * scale->z;
}

extern "C" Vec* vec3_to_vec(VEC3* v);
extern "C" const Vec* vec3_to_const_vec(const VEC3* v);

/* 0x80050850 (0x58): normalises `src` into `dst` and returns `dst`. */
extern "C" VEC3* vec3_normalize_into(VEC3* dst, const VEC3* src)
{
    PSVECNormalize(vec3_to_const_vec(src), vec3_to_vec(dst));
    return dst;
}

/* 0x800508A8 (0x4): the vector as the SDK's `Vec`. */
extern "C" Vec* vec3_to_vec(VEC3* v)
{
    return (Vec*)v;
}

/* 0x800508AC (0x4): the vector as the SDK's read-only `Vec`. */
extern "C" const Vec* vec3_to_const_vec(const VEC3* v)
{
    return (const Vec*)v;
}

extern "C" MTX34* mtx34_rot_xyz_rad(MTX34* mtx, f32 x, f32 y, f32 z);

/* 0x800508B0 (0x64): sets `mtx` to the rotation by the three 16-bit angles of `ang` (X, then Y, then Z). */
void cpSetRotMatrix(_CP_VECTOR* ang, MTX34* mtx)
{
    VEC3 rad;
    VEC3_ctor(&rad);
    cp_vector_to_rad(ang, &rad);
    mtx34_identity(mtx);
    mtx34_rot_xyz_rad(mtx, rad.x, rad.y, rad.z);
}

/* 0x80050914 (0x14): sets `mtx` to the rotation by the three angles in radians. */
extern "C" MTX34* mtx34_rot_xyz_rad(MTX34* mtx, f32 x, f32 y, f32 z)
{
    return nw4r::math::MTX34RotXYZFIdx(mtx, x * 40.743664f, y * 40.743664f,
                                       z * 40.743664f);
}

/* 0x80050928 (0x68): sets `mtx` to the rotation about Y, then X, then Z by the three angles of `ang`. */
extern "C" void cpSetRotMatrixYXZ(_CP_VECTOR* ang, MTX34* mtx)
{
    VEC3 unused;
    VEC3_ctor(&unused);
    mtx34_identity(mtx);
    rotMatrixY(ang->y, mtx);
    rotMatrixX(ang->x, mtx);
    rotMatrixZ(ang->z, mtx);
}

/* 0x80050990 (0x68): sets `mtx` to the rotation about Z, then X, then Y by the three angles of `ang`. */
void cpSetRotMatrixZXY(_CP_VECTOR* ang, MTX34* mtx)
{
    VEC3 unused;
    VEC3_ctor(&unused);
    mtx34_identity(mtx);
    rotMatrixZ(ang->z, mtx);
    rotMatrixX(ang->x, mtx);
    rotMatrixY(ang->y, mtx);
}

/* --- angles and distances --------------------------------------------------------------------------- */

/* 0x800509F8 (0x48): the 16-bit angle of the direction (`x`, `y`) in the `atan2f` sense. */
u16 atan2ang(f32 y, f32 x)
{
    return (s32)(0.5f + atan2f(y, x) * 65536.0f / 6.2831855f);
}

/* 0x80050A40 (0x50): the 16-bit yaw from (`x0`, `z0`) toward (`x1`, `z1`). */
extern "C" u16 calc_xz_angle(f32 x0, f32 z0, f32 x1, f32 z1)
{
    return (s32)(0.5f + atan2f(x1 - x0, z1 - z0) * 65536.0f / 6.2831855f);
}

/* 0x80050A90 (0x14): the 16-bit yaw from `from` toward `to`. */
u16 calcVecAng2(VEC3* from, VEC3* to)
{
    return calc_xz_angle(from->x, from->z, to->x, to->z);
}

/* 0x80050AA4 (0x50): the 16-bit yaw of the vector. */
u16 calcVecAng3(VEC3* v)
{
    return (s32)(0.5f + atan2f(v->x, v->z) * 65536.0f / 6.2831855f);
}

/* 0x80050AF4 (0x54): the 16-bit angle of the vector in the XY plane. */
extern "C" u16 vec3_angle_xy(const VEC3* v)
{
    return (s32)(0.5f + atan2f(-v->x, v->y) * 65536.0f / 6.2831855f);
}

extern "C" f32 sqrt_f32(f32 x);

/* 0x80050B48 (0x78): the 16-bit pitch of the vector. */
u16 calcVecAngX(VEC3* v)
{
    return (s32)(0.5f + atan2f(-v->y, sqrt_f32(v->x * v->x + v->z * v->z)) * 65536.0f / 6.2831855f);
}

/* 0x80050BC0 (0x80): the square root, warning on a negative input and answering 0 for one. */
extern "C" f32 sqrt_f32(f32 x)
{
    if (!(x >= 0.0f)) {
        nw4r::db::Warning("arithmetic.h", 627, "FSqrt: Input is out of the domain.");
    }
    return (x <= 0.0f) ? 0.0f : x * nw4r::math::FrSqrt(x);
}

extern "C" void subVec3(VEC3* out, const VEC3* a, const VEC3* b);

/* 0x80050C40 (0x60): the 16-bit pitch of the direction from `from` toward `to`. */
extern "C" u16 calcVecAngX2(VEC3* from, VEC3* to)
{
    VEC3 dir;
    VEC3 diff;
    VEC3_ctor(&dir);
    subVec3(&diff, to, from);
    copyVec3(&dir, &diff);
    return calcVecAngX(&dir);
}

extern "C" void PSVECSubtract(VEC3* out, const VEC3* a, const VEC3* b);

/* 0x80050CA0 (0x54): `out = a - b`. */
extern "C" void subVec3(VEC3* out, const VEC3* a, const VEC3* b)
{
    VEC3_ctor(out);
    PSVECSubtract(out, a, b);
}

/* 0x80050CF4 (0x24): `out = a - b` with paired-single arithmetic. */
extern "C" asm void PSVECSubtract(register VEC3* out, register const VEC3* a, register const VEC3* b)
{
    nofralloc
    psq_l f0, 0(a), 0, 0
    psq_l f1, 0(b), 0, 0
    ps_sub f0, f0, f1
    psq_st f0, 0(out), 0, 0
    psq_l f0, 8(a), 1, 0
    psq_l f1, 8(b), 1, 0
    ps_sub f0, f0, f1
    psq_st f0, 8(out), 1, 0
    blr
}

/* 0x80050D18 (0xCC): the 16-bit yaw and pitch of the vector. */
void calcVecAngXY(VEC3* v, u32* ang_x, u32* ang_y)
{
    *ang_y = (u16)(s32)(0.5f + atan2f(v->x, v->z) * 65536.0f / 6.2831855f);
    *ang_x = (u16)(s32)(0.5f + atan2f(-v->y, sqrt_f32(v->x * v->x + v->z * v->z)) * 65536.0f / 6.2831855f);
}

/* 0x80050DE4 (0x7C): the 16-bit pitch angle of the matrix's Z axis. */
extern "C" u16 calcMtxAngX(MTX34* mtx)
{
    VEC3 axis;
    VEC3_ctor(&axis);
    setVector3(&axis, mtx->m[0][2], mtx->m[1][2], mtx->m[2][2]);
    return (s32)(0.5f + atan2f(axis.z, -axis.y) * 65536.0f / 6.2831855f);
}

/* 0x80050E60 (0x4C): the 16-bit yaw of the matrix's Z axis. */
extern "C" u16 calcMtxAng3(MTX34* mtx)
{
    VEC3 axis;
    VEC3_ctor(&axis);
    setVector3(&axis, mtx->m[0][2], mtx->m[1][2], mtx->m[2][2]);
    return calcVecAng3(&axis);
}

extern "C" f32 vec3_length_sq(const VEC3* v);

/* 0x80050EAC (0x30): the squared distance between `a` and `b`. */
extern "C" f32 vec3_dist_sq(const VEC3* a, const VEC3* b)
{
    VEC3 diff;
    subVec3(&diff, b, a);
    return vec3_length_sq(&diff);
}

/* 0x80050EDC (0x18): the squared length of the vector. */
extern "C" asm f32 vec3_length_sq(register const VEC3* v)
{
    nofralloc
    psq_l f0, 0(v), 0, 0
    ps_mul f0, f0, f0
    lfs f1, 8(v)
    ps_madd f1, f1, f1, f0
    ps_sum0 f1, f1, f0, f0
    blr
}

extern "C" f32 vec3_len(const VEC3* v);

/* 0x80050EF4 (0x30): the distance between `a` and `b`. */
extern "C" f32 vec3_distance(const VEC3* a, const VEC3* b)
{
    VEC3 diff;
    subVec3(&diff, b, a);
    return vec3_len(&diff);
}

/* 0x80050F24 (0x24): the length of the vector. */
extern "C" f32 vec3_len(const VEC3* v)
{
    return PSVECMag(vec3_to_const_vec(v));
}

/* 0x80050F48 (0x38): the squared distance between `a` and `b` in the XZ plane. */
f32 calcDistanceSqXZ(VEC3* a, VEC3* b)
{
    VEC3 diff;
    subVec3(&diff, b, a);
    diff.y = 0.0f;
    return vec3_length_sq(&diff);
}

/* 0x80050F80 (0x38): the distance between `a` and `b` in the XZ plane. */
extern "C" f32 calcVecDistXZ(const VEC3* a, const VEC3* b)
{
    VEC3 diff;
    subVec3(&diff, b, a);
    diff.y = 0.0f;
    return vec3_len(&diff);
}

/* 0x80050FB8 (0xAC): rotates the vector about X by the 16-bit angle `angle`. */
void rotVecX(VEC3* v, u32 angle)
{
    f32 y = v->y;
    f32 z = v->z;
    f32 s = sin_idx_get(angle);
    f32 c = anim_tick_cos(angle);
    v->y = c * y - s * z;
    v->z = s * y + c * z;
}

/* 0x80051064 (0xB0): rotates the vector about Y by the 16-bit angle `angle`. */
void rotVecY(VEC3* v, u32 angle)
{
    f32 x = v->x;
    f32 z = v->z;
    f32 s = sin_idx_get(angle);
    f32 c = anim_tick_cos(angle);
    v->x = c * x + s * z;
    v->z = -s * x + c * z;
}

/* 0x80051114 (0xAC): rotates the vector about Z by the 16-bit angle `angle`. */
void rotVecZ(VEC3* v, u32 angle)
{
    f32 x = v->x;
    f32 y = v->y;
    f32 s = sin_idx_get(angle);
    f32 c = anim_tick_cos(angle);
    v->x = c * x - s * y;
    v->y = s * x + c * y;
}

/* 0x800511C0 (0x54): rotates the vector about X, then Y, then Z by the three angles of `ang`. */
void rotVecXYZ(VEC3* v, _CP_VECTOR* ang)
{
    rotVecX(v, ang->x);
    rotVecY(v, ang->y);
    rotVecZ(v, ang->z);
}

/* 0x80051214 (0x54): rotates the vector about Z, then X, then Y by the three angles of `ang`. */
void rotVecZXY(VEC3* v, _CP_VECTOR* ang)
{
    rotVecZ(v, ang->z);
    rotVecX(v, ang->x);
    rotVecY(v, ang->y);
}

/* 0x80051268 (0x58): the unit Z vector rotated by the three angles of `ang` (unrotated without `ang`). */
VEC3 getDirVec(_CP_VECTOR* ang)
{
    VEC3 dir;
    setVec3(&dir, 0.0f, 0.0f, 1.0f);
    if (ang != NULL) {
        rotVecXYZ(&dir, ang);
    }
    return dir;
}

extern "C" VEC3* vec3_scale_in_place(VEC3* v, f32 scale);
extern "C" void addVec3(VEC3* out, const VEC3* a, const VEC3* b);

/* 0x800512C0 (0xB8): pulls `pos` to within `limit` of `center`; returns how far it was beyond the limit. */
extern "C" f32 vec3_clamp_distance(VEC3* center, VEC3* pos, f32 limit)
{
    VEC3 diff;
    VEC3 sum;
    f32 len;

    subVec3(&diff, pos, center);
    len = vec3_len(&diff);
    if (len > limit) {
        vec3_normalize_into(&diff, &diff);
        vec3_scale_in_place(&diff, limit);
        addVec3(&sum, center, &diff);
        copyVec3(pos, &sum);
    }
    return len - limit;
}

extern "C" void vec3_add_ps(VEC3* out, const VEC3* a, const VEC3* b);

/* 0x80051378 (0x54): `out = a + b`. */
extern "C" void addVec3(VEC3* out, const VEC3* a, const VEC3* b)
{
    VEC3_ctor(out);
    vec3_add_ps(out, a, b);
}

/* 0x800513CC (0x24): `out = a + b` with paired-single arithmetic. */
extern "C" asm void vec3_add_ps(register VEC3* out, register const VEC3* a, register const VEC3* b)
{
    nofralloc
    psq_l f0, 0(a), 0, 0
    psq_l f1, 0(b), 0, 0
    ps_add f0, f0, f1
    psq_st f0, 0(out), 0, 0
    psq_l f0, 8(a), 1, 0
    psq_l f1, 8(b), 1, 0
    ps_add f0, f0, f1
    psq_st f0, 8(out), 1, 0
    blr
}

extern "C" void vec3_scale_by(VEC3* out, const VEC3* in, f32 scale);

/* 0x800513F0 (0x34): scales the vector in place and returns it. */
extern "C" VEC3* vec3_scale_in_place(VEC3* v, f32 scale)
{
    vec3_scale_by(v, v, scale);
    return v;
}

/* 0x80051424 (0x1C): `out = in * scale` with paired-single arithmetic. */
extern "C" asm void vec3_scale_by(register VEC3* out, register const VEC3* in, register f32 scale)
{
    nofralloc
    psq_l f0, 0(in), 0, 0
    ps_muls0 f0, f0, scale
    psq_st f0, 0(out), 0, 0
    psq_l f0, 8(in), 1, 0
    ps_muls0 f0, f0, scale
    psq_st f0, 8(out), 1, 0
    blr
}

extern "C" void assignVec3(VEC3* dst, const VEC3* src);

/* 0x80051440 (0x50): rotates the vector by `mtx`'s 3x3 part. */
void mulVecMat(VEC3* v, MTX34* mtx)
{
    VEC3 src;
    assignVec3(&src, v);
    mtx34_rotate_vec3(v, mtx, &src);
}

/* 0x80051490 (0x1C): copies `src` into `dst`. */
extern "C" void assignVec3(VEC3* dst, const VEC3* src)
{
    *dst = *src;
}

extern "C" VEC3* mtx34_mult_vec3(VEC3* out, const MTX34* mtx, const VEC3* v);

/* 0x800514AC (0x50): transforms the vector by `mtx` (rotation and translation). */
void mulVecMatAddTrans(VEC3* v, MTX34* mtx)
{
    VEC3 src;
    assignVec3(&src, v);
    mtx34_mult_vec3(v, mtx, &src);
}

extern "C" const f32 (*mtx34_const_ptr(const MTX34* mtx))[4];

/* 0x800514FC (0x74): `out = mtx * v` and returns `out`. */
extern "C" VEC3* mtx34_mult_vec3(VEC3* out, const MTX34* mtx, const VEC3* v)
{
    PSMTXMultVec(mtx34_const_ptr(mtx), vec3_to_const_vec(v), vec3_to_vec(out));
    return out;
}

/* 0x80051570 (0x4): the matrix as the SDK's read-only `Mtx` rows. */
extern "C" const f32 (*mtx34_const_ptr(const MTX34* mtx))[4]
{
    return mtx->m;
}

/* 0x80051574 (0x17C): `dst = dst * src` for the 3x3 rotation parts. */
extern "C" void mtx34_concat_assign(MTX34* dst, const MTX34* src)
{
    MTX34 tmp;
    MTX34_ctor(&tmp);
    tmp.m[0][0] = dst->m[0][2] * src->m[2][0] + (dst->m[0][0] * src->m[0][0] + dst->m[0][1] * src->m[1][0]);
    tmp.m[0][1] = dst->m[0][2] * src->m[2][1] + (dst->m[0][0] * src->m[0][1] + dst->m[0][1] * src->m[1][1]);
    tmp.m[0][2] = dst->m[0][2] * src->m[2][2] + (dst->m[0][0] * src->m[0][2] + dst->m[0][1] * src->m[1][2]);
    tmp.m[1][0] = dst->m[1][2] * src->m[2][0] + (dst->m[1][0] * src->m[0][0] + dst->m[1][1] * src->m[1][0]);
    tmp.m[1][1] = dst->m[1][2] * src->m[2][1] + (dst->m[1][0] * src->m[0][1] + dst->m[1][1] * src->m[1][1]);
    tmp.m[1][2] = dst->m[1][2] * src->m[2][2] + (dst->m[1][0] * src->m[0][2] + dst->m[1][1] * src->m[1][2]);
    tmp.m[2][0] = dst->m[2][2] * src->m[2][0] + (dst->m[2][0] * src->m[0][0] + dst->m[2][1] * src->m[1][0]);
    tmp.m[2][1] = dst->m[2][2] * src->m[2][1] + (dst->m[2][0] * src->m[0][1] + dst->m[2][1] * src->m[1][1]);
    tmp.m[2][2] = dst->m[2][2] * src->m[2][2] + (dst->m[2][0] * src->m[0][2] + dst->m[2][1] * src->m[1][2]);
    copyMat33(dst, &tmp);
}

/* 0x800516F0 (0x4): the 3x3 matrix record's (empty) constructor. */
extern "C" void MTX33_ctor(nw4r::math::MTX33* mtx)
{
}

/* 0x800516F4 (0x4C): copies the 3x3 rotation of `src` into `dst` (the translation column is left). */
void copyMat33(MTX34* dst, MTX34* src)
{
    for (int row = 0; row < 3; row++) {
        dst->m[row][0] = src->m[row][0];
        dst->m[row][1] = src->m[row][1];
        dst->m[row][2] = src->m[row][2];
    }
}

extern "C" VEC3* vec3_cross(VEC3* out, const VEC3* a, const VEC3* b);

/* 0x80051740 (0xE0): sets `mtx`'s rotation to the orthonormal basis of the X direction `x_dir` and the Y hint
 * `y_dir` (Z = X x Y, then Y = Z x X). */
extern "C" void mtx34_basis_set(MTX34* mtx, const VEC3* x_dir, const VEC3* y_dir)
{
    VEC3 x_axis;
    VEC3 y_axis;
    VEC3 z_axis;
    VEC3 y_hint;
    VEC3_ctor(&x_axis);
    VEC3_ctor(&y_axis);
    VEC3_ctor(&z_axis);
    VEC3_ctor(&y_hint);
    vec3_normalize_into(&x_axis, x_dir);
    vec3_normalize_into(&y_hint, y_dir);
    vec3_cross(&z_axis, &x_axis, &y_hint);
    vec3_cross(&y_axis, &z_axis, &x_axis);
    mtx->m[0][0] = x_axis.x;
    mtx->m[1][0] = x_axis.y;
    mtx->m[2][0] = x_axis.z;
    mtx->m[0][1] = y_axis.x;
    mtx->m[1][1] = y_axis.y;
    mtx->m[2][1] = y_axis.z;
    mtx->m[0][2] = z_axis.x;
    mtx->m[1][2] = z_axis.y;
    mtx->m[2][2] = z_axis.z;
}

/* 0x80051820 (0x74): `out = a x b` and returns `out`. */
extern "C" VEC3* vec3_cross(VEC3* out, const VEC3* a, const VEC3* b)
{
    PSVECCrossProduct(vec3_to_const_vec(a), vec3_to_const_vec(b), vec3_to_vec(out));
    return out;
}

/* 0x80051894 (0x2E8): blends `a` and `b` with the weights `wa`/`wb` into `out`: the rotation re-orthonormalised
 * from the blended X and Y columns, the column scales blended unless `keep_unit`, the translation blended. */
extern "C" void mtx34_blend(MTX34* out, const MTX34* a, const MTX34* b, u8 keep_unit, f32 wa, f32 wb)
{
    VEC3 y_dir;
    VEC3 x_dir;
    VEC3 unused;
    VEC3 column;
    f32 len;
    f32 scale;

    VEC3_ctor(&y_dir);
    VEC3_ctor(&x_dir);
    VEC3_ctor(&unused);
    VEC3_ctor(&column);
    x_dir.x = a->m[0][0] * wa + b->m[0][0] * wb;
    x_dir.y = a->m[1][0] * wa + b->m[1][0] * wb;
    x_dir.z = a->m[2][0] * wa + b->m[2][0] * wb;
    y_dir.x = a->m[0][1] * wa + b->m[0][1] * wb;
    y_dir.y = a->m[1][1] * wa + b->m[1][1] * wb;
    y_dir.z = a->m[2][1] * wa + b->m[2][1] * wb;
    mtx34_basis_set(out, &x_dir, &y_dir);
    if (keep_unit == 0) {
        setVector3(&column, a->m[0][0], a->m[1][0], a->m[2][0]);
        len = vec3_len(&column);
        setVector3(&column, b->m[0][0], b->m[1][0], b->m[2][0]);
        scale = len * wa + vec3_len(&column) * wb;
        out->m[0][0] *= scale;
        out->m[1][0] *= scale;
        out->m[2][0] *= scale;
        setVector3(&column, a->m[0][1], a->m[1][1], a->m[2][1]);
        len = vec3_len(&column);
        setVector3(&column, b->m[0][1], b->m[1][1], b->m[2][1]);
        scale = len * wa + vec3_len(&column) * wb;
        out->m[0][1] *= scale;
        out->m[1][1] *= scale;
        out->m[2][1] *= scale;
        setVector3(&column, a->m[0][2], a->m[1][2], a->m[2][2]);
        len = vec3_len(&column);
        setVector3(&column, b->m[0][2], b->m[1][2], b->m[2][2]);
        scale = len * wa + vec3_len(&column) * wb;
        out->m[0][2] *= scale;
        out->m[1][2] *= scale;
        out->m[2][2] *= scale;
    }
    out->m[0][3] = a->m[0][3] * wa + b->m[0][3] * wb;
    out->m[1][3] = a->m[1][3] * wa + b->m[1][3] * wb;
    out->m[2][3] = a->m[2][3] * wa + b->m[2][3] * wb;
}

extern "C" void vec3_polar_offset(VEC3* out, const VEC3* center, u16 yaw, f32 dist);
extern "C" void vec3_polar_offset2(VEC3* out, const VEC3* center, u16 pitch, u16 yaw, f32 dist);

/* 0x80051B7C (0xF4): a random point around `center` within `radius`: on the ground plane (`mode` 0) or in any
 * direction (`mode` 1). */
extern "C" void vec3_random_offset(VEC3* out, const VEC3* center, u8 mode, f32 radius)
{
    radius *= ran_suu(0) / 65535.0f;
    switch (mode) {
    case 0:
        vec3_polar_offset(out, center, ran_suu(0), radius);
        break;
    case 1:
        vec3_polar_offset2(out, center, ran_suu(0), ran_suu(0), radius);
        break;
    }
}

/* 0x80051C70 (0xA4): the point `dist` away from `center` in the ground-plane direction `yaw`. */
extern "C" void vec3_polar_offset(VEC3* out, const VEC3* center, u16 yaw, f32 dist)
{
    VEC3 offset;
    VEC3_ctor(&offset);
    offset.x = 0.0f;
    offset.y = 0.0f;
    offset.z = dist;
    rotVecY(&offset, yaw);
    out->x = center->x + offset.x;
    out->y = center->y;
    out->z = center->z + offset.z;
}

/* 0x80051D14 (0xC4): the point `dist` away from `center` in the direction `pitch`/`yaw`. */
extern "C" void vec3_polar_offset2(VEC3* out, const VEC3* center, u16 pitch, u16 yaw, f32 dist)
{
    VEC3 offset;
    VEC3_ctor(&offset);
    offset.x = 0.0f;
    offset.y = 0.0f;
    offset.z = dist;
    rotVecX(&offset, pitch);
    rotVecY(&offset, yaw);
    out->x = center->x + offset.x;
    out->y = center->y + offset.y;
    out->z = center->z + offset.z;
}

extern "C" void vec3_scale(VEC3* out, const VEC3* in, f32 scale);

/* 0x80051DD8 (0x108): the point `dist` away, in the ground-plane direction `yaw`, from the point at `t` (clamped
 * to 0..1) along the segment from `b` to `a`. */
extern "C" void vec3_segment_offset(VEC3* out, const VEC3* a, const VEC3* b, u16 yaw, f32 dist, f32 t)
{
    VEC3 dir;
    VEC3 pos;
    VEC3 tmp;
    VEC3 sum;
    VEC3 step;

    VEC3_ctor(&dir);
    VEC3_ctor(&pos);
    subVec3(&tmp, a, b);
    copyVec3(&dir, &tmp);
    if (t > 1.0f) {
        t = 1.0f;
    }
    if (t < 0.0f) {
        t = 0.0f;
    }
    vec3_scale(&step, &dir, t);
    addVec3(&sum, b, &step);
    copyVec3(&pos, &sum);
    vec3_polar_offset(out, &pos, yaw, dist);
}

/* 0x80051EE0 (0x54): `out = in * scale`. */
extern "C" void vec3_scale(VEC3* out, const VEC3* in, f32 scale)
{
    VEC3_ctor(out);
    vec3_scale_by(out, in, scale);
}

/* 0x80051F34 (0x100): `vec3_segment_offset` in the direction `pitch`/`yaw`. */
extern "C" void vec3_segment_offset2(VEC3* out, const VEC3* a, const VEC3* b, u16 pitch, u16 yaw, f32 dist, f32 t)
{
    VEC3 dir;
    VEC3 pos;
    VEC3 tmp;
    VEC3 sum;
    VEC3 step;

    VEC3_ctor(&dir);
    VEC3_ctor(&pos);
    subVec3(&tmp, a, b);
    copyVec3(&dir, &tmp);
    if (t > 1.0f) {
        t = 1.0f;
    }
    if (t < 0.0f) {
        t = 0.0f;
    }
    vec3_scale(&step, &dir, t);
    addVec3(&sum, b, &step);
    copyVec3(&pos, &sum);
    vec3_polar_offset2(out, &pos, pitch, yaw, dist);
}

extern "C" f32 vec3_dot(const VEC3* a, const VEC3* b);
extern "C" f32 abs_f32(f32 value);

/* 0x80052034 (0x1D8): whether `p` lies in the box along the segment `a`..`b`: within `size` above `a`, within
 * `half_width` of the segment's line in the ground plane, and between its two ends. */
extern "C" u32 vec3_segment_box_ck(const VEC3* p, const VEC3* a, const VEC3* b, f32 size, f32 half_width)
{
    VEC3 side;
    VEC3 seg;
    VEC3 rel;

    VEC3_ctor(&side);
    VEC3_ctor(&seg);
    VEC3_ctor(&rel);
    if (p->y < a->y || p->y > a->y + size) {
        return 0;
    }
    seg.x = b->x - a->x;
    seg.y = b->y - a->y;
    seg.z = b->z - a->z;
    side.x = -size * seg.z;
    side.y = 0.0f;
    side.z = seg.x * size;
    if (vec3_length_sq(&side) > 1e-5f) {
        vec3_normalize_into(&side, &side);
    }
    rel.x = p->x - a->x;
    rel.y = p->y - a->y;
    rel.z = p->z - a->z;
    if (abs_f32(vec3_dot(&side, &rel)) > half_width) {
        return 0;
    }
    if (vec3_dot(&seg, &rel) < 0.0f) {
        return 0;
    }
    rel.x = p->x - b->x;
    rel.y = p->y - b->y;
    rel.z = p->z - b->z;
    return !(vec3_dot(&seg, &rel) > 0.0f);
}

/* 0x8005220C (0x8): the absolute value. */
extern "C" asm f32 abs_f32(register f32 value)
{
    nofralloc
    fabs f1, value
    blr
}

/* 0x80052214 (0x20): the dot product of `a` and `b`. */
extern "C" asm f32 vec3_dot(register const VEC3* a, register const VEC3* b)
{
    nofralloc
    psq_l f0, 4(a), 0, 0
    psq_l f1, 4(b), 0, 0
    ps_mul f0, f0, f1
    psq_l f2, 0(a), 1, 0
    psq_l f1, 0(b), 1, 0
    ps_madd f1, f2, f1, f0
    ps_sum0 f1, f1, f0, f0
    blr
}

/* 0x80052234 (0x54): `out = a * t + b * (1 - t)`. */
extern "C" void vec3_lerp(VEC3* out, const VEC3* a, const VEC3* b, f32 t)
{
    f32 s = 1.0f - t;
    out->x = a->x * t + b->x * s;
    out->y = a->y * t + b->y * s;
    out->z = a->z * t + b->z * s;
}

/* --- key-frame curves ------------------------------------------------------------------------------- */

/* 0x80052288 (0x78): the cubic Hermite curve through (`t0`, `p0`, slope `m0`) and (`t1`, `p1`, slope `m1`) at `t`. */
extern "C" f32 hermite_interp(f32 t, f32 p0, f32 t0, f32 m0, f32 p1, f32 t1, f32 m1)
{
    f32 d = t - t0;
    f32 inv = 1.0f / (t1 - t0);
    f32 inv2 = inv * inv;
    f32 d2 = d * d;
    f32 d2i = d2 * inv;
    f32 d3i2 = inv2 * (d2 * d);
    f32 h2 = 2.0f * d3i2 * inv;
    f32 h3 = 3.0f * d2 * inv2;
    f32 h11 = d3i2 - d2i;
    return m1 * h11 + (m0 * (d + (h11 - d2i)) + (p0 * (1.0f + (h2 - h3)) + p1 * (-h2 + h3)));
}

/* 0x80052300 (0x70): the value at `frame` of the linear key curve `keys` ({frame, value} pairs, a negative frame
 * ending the curve with its value). */
f32 getKeyData(f32* keys, f32 frame)
{
    for (;;) {
        if (keys[0] < 0.0f) {
            return keys[1];
        }
        if (frame >= keys[0] && frame <= keys[2]) {
            return keys[1] + (keys[3] - keys[1]) * ((frame - keys[0]) / (keys[2] - keys[0]));
        }
        keys += 2;
    }
}

/* 0x80052370 (0x98): the two values at `frame` of the key curve `keys` ({frame, v0, v1} triples). */
extern "C" void anim_key_curve_eval(f32* keys, f32 frame, f32* out0, f32* out1)
{
    for (;;) {
        if (keys[0] < 0.0f) {
            *out0 = keys[1];
            *out1 = keys[2];
            return;
        }
        if (frame >= keys[0] && frame <= keys[3]) {
            f32 r = (frame - keys[0]) / (keys[3] - keys[0]);
            *out0 = keys[1] + r * (keys[4] - keys[1]);
            *out1 = keys[2] + r * (keys[5] - keys[2]);
            return;
        }
        keys += 3;
    }
}

/* 0x80052408 (0xB8): the three values at `frame` of the key curve `keys` ({frame, v0, v1, v2} quadruples). */
void getKeyData3(f32* keys, f32 frame, f32* out0, f32* out1, f32* out2)
{
    for (;;) {
        if (keys[0] < 0.0f) {
            *out0 = keys[1];
            *out1 = keys[2];
            *out2 = keys[3];
            return;
        }
        if (frame >= keys[0] && frame <= keys[4]) {
            f32 r = (frame - keys[0]) / (keys[4] - keys[0]);
            *out0 = keys[1] + r * (keys[5] - keys[1]);
            *out1 = keys[2] + r * (keys[6] - keys[2]);
            *out2 = keys[3] + r * (keys[7] - keys[3]);
            return;
        }
        keys += 4;
    }
}

/* 0x800524C0 (0x124): `to`, pulled back toward `from` by `k` times how far `dir` points against the move (`to`
 * itself when the move is null or `dir` does not oppose it). */
extern "C" void vec3_move_damp(const VEC3* from, const VEC3* to, const VEC3* dir, f32 k, VEC3* out)
{
    VEC3 move;
    VEC3 unit;
    VEC3 tmp;
    VEC3 sum;
    VEC3 step;
    f32 d;

    VEC3_ctor(&move);
    VEC3_ctor(&unit);
    subVec3(&tmp, to, from);
    copyVec3(&move, &tmp);
    if (vec3_length_sq(&move) <= 1e-5f) {
        copyVec3(out, to);
        return;
    }
    vec3_normalize_into(&unit, &move);
    d = vec3_dot(dir, &unit);
    if (d >= 0.0f) {
        copyVec3(out, to);
        return;
    }
    vec3_scale(&step, &move, 1.0f + d * k);
    addVec3(&sum, from, &step);
    copyVec3(out, &sum);
}

/* --- 2D rectangles ---------------------------------------------------------------------------------- */

/* 0x800525E4 (0xC): stores the 2D point (`x`, `y`). */
extern "C" void ivec2_set(s16* pt, s16 x, s16 y)
{
    pt[0] = x;
    pt[1] = y;
}

/* 0x800525F0 (0x6C): sets the rectangle at (`x`, `y`) of size `w` x `h` with the colour `color`. */
extern "C" void draw_rect_set(DrawRect* rect, s16 x, s16 y, s16 w, s16 h, u32 color)
{
    ivec2_set(&rect->x0, x, y);
    ivec2_set(&rect->x1, x + w, y + h);
    rect->color = color;
}

extern "C" void uv_pair_copy(s16* dst, const s16* src);
extern "C" void tex_uv_copy(s16* dst, const s16* src);

/* 0x8005265C (0x88): sets the textured rectangle at `pos` of size `w` x `h`, its colour and texture pairs. */
extern "C" void draw_rect_2d_tex_init(_DRAW_RECT_2D_TEX* rect, const s16* pos, s16 w, s16 h, u32 color, const s16* uv0,
                                      const s16* uv1)
{
    uv_pair_copy(&rect->x, pos);
    ivec2_set(&rect->end_x, pos[0] + w, pos[1] + h);
    rect->color = color;
    tex_uv_copy(&rect->u0, uv0);
    tex_uv_copy(&rect->u1, uv1);
}

/* 0x800526E4 (0x14): copies one 2D point. */
extern "C" void tex_uv_copy(s16* dst, const s16* src)
{
    dst[0] = src[0];
    dst[1] = src[1];
}

/* 0x800526F8 (0x14): copies one 2D point (the coordinate pair at the head of a record). */
extern "C" void uv_pair_copy(s16* dst, const s16* src)
{
    dst[0] = src[0];
    dst[1] = src[1];
}
