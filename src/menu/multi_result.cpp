/* menu/multi_result.cpp - the multiplayer-result screen's box cursor band, plus the enemy
 * action/substate dispatchers that share its address range.
 *
 * `.text` 0x8039D278..0x803A3A50 (80 functions, 0x67D8 B), registered whole as the proposal
 * `8039D278` asked.  The seam is UNPROVEN and the range is *not* one translation unit: it is a
 * sequence of objects the linker placed next to each other, so this file reconstructs the functions
 * whose own evidence is complete and leaves the rest to the seam re-draw.
 *
 * SEAM EVIDENCE (measured):
 *   - the left edge 0x8039D278 is the cap `menu/menu_result.cpp` was registered at, and the two
 *     bands share a record layout: that unit's `q_result_phase_enter`/`fn_8039D0C8` iterate the same
 *     +0x33DC array of 0x18-byte records that `multi_box_records_step` walks, so the real boundary
 *     may sit further down.  Nothing in the range carries a `__FILE__` string (the only menu source
 *     names in the DOL - `menu_item.cpp`, `menu_note.cpp`, `menu_placeinfo.cpp`, `arenatask.cpp`,
 *     `Hmenu_message.cpp`, ... - are all referenced from other bands).
 *   - three pool/call clusters are visible inside the range: the box band 0x8039D278..0x8039E5CC
 *     (this file's `multi_box_*`), an `_ENEMY_WORK` action band 0x8039E5CC..0x803A11D4 (40
 *     functions sharing the `.sdata2` run 0x8079C330..0x8079C438 - `tudiscover at 0x8039FD4C`), and
 *     a quest/game-mode band above it.
 *   - SDK objects are interleaved with them: the runtime dump names `DBClose` (0x8039E714, 4 B) and
 *     `gdev_cc_shutdown` (0x803A1108, 8 B) inside the enemy band and `GoalOverlay::SceneCreated`
 *     (0x803A1680) / `homebutton::MotorCallback` (0x803A26A8) in the quest band, i.e. objects from
 *     other libraries sit between the game ones - no single seam can cover them.
 *
 * NAME: module `menu` (evidence class 3) - the range's head is the multiplayer result screen and its
 * neighbours are `menu/menu_result.cpp` below and the `menu/*` band above; the two symbols the
 * original compiler mangled are the box helpers on `_multi_result_work`, and every callee of the
 * head band is the menu library's (`get_qResult_work`, `q_result_phase_is_2/3`, `GetItemData`).
 * The file name is a GUESS for the same reason the seam is unproven: the band's own `__FILE__`
 * string does not exist.  Every function name in this file is a GUESS derived from its body - the
 * runtime dump answers `zz_` for all of them: its only in-range names are the two `multi_box_*`
 * manglings plus a handful of *junk* duplicates (`DBClose` at 295 addresses, `gdev_cc_shutdown` at
 * 133, `GoalOverlay::SceneCreated` at 72), which are not evidence.
 *
 * FLAGS: `menu`'s `cflags_menu` (`-O3 -inline noauto -opt nopeephole -Cpp_exceptions on`,
 * Wii/1.3) - the band is C++ (its two defining symbols carry real manglings) and its box bodies
 * keep unfused narrow loads.
 *
 * RESIDUAL: 24 of the 80 functions are written - 21 of them at 100.0 %, `multi_box_phase_ck` at
 * 99.59 %, `multi_box_grid_clear` at 97.40 % and `multi_box_rem_exist_ck` at 96.27 % (2
 * instructions short: the target keeps a *dead* loop counter, see that body's note).  `.text`
 * 2840 B of 26584 B.  The two colouring residuals are the allocator's, not the source's:
 *   - `multi_box_phase_ck` is 440/440 B and differs only in which callee-saved register holds the
 *     ready mask (`r30` retail, `r31` ours) over the loop index.  Declaring the mask/index/n at
 *     function scope is what closed the 4-byte gap (a `break` - not a second constant return -
 *     stops MWCC if-converting the mask test into a branchless `srwi`); the register pair itself
 *     did not move for a swapped declaration order, so it is a ceiling (playbook 22).
 *   - `multi_box_grid_clear` is 180/180 B and differs the same way (`items`/`i` mirrored) plus a
 *     commutative `mullw` operand order; both spellings of the product measure 97.40 %.
 * The functions still unwritten are blocked by evidence, not by effort:
 *   - the enemy band's shared `.sdata2` run 0x8079C330..0x8079C438: 65 four-byte floats whose only
 *     dump name is `FLOAT_<addr>`.  ~15 in-range bodies (`em_action1_sub0/1/2/7`, `em_action2_sub0..5`,
 *     `em_action3_sub0/2`, `em_action6_sub0/1`, `fn_8039E5CC`, `fn_803A1110`, ...) take them as motion
 *     parameters; naming them from the value alone would invent semantics the binary does not carry,
 *     so they stay unclaimed until the enemy band's own unit registers and claims the run.
 *   - the `+0x328` union in `enemy/ENEMY_WORK.h` has no byte view: `fn_8039E68C`, `fn_8039E718`
 *     and `em_action1_sub5` all write +0x328/+0x329 as *separate bytes* (the target emits two `stb`
 *     where the existing `s16 field_0x328` view would emit one `sth`), so they need a named byte-pair
 *     union member added by whoever owns that header next.
 *   - `multi_box_cursor_clamp` (0x8C) and `multi_box_grid_step` (0x308) read the pad key words at
 *     `+0x2C4`/`+0x2D4` of `Psw[player_no]`; `PlayerPad` is a *local* type in `src/mh3_pad.cpp`, and
 *     reaching it from here is the rule-1 move (two users) that must be measured against `mh3_pad`.
 *   - `multi_box_result_step` (0x284) takes an object whose `+0x150` is the screen - the caller that
 *     names that type is not in a registered unit, so its parameter has no evidence yet.
 *   - `fn_8039DBC8` (0x780) is the save/VS-wpad band: it needs `fn_8004F3B4`, `ai_npc_reaction_forward` and
 *     `ai_torch_ck`'-style names whose bodies are outside this range.
 * The 24 renames this band needed are in the map (see the previous commit's message); the sweep is
 * complete, so a later pass can write the rest in place.
 *
 * Data (measured, `datagap.py --unit menu/multi_result`): `ours-extra .data 128B, .rela.data 384B,
 * .sdata2 8B`.  The 128 B are the three switch tables the dispatchers own - `.data` 0x805F1774
 * (0x28, `em_action1_dispatch`), 0x805F179C (0x20, `em_action2_dispatch`) and 0x805F1DE0 (0x38,
 * `em_action_dispatch`) - and the 8 B are the two float literals of `em_action2_dispatch`.  They are
 * deliberately NOT claimed in `splits.txt`: the first two tables are one contiguous 0x48 B run, but
 * the third is 0x624 B further on with the enemy band's own `.data` labels in between, so claiming
 * the unit's `.data` means claiming a run it does not own (and a partial `.sdata2` claim is not
 * linkable at all - playbook 23).  The claim belongs to the seam re-draw, once the objects between
 * the two runs are attributed.
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
