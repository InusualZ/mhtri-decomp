/*
 * homebutton/fn_8053E808.cpp - STUB (no bodies yet).
 *
 * `.text` 0x8053E808..0x805425E4.  Sections of the candidate unit: .text 0x8053E808..0x805425E4; .ctors 0x8056F3F8..0x8056F3FC; .rodata 0x8057AB98..0x8057B098; .data 0x8064DE88..0x8064E760; .sdata 0x80794568..0x807945C8; .sdata2 0x8079D620..0x8079D628.
 *
 * WHAT IT IS. a unit of the HOME-button menu code (HBM, the Wii HOME overlay): layout panel/controller classes of the C++ run that ends in the software keyboard; every function is a `fn_<addr>` placeholder in the map and the `.data` is class vtables and layout vocabulary.
 *   merged candidate pieces (guess cuts the renderer does not emit): fn_8053E808, fn_8053E808.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class ctors: detail line: .ctors word 0x8056F3F4 = 0x8053E5F8 (fn_8053E5F8, ends 0x8053E7D0): closure ends 0x8053E7D0, with own vtable slots 0x8053E808: fn_8053E7D0..fn_8053E800 are seven 8-byte slots of lbl_8064DC30 (.data 0x8064DC30), whose address fn_805385D0 materialises at 0x805385F8 inside the unit (`callers.py 0x8064DC30`): the TU ends at 0x8053E808, the first non-slot function (0xB0 B).
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit homebutton/fn_8053E808.cpp`), and the pass that writes the bodies defines them.
 */
