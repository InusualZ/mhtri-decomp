/*
 * menu/menu_effect_slot.cpp - STUB (no bodies yet).
 *
 * `.text` 0x80348A48..0x80349DD8 (16 functions, 5008 bytes).  Sections of the candidate unit: .text 0x80348A48..0x80349DD8; extab 0x80016F74..0x80016FE4; extabindex 0x80036594..0x8003663C.
 *
 * WHAT IT IS. the second half of the old `ef/eft_slot.cpp` range: 16 functions after the slot pool's last body, with no `.data`/`.bss` of their own.
 *
 * WHY IT SITS HERE. the reconciled candidate cuts `ef/eft_slot.cpp`'s old range at 0x80348A48; the module is `menu` because the candidate files the band with the menu units that follow it.  GUESS (rule 7): the file stem is derived from the band's role and neighbours, not from a `__FILE__` string.
 *
 * UNKNOWN. every body and the internal seams.
 *
 * FLAGS. `cflags_menu`, the lib's group (unmeasured).
 *
 * The unit's data claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are in the map (`ledger.py unit menu/menu_effect_slot.cpp`), and the pass that writes the bodies defines them.
 */
