/*
 * Pl/pl_act_class3.cpp - the player act-state handlers of `_PLW::field_0x002` class 3 and their dispatcher
 *   `fn_8034EE18` (no body written yet).
 * RANGE. .text 0x8034D2B0-0x8034F138 (24 functions); .data 0x805E9248-0x805EBBE0 (the act tables), .sdata
 *   0x80793308-0x80793330 (five 8-byte table-pair words), .sdata2 0x8079B3A8-0x8079B3C8, extab, extabindex.
 *   `fn_8034EE18` (0x320 B) switches on `_PLW::act_no` (`act_no <= 0x43`, `jumptable_805E9AD8`) and tail-calls the
 *   handlers with a (variant, arg) pair; its only caller is `Pl/pl_act_step.cpp`'s `fn_8025DE38` (`b fn_8034EE18` at
 *   0x8025DEB0), whose `jumptable_805C5648` picks one dispatcher per `field_0x002`: 0 `fn_802829B4`, 1 `fn_802840DC`,
 *   2 `fn_80286DF8`, 3 `fn_8034EE18`, 4-6 `fn_80288848`, 7 `fn_8028B330`, 8 `Pl_act_frame_dispatch`.
 *   Left seam 0x8034D2B0: the `extabindex` run (`fn_8034CDDC` is the menu unit's last record), the `.sdata2` run
 *   (0x8079B3A0 is the menu unit's last word) and the callee set all break there.  Right seam 0x8034F138:
 *   `em024_action11_state5_ck` reads `_ENEMY_WORK` and its only caller (`camera/camera_main.cpp`'s `camera_kill_cut_start`)
 *   gates it on enemy id 0x18, so it opens `enemy/em024_ai.cpp`.
 *   The `.data` run is contiguous from `menu/menu_note.cpp`'s jump table to `em024_prog_tbl`, but seven 0x9C tables
 *   (0x805EA594-0x805EA93C), 0x805EB46C-0x805EB7D4, 0x805EB870/0x805EBA54 and four 0xB0 tables are read only from
 *   `Pl/pl_act_step.cpp` and `Pl/pl_act.cpp`: they may be a separate data-only object between this band and em024
 *   (`datagap.py --unit` would show it).
 * NAMES. The file name is a GUESS: the selector's meaning is unproven (the weapon type is the likely one - values 4-6
 *   share one dispatcher, the shape of three ranged weapons), so the name says `class3`.
 * RESIDUALS. All 24 functions unwritten (objdiff scores them 0): 0x8034D2B0-0x8034F138.  flipcheck: the object emits
 *   none of the claimed sections.
 */

#include "types.h"
