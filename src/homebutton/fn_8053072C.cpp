/*
 * homebutton/fn_8053072C.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x8053072C..0x80533474.  Sections of the candidate unit: .text 0x8053072C..0x80533474; .data 0x8064A878..0x8064A8E8; .sdata2 0x8079D5B0..0x8079D5D0; .sbss2 0x8079D7F0..0x8079D7F8.
 *
 * WHAT IT IS. a unit of the HOME-button menu code (HBM, the Wii HOME overlay): layout panel/controller classes of the C++ run that ends in the software keyboard; every function is a `fn_<addr>` placeholder in the map and the `.data` is class vtables and layout vocabulary.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class ctors: .ctors word 0x8056F3F0 = 0x80530720 = fn_80530720 (ends 0x8053072C): the static-init function of a TU; the closure of the sinit's callees and address-taken functions after it is empty, so the TU ends at L = 0x8053072C.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit homebutton/fn_8053072C.cpp`), and the pass that writes the bodies defines them.
 */
