/*
 * menu/multi_result.cpp - the multiplayer-result screen's box cursor band (`multi_box_*` on `_multi_result_work`), then
 *   the enemy action/substate dispatchers and a quest/game-mode band that share its address range.  C++ (the two box
 *   helpers carry the original manglings).
 * RANGE. .text 0x8039D278-0x803A3A50 (80 functions); extab, extabindex, .rodata 0x80570BA0-0x80570C20, .data
 *   0x805F1708-0x805F2038, .sdata 0x80793520-0x80793530, .sbss 0x80794C08-0x80794C18, .sdata2 0x8079C330-0x8079C448.
 *   The range is a sequence of objects, not one TU: the box band 0x8039D278-0x8039E5CC, an `_ENEMY_WORK` action band
 *   0x8039E5CC-0x803A11D4 (40 functions sharing the `.sdata2` run 0x8079C330-0x8079C438) and a quest band.  The dump's
 *   SDK names inside the range are linker-folded duplicates of tiny bodies, not evidence.  The left edge is `menu/menu_result.cpp`'s cap, and that unit's
 *   `q_result_phase_enter`/`fn_8039D0C8` walk the same +0x33DC array of 0x18-byte records as `multi_box_records_step`.
 * FLAGS. `cflags_menu` (configure.py).
 * NAMES. Module `menu`: the head is the multiplayer result screen, between `menu/menu_result.cpp` and the `menu/*` band,
 *   and its callees are the menu library's (`get_qResult_work`, `q_result_phase_is_2/3`, `GetItemData`).  No `__FILE__`
 *   string reaches the range, so the file name and every function name but the two `multi_box_*` manglings are GUESSes
 *   from the bodies.
 * RESIDUALS. 56 rows unwritten (objdiff scores them zero): 0x8039D5B0-0x8039D944, 0x8039DBC8-0x8039E714,
 *   0x8039E718-0x8039E7CC, 0x8039E878-0x8039EAA0, 0x8039EBD0-0x8039ECA8, 0x8039ED24-0x8039EDF0,
 *   0x8039EED0-0x8039F380, 0x8039F3E4-0x8039F758, 0x8039F79C-0x8039FB5C, 0x8039FC7C-0x803A1108,
 *   0x803A1110-0x803A11D4, 0x803A1220-0x803A1680, 0x803A168C-0x803A3A50.  Known needs:
 *  - the enemy band's `.sdata2` floats (0x8079C330-0x8079C438, dump names `FLOAT_<addr>` only) are motion parameters of
 *    `em_action1_sub0/1/2/7`, `em_action2_sub0..5`, `em_action3_sub0/2`, `em_action6_sub0/1`, `fn_8039E5CC`, `fn_803A1110`;
 *  - `fn_8039E68C`, `fn_8039E718` and `em_action1_sub5` write `_ENEMY_WORK` +0x328/+0x329 as two bytes (two `stb`);
 *  - `multi_box_result_step`'s object (+0x150 is the screen) is typed by no registered caller;
 *  - `fn_8039DBC8` (the save/VS-wpad step) needs `fn_8004F3B4` and callees outside the range.
 *   The 3 partial rows:
 *  - `multi_box_rem_exist_ck`: 2 instructions short, the target keeps a dead loop counter;
 *  - `multi_box_phase_ck`: the ready mask in r31 against retail's r30 (playbook 22);
 *  - `multi_box_grid_clear`: `items`/`i` mirrored and a `mullw` operand order (both spellings measured).
 *   flipcheck: `.rodata` (0x80), `.sdata` (0x10) and `.sbss` (0x10) claimed but not emitted; short `.text` 0xB18 of
 *   0x67D8, extab 0x60 of 0x1F0, extabindex 0x90 of 0x2E8, `.data` 0x80 of 0x930 (the three switch tables of
 *   `em_action1_dispatch`, `em_action2_dispatch`, `em_action_dispatch`), `.sdata2` 0x8 of 0x118; the bytes of all five
 *   differ; the `.sdata`/`.sdata2` pools are partial (a candidate fold with `lobby/lb_quest_screen`).
 * SHAPES. `multi_box_phase_ck` declares the mask, index and count at function scope and leaves its loop with a `break`
 *   (a second constant return makes MWCC if-convert the mask test into a branchless `srwi`).
 */

