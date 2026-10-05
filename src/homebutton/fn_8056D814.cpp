/*
 * homebutton/fn_8056D814.cpp - STUB (no bodies yet).
 *
 * `.text` 0x8056D814..0x8056F2B4.  Sections of the candidate unit: .text 0x8056D814..0x8056F2B4; .rodata 0x8057C760..0x8057C808; .data 0x80658458..0x806584E4; .bss 0x80790DF8..0x80790E04; .sdata 0x80794750..0x80794754; .sdata2 0x8079D7B0..0x8079D7C8.
 *
 * WHAT IT IS. the last `.text` TU of the HOME-button code: 17 functions of the keyboard widget classes, with the `.ctors` word whose sinit registers the destructor `fn_8056D7D4`.
 *   merged candidate pieces (guess cuts the renderer does not emit): fn_8056D814, fn_8056F02C.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class ctors: .ctors word 0x8056F428 = 0x8056D78C = fn_8056D78C (size 0x48, ends 0x8056D7D4): the static-init function of one TU.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit homebutton/fn_8056D814.cpp`), and the pass that writes the bodies defines them.
 */
