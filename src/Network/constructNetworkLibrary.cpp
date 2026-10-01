/*
 * Network/constructNetworkLibrary.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x804189C8..0x80419EC4.  Sections of the candidate unit: extab 0x8001CE4C..0x8001CF70; extabindex 0x8003D80C..0x8003D92C; .text 0x804189C8..0x80419EC4; .data 0x80602AF8..0x80603118; .sbss 0x80794CD0..0x80794CD8; .sdata2 0x8079C878..0x8079C888.
 *
 * WHAT IT IS. `constructNetworkLibrary` (the network singletons' construction entry point, 0x804189C8) with `clearNetworkPat` and
 *   `deleteNetworkSessionManagerPat` (37 functions, 4 named; `.data` holds 27 symbols).
 *
 * WHY IT SITS HERE. phase 1 grade guess: an unowned run between the registered `Network/constructNetworkWiiMediator.cpp` (ends 0x804189C8) and
 *   `Network/NetworkPat.cpp` (starts 0x80419EC4).  The name is the first map symbol of the run.
 *
 * UNKNOWN. every body.
 *
 * FLAGS. the `Network` lib's `cflags_network` (unmeasured).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit Network/constructNetworkLibrary.cpp`), and the pass that writes the bodies defines them.
 */