#include "types.h"
#include "menu/multi_result.h"
#include "menu/menu_result.h"    /* `q_result_phase_enter` (rule 2: its owner's header) */
#include "menu/menu_item.h"      /* `GetItemData`/`ItemDataRecord` */
#include "fn_8004CAD8.h"         /* `get_vsUser_work`/`score_add_clamped`/_vs_user_data */
#include "fn_80056F24.h"         /* `system_copy_filter_clear` */
#include "unsplit/enemy.h"   /* the enemy band's helpers (rule 2: their owners' band header) */
#include "enemy/fn_8012BDF4.h" /* `em_busy_set` */

/* The box band's phase latch: enter the phase the screen names, then step the sub-state on.  Cases 0
 * and 1 enter the same phase - the target keeps the two bodies separately, so the source does too. */
void multi_box_phase_apply(QResultScreen* self) {
    switch (self->phase) {
    case 0:
        q_result_phase_enter(self, 1);
        self->sub_state++;
        break;
    case 1:
        q_result_phase_enter(self, 1);
        self->sub_state++;
        break;
    case 5:
        system_copy_filter_clear();
        q_result_phase_enter(self, 5);
        self->sub_state++;
        break;
    case 8:
        q_result_phase_enter(self, 8);
        self->sub_state++;
        break;
    case 9:
        q_result_phase_enter(self, 9);
        self->sub_state++;
        break;
    }
}

/* Returns the box grid slot the cursor is on: `x + y * width` of the record's own cursor. */
u16 multi_box_cursor_index(_multi_result_work* box) {
    return (u16)(box->cursor_x + box->cursor_y * box->box_w);
}

/* The grid slot the cursor is on: the player's own box when the record's player number is 0, the
 * shared one otherwise. */
MultiResultBoxItem* multi_box_cursor_item_get(_multi_result_work* box) {
    MultiResultBoxGrids* work = box->work;
    MultiResultBoxItem* items = box->player_no == 0 ? work->my_box : work->other_box;

    return &items[multi_box_cursor_index(box)];
}

/* Whether the player's box still has anything in it: 1 as soon as one of the 16 slots carries an
 * item with a positive count. */
u32 multi_box_rem_exist_ck(_multi_result_work* box) {
    MultiResultBoxGrids* work = box->work;
    MultiResultBoxItem* items = box->player_no == 0 ? work->my_box : work->other_box;
    u32 i;

    if (items != NULL) {
        /* Two groups of eight slots, the eight checks written out: retail keeps one pointer walking
         * 0x20 B per group with fixed offsets 0..0x1C (an inner `for` over eight slots makes MWCC index
         * the second group through a second pointer and costs the function 4 B).  RESIDUAL: the
         * target also keeps a *dead* loop counter - `li r3, 0` plus `addi r3, r3, 7` per group, which
         * also puts the walking pointer in r4 - and MWCC eliminates it from every spelling tried
         * (for-increment, body increment, hoisted init), so this function is 2 instructions short
         * (316 B vs 324 B, 96.27 %). */
        for (i = 0; i < 14; i += 7) {
            if (items[0].item_id != 0 && items[0].count > 0) return 1;
            if (items[1].item_id != 0 && items[1].count > 0) return 1;
            if (items[2].item_id != 0 && items[2].count > 0) return 1;
            if (items[3].item_id != 0 && items[3].count > 0) return 1;
            if (items[4].item_id != 0 && items[4].count > 0) return 1;
            if (items[5].item_id != 0 && items[5].count > 0) return 1;
            if (items[6].item_id != 0 && items[6].count > 0) return 1;
            if (items[7].item_id != 0 && items[7].count > 0) return 1;
            items += 8;
        }
    }
    return 0;
}

/* Credits every item the player's box still holds into the VS user block's point counter, then
 * empties the 16 slots.  The `items != NULL` guard is retail's own (the grids hang off `work`). */
