/*
 * stage/shell.cpp - the shell pool: the 64 work records of thrown/fired objects (`_SHELL_W`), the 64-byte
 *   work-block allocator and model-slot pool behind them, and the serial/niku sync tables.
 * RANGE. .text 0x802AA6A8-0x802ABD28 (58 functions); extab 0x80013984-0x80013AC4, extabindex
 *   0x800314C4-0x800316A4, .ctors 0x8056F374-0x8056F378 (MWCC's own word for `__sinit_\shell_cpp`), .bss
 *   0x806AD698-0x806B8798 (`shell_pool` 0x3C0, `shell_work_tbl` 64 x 0x10C, `shell_chara_heap_tbl` 64 x 0x168,
 *   `shell_block_pool` 0x1040), .sdata2 0x8079A400-0x8079A410 (the claim's last word is the next TU's 8-byte
 *   alignment pad; ours emits 12 B and the link pads it, trial flip OK).
 *   Seams: one `.ctors` word per TU (0x8056F374 this TU's `__sinit`, 0x8056F378 `Pl/pl_yure.cpp`'s); the left edge
 *   `shell_work_init` memsets the pool and calls `serial_table_init`/`niku_table_init` (callers `Pl/fn_80288CEC.cpp`,
 *   `enemy/em_pop.cpp`); the right edge is `__sinit_\shell_cpp` plus its two element constructors, after which
 *   `Pl/pl_yure.cpp` starts (own `__sinit`, own data).
 * FLAGS. `cflags_main`; `#pragma peephole off` file-wide (the stage band's deviation, see `stage/stg_w.cpp`; dropping
 *   it costs 33 rows).
 * NAMES. Module and file name are a GUESS (no `__FILE__` string): the records are the projectile work `ef.h` calls
 *   `_SHELL_W`; the address neighbour is `stage/stg_w.cpp`.  GUESS from behaviour: `serial_*` (the network-synced
 *   shell slots: `lb_sub0d_send`, the `isServerSelectState` gate), `niku_*` (a second 12-row table),
 *   `player_0x05`/`seq_0x06` (the owner's player number and that player's running sequence number),
 *   `shell_block_*`, `ShellPool`, `ShellCharaHeap`, `ShellSerialEntry`, `ShellNikuEntry`, `ShellBlockPool`.
 *   `shell_set_func_ptr` (`.sbss` 0x80794B60) is NOT defined here: no function of this TU touches `.sbss` but `pRoot`.
 * RESIDUALS.
 *   - `pull_shell_work`: retail's found path jumps from the search loop straight to the claim tail, past the loop's
 *     `return NULL` (a `goto`-shaped exit, idea 217/213 class).  A scratch `goto claimed` measures 100 % at 0x100 B.
 *     The inlined `shell_work_find_free` is the best goto-free shape (+4 B): its found path branches straight to
 *     the join, but the join still compares the helper's materialised NULL (`li r30,0; cmpwi r30,0; beq end`
 *     against retail's `li r3,0; b end`).  `tools/mwcc-debugger` dump 03 (after-unreachable-block-deletion, the
 *     first PCode) already holds that compare for every goto-free spelling (flag, pointer flag, `i >= max`
 *     re-test, helper with `if (!shell) return NULL`, claim inside the loop, outer `for (;;)`, one-case
 *     `switch`), and only the `goto` has the direct edge: the frontend's flow graph is the source's, and no later
 *     pass threads it (idea 217).  An inline does not supply the edge: dump 03 lowers every inlined `return X` to
 *     `li rT,X; b <the inline's one exit block>`, laid out in source order, so the whole search+claim as an inline
 *     (claim and `return` inside the loop, wrapper or guard outside) keeps the claim between the test and the
 *     increment (252 B), a `return claim(...)` inline the same, and a BOOL/out-pointer inline or `?:` caller still
 *     compares the materialised constant at the exit join (268-276 B).
 *   - `shell_block_alloc`: retail shares one `return NULL` block between the skip loop's and the run loop's bound
 *     exits (`bge fail` twice, the block after the fill) and keeps the fill out of line after both loops (`beq
 *     fill`).  A scratch `goto fail`/`goto fill` copy measures 100 %.  Goto-free, `break` on both run-loop exits
 *     with the CSE'd `*map` re-test after the loop puts the fill after the loops (+12 B: the re-test's
 *     `cmpwi r0,0; bne`, and the two `return NULL` statements stay two `li r3,0; blr` blocks, so the bound branch
 *     inverts to `blt` back).  Dump 03 shows two `li r3,0` blocks in every goto-free spelling (the inline search
 *     helper with two `return -1` merges them through its return join, then re-tests the result, 336 B); the
 *     single-loop, `i >= max` re-test and inline-fill spellings measure lower.  Inlining gives the multi-level exit
 *     (a find-run inline's found `return` branches from the run loop straight to its exit join, as retail's `beq
 *     fill`) but one exit join for every `return`: each `return NULL` stays its own `li rT,0; b join` block in
 *     dump 03, the fill behind the join re-tests the result (328 B), and the whole body as an inline adds a result
 *     temporary (320 B, lower).
 *   - The +4 B of `pull_shell_work` and the +12 B of `shell_block_alloc` shift every later symbol, so the object's
 *     `.text` is 0x1690 against the claimed 0x1680 and the extabindex size byte of `pull_shell_work` differs.  Every
 *     other row, `.ctors`, extab, extabindex, `.bss` and `.sdata2` link byte-identical (a trial flip of the scratch
 *     goto copy builds main.dol OK).
 *   - `__declspec(export)` on `shell_block_clear_all`, `serial_st_change_by_key`, `niku_st_change_by_key`: nothing in
 *     the link references them (their callers are in RSOs), and the target object force-activates them.
 * SHAPES.
 *   - `_SHELL_W` and `ShellCharaHeap` carry a declared constructor defined out of line after the pool objects
 *     (`__declspec(noinline) inline`), so MWCC emits `__sinit_\shell_cpp` (two `__construct_array` calls, in the
 *     objects' definition order), the anonymous `.ctors` word, and the two constructors after `__sinit` in retail's
 *     order (the deferred list is last-in first-out; ideas 193, 242).  A named `__declspec(section ".ctors")` word
 *     is dead-stripped by the link and shifts the merged table.
 *   - The pool objects are defined after their users, which keeps each symbol's own `lis/addi` (defined first, the
 *     compiler folds them onto one section base).
 *   - A guarded body is an early `if (a == NULL || b == NULL) return;` (`shell_attack_set`, `shell_set_prim`): the
 *     `||`'s last term keeps retail's `beq L; b end` pair (idea 218); the nested `if`s fold it to one `bne`.
 *   - `shell_block_free` computes `first` in two statements (`first = (u32)(a - b); first >>= 6;`) so the difference
 *     and the shift share a register; `shell_block_alloc` fills through the advancing `first_p` pointer after both
 *     loops (both run-loop exits `break`, then `if (*map == 0) break;` out of the outer loop reuses the loaded
 *     byte) and its declaration order (`n, run, i, first, pool, map, first_p`) is the colouring order retail shows.
 *   - `(u32)stage_water_enabled_ck() == 1` keeps retail's `cmplwi` (the owner declares the function `s32`).
 *
 * Inventory and evidence: `python tools/units/ledger.py unit stage/shell.cpp`.
 */

