/*
 * Network/network_shared_data.cpp - the Network band's shared small-data pool, a data-only unit whose source defines
 *   nothing (playbook 23/53 route 2, playbook 54; the model is `Pl/pl_frame_data.cpp`).
 * RANGE. no `.text`; `.sdata2` 0x8079C690-0x8079C758 (200 B, the rate/timer float pool) and `.sdata`
 *   0x80793900-0x80793930 (48 B, the rate/interval words).  Both ends are proven by the reader set: `.sdata2` ends at
 *   the first row of `Network/NetworkSessionManagerPat.cpp`'s claim, `.sdata` at `networkSessionTimeoutSeconds`, read
 *   outside the band. The 4-byte hole `lbl_8079C70C` joins the two reader pools 0x8079C6D8-0x8079C70C and
 *   0x8079C710-0x8079C758 into one run, claimed whole (two ranges of one section in one unit are a link-order cycle,
 *   playbook 53).
 * NAMES. The file name is descriptive (no `__FILE__` string covers the pool).
 * RESIDUALS. none (a `NonMatching` unit contributes the original bytes).  The interior words are still `lbl_8079C6xx`
 *   rows, named when the bodies reading them are written and declared in `Network/network_shared_data.h` then.
 * SHAPES. The pool has several readers - the transport units and `Network/NetworkSessionManager.cpp` share
 *   0x8079C6EC..0x8079C754 (`callers.py 0x8079C6EC`), unsplit Network code reads 0x8079C690 and 0x8079C750 - so no
 *   consumer may claim it (it would take rows only the other reads); one owner lets every consumer include
 *   `Network/network_shared_data.h` (rule 2).  The rows before it (0x8079C660-0x8079C690) are unsplit-only readers.
 */

#include "types.h"
#include "Network/network_shared_data.h"
