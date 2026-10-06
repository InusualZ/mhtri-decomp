/*
 * NHTTP declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * The NHTTP library's info block `NHTTPi_systemInfo` (`.bss` 0x80762C60, 0x51A B) is inside `NHTTP/d_nhttp.c`'s
 * `.bss` claim (0x80762C20..0x80763900), so it is declared in `NHTTP/d_nhttp.h` (rule 2).
 *
 * The singleton slot `NHTTPi_systemInfoP` (`.sbss` 0x80795884) that `NHTTPi_GetSystemInfoP` lazily
 * points at it was declared here too until `d_nhttp.c` claimed the 16-byte `.sbss` run
 * 0x80795878..0x80795888 (rule 12); the slot is now that unit's own object, so this header no
 * longer names it.
 *
 * `struct NHTTPInfo`'s definition stays with the unit that reconstructs it
 * (`NHTTP/d_nhttp.h`), so only the forward declaration is here.
 */
#ifndef MHTRI_UNSPLIT_NHTTP_H
#define MHTRI_UNSPLIT_NHTTP_H

#include "types.h"

struct NHTTPInfo;

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The four `.data` literal groups `NHTTP/NHTTP_bgnend.c`'s bodies address.  dtk assigns them to
 * the auto data run `main/auto_07_80629C40_data`, which no registered unit claims, so rule 2 puts
 * them here (the names are derived from the strings they carry, which are read out of the DOL):
 *
 *   NHTTPi_startupMessages   0x80630A08 (0x48 B) - +0x00 "NCDGetCurrentIpConfig err = %d\n",
 *                            +0x20 "NHTTP_bgnend.c", +0x30 "NCDGetCurrentIpConfig";
 *                            `NHTTPi_Startup` loads the group base once and adds the offsets.
 *   NHTTPi_connRestWarning   0x80630A50 (0x3A B) - "*warning: %d connections rests! ...".
 *   NHTTPi_allocFailMessage  0x80630A90 (0x1B B) - "failed to allocate memory\n".
 *   NHTTPi_postDataRawMessage 0x80630AAC (0x39 B) - "already called NHTTPAddPostDataRaw ...".
 */

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_NHTTP_H */
