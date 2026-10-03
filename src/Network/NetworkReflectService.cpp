/*
 * Network/NetworkReflectService.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x8041A194..0x8041A87C.  Sections of the candidate unit: extab 0x8001CF88..0x8001CFF0; extabindex 0x8003D950..0x8003D9BC; .text 0x8041A194..0x8041A87C; .data 0x80603118..0x80603154; .sbss 0x80794CD8..0x80794CE0.
 *
 * WHAT IT IS. the `NetworkReflectService` class: `__ct__21NetworkReflectServiceFv`, `initReflectService`, `finalizeReflectService`,
 *   `reflectServiceStart`/`Stop`/`Agree`, `setReflectServicePage` and `notify` (14 functions, 8 named).
 *
 * WHY IT SITS HERE. phase 1 grade guess: an unowned run between the registered `Network/NetworkPat.cpp` (ends 0x8041A194) and
 *   `Network/fn_8041A87C.cpp` (starts 0x8041A87C); phase1-e.md open question 8 notes the registered neighbour holds the rest of
 *   the reflect service plus `NetworkTimedHandler`, so this seam may move.
 *
 * UNKNOWN. every body.
 *
 * FLAGS. the `Network` lib's `cflags_network` (unmeasured).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit Network/NetworkReflectService.cpp`), and the pass that writes the bodies defines them.
 */

#include "Network/NetworkReflectService.h"
