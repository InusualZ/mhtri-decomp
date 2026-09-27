/* menu/multi_result.cpp - the multiplayer-result screen's box cursor band, plus the enemy
 * action/substate dispatchers that share its address range.
 *
 * `.text` 0x8039D278..0x803A3A50 (80 functions, 0x67D8 B), registered whole as the proposal
 * `8039D278` asked.  The seam is UNPROVEN and the range is *not* one translation unit: it is a
 * sequence of objects the linker placed next to each other, so this file reconstructs the functions
 * whose own evidence is complete and leaves the rest to the seam re-draw.
 *
 * SEAM EVIDENCE (measured, 2026-09-27):
 *   - the left edge 0x8039D278 is the cap `menu/menu_result.cpp` was registered at, and the two
 *     bands share a record layout: that unit's `fn_8039D110`/`fn_8039D0C8` iterate the same
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
 * RESIDUAL: 15 of the 80 functions are written - 14 of them at 100.0 % and `multi_box_rem_exist_ck`
 * at 96.27 % (2 instructions short: the target keeps a *dead* loop counter, see that body's note).
 * `.text` 1160 B of 26584 B.  The rest are blocked by *other units' unrenamed
 * symbols*, which a batch may not leave behind as new rule-7 findings: e.g. `fn_8039DBC8` (0x780 B)
 * needs `fn_8004D0E8`/`fn_8004F3B4`/`fn_802A8EFC`, `fn_8039E348` (0x284 B) needs `fn_80058F08`/
 * `fn_80047074`/`fn_803980F0`, the enemy band's bodies need `fn_80127F48`/`fn_80130478`/
 * `fn_8012F5B8`/`fn_8012F93C` (456-782 reference sites repo-wide, so a rename is not this lane's),
 * and `fn_8039D944` (0x1B8 B, understood) needs `get_vsUser_work` declared by its owner,
 * `fn_8004CAD8.cpp`.  The dispatchers below are written and the handlers they tail-call are
 * declared (their map rows are renamed in this change), so the next pass can fill them in place.
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
