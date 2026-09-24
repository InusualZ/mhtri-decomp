/* enemy/fn_80170FA8.cpp - four `_ENEMY_WORK` action handlers, `.text` 0x80170FA8..0x80171194 (492 B).
 * rule 7 deferred: the symbol map has only fn_XXXXXXXX for this range (checked with symedit: every fn_
 * name this file uses is a bare .text entry in config/RMHE08/symbols.txt)
 *
 * What it is.  Four steps of one enemy action.  `fn_80170FA8`, `fn_80171038` and `fn_801710B4` are the
 * same two-state machine on `state_0x05`: state 0 advances the state and posts the action's message
 * through `fn_80130478` + `fn_8012F62C` (`0xC9`) or `fn_8012F5B8` (`0xE`); state 1 waits on the shared
 * frame checks (`em_frame_check`, `fn_8012F93C`) and then fires the action result (`fn_80128A14`) or
 * the next move (`fn_80127F48`).  `fn_80171130` is the dispatcher: it reads `state_sub` (+0x1E6) and
 * tail-calls one handler per action code, codes 0 and 9 doing nothing.
 *
 * Object: `_ENEMY_WORK`, included from `include/enemy.h` (name evidence: the mangled callee
 * `em_frame_check__FP11_ENEMY_WORKUsff` carries the 11-character type name).  Included, not copied
 * (docs/plan.md 6.5 rule 1).
 *
 * Language: C++.  The one mangled callee is a C++ free function: its map spelling
 * `em_frame_check__FP11_ENEMY_WORKUsff` decodes to `em_frame_check(_ENEMY_WORK*, u16, f32, f32)`, so
 * the source names the owner's real identifier and the front-end emits the map's mangling (rule 9);
 * writing the mangled spelling as the callee would be a rule-9 violation the gate refuses.  The four
 * unit functions stay plain symbols through `extern "C"`, so the map's `fn_XXXXXXXX` stems pair.
 *
 * Data: the unit owns no pool.  `lbl_80797910` (0.0f) and `lbl_80797928` (170.0f) are shared `.sdata2`
 * constants (declared, not defined here).  The `.data` jump table `jumptable_805A83DC` is emitted by the
 * compiler from the dispatch switch; its run 0x805A83DC..0x805A8418 is requested through the outbox's
 * `range` rather than claimed here, because the emitted section is 8-aligned where the retail range is
 * 4-aligned (the preceding `em012_prog_tbl` ends there) - the split warns on that claim, and a data
 * range that does not lay out byte-for-byte is not a claim (docs/plan.md 8.4).
 *
 * Inventory, addresses and sizes: `python tools/units/ledger.py unit enemy/fn_80170FA8.cpp`.
 */

#include "types.h"
#include "enemy.h"
#include "unsplit/enemy.h"

/* The one mangled callee, declared by its owner's real name so the C++ front-end reproduces the map's
 * `em_frame_check__FP11_ENEMY_WORKUsff`.  `include/unsplit/enemy.h` carries the C spelling of the same
 * symbol for the units that stay C (rule 9: a mangled spelling is never the callable identifier). */
u32 em_frame_check(_ENEMY_WORK* self, u16 a, f32 b, f32 c);

/* The shared `.sdata2` pool entries this unit loads (declared only, never defined here). */
extern f32 lbl_80797910; /* 0.0f */
extern f32 lbl_80797928; /* 170.0f */

/* state 0: advance and post action 0xC9.  state 1: wait for the frame check, then fire action 0xD. */
extern "C" void fn_80170FA8(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 = (u8)(self->state_0x05 + 1);
        fn_80130478(self, 0);
        fn_8012F62C(self, 0xC9, 6, 0);
        break;
    case 1:
        if (em_frame_check(self, 1, lbl_80797928, lbl_80797910) == 1U) {
            fn_80128A14(self, 1, 0xD);
        }
        break;
    }
}

/* state 0: advance and post action 0xC9.  state 1: advance the move when the frame check passes. */
extern "C" void fn_80171038(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 = (u8)(self->state_0x05 + 1);
        fn_80130478(self, 0);
        fn_8012F62C(self, 0xC9, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        break;
    }
}

/* state 0: advance and post action 0xE.  state 1: advance the move when the frame check passes. */
extern "C" void fn_801710B4(_ENEMY_WORK* self) {
    switch (self->state_0x05) {
    case 0:
        self->state_0x05 = (u8)(self->state_0x05 + 1);
        fn_80130478(self, 0);
        fn_8012F5B8(self, 0xE, 6, 0);
        break;
    case 1:
        if (fn_8012F93C(self) == 1U) {
            fn_80127F48(self);
        }
        break;
    }
}

/* The action-code dispatcher: `state_sub` 0..14, codes 0 and 9 return without a handler. */
extern "C" void fn_80171130(_ENEMY_WORK* self) {
    switch (self->state_sub) {
    case 1: fn_80170A54(self); break;
    case 2: fn_80170AD0(self); break;
    case 3: fn_80170B4C(self); break;
    case 4: fn_80170C68(self); break;
    case 5: fn_80170D04(self); break;
    case 6: fn_80170D74(self); break;
    case 7: fn_80170DF0(self); break;
    case 8: fn_80170E78(self); break;
    case 10: fn_80170EF4(self, 1); break;
    case 11: fn_80170EF4(self, 0); break;
    case 12: fn_80170FA8(self); break;
    case 13: fn_80171038(self); break;
    case 14: fn_801710B4(self); break;
    case 0:
    case 9: break;
    }
}
