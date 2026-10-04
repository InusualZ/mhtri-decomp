/*
 * Network/PatInterface.cpp - STUB (phase 4, docs/splits/phase4; no bodies yet).
 *
 * `.text` 0x803FCC34..0x803FE8E4.  Sections of the candidate unit: extab 0x8001BCAC..0x8001BD9C; extabindex 0x8003BF40..0x8003C06C; .text 0x803FCC34..0x803FE8E4; .data 0x80600978..0x80600BA0; .sbss 0x80794CB0..0x80794CB8; .sdata2 0x8079C7D0..0x8079C7D8.
 *
 * WHAT IT IS. the `PatInterface` singleton: `__ct__12PatInterfaceFv`, `PatInterface_clear`, `PatInterface_isReady`, the reflect page range,
 *   the callback and the call-stack helpers, the server-type choice (`setConnectServerType`, `chooseServerAddress`), the
 *   game/server time getters and the Fmp accessors (87 functions, 66 named; `mpInstance__12PatInterface` is its `.sbss` word).
 *
 * WHY IT SITS HERE. phase 1 grade medium: the `.ctors` closure is empty and a `.data` vtable-then-string seam ends exactly at 0x803FCC34, where
 *   `__ct__12PatInterfaceFv` starts; the right edge is the registered `Network/network_state.cpp`.
 *
 * UNKNOWN. every body; the C-linkage surface the callers use is declared in `include/Network/PatInterface.h`.
 *
 * NAMES (GUESS, integrator 2026-10-04, from the bodies, in the scheme of `errorRecordCode613c`/`getErrorInfo654c`):
 *   `clearErrorRecord613c` 0x803FD674 clears the 0x208-byte record at +0x613C while no error is pending (+0x654C);
 *   `buildErrorInfo613c` 0x803FD6D8 fills a caller's three-word error from a code and the record's code;
 *   `getErrorInfoOrCode654c` 0x803FD7BC copies the pending error and substitutes a negative code for 0x80000000.
 *
 * FLAGS. the `Network` lib's `cflags_network` (unmeasured).
 *
 * The unit's `.data`/`.sdata`/`.sbss` claims are the candidate's (config/RMHE08/splits.txt); the symbols they hold are
 * in the map (`ledger.py unit Network/PatInterface.cpp`), and the pass that writes the bodies defines them.
 */

#include "Network/PatInterface.h"
