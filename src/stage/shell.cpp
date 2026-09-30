/*
 * stage/shell.cpp - the shell pool: the 64 work records of thrown/fired objects (`_SHELL_W`), the 64-byte
 * work-block allocator and model-slot pool behind them, and the serial/niku sync tables.
 *
 * `.text` 0x802AA6A8-0x802ABD28 (58 functions, 5760 B), extab 0x80013984-0x80013AC4, extabindex
 * 0x800314C4-0x800316A4, `.ctors` 0x8056F374 (`shell_pool_static_init`), `.bss` 0x806AD698-0x806B8798 (the pool,
 * 0x3C0 + 64 x 0x10C + 64 x 0x168 + 0x1040 B) and `.sdata2` 0x8079A400-0x8079A410.  53 of 58 rows at 100 %;
 * still `NonMatching`.  Registered 2026-09-30 from the tail of `menu/menu_message.cpp`.
 *
 * Seams (evidence: `.ctors` words 0x8056F374 -> `shell_pool_static_init` and 0x8056F378 -> `yure_static_init`, one per TU;
 * `callers.py 0x802AA6A8` lists only `Pl/fn_80288CEC.cpp`, `enemy/em_pop.cpp` and one unsplit caller).  Left edge 0x802AA6A8, not the old 0x802AA764: the
 * preceding function `shell_work_init` memsets the pool and calls `serial_table_init`/`niku_table_init`, so it
 * is the shell TU's own initialiser (callers `Pl/fn_80288CEC.cpp`, `enemy/em_pop.cpp`).  Right edge 0x802ABD28
 * is the one `.ctors` word per TU: this TU's `__sinit` (`shell_pool_static_init`, 0x802ABC4C) is followed by its two
 * compiler-made element constructors, and everything after is `Pl/pl_yure.cpp` (own `__sinit` at 0x802AD9A8, own
 * `.data`/`.sdata`/`.sdata2`/`.bss`, no reference to this pool).  Module and file name are a GUESS (no `__FILE__`
 * string): the records are the projectile work `ef.h` calls `_SHELL_W`; the address neighbour is `stage/stg_w.cpp`.
 *
 * Names are GUESSES from behaviour: `serial_*` rows are the network-synced shell slots (`lb_sub0d_send`, the
 * `isServerSelectState` gate), `niku_*` a second 12-row table; `player_0x05`/`seq_0x06` are the owner's player number
 * and that player's running sequence number.  `shell_set_func_ptr` (`.sbss` 0x80794B60) is NOT defined here: no
 * function of this TU touches `.sbss` but `pRoot`.
 *
 * Flags: `cflags_main` plus `#pragma peephole off` (the stage band's deviation, see `stage/stg_w.cpp`; dropping it
 * costs 33 rows).  The pool objects are defined after their users, which is what keeps each symbol's own
 * `lis/addi` (defined first, the compiler folds them onto one section base).
 *
 * Residuals:
 *   * `shell_block_alloc` 77.86: retail returns NULL from every loop exit through one shared tail and never
 *     re-tests the bound after the skip loop (a `goto`-shaped exit); ours duplicates the tail (308 vs 308 B, 4 rows).
 *   * `pull_shell_work` 93.59, 0x110 B vs 0x100 in the target (+16 B, shifts every later symbol so the object's `.text` is
 *     0x1690 vs the claimed 0x1680): retail threads the found flag into a jump from the loop to the tail; ours keeps the flag
 *     (one register, one test).  A `goto claimed` exit measures 100 % and 0x100 B, so the shape is known but rule 8 forbids
 *     it.  Conformant shapes tried: a pointer result instead of the flag 93.28, `i >= max` test 91.70, a one-case `switch`
 *     93.59 (no change), `#pragma peephole on` around the function 91.88 (268 B).
 *   * `shell_set_prim` 98.23 and `shell_attack_set` 99.04: retail keeps `beq L; b end` where ours folds to `bne end`.
 *   * `shell_block_free` 98.79: register numbers only.
 *   * `.sdata2`: the target claim is 0x10 B, ours 0xC - the last word is alignment padding.
 *   * `__declspec(export)` on `shell_block_clear_all`, `serial_st_change_by_key`, `niku_st_change_by_key`: nothing in the
 *     link references them (their callers are in RSOs), and the target object force-activates them.
 *
 * Inventory and evidence: `python tools/units/ledger.py unit stage/shell.cpp`.
 */

