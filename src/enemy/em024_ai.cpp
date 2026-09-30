/* enemy/em024_ai.cpp - the em024 monster's AI, `.text` 0x8034F138..0x80358624 (69 functions / 38124 B),
 * with its extab 0x800171A4..0x8001736C (57 records), extabindex 0x800368DC..0x80036B88 (57 x 12 B),
 * `.data` 0x805EBBE0..0x805ED0C0 (`em024_prog_tbl`, its switch jump tables and per-motion program
 * tables), `.sdata` 0x80793330..0x80793338 and `.sdata2` 0x8079B3C8..0x8079B640 (the unit's float
 * pool).  Re-cut (2026-09-29): the original registration 0x8034C1D0..0x80358624 was three
 * translation units - `menu/menu_row.cpp` (0x8034C1D0..0x8034D2B0) and `Pl/pl_act_class3.cpp`
 * (0x8034D2B0..0x8034F138) took the other two, and the 14 bodies this file used to carry moved to
 * `menu/menu_row.cpp` unchanged.  No body is written here yet; the file is the registration and the
 * evidence.
 *
 * WHAT IT IS.  The em024 monster's frames drive the shared `_ENEMY_WORK` record through
 * `em_frame_check`/`em_after_frame_check`/`em_act_ck`/`em_mot_set`/`em_mot_end_ck`/
 * `em_move_mode_set`/`em_action_finish`, read `em_get_mot_no()` and `em_parts_damage_level_get`, set
 * the model's TEV material (`MHchar::setTevKColor`/`move`), request sound and build effect models.
 * The biggest body is `fn_80356664` (0x1A84 B), a switch over `em_get_mot_no()`'s 0x72 motions.
 * The first function, `fn_8034F138` (44 B), is the `action == 0xB && state_sub == 5` predicate that
 * `camera/fn_802B5C58.cpp` (`fn_802BC564`) calls for enemy id 0x18.
 *
 * MODULE AND NAME (evidence order).  1. No `__FILE__` string is reachable from the range.  2. The dump
 * answers `zz_` placeholders.  3. Module `enemy` and monster `em024`: `em024_prog_tbl`
 * (`.data` 0x805EBBE0, 0x70 B) lists seven of the band's entry points (0x8034F334, 0x8034F524,
 * 0x803562B4, 0x8034F410, 0x8034F414, `fn_80356664`, `fn_803580E8`), sits at the head of this
 * unit's own `.data` run next to its jump tables (`jumptable_805EBD78`, `jumptable_805EC1C0`,
 * `jumptable_805ECE88`, `jumptable_805ECEBC`, `jumptable_805ECEF4`), and 0x18 is the enemy id
 * `fn_8034F138`'s caller tests.  MARKED GUESS: the `em024` file name (the id 0x18 == 24 agrees).
 *
 * ONE TU.  The float `lbl_8079B3CC` is loaded by 28 functions from `fn_8034F164` to `fn_803580E8` and
 * by nothing outside them (`tools/units/callers.py 0x8079B3CC`); a `.sdata2` pool entry is per-TU,
 * so the 69 functions are one object and this band cannot be cut further without new evidence.
 * That is 69 functions: a decompiler lane finishes it in address order (the largest: `fn_80356664`
 * 0x1A84, `fn_80352E24` 0xD88, `fn_80350D74` 0xBE0, `fn_8035598C` 0x860, `fn_803544F8` 0x788,
 * `fn_803580E8` 0x53C).
 *
 * SEAMS.  Left edge 0x8034F138: see `Pl/pl_act_class3.cpp`.  Right edge 0x80358624: unchanged from
 * the original registration - the next owners are `ef/eft052.cpp` and its band, whose pool
 * starts at `lbl_8079B640` and whose `.data` starts at `lbl_805ED0C0`.
 *
 * LANGUAGE.  C++ (`-Cpp_exceptions on`, `cflags_main`): the range reaches genuinely mangled callees
 * (`em_frame_check__FP11_ENEMY_WORKUsff`, `setVector3__FPQ34nw4r4math4VEC3fff`, `MHchar::setTevKColor`),
 * so every plain `fn_` definition will be `extern "C"` (playbook 42).
 *
 * DATA.  `.data`, `.sdata` and `.sdata2` are claimed, and nothing is emitted yet, so the target
 * object carries them as `target-extra` until the bodies and tables are written.
 */

#include "types.h"
