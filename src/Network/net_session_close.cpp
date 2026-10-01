/*
 * Network/net_session_close.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x80432104..0x80437270.  Sections of the candidate unit: extab 0x8001D824..0x8001DA94; extabindex 0x8003E214..0x8003E514; .text 0x80432104..0x80437270; .ctors 0x8056F3C4..0x8056F3C8; .data 0x80603D70..0x80603E90; .bss 0x806E1770..0x806E1B38.
 *
 * WHAT IT IS. the session close / abort and pending-action state (`net_session_close_start`, `net_session_close_state_get`,
 *   `net_session_abort_start`, `isSessionStartDone`, `runPendingAction1`..`8`, the phase-slot and link-state helpers; 88 functions,
 *   23 named) with the unit's `.bss` slot table (6 symbols) and `.ctors` word.
 *
 * WHY IT SITS HERE. phase 1 grade medium: the `.ctors` word's closure (the static initialiser at 0x80431CD8 and its callees) ends exactly at
 *   0x80437270; the left edge 0x80432104 is the end of the `Network/network_pat_control` fold (the unsigned-to-double constant
 *   is read on both sides).
 *
 * UNKNOWN. every body.
 *
 * FLAGS. `cflags_main`, the link neighbour `Network/network_pat_control.cpp`'s group (unmeasured).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit Network/net_session_close.cpp`), and the pass that writes the bodies defines them.
 */
