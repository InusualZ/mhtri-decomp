/*
 * Network/network_layer_io.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x804006A8..0x80413C64.  Sections of the candidate unit: extab 0x8001BE24..0x8001CA4C; extabindex 0x8003C138..0x8003D2F0; .text 0x804006A8..0x80413C64; .rodata 0x80570E20..0x80570E70; .data 0x80600BA0..0x80602428; .sdata 0x80793970..0x80793990; .sbss 0x80794CB8..0x80794CC0; .sdata2 0x8079C7E8..0x8079C874.
 *
 * WHAT IT IS. the network layer's request/response I/O: the `sendReq*` request builders (`sendReqFmpListVersion` ... `sendReqLayerInfo`,
 *   `sendReqTerms`, `sendReqAuthenticationToken`, `sendReqBinary*`, ...) and the response handlers behind them (415 functions,
 *   280 named in the map; `.data` holds 182 symbols).
 *
 * WHY IT SITS HERE. phase 1 grade guess: 80 KB with no pool or `.data` constraint at all, so the unit is the whole run between the registered
 *   `Network/network_state.cpp` (ends 0x804006A8) and `Network/NetworkWiiMediator.cpp` (starts 0x80413C64).  The extent is the
 *   registry edges, not an evidenced TU: it is almost certainly several files.
 *
 * UNKNOWN. every body and the internal seams; the name is the band's own vocabulary (GUESS).
 *
 * FLAGS. the `Network` lib's `cflags_network` (unmeasured).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit Network/network_layer_io.cpp`), and the pass that writes the bodies defines them.
 */
