/*
 * homebutton/fn_80530680.cpp - STUB (no bodies yet).
 *
 * `.text` 0x80530680..0x8053072C.  Sections of the candidate unit: .text 0x80530680..0x8053072C; .ctors 0x8056F3F0..0x8056F3F4; .data 0x8064A800..0x8064A878; .sbss 0x80795A40..0x80795A48.
 *
 * WHAT IT IS. a unit of the HOME-button menu code (HBM, the Wii HOME overlay): layout panel/controller classes of the C++ run that ends in the software keyboard; every function is a `fn_<addr>` placeholder in the map and the `.data` is class vtables and layout vocabulary.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class ctors: .ctors word 0x8056F3EC = 0x80530674 = fn_80530674 (ends 0x80530680): the static-init function of a TU; the closure of the sinit's callees and address-taken functions after it is empty, so the TU ends at L = 0x80530680.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit homebutton/fn_80530680.cpp`), and the pass that writes the bodies defines them.
 */
