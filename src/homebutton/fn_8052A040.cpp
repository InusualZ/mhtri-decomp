/*
 * homebutton/fn_8052A040.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x8052A040..0x8052B004.  Sections of the candidate unit: .text 0x8052A040..0x8052B004; .ctors 0x8056F3E0..0x8056F3E4; .data 0x806498C8..0x8064A5E8; .sbss 0x80795A18..0x80795A20; .sdata2 0x8079D550..0x8079D570.
 *
 * WHAT IT IS. a unit of the HOME-button menu code (HBM, the Wii HOME overlay): layout panel/controller classes of the C++ run that ends in the software keyboard; every function is a `fn_<addr>` placeholder in the map and the `.data` is class vtables and layout vocabulary.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class vtable-owner: unnamed C++ run after kbd_init_keyboard.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `main` lib's `cflags_main` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit homebutton/fn_8052A040.cpp`), and the pass that writes the bodies defines them.
 */
