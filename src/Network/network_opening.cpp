/*
 * Network/network_opening.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x804155D4..0x80418988.  Sections of the candidate unit: extab 0x8001CBF4..0x8001CE34; extabindex 0x8003D524..0x8003D800; .text 0x804155D4..0x80418988; .rodata 0x80570E70..0x80570E98; .sdata2 0x8079C874..0x8079C878.
 *
 * WHAT IT IS. the connection opening and terms-of-use steps (`openingStart`, `setConnectionPaths`, `getNASToken`, `startTermsCheck`,
 *   `startTermsUpdate`, `isTermsUpdateFinished`) plus the network log context and socket pool helpers
 *   (`networkLog_destroyContext`, `networkSocketPool_acquire`/`_release`); 95 functions, 11 named.
 *
 * WHY IT SITS HERE. phase 1 grade guess: an unowned run between the registered `Network/NetworkWiiMediator.cpp` (ends 0x804155D4) and
 *   `Network/constructNetworkWiiMediator.cpp` (starts 0x80418988).
 *
 * UNKNOWN. every body; the name is a GUESS from `openingStart`.
 *
 * FLAGS. the `Network` lib's `cflags_network` (unmeasured).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit Network/network_opening.cpp`), and the pass that writes the bodies defines them.
 */