#include "types.h"

#include "Runtime.PPCEABI.H/memset.h"
#include "Network/network_pat_control.h"
#include "Pl/fn_80295EF4.h"
#include "ef/effect.h"
#include "ef/eft_res.h"
#include "ef/fn_800CDB2C.h"
#include "enemy/ENEMY_WORK.h"
#include "enemy/fn_801251D0.h"
#include "enemy/fn_8012EC74.h"
#include "g3d/g3d_calcworld.h"
#include "lobby/mhchar_construct.h"
#include "lobby/lb_sub0d_send.h"
#include "menu/hit_flags_clear.h"
#include "mh3_pad/vec3.h"
#include "pl.h"
#include "sound/sound_job_request.h"
#include "stage/shell.h"
#include "stage/stg_w.h"
#include "unsplit/g3d.h"
#include "unsplit/Runtime.PPCEABI.H.h"

#pragma peephole off

/* The pooled g3d work record `MHchar::g3d_0x10C` points at; only its model id is read here.
 * size: 0x8 (lower bound) */
struct _g3d_work {
    /* +0x00 */ u8 pad_0x00[0x4];
    /* +0x04 */ u32 model_id_0x04;
};

/* The 64-byte work-block pool `shell_block_alloc` hands runs of: the blocks, then one used byte per block. */
struct ShellBlockPool {
    /* +0x0000 */ u8 blocks_0x0000[SHELL_WORK_MAX][64];
    /* +0x1000 */ u8 map_0x1000[SHELL_WORK_MAX];
};  /* size: 0x1040 */

extern ShellBlockPool shell_block_pool;

extern "C" void shell_work_init(void) {
    memset(&shell_pool, 0, sizeof(ShellPool));
    serial_table_init();
    niku_table_init();
    shell_pool.work_0x08 = shell_work_tbl;
    shell_pool.work_max_0x04 = SHELL_WORK_MAX;
    memset(shell_work_tbl, 0, shell_pool.work_max_0x04 * sizeof(_SHELL_W));
    shell_pool.heap_0x14 = shell_chara_heap_tbl;
    shell_pool.heap_max_0x10 = SHELL_WORK_MAX;
    memset(shell_chara_heap_tbl, 0, shell_pool.heap_max_0x10 * sizeof(ShellCharaHeap));
    shell_pool.blocks_0x20 = shell_block_pool.blocks_0x0000[0];
    shell_pool.block_max_0x1C = SHELL_WORK_MAX;
    shell_pool.block_map_0x24 = shell_block_pool.map_0x1000;
    memset(&shell_block_pool, 0, sizeof(ShellBlockPool));
    shell_pool.ready_0x00 = 1;
}

_SHELL_W* pull_shell_work(u32 work_size) {
    ShellPool* pool = &shell_pool;
    _SHELL_W* shell;
    s32 i;
    s32 found = 0;

    if (pool->work_used_0x0C >= pool->work_max_0x04) {
        return NULL;
    }
    shell = pool->work_0x08;
    for (i = 0; i < pool->work_max_0x04; i++, shell++) {
        if (shell->active_0x00 == 0) {
            pool->work_used_0x0C++;
            found = 1;
            break;
        }
    }
    if (!found) {
        return NULL;
    }
    shell->work_blocks_0x104 = shell_block_count(work_size);
    shell->work_0x100 = shell_block_alloc(shell->work_blocks_0x104);
    if (shell->work_blocks_0x104 > 0 && shell->work_0x100 == NULL) {
        return NULL;
    }
    shell->active_0x00 = 1;
    shell->state_0x04 = 0;
    shell->state_0x05 = 0;
    shell->state_0x06 = 0;
    shell->state_0x07 = 0;
    hit_id_list_clear(shell->hit_ids_0xCE, &shell->hit_id_count_0xF6);
    shell->hit_id_0x5A = 0xFFFF;
    shell->serial_slot_0x0B = 0xFF;
    shell->group_0x108 = 0;
    return shell;
}

