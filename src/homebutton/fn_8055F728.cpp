/*
 * homebutton/fn_8055F728.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x8055F728..0x8055FB70.  Sections of the candidate unit: .text 0x8055F728..0x8055FB70; .ctors 0x8056F418..0x8056F41C; .rodata 0x8057BDB0..0x8057C1FC; .data 0x806525A4..0x806553C0.
 *
 * WHAT IT IS. a sinit-only TU of the HOME-button software keyboard: the static-initialiser function that builds a table of layout objects (0x448 B, no other function).
 *
 * WHY IT SITS HERE. phase 1 grade medium, class ctors: .ctors word 0x8056F414 = 0x8055F6E8 = fn_8055F6E8 (size 0x40, ends 0x8055F728): the static-init function of one TU.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit homebutton/fn_8055F728.cpp`), and the pass that writes the bodies defines them.
 */
