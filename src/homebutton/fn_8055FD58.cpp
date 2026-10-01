/*
 * homebutton/fn_8055FD58.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x8055FD58..0x8056083C.  Sections of the candidate unit: .text 0x8055FD58..0x8056083C; .ctors 0x8056F420..0x8056F424; .data 0x80655BD8..0x80655F50.
 *
 * WHAT IT IS. a sinit-only TU of the HOME-button software keyboard: the static-initialiser function of a table of layout objects (0xAE4 B, no other function).
 *
 * WHY IT SITS HERE. phase 1 grade medium, class ctors: .ctors word 0x8056F41C = 0x8055FB70 = fn_8055FB70 (size 0x1E8, ends 0x8055FD58): the static-init function of one TU.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit homebutton/fn_8055FD58.cpp`), and the pass that writes the bodies defines them.
 */