void push_shell_work(_SHELL_W* shell) {
    ShellPool* pool = &shell_pool;

    if (shell->work_0x100 != NULL) {
        if (shell->release_0xFC != NULL) {
            shell->release_0xFC(shell);
        }
        shell_block_free(shell->work_0x100, shell->work_blocks_0x104);
    }
    memset(shell, 0, sizeof(_SHELL_W));
    if (pool->work_used_0x0C > 0) {
        pool->work_used_0x0C--;
    }
}

extern "C" MHchar* pull_shell_chara_heap(void) {
    ShellPool* pool = &shell_pool;
    ShellCharaHeap* slot;
    s32 i;

    if (pool->heap_used_0x18 >= pool->heap_max_0x10) {
        return NULL;
    }
    slot = pool->heap_0x14;
    for (i = 0; i < pool->heap_max_0x10; i++, slot++) {
        if (slot->used_0x00 == 0) {
            slot->used_0x00 = 1;
            pool->heap_used_0x18++;
            mhchar_reset((MHchar*)slot->chara_0x04);
            return (MHchar*)slot->chara_0x04;
        }
    }
    return NULL;
}

s32 pull_shell_chara_heap_num(MHchar** out, u32 count) {
    MHchar** p;
    u32 i;
    s32 pulled = 0;

    for (i = 0, p = out; i < count; p++, i++) {
        *p = pull_shell_chara_heap();
        if (*p != NULL) {
            pulled++;
        }
    }
    return pulled;
}

extern "C" void push_shell_chara_heap(MHchar* chara) {
    ShellPool* pool = &shell_pool;
    ShellCharaHeap* slot;
    s32 i;

    if (chara->g3d_0x10C != 0) {
        push_g3d_wk(chara->g3d_0x10C);
        chara->g3d_0x10C = 0;
    }
    slot = pool->heap_0x14;
    for (i = 0; i < pool->heap_max_0x10; i++, slot++) {
        if ((MHchar*)slot->chara_0x04 == chara) {
            slot->used_0x00 = 0;
            if (pool->heap_used_0x18 > 0) {
                pool->heap_used_0x18--;
            }
            break;
        }
    }
}

void push_shell_chara_heap_num(MHchar** charas, u32 count) {
    MHchar** p;
    u32 i;

    for (i = 0, p = charas; i < count; p++, i++) {
        if (*p != 0) {
            push_shell_chara_heap(*p);
            *p = 0;
        }
    }
}

extern "C" u8* shell_block_alloc(s32 count) {
    ShellPool* pool = &shell_pool;
    u8* map = pool->block_map_0x24;
    s32 i;
    s32 run;
    s32 first;
    s32 n;
    u8* first_p;

    if (count <= 0) {
        return NULL;
    }
    i = 0;
    for (;;) {
        run = 0;
        while (*map != 0) {
            map++;
            i++;
            if (i >= pool->block_max_0x1C) {
                return NULL;
            }
        }
        first_p = map;
        first = i;
        for (;;) {
            run++;
            if (*map != 0) {
                break;
            }
            if (run == count) {
                for (n = 0; n < run; n++) {
                    first_p[n] = 1;
                }
                return pool->blocks_0x20 + first * 64;
            }
            map++;
            i++;
            if (i >= pool->block_max_0x1C) {
                return NULL;
            }
        }
    }
}

extern "C" void shell_block_free(u8* blocks, s32 count) {
    u32 first;
    s32 n;
    u8* map;
    ShellPool* pool = &shell_pool;

    first = (u32)(blocks - pool->blocks_0x20) >> 6;
    memset(pool->blocks_0x20 + first * 64, 0, count * 64);
    map = pool->block_map_0x24 + first;
    n = 0;
    while (n < count) {
        *map = 0;
        map++;
        n++;
    }
}

extern "C" __declspec(export) void shell_block_clear_all(void) {
    ShellPool* pool = &shell_pool;
    if (pool->block_max_0x1C != 0) {
        if (pool->blocks_0x20 != NULL) {
            memset(pool->blocks_0x20, 0, pool->block_max_0x1C * 64);
        }
        if (pool->block_map_0x24 != NULL) {
            memset(pool->block_map_0x24, 0, pool->block_max_0x1C);
        }
    }
}

