/*
 * enemy/em019_ai.cpp - the em019 monster-AI file's body, `.text` 0x80378F9C..0x8037EA64
 * (61 functions / 0x5AC8 B) with extab 0x80017C44..0x80017DBC (47 records) and extabindex
 * 0x800378CC..0x80037B00 (47 x 12 B).
 *
 * WHAT IT IS.  Monster-AI code of the em019 program, continued from `enemy/em020_ai.cpp`: every body
 * takes the shared `_ENEMY_WORK` record (`include/enemy/ENEMY_WORK.h`) and drives it through the
 * enemy core API (`em_frame_check`, `get_joint_wpos_em`, `get_em_chg_scale`, `fn_8012F5B8` /
 * `fn_8012F62C` / `fn_8012F93C`) and the game's work blocks (`system_w`, `lobby_w`,
 * `get_move_work_adrs`, `my_player_no`, `Psw`).
 *
 * MODULE AND NAME (brief section 2, evidence order).  1. No `__FILE__` string is reachable from the
 * range (same check as `enemy/em020_ai.cpp`: every relocation target resolves to a program table, a
 * work block, the shared `.sdata2` float pool or a call).  2. `dumpmap.py lookup` answers
 * `zz_<addr>_` everywhere except `em019_prog_tbl`.  3. The module is `enemy` and the file `em019` from
 * the `.data` program table `em019_prog_tbl` (0x805EE518, `scope:global`, size 0x6C), whose
 * entry-point list is this band's own functions (`fn_80379090` +0x00, `fn_80379124` +0x04,
 * `fn_8037946C` +0x08, `fn_8037924C` +0x10, `fn_8037939C` +0x14, `fn_8037F524` +0x24) plus the next
 * file's head (`fn_8037F940`, `fn_80382310`, ... - the same cross-file pattern `em024_prog_tbl`
 * shows for `eft052`), and from the code (`_ENEMY_WORK` field for field, `em_*` callees only).  The
 * name follows the module's `em*` scheme (`em024_ai.cpp`, `em035_prog.cpp`).
 *
 * SEAM (unproven - the range is an `attribute.py` `--max-bytes` run, not a TU boundary).  Left edge:
 * `fn_80378F7C` (the last function of `enemy/em020_ai.cpp`) is called only from 0x8036C284/0x8036C6E8
 * (em020 side) while `fn_80378F9C` (the first function here) is called only from 0x8037939C upward,
 * and the `.data` block boundary is 0x805EE518 - `em019_prog_tbl`, the first `.data` of this file's
 * block.  Right edge: `tudiscover.py at 0x8037E0E8` reports the 48-function match set
 * 0x80379694..0x8037EA64 with a strong seam at 0x8037F940 (`.data` jumptable_805EF4F4 ->
 * jumptable_805EF52C; `.sdata2` lbl_8079BE88 -> lbl_8079BE8C), so this file's real extent is
 * 0x80378F9C..0x8037F940; the brief's 0x8037EA64 cut is filed as a `range` config_request and the
 * tail (0x8037EA64..0x8037F940, 12 functions) is left to its own lane.
 *
 * rule 7 deferred: the names this file *references* in other units are still the map's generated
 * `fn_XXXXXXXX` stems (the enemy core band 0x8012xxxx/0x8013xxxx, the lobby 0x802xxxxx band and the
 * game-root 0x8042xxxx band, checked with `tools/symbols/symedit.py range`), and so are the
 * addresses of its own range that are not reconstructed yet - they are *declared*, never defined, so
 * the map row still names the target.  Every symbol this file DEFINES is named from its own body and
 * renamed in the map with `symedit.py rename`.
 *
 * Sections this unit claims: `.text` 0x80378F9C..0x8037EA64, extab 0x80017C44..0x80017DBC,
 * extabindex 0x800378CC..0x80037B00.
 *
 * Residuals: the range is registered as `NonMatching`; the bodies still unwritten keep the map's
 * `fn_XXXXXXXX` names, and the ones written but not yet byte-identical are listed in the outbox with
 * their first divergence.  Re-measure with `ninja build/RMHE08/report.json` +
 * `python tools/objdiff/symdiff.py -u enemy/em019_ai.cpp <symbol>`.
 */

#include "types.h"
#include "enemy/ENEMY_WORK.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The addresses of this unit's range that are not reconstructed yet (rule 7 deferral above).  They
 * are declared, never defined: the map row still names the target object's symbol. */
