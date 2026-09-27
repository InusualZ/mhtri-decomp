/*
 * NHTTP declarations with no registered owner (docs/plan.md 6.5 rule 2).
 *
 * The NHTTP library's two data objects - the system-info block `NHTTPi_systemInfo` (`.bss`
 * 0x80762C60, 0x51A B) and the singleton slot `NHTTPi_systemInfoP` (`.sbss` 0x80795884) that
 * `NHTTPi_GetSystemInfoP` lazily points at it - are owned by no registered unit: neither address is
 * inside a `splits.txt` range, and the registered NHTTP units (`NHTTP_bgnend.c`, `NHTTP_os_RVL.c`,
 * `d_nhttp.c`, all `.text` only) do not claim them.  They were declared in
 * `include/NHTTP/d_nhttp.h` (a unit's own header) until the networking conformance pass; rule 2
 * puts an unowned symbol in the band header, so they live here and `d_nhttp.h` includes this file.
 *
 * `struct NHTTPInfo`'s definition stays with the unit that reconstructs it
 * (`include/NHTTP/d_nhttp.h`), so only the forward declaration is here.
 */
#ifndef MHTRI_UNSPLIT_NHTTP_H
#define MHTRI_UNSPLIT_NHTTP_H

#include "types.h"

struct NHTTPInfo;

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80795884 - the lazily-set slot `NHTTPi_GetSystemInfoP` returns. */
extern struct NHTTPInfo* NHTTPi_systemInfoP;

/* 0x80762C60 - the info block that slot points at. */
extern struct NHTTPInfo NHTTPi_systemInfo;

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_NHTTP_H */