extern "C" s32 shell_block_count(u32 size) {
    u32 blocks = size >> 6;

    if ((size - (blocks << 6)) & 0x3F) {
        blocks++;
    }
    return blocks;
}

nw4r::ef::Effect* res_eft_create_shell(u32 group, u32 id) {
    return res_eft_create(group, id, 0);
}

extern "C" void shell_area_release(u8 group) {
    s32 i;
    _SHELL_W* shell = shell_pool.work_0x08;

    for (i = 0; i < shell_pool.work_max_0x04; i++, shell++) {
        if (shell->active_0x00 != 0 && shell->group_0x108 == group) {
            shell->release_area_0xF8(shell);
            hit_attack_list_push(SHELL_HIT(shell));
            hit_body_list_push(SHELL_BODY(shell));
        }
    }
}

void res_shell_model_create(MHchar* model, u32 id, u32 arg) {
    res_eft_model_create(model, id, arg);
    model->ready = 0;
}

s32 shell_check_master_beflag(_SHELL_W* shell) {
    switch (shell->master_kind_0x09) {
    case 0:
        if (*(u8*)shell->master_0x0C == 0) {
            return 0;
        }
        break;
    case 1:
        if (*(u8*)shell->master_0x0C == 0) {
            return 0;
        }
        break;
    case 2:
        if (*(u8*)shell->master_0x0C == 0) {
            return 0;
        }
        break;
    }
    return 1;
}

/* untyped: opaque handle - an `MHchar` (draw_set, draw_set2) or an `nw4r::ef::Effect` (draw_set_eff) */
void shell_set_prim(_SHELL_W* shell, void* prim, u8 kind) {
    MHchar* model = (MHchar*)prim;

    if (kind != 0 || (u32)model->field_0x118 == model->g3d_0x10C->model_id_0x04) {
        if (kind == 0) {
            g3d_root_model_bind(pRoot, model->field_0x118);
        }
        if (shell->visible_0x01 != 0) {
            if (shell->area_0x08 == get_now_areano()) {
                switch (kind) {
                case 0:
                    sound_job_request(1, model->ready, model->field_0x118, 0, 0, 0, 0);
                    return;
                case 1:
                    sound_job_request(1, 1, (s32)prim, 2, 0, 0, 0);
                    break;
                }
            }
        }
    }
}

void shell_attack_set(_SHELL_W* shell, _HIT_DATA* hit_tbl, _HIT_SIZE_DATA** size_tbl, s32 index,
                      u8 attack_flags, u8 attack_kind) {
    _HIT_W* hit = SHELL_HIT(shell);
    _ENEMY_WORK* enemy;

    if (hit_tbl != NULL && size_tbl != NULL) {
        hit_flags_clear(hit);
        hit_tbl += index;
        hit->size_data_0x08 = size_tbl[hit_tbl->size_index_0x0F];
        hit_source_set(hit, 2, shell);
        hit->power_0x38 = hit_tbl->power_0x00;
        hit->knock_0x3C = hit_tbl->knock_0x02;
        hit_data_apply(hit, hit_tbl);
        switch (shell->master_kind_0x09) {
        case 0:
            hit_owner_set(hit, 0, shell->master_0x0C);
            break;
        case 1:
            enemy = (_ENEMY_WORK*)shell->master_0x0C;
            if (hit->life_0x40 > 0) {
                hit->life_0x40 = (s16)(hit->life_0x40 * enemy->shell_rate_0x7B4);
                if (hit->life_0x40 <= 0) {
                    hit->life_0x40 = 1;
                }
            }
            if (em_get_rank(enemy) <= 1) {
                attack_flags &= 0xEF;
            }
            hit_owner_set(hit, 1, shell->master_0x0C);
            em_hit_buff_apply(enemy, hit);
            break;
        case 2:
            hit_owner_set(hit, 3, shell->master_0x0C);
            break;
        case 3:
            hit_owner_set(hit, 2, shell);
            break;
        }
        hit->attack_flags_0x31 = attack_flags;
        hit->attack_kind_0x32 = attack_kind;
    }
}

void shell_hit_cont(_SHELL_W* shell) {
    hit_knock_set(SHELL_HIT(shell), 100);
}

