/*
 * nw4r/fn_8050661C.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x8050661C..0x805073C0.  Sections of the candidate unit: .text 0x8050661C..0x805073C0; .data 0x8062FC48..0x8062FD00; .bss 0x80760CD0..0x80760F00; .sdata 0x807941E8..0x807941F8; .sbss 0x80795780..0x807957B0.
 *
 * WHAT IT IS. nw4r code that ends at the `.ctors` cut before `dwc_error` (0x805073C0); no named function.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class ctors: .ctors word 0x8056F3DC = 0x80506598 = fn_80506598 (ends 0x8050661C): the static-init function of a TU; the closure of the sinit's callees and address-taken functions after it is empty, so the TU ends at L = 0x8050661C.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit nw4r/fn_8050661C.cpp`), and the pass that writes the bodies defines them.
 */