void multi_box_grid_clear(_multi_result_work* box, _vs_user_data* user) {
    MultiResultBoxGrids* work = box->work;
    MultiResultBoxItem* items = box->player_no == 0 ? work->my_box : work->other_box;

    if (items != NULL) {
        u32 i;

        for (i = 0; i < 16; i++) {
            if (items[i].item_id != 0 && items[i].count > 0) {
                score_add_clamped(GetItemData(items[i].item_id)->field_0x010 * items[i].count,
                                  &user->point_0x18);
                items[i].item_id = 0;
                items[i].count = 0;
            }
        }
    }
}

/* Whether the screen may enter the phase `mode` names: 1 is refused while the quest result is still
 * showing, and 8 waits until every player has seven of the eight slots settled. */
u32 multi_box_phase_ck(QResultScreen* self, u8 mode) {
    u16 mask = 0;
    u8 n = 0;
    s32 i;

    switch (mode) {
    case 0:
        return 0;
    case 1:
        if (q_result_phase_is_2(self) == 1 || q_result_phase_is_3(self) == 1) {
            return 0;
        }
        break;
    case 8: {
        s32 i;

        for (i = 0; i < self->field_0x0007; i++) {
            _vs_user_data* user = get_vsUser_work(i);

            mask |= user->ready_mask_0xBC;
            n = 0;
            if (user->slot_a_0x6C[0] != 0 || user->slot_b_0x94[0] != 0) n++;
            if (user->slot_a_0x6C[1] != 0 || user->slot_b_0x94[1] != 0) n++;
            if (user->slot_a_0x6C[2] != 0 || user->slot_b_0x94[2] != 0) n++;
            if (user->slot_a_0x6C[3] != 0 || user->slot_b_0x94[3] != 0) n++;
            if (user->slot_a_0x6C[4] != 0 || user->slot_b_0x94[4] != 0) n++;
            if (user->slot_a_0x6C[5] != 0 || user->slot_b_0x94[5] != 0) n++;
            if (user->slot_a_0x6C[6] != 0 || user->slot_b_0x94[6] != 0) n++;
            if (n >= 7) break;
        }
        if (n >= 7) {
            if ((mask & 0x380) != 0x380) {
                break;
            }
            return 0;
        }
        return 0;
    }
    }
    return 1;
}

/* Advances the screen's phase: each phase names the next one, and the phase only latches once the
 * box band says the screen is ready for it.  Returns 1 once a phase was latched. */
u32 multi_box_phase_step(QResultScreen* self, u8 mode) {
    u32 next;

    for (;;) {
        switch (mode) {
        case 0:
            if (q_result_phase_is_2(self) == 1 || q_result_phase_is_3(self) == 1) {
                next = 9;
            } else {
                next = 1;
            }
            break;
        case 1:
            next = 5;
            break;
        case 5:
            next = 8;
            break;
        case 8:
            next = 9;
            break;
        default:
            return 0;
        }
        if (multi_box_phase_ck(self, (u8)next) == 0) {
            mode = (u8)next;
            continue;
        }
        self->phase = next;
        return 1;
    }
}

/* Runs the handler for the enemy's current action id (only the ids the range implements). */
void em_action_dispatch(struct _ENEMY_WORK* self) {
    switch (self->action) {
    case 0:
        em_action0_dispatch(self);
        break;
    case 1:
        em_action1_dispatch(self);
        break;
    case 2:
        em_action2_dispatch(self);
        break;
    case 7:
        em_action3_dispatch(self);
        break;
    case 10:
        em_action4_dispatch(self);
        break;
    case 11:
        em_action5_dispatch(self);
        break;
    case 12:
        em_action6_dispatch(self);
        break;
    case 13:
        em_action7_dispatch(self);
        break;
    }
}

/* Action 0's sub-state handler. */
void em_action0_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_action0_step(self);
        break;
    case 1:
        em_action0_step(self);
        break;
    case 2:
        em_action0_step(self);
        break;
    }
}