void shell_erase_hit(_SHELL_W* shell) {
    SHELL_HIT(shell)->active_0x05 = 0;
}

void shell_erase_body(_SHELL_W* shell) {
    SHELL_BODY(shell)->active_0x05 = 0;
}

/* untyped: caller-owned payload - the owner's work record (player, enemy or NPC by `kind`) */
void shell_master_set(_SHELL_W* shell, void* master, u8 kind) {
    shell->master_0x0C = master;
    shell->master_kind_0x09 = kind;
}

extern "C" void shell_place_model(_SHELL_W* shell, MHchar* model) {
    model->field_0x28 = shell->rot_x_0x24;
    model->field_0x2C = shell->rot_y_0x28;
    model->field_0x30 = shell->rot_z_0x2C;
    copyVec3(&model->pos_0x04, &shell->pos_0x18);
}

void shell_draw_set(_SHELL_W* shell, MHchar* model) {
    shell_place_model(shell, model);
    model->move(0);
    shell_set_prim(shell, model, 0);
}

void shell_draw_set2(_SHELL_W* shell, MHchar* model, nw4r::math::MTX34* mtx) {
    shell_place_model(shell, model);
    model->move2(mtx, 0);
    shell_set_prim(shell, model, 0);
}

void shell_draw_set_eff(_SHELL_W* shell, nw4r::ef::Effect* effect) {
    effect_move(effect);
    shell_set_prim(shell, effect, 1);
}

void shell_rate_add(_SHELL_W* shell) {
    addVec3To(&shell->pos_0x18, &shell->vel_0x3C);
}

void shell_rate_add_g(_SHELL_W* shell) {
    addVec3To(&shell->pos_0x18, &shell->vel_0x3C);
    addVec3To(&shell->vel_0x3C, &shell->vel_0x48);
}

void set_shell_scr_id(_SHELL_W* shell, u8 area_bits, u8 water_bits) {
    shell->scr_id_0x0A = area_bits | water_bits;
}

s32 get_shell_in_water(_SHELL_W* shell) {
    if (shell->pos_0x18.y < 0.0f) {
        if ((shell->scr_id_0x0A & 4) != 0) {
            if (stage_water_area_ck() == 1) {
                return 1;
            }
        } else if ((shell->scr_id_0x0A & 2) != 0) {
            if (stage_water_enabled_ck() == 1) {
                return 1;
            }
        }
    }
    return 0;
}

extern "C" void serial_entry_clear(ShellSerialEntry* entry) {
    entry->shell_0x00 = 0;
    entry->mode_0x04 = 0;
    entry->player_0x05 = 0xFF;
    entry->seq_0x06 = 0xFF;
    entry->state_0x07 = 0;
    entry->word_0x08 = 0xFFFF;
}

extern "C" void serial_table_init(void) {
    ShellSerialEntry* entry = shell_pool.serial_0x28;
    u32 i = 0;

    do {
        serial_entry_clear(entry);
        i++;
        entry++;
    } while (i < SHELL_SERIAL_MAX);
    shell_pool.serial_next_0x328[0] = 0;
    shell_pool.serial_next_0x328[1] = 0;
    shell_pool.serial_next_0x328[2] = 0;
    shell_pool.serial_next_0x328[3] = 0;
}

extern "C" ShellSerialEntry* serial_entry_at(u8 index) {
    return &shell_pool.serial_0x28[index];
}

ShellSerialEntry* get_shell_serial_sw(_SHELL_W* shell) {
    u32 i = 0;
    ShellSerialEntry* entry;

    do {
        entry = serial_entry_at(i);
        if (entry->state_0x07 != 0 && entry->shell_0x00 == shell) {
            return entry;
        }
        i++;
    } while (i < SHELL_SERIAL_MAX);
    return NULL;
}

extern "C" ShellSerialEntry* serial_find(u8 player, u8 seq) {
    u32 i = 0;
    ShellSerialEntry* entry;

    do {
        entry = serial_entry_at(i);
        if (entry->state_0x07 != 0 && entry->player_0x05 == player && entry->seq_0x06 == seq) {
            return entry;
        }
        i++;
    } while (i < SHELL_SERIAL_MAX);
    return NULL;
}

