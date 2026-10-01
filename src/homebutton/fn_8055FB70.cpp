/*
 * homebutton/fn_8055FB70.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x8055FB70..0x8055FD58.  Sections of the candidate unit: .text 0x8055FB70..0x8055FD58; .ctors 0x8056F41C..0x8056F420; .data 0x80655630..0x80655A98; .sdata 0x80794660..0x80794668.
 *
 * WHAT IT IS. a sinit-only TU of the HOME-button software keyboard: the static-initialiser function of a table of layout objects (0x1E8 B, no other function).
 *
 * WHY IT SITS HERE. phase 1 grade medium, class ctors: .ctors word 0x8056F418 = 0x8055F728 = fn_8055F728 (size 0x448, ends 0x8055FB70): the static-init function of one TU.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit homebutton/fn_8055FB70.cpp`), and the pass that writes the bodies defines them.
 */
