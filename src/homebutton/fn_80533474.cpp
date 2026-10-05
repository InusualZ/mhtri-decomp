/*
 * homebutton/fn_80533474.cpp - STUB (no bodies yet).
 *
 * `.text` 0x80533474..0x8053E808.  Sections of the candidate unit: .text 0x80533474..0x8053E808; .ctors 0x8056F3F4..0x8056F3F8; .rodata 0x80579450..0x8057AB98; .data 0x8064A8E8..0x8064A8F8; .bss 0x807901C0..0x807901F0; .sdata 0x807944B8..0x80794568; .sbss 0x80795A48..0x80795A50; .sdata2 0x8079D5D0..0x8079D620.
 *
 * WHAT IT IS. a unit of the HOME-button menu code (HBM, the Wii HOME overlay): layout panel/controller classes of the C++ run that ends in the software keyboard; every function is a `fn_<addr>` placeholder in the map and the `.data` is class vtables and layout vocabulary.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class pool: 1 pooled .sdata2 value(s) are held at two pool addresses (one TU never holds a value twice); for the best of them: .sdata2 value 4330000000000000 held at 0x8079D5C8 and again at 0x8079D5D8 (copy 1 last read by 0x80533404, copy 2 first read by 0x80533474); the other boundaries of the interval are excluded by must-link spans (none needed: the interval is one boundary wide).
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit homebutton/fn_80533474.cpp`), and the pass that writes the bodies defines them.
 */