ShellSerialEntry* serial_set(_SHELL_W* shell, u8 mode, u8 player) {
    ShellPool* pool = &shell_pool;
    u8 i;
    ShellSerialEntry* entry;

    for (i = 0; i < SHELL_SERIAL_MAX; ++i) {
        entry = serial_entry_at(i);
        if (entry->state_0x07 == 0) {
            entry->shell_0x00 = shell;
            entry->mode_0x04 = mode;
            entry->player_0x05 = player;
            entry->seq_0x06 = pool->serial_next_0x328[player]++;
            entry->state_0x07 = 1;
            shell->serial_slot_0x0B = i;
            return entry;
        }
    }
    shell->serial_slot_0x0B = 0xFF;
    return NULL;
}

extern "C" void serial_state_raise(ShellSerialEntry* entry, u8 state) {
    if (entry->state_0x07 < state) {
        if (isServerSelectState() == 1) {
            lb_sub0d_send(my_player_no(), entry, state);
        } else {
            entry->state_0x07 = state;
        }
    }
}

s32 serial_st_change_sw(_SHELL_W* shell, u8 state) {
    ShellSerialEntry* entry = get_shell_serial_sw(shell);

    if (entry != NULL) {
        serial_state_raise(entry, state);
        return 1;
    }
    return 0;
}

extern "C" __declspec(export) s32 serial_st_change_by_key(u8 player, u8 seq, u8 state) {
    ShellSerialEntry* entry = serial_find(player, seq);

    if (entry != NULL) {
        serial_state_raise(entry, state);
        return 1;
    }
    return 0;
}

extern "C" void serial_state_set_word(ShellSerialEntry* entry, u8 state, u16 word) {
    if (entry != NULL && entry->state_0x07 < state) {
        entry->state_0x07 = state;
        entry->word_0x08 = word;
    }
}

extern "C" void serial_state_set_word_net(ShellSerialEntry* entry, u16 word) {
    u8 limit;

    if (isServerSelectState() == 1) {
        limit = 3;
    } else {
        limit = 4;
    }
    if (entry->state_0x07 < limit) {
        entry->word_0x08 = word;
        serial_state_raise(entry, limit);
    }
}

extern "C" s32 serial_state_reached(ShellSerialEntry* entry, u8 state) {
    return entry->state_0x07 >= state;
}

s32 serial_st_check_sw(_SHELL_W* shell, u8 state) {
    ShellSerialEntry* entry = get_shell_serial_sw(shell);

    if (entry != NULL) {
        return serial_state_reached(entry, state);
    }
    return 0;
}

void serial_erase(_SHELL_W* shell) {
    ShellSerialEntry* entry = get_shell_serial_sw(shell);

    if (entry != NULL) {
        serial_entry_clear(entry);
    }
}

extern "C" void niku_entry_clear(ShellNikuEntry* entry) {
    entry->shell_0x00 = 0;
    entry->mode_0x04 = 0;
    entry->player_0x05 = 0xFF;
    entry->seq_0x06 = 0;
    entry->state_0x07 = 0;
    entry->value_0x08 = 0xFF;
}

extern "C" void niku_table_init(void) {
    ShellNikuEntry* entry = shell_pool.niku_0x32C;
    u32 i = 0;

    do {
        niku_entry_clear(entry);
        i++;
        entry++;
    } while (i < SHELL_NIKU_MAX);
    shell_pool.niku_next_0x3BC[0] = 0;
    shell_pool.niku_next_0x3BC[1] = 0;
    shell_pool.niku_next_0x3BC[2] = 0;
    shell_pool.niku_next_0x3BC[3] = 0;
}

extern "C" ShellNikuEntry* niku_entry_at(u8 index) {
    return &shell_pool.niku_0x32C[index];
}

ShellNikuEntry* get_niku_info_sw(_SHELL_W* shell) {
    u32 i = 0;
    ShellNikuEntry* entry;

    do {
        entry = niku_entry_at(i);
        if (entry->state_0x07 != 0 && entry->shell_0x00 == shell) {
            return entry;
        }
        i++;
    } while (i < SHELL_NIKU_MAX);
    return NULL;
}

