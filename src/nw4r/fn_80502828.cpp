/*
 * nw4r/fn_80502828.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80502828..0x80504A3C.  Sections of the candidate unit: .text 0x80502828..0x80504A3C; .data 0x8062FAF0..0x8062FBE8; .bss 0x80760C98..0x80760CB8; .sbss 0x80795768..0x80795770; .sdata2 0x8079D4F0..0x8079D510.
 *
 * WHAT IT IS. nw4r code between two `.ctors` cuts (0x80502828 and 0x80504A3C); no named function.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class ctors: .ctors word 0x8056F3D8 = 0x80502810 = fn_80502810 (ends 0x80502828): the static-init function of a TU; the closure of the sinit's callees and address-taken functions after it is empty, so the TU ends at L = 0x80502828.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit nw4r/fn_80502828.cpp`), and the pass that writes the bodies defines them.
 */
