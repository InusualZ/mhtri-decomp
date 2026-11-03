/*
 * Network/network_shared_data.cpp - the Network band's shared small-data pool, a data-only unit whose source defines
 *   nothing (playbook 23/53 route 2, playbook 54; the model is `Pl/pl_frame_data.cpp`).
 * RANGE. no `.text`; `.sdata2` 0x8079C6C8-0x8079C758 (144 B, the rate/timer float pool) and `.sdata`
 *   0x80793900-0x80793930 (48 B, the rate/interval words, ending at `networkSessionTimeoutSeconds`, read outside the
 *   band).  The `.sdata2` start is the end of `Network/NetworkConnectionStable.cpp`'s own pool 0x8079C698-0x8079C6C8,
 *   its end the first row of `Network/NetworkSessionManagerPat.cpp`'s claim; the block is linked right after
 *   `Network/NetworkConnectionStable.cpp` (the next `.sdata`/`.sdata2` claims, 0x80793930/0x8079C758, come later).
 * NAMES. The file name is descriptive (no `__FILE__` string covers the pool).
 * RESIDUALS. none (a `NonMatching` unit contributes the original bytes).  The interior words are still `lbl_8079C6xx`
 *   rows, named when the bodies reading them are written.
 * SHAPES. The pool has several readers - the transport units and `Network/NetworkSessionManager.cpp` share
 *   0x8079C6EC..0x8079C754 (`callers.py 0x8079C6EC`), unsplit Network code reads 0x8079C750 - so no
 *   consumer may claim it (it would take rows only the other reads); one owner lets every consumer include
 *   `Network/network_shared_data.h` (rule 2).  The rows 0x8079C660-0x8079C690 have unsplit-only readers.
 */

#include "types.h"
#include "Network/network_shared_data.h"
