/*
 * enemy/em_act_step_tail.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80330194..0x8033041C (4 functions, 648 bytes).  Sections of the candidate unit: .text 0x80330194..0x8033041C; extab 0x80016614..0x8001662C; extabindex 0x80035784..0x800357A8.
 *
 * WHAT IT IS. the tail of the enemy work record's action-step band: four small bodies after `enemy/em_act_step.cpp`'s last step function.
 *
 * WHY IT SITS HERE. the reconciled candidate cuts `enemy/em_act_step.cpp`'s old range at 0x80330194 (its `.data` and `.bss` stay with the first half); `enemy/em_pl_frame` (now `hud/pl_frame_sync`) follows at 0x8033041C.  GUESS (rule 7): the file stem is derived from the band's role and neighbours, not from a `__FILE__` string.
 *
 * UNKNOWN. every body and the internal seams.
 *
 * FLAGS. `cflags_main`, the lib's group (unmeasured).
 *
 * The unit's data claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are in the map (`ledger.py unit enemy/em_act_step_tail.cpp`), and the pass that writes the bodies defines them.
 */
