/*
 * Pl/pl_act_step_data.cpp - STUB (phase 4, docs/splits/phase4; data-only unit, no bodies).
 *
 * Sections of the candidate unit: `.data` 0x805BDC48..0x805C15A8 (0x3960 B, 72 symbols) and `.bss` 0x806AACC0..0x806AB3E0 (0x720 B, 10 symbols).
 * It has no `.text`.
 *
 * WHAT IT IS. the data of the player actor's per-frame step band (`Pl/pl_act_step`, window b): the motion/step jump tables, the step
 *   tables and the work arrays that band's static initialisers construct.  It sits between `lobby/lb_equip_page` (below) and
 *   `Pl/fn_80229ECC` (above) in the link order, as the phase 4 candidate has it.
 *
 * WHY IT SITS HERE. the reconciled candidate (docs/splits/proposals/phase2-reconcile.json) attaches the run to no function unit of
 *   window c: its readers (`tools/units/callers.py 0x805BDC48`) are `fn_80246B7C` and the step band's functions
 *   (0x80250C10..0x80253994), which window b folds into `Pl/pl_act_step`.  The
 *   data-only unit takes the window of the unit before it in the file (c).  The name follows the unit that reads it.
 *
 * UNKNOWN. every symbol's type and extent; the pass that writes `Pl/pl_act_step`'s bodies defines them.
 *
 * FLAGS. the `Pl` lib's `cflags_pl`.
 */

#include "types.h"