extern "C" ShellNikuEntry* niku_find(u8 player, u8 seq) {
    u32 i = 0;
    ShellNikuEntry* entry;

    do {
        entry = niku_entry_at(i);
        if (entry->state_0x07 != 0 && entry->player_0x05 == player && entry->seq_0x06 == seq) {
            return entry;
        }
        i++;
    } while (i < SHELL_NIKU_MAX);
    return NULL;
}

ShellNikuEntry* niku_set(_SHELL_W* shell, u8 mode, u8 player) {
    ShellPool* pool = &shell_pool;
    u32 i = 0;
    ShellNikuEntry* entry;

    do {
        entry = niku_entry_at(i);
        if (entry->state_0x07 == 0) {
            entry->shell_0x00 = shell;
            entry->mode_0x04 = mode;
            entry->player_0x05 = player;
            entry->seq_0x06 = pool->niku_next_0x3BC[player]++;
            entry->state_0x07 = 1;
            return entry;
        }
        i++;
    } while (i < SHELL_NIKU_MAX);
    return NULL;
}

extern "C" void niku_state_set(ShellNikuEntry* entry, u8 state) {
    entry->state_0x07 = state;
}

s32 niku_st_change_sw(_SHELL_W* shell, u8 state) {
    ShellNikuEntry* entry = get_niku_info_sw(shell);

    if (entry != NULL) {
        niku_state_set(entry, state);
        return 1;
    }
    return 0;
}

extern "C" __declspec(export) s32 niku_st_change_by_key(u8 player, u8 seq, u8 state) {
    ShellNikuEntry* entry = niku_find(player, seq);

    if (entry != NULL) {
        niku_state_set(entry, state);
        return 1;
    }
    return 0;
}

extern "C" void niku_enemy_attach(ShellNikuEntry* entry, _ENEMY_WORK* enemy) {
    entry->value_0x08 = enemy->group;
    niku_state_set(entry, 2);
    ShellSerialEntry* serial = get_shell_serial_sw(entry->shell_0x00);
    if (serial != NULL) {
        serial_state_set_word_net(serial, enemy->field_0x01A);
    }
}

extern "C" u32 niku_enemy_serial_matches(ShellNikuEntry* entry, _ENEMY_WORK* enemy) {
    ShellSerialEntry* serial;

    if (entry == NULL) {
        return 0;
    }
    serial = get_shell_serial_sw(entry->shell_0x00);
    if (serial != NULL) {
        if (serial->state_0x07 == 4 && serial->word_0x08 == enemy->field_0x01A) {
            return 1;
        }
        return 0;
    }
    return 1;
}

void niku_erase(_SHELL_W* shell) {
    ShellNikuEntry* entry = get_niku_info_sw(shell);

    if (entry != NULL) {
        niku_entry_clear(entry);
    }
}

extern "C" void shell_pool_static_init(void) {
    __construct_array(shell_work_tbl, (void*)shell_work_construct, 0, sizeof(_SHELL_W), SHELL_WORK_MAX);
    __construct_array(shell_chara_heap_tbl, (void*)shell_chara_heap_construct, 0, sizeof(ShellCharaHeap),
                      SHELL_WORK_MAX);
}

/* The `.ctors` word (0x8056F374) the split assigns to this unit - it points at the static initialiser. */
__declspec(section ".ctors") void* const shell_pool_ctor = (void*)shell_pool_static_init;

extern "C" ShellCharaHeap* shell_chara_heap_construct(ShellCharaHeap* self) {
    mhchar_construct((MHchar*)self->chara_0x04);
    return self;
}

extern "C" _SHELL_W* shell_work_construct(_SHELL_W* self) {
    VEC3_ctor(&self->pos_0x18);
    VEC3_ctor(&self->vec_0x30);
    VEC3_ctor(&self->vel_0x3C);
    VEC3_ctor(&self->vel_0x48);
    return self;
}

/* The pool's storage, defined after every user (the compiler addresses a same-file object relative to
 * one section base; the retail code loads each symbol separately, i.e. it saw the definitions last). */
ShellPool shell_pool;
_SHELL_W shell_work_tbl[SHELL_WORK_MAX];
ShellCharaHeap shell_chara_heap_tbl[SHELL_WORK_MAX];
ShellBlockPool shell_block_pool;
