/* Pl/pl_act_class3.cpp - the player act-state handlers of weapon class 3, `.text` 0x8034D2B0..0x8034F138
 * (24 functions / 7816 B), with its extab 0x800170F4..0x800171A4 (22 records), extabindex
 * 0x800367D4..0x800368DC (22 x 12 B), `.data` 0x805E9248..0x805EBBE0 (the act tables), `.sdata`
 * 0x80793308..0x80793330 (five 8-byte table-pair words) and `.sdata2` 0x8079B3A8..0x8079B3C8 (the
 * unit's float pool).  Re-cut out of `enemy/em024_ai.cpp` (2026-09-29): that unit's range
 * 0x8034C1D0..0x80358624 was three translation units, and this is the second of them.  No body is
 * written yet; this file is the registration and the evidence.
 *
 * WHAT IT IS.  Every function drives the player work `_PLW`: `Pl_act_set_motion`,
 * `Pl_act_set_step_table`, `Pl_chr_set_attr_default`, `Pl_motion_end_ck`, `Pl_frame_check`,
 * `Pl_master_ck`, `Pl_Skill_ck`, `pl_act_enter`, `pl_act_arm_flags`, and it stores `_PLW::act_step_0x05`
 * and `_PLW::field_0x565`.  The band ends with `fn_8034EE18` (0x320 B), the class's dispatcher: a
 * switch on `_PLW::act_no` (`act_no <= 0x43`, `jumptable_805E9AD8`, 0x44 entries) whose cases tail-call
 * the handlers above with a (variant, arg) pair.  It is called only from `Pl/fn_80258FCC.cpp`'s
 * `fn_8025DE38` (`b fn_8034EE18` at 0x8025DEB0), whose `jumptable_805C5648` selects one dispatcher
 * per `_PLW::field_0x002` value: 0 `fn_802829B4`, 1 `fn_802840DC`, 2 `fn_80286DF8`, **3
 * `fn_8034EE18`**, 4-6 `fn_80288848`, 7 `fn_8028B330`, 8 `fn_80334758`.  The other dispatchers sit in
 * their own registered `Pl` units, so this is the class-3 band.
 *
 * MODULE AND NAME (evidence order).  1. No `__FILE__` string is reachable from the range.  2. The
 * dump answers `zz_` placeholders.  3. Module `Pl`: every handler takes `_PLW*`, the callees are the
 * `Pl` siblings' and the dispatcher is a `Pl/fn_80258FCC` jump-table entry.  4. GUESS: the file name.
 * `_PLW::field_0x002` is the selector value 3; its meaning (the weapon type is the likely one - the
 * three consecutive values 4-6 share one dispatcher, the shape of three ranged weapons) is not
 * proven, so the name says `class3`, not a weapon.
 *
 * SEAMS.  Left edge 0x8034D2B0: the `extabindex` run breaks there (`fn_8034CDDC` is the last menu
 * record, `fn_8034D2B0` the first player one), the `.sdata2` pool run breaks (0x8079B3A0 is
 * the menu unit's last word, 0x8079B3A8 the first `_PLW` constant) and the callee set changes.
 * Right edge 0x8034F138: `fn_8034F138` reads `_ENEMY_WORK::action`/`state_sub` and its only caller
 * (`camera/fn_802B5C58.cpp`, `fn_802BC564`) gates it on the enemy id byte == 0x18, so it opens the
 * em024 unit; the first enemy `extabindex` record is `fn_8034F164` and `.sdata2` 0x8079B3C8 is
 * the enemy pool's first word.
 *
 * DATA.  The `.data` run 0x805E9248..0x805EBBE0 is contiguous from `menu/menu_note.cpp`'s
 * jump table (ends 0x805E9248) to `em024_prog_tbl` (0x805EBBE0), and every label in it is read by
 * this band or by a `Pl` unit through it: the two `.sdata` pointers `lbl_80793308` (-> 0x805E9248,
 * 0x805E9298), `lbl_80793310`..`lbl_80793328` point at the act tables, `lbl_805E92E8` is loaded by
 * `fn_8034D2B0`, `jumptable_805E9AD8` by the dispatcher.  CAVEAT: seven 0x9C tables
 * (0x805EA594..0x805EA93C, read by `Pl/fn_802489D4`), 0x805EB46C..0x805EB7D4 (`fn_80249A84`),
 * 0x805EB870/0x805EBA54 (`fn_802771A0`) and four 0xB0 tables (`fn_80255388`, `fn_802561D8`,
 * `fn_80255C60`, `fn_8025637C`) are read only from other `Pl` units, never from this band's
 * text: they may be a separate data-only object that sits between this band and the em024 unit
 * in link order.  The claim keeps them here because one contiguous run cannot be split without an
 * owner boundary, and `datagap.py --unit` is the measurement that would show it.
 */

#include "types.h"
