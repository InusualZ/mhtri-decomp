/*
 * homebutton/fn_8052E0CC.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x8052E0CC..0x80530680.  Sections of the candidate unit: .text 0x8052E0CC..0x80530680; .ctors 0x8056F3EC..0x8056F3F0; .data 0x8064A738..0x8064A800; .sbss 0x80795A38..0x80795A40; .sdata2 0x8079D598..0x8079D5B0.
 *
 * WHAT IT IS. a unit of the HOME-button menu code (HBM, the Wii HOME overlay): layout panel/controller classes of the C++ run that ends in the software keyboard; every function is a `fn_<addr>` placeholder in the map and the `.data` is class vtables and layout vocabulary.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class ctors: .ctors word 0x8056F3E8 = 0x8052E0C0 = fn_8052E0C0 (ends 0x8052E0CC): the static-init function of a TU; the closure of the sinit's callees and address-taken functions after it is empty, so the TU ends at L = 0x8052E0CC.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit homebutton/fn_8052E0CC.cpp`), and the pass that writes the bodies defines them.
 */
