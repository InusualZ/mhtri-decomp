/*
 * Runtime.PPCEABI.H/runtime.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80456CE0..0x80457420.  Sections of the candidate unit: .text 0x80456CE0..0x80457420; .rodata 0x80572438..0x80572450.
 *
 * WHAT IT IS. the PPC EABI runtime helpers: `__cvt_fp2unsigned` and the `_savefpr_*`/`_restfpr_*`/`_savegpr_*`/`_restgpr_*` register
 *   save and restore entry points (85 functions, all named in the map) with a small `.rodata` constant.
 *
 * WHY IT SITS HERE. phase 1 grade guess: an unowned run (registry edge); the name is the library's `runtime` file.
 *
 * UNKNOWN. every body (the save/restore entry points are most likely `asm` functions in the original source, GUESS).
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit Runtime.PPCEABI.H/runtime.cpp`), and the pass that writes the bodies defines them.
 */
