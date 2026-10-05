/*
 * homebutton/fn_8052C880.cpp - STUB (no bodies yet).
 *
 * `.text` 0x8052C880..0x8052E0CC.  Sections of the candidate unit: .text 0x8052C880..0x8052E0CC; .ctors 0x8056F3E8..0x8056F3EC; .data 0x8064A6B0..0x8064A738; .sdata 0x807944B0..0x807944B8; .sbss 0x80795A30..0x80795A38; .sdata2 0x8079D580..0x8079D598.
 *
 * WHAT IT IS. a unit of the HOME-button menu code (HBM, the Wii HOME overlay): layout panel/controller classes of the C++ run that ends in the software keyboard; every function is a `fn_<addr>` placeholder in the map and the `.data` is class vtables and layout vocabulary.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class ctors: .ctors word 0x8056F3E4 = 0x8052C874 = fn_8052C874 (ends 0x8052C880): the static-init function of a TU; the closure of the sinit's callees and address-taken functions after it is empty, so the TU ends at L = 0x8052C880.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit homebutton/fn_8052C880.cpp`), and the pass that writes the bodies defines them.
 */
