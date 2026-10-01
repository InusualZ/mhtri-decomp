/*
 * Runtime.PPCEABI.H/CPlusLibPPC.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80456704..0x80456C88.  Sections of the candidate unit: extab 0x8001E3B0..0x8001E3F0; extabindex 0x8003F144..0x8003F174; .text 0x80456704..0x80456C88; .sdata 0x80793CC0..0x80793CC8.
 *
 * WHAT IT IS. the MSL C++ array runtime (`unexpected`, `__construct_new_array`, `__construct_array`, `__destroy_arr`, one `dtor_` helper; 9
 *   functions, 5 named) with 8 B of `.sdata` and an extab record set.
 *
 * WHY IT SITS HERE. phase 1 grade guess: an unowned run after the registered `global_destructor_chain.c` (ends 0x80456704); the file name is the
 *   MSL library's own (`CPlusLibPPC.cp`).
 *
 * UNKNOWN. every body.
 *
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi` (unmeasured).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit Runtime.PPCEABI.H/CPlusLibPPC.cpp`), and the pass that writes the bodies defines them.
 */