/* Action 1's sub-state handlers. */
void em_action1_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_action1_sub0(self);
        break;
    case 1:
        em_action1_sub1(self);
        break;
    case 2:
        em_action1_sub2(self, 0);
        break;
    case 3:
        em_action1_sub3(self);
        break;
    case 4:
        em_action1_sub4(self);
        break;
    case 5:
        em_action1_sub5(self);
        break;
    case 6:
        em_action1_sub6(self);
        break;
    case 7:
        em_action1_sub7(self);
        break;
    case 8:
        em_action1_sub2(self, 1);
        break;
    case 9:
        em_action1_sub8(self);
        break;
    }
}

/* Action 2's sub-state handlers. */
void em_action2_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_action2_sub0(self, 0.0f);
        break;
    case 1:
        em_action2_sub1(self, 0);
        break;
    case 2:
        em_action2_sub2(self, 0);
        break;
    case 3:
        em_action2_sub3(self);
        break;
    case 4:
        em_action2_sub4(self, 0);
        break;
    case 5:
        em_action2_sub4(self, 1);
        break;
    case 6:
        em_action2_sub5(self, -400.0f);
        break;
    case 7:
        em_action2_sub1(self, 1);
        break;
    }
}

/* Action 3's sub-state handlers. */
void em_action3_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_action3_sub0(self, 0);
        break;
    case 1:
        em_action3_sub1(self);
        break;
    case 2:
        em_action3_sub2(self);
        break;
    case 3:
        em_action3_sub0(self, 1);
        break;
    }
}

/* Action 6's sub-state handlers. */
void em_action6_dispatch(struct _ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 0:
        em_action6_sub0(self);
        break;
    case 1:
        em_action6_sub1(self);
        break;
    }
}

/* Action 0's per-frame step: arm the motion once, then finish the action when the motion ends. */
void em_action0_step(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set_ck(self, 1, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 1's sub-state 3: the two-stage motion, the second stage starting when the first ends. */
void em_action1_sub3(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 23, 20, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            self->state++;
            em_mot_set(self, 24, 20, 0);
        }
        break;
    case 2:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 1's sub-state 4: the single-stage motion. */
void em_action1_sub4(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 25, 10, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 1's sub-state 6: the single-stage motion, with the busy flag left to sub-state 8. */
void em_action1_sub6(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 29, 16, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 1's sub-state 8: the record stays busy for the whole step, which runs the short motion. */
void em_action1_sub8(struct _ENEMY_WORK* self) {
    em_busy_set(self);
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 12, 4, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            em_action_finish(self);
        }
        break;
    }
}

/* Action 7's step: after the motion, hand the record on when its area already holds a team-19
 * enemy, otherwise finish the action. */
void em_action7_step(struct _ENEMY_WORK* self) {
    switch (self->state) {
    case 0:
        self->state++;
        em_move_mode_set(self, 0);
        em_mot_set(self, 29, 16, 0);
        break;
    case 1:
        if (em_mot_end_ck(self) == 1) {
            if (em_area_team_ck(self->area_no) == 1) {
                em_state_set(self, 13, 0);
            } else {
                em_action_finish(self);
            }
        }
        break;
    }
}

/* Action 7's single step, run while the sub-state is still 0. */
void em_action7_dispatch(struct _ENEMY_WORK* self) {
    if (self->state_sub == 0) {
        em_action7_step(self);
    }
}

/* Sets the flow object's sub-state to 1 (called from the game-mode dispatcher's state machine). */
void game_mode_sub_state_set1(GameModeTask* self) {
    self->sub_state = 1;
}

/* Part 0 of the enemy's damage table is untouched. */
u32 em_parts_damage0_ck(struct _ENEMY_WORK* self, u8 part) {
    if (part == 0) {
        if ((em_parts_damage_level_get(self, 0) & 1) == 0) {
            return 1;
        }
    }
    return 0;
}

/* An empty stub the range holds (the runtime dump's name for it is a junk duplicate). */
void em_action_nop(void) {
}

/* A `return 0` stub the range holds (the runtime dump's name for it is a junk duplicate). */
s32 em_action_ret0(void) {
    return 0;
}
