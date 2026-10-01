/*
 * SC/sc.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x804DAE40..0x804E45B0.  Sections of the candidate unit: .text 0x804DAE40..0x804E45B0; .rodata 0x80573B58..0x80573C40; .data 0x80629CA8..0x8062AB68; .bss 0x8074E460..0x8075B110; .sdata 0x80794000..0x80794148; .sbss 0x80795440..0x807955C8; .sdata2 0x8079D330..0x8079D3C0.
 *
 * WHAT IT IS. the SC (system configuration) and SI (serial interface) libraries and the THP decoder run before TPL: `SCInit`, `SCFindByteArrayItem`, `SIInit`, `SITransfer`, ... up to 0x804E45B0.
 *   GUESS: named after the first library it holds (SC), the unit is a merged block of SC, SI and THP.
 *   merged candidate pieces (guess cuts the renderer does not emit): fn_804DAE40, sc, si, fn_804DF200, thp.
 *
 * WHY IT SITS HERE. phase 1 grade medium, class inherited: 0x804DAE40 follows the registered unit RSO/runtime.c with no function in between (<= 16 B pad); baseline order/coverage/text-cut PASS.
 *
 * UNKNOWN. every body and, for a merged block, the file boundaries between its pieces.
 *
 * FLAGS. the `OS` lib's `cflags_os` (unmeasured until bodies exist).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit SC/sc.cpp`), and the pass that writes the bodies defines them.
 */