#include "types.h"

#include "Runtime.PPCEABI.H/memset.h"
#include "Network/network_pat_control.h"
#include "menu/hit_attack_list_push.h"
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
#include "sound/mhchar.h" /* mhchar_reset (rule 2) */
#include "sound/sound_job_request.h"
#include "stage/shell.h"
#include "stage/stg_w.h"
#include "unsplit/g3d.h"

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

/* Takes the first inactive work record and counts it as used; NULL when every record is active. */
static inline _SHELL_W* shell_work_find_free(ShellPool* pool) {
    _SHELL_W* shell = pool->work_0x08;
    s32 i;

    for (i = 0; i < pool->work_max_0x04; i++, shell++) {
        if (shell->active_0x00 == 0) {
            pool->work_used_0x0C++;
            return shell;
        }
    }
    return NULL;
}

_SHELL_W* pull_shell_work(u32 work_size) {
    ShellPool* pool = &shell_pool;
    _SHELL_W* shell;

    if (pool->work_used_0x0C >= pool->work_max_0x04) {
        return NULL;
    }
    shell = shell_work_find_free(pool);
    if (shell != NULL) {
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
    }
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
    s32 n;
    s32 run;
    s32 i;
    s32 first;
    ShellPool* pool = &shell_pool;
    u8* map = pool->block_map_0x24;
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
                break;
            }
            map++;
            i++;
            if (i >= pool->block_max_0x1C) {
                return NULL;
            }
        }
        if (*map == 0) {
            break;
        }
    }
    for (n = 0; n < run; n++) {
        *first_p = 1;
        first_p++;
    }
    return pool->blocks_0x20 + first * 64;
}

extern "C" void shell_block_free(u8* blocks, s32 count) {
    u32 first;
    s32 n;
    u8* map;
    ShellPool* pool = &shell_pool;

    first = (u32)(blocks - pool->blocks_0x20);
    first >>= 6;
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
        if (shell->visible_0x01 == 0 || shell->area_0x08 != get_now_areano()) {
            return;
        }
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

void shell_attack_set(_SHELL_W* shell, _HIT_DATA* hit_tbl, _HIT_SIZE_DATA** size_tbl, s32 index,
                      u8 attack_flags, u8 attack_kind) {
    _HIT_W* hit = SHELL_HIT(shell);
    _ENEMY_WORK* enemy;

    if (hit_tbl == NULL || size_tbl == NULL) {
        return;
    }
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
            if ((u32)stage_water_enabled_ck() == 1) {
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

/* The pool's storage, defined after every user (the compiler addresses a same-file object relative to
 * one section base; the retail code loads each symbol separately, i.e. it saw the definitions last).
 * The two constructed arrays make MWCC emit `__sinit_\shell_cpp` (two `__construct_array` calls, in definition
 * order) and the unit's anonymous `.ctors` word. */
ShellPool shell_pool;
_SHELL_W shell_work_tbl[SHELL_WORK_MAX];
ShellCharaHeap shell_chara_heap_tbl[SHELL_WORK_MAX];
ShellBlockPool shell_block_pool;

/* The element constructors, defined out of line after the objects so MWCC emits them after `__sinit` in
 * retail's order (the deferred list is last-in first-out: the heap slot's, then the work record's). */
__declspec(noinline) inline ShellCharaHeap::ShellCharaHeap() {
    mhchar_construct((MHchar*)chara_0x04);
}

__declspec(noinline) inline _SHELL_W::_SHELL_W() {
    VEC3_ctor(&pos_0x18);
    VEC3_ctor(&vec_0x30);
    VEC3_ctor(&vel_0x3C);
    VEC3_ctor(&vel_0x48);
}