void fn_8037983C(struct _ENEMY_WORK* self);
void fn_803799A8(struct _ENEMY_WORK* self);
void fn_80379AC4(struct _ENEMY_WORK* self);
void fn_80379B50(struct _ENEMY_WORK* self);
void fn_80379BD0(struct _ENEMY_WORK* self);
void fn_80379C60(struct _ENEMY_WORK* self);
void fn_80379CDC(struct _ENEMY_WORK* self);
void fn_80379D64(struct _ENEMY_WORK* self);
void fn_8037A848(struct _ENEMY_WORK* self, u32 a);
void fn_8037A964(struct _ENEMY_WORK* self);
void fn_8037AAAC(void);
void fn_8037ACA8(struct _ENEMY_WORK* self, u32 a);
void fn_8037ADE4(struct _ENEMY_WORK* self, u32 a);
void fn_8037A508(struct _ENEMY_WORK* self, u32 a);
void fn_8037A600(struct _ENEMY_WORK* self, u32 a);
void fn_8037A718(struct _ENEMY_WORK* self);
void fn_803794F8(struct _ENEMY_WORK* self);
void fn_80379594(struct _ENEMY_WORK* self);
void fn_80379614(struct _ENEMY_WORK* self);
void fn_80379694(struct _ENEMY_WORK* self);
void fn_8037975C(struct _ENEMY_WORK* self);

/* The em019 action start: the shared action runner's entry wrapper.
 * 0x8037E0D0 */
void em019_action_start(void)
{
    fn_8037AAAC();
}

/* The em019 action start's idle gate: runs the action only from sub-state 0.
 * 0x8037E0D4 */
void em019_action_start_if_idle(struct _ENEMY_WORK* self)
{
    if (self->state_sub == 0) {
        em019_action_start();
    }
}

/* Arms the action's "run" flag at `+0x358`.
 * 0x80379084 */
void em019_action_run_flag_set(struct _ENEMY_WORK* self)
{
    self->field_0x358 = 1;
}

/* The em019 per-motion step dispatcher: `+0x1E6` selects one of the eight step bodies, each of which
 * is tail-called.
 * 0x80379DE4 */
void em019_state_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        fn_8037983C(self);
        return;
    case 1:
        fn_803799A8(self);
        return;
    case 2:
        fn_80379AC4(self);
        return;
    case 3:
        fn_80379B50(self);
        return;
    case 4:
        fn_80379BD0(self);
        return;
    case 5:
        fn_80379C60(self);
        return;
    case 6:
        fn_80379CDC(self);
        return;
    case 7:
        fn_80379D64(self);
        return;
    }
}

/* The em019 battle step dispatcher: `+0x1E6` selects one of the twelve battle step bodies.
 * 0x8037AF08 */
void em019_battle_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        fn_8037A848(self, 0);
        return;
    case 1:
        fn_8037A964(self);
        return;
    case 2:
        fn_8037AAAC();
        return;
    case 3:
        fn_8037ACA8(self, 0);
        return;
    case 4:
        fn_8037ACA8(self, 1);
        return;
    case 5:
        fn_8037ADE4(self, 0);
        return;
    case 6:
        fn_8037ACA8(self, 2);
        return;
    case 7:
        fn_8037A848(self, 1);
        return;
    case 8:
        fn_8037ACA8(self, 3);
        return;
    case 9:
        fn_8037ADE4(self, 1);
        return;
    case 10:
        fn_8037ADE4(self, 2);
        return;
    case 11:
        fn_8037A848(self, 2);
        return;
    }
}

/* The em019 approach step dispatcher: `+0x1E6` picks the approach or the retreat body.
 * 0x8037A7F0 */
void em019_approach_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        fn_8037A508(self, 0);
        break;
    case 1:
        fn_8037A600(self, 0);
        break;
    case 2:
        fn_8037A718(self);
        break;
    case 3:
        fn_8037A600(self, 1);
        break;
    case 4:
        fn_8037A508(self, 1);
        break;
    }
}

/* The em019 motion step dispatcher: `+0x1E6` picks one of the five motion bodies.
 * 0x803797F4 */
void em019_motion_dispatch(struct _ENEMY_WORK* self)
{
    switch (self->state_sub) {
    case 0:
        fn_803794F8(self);
        break;
    case 1:
        fn_80379594(self);
        break;
    case 2:
        fn_80379614(self);
        break;
    case 3:
        fn_80379694(self);
        break;
    case 7:
        fn_8037975C(self);
        break;
    }
}

#ifdef __cplusplus
}
#endif
