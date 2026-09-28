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
extern const char NHTTPi_startupMessages[];
extern const char NHTTPi_connRestWarning[];
extern const char NHTTPi_allocFailMessage[];
extern const char NHTTPi_postDataRawMessage[];

/* 0x80630AE8 (0x30+ B, `.data`) - the `NHTTP_os_RVL.c` assert group: +0x00 the function name
 * "NHTTPi_CheckCurrentThread", +0x1C "%s:illegal thread
", +0x30 "NHTTP_os_RVL.c". */
extern const char NHTTPi_threadCheckMessages[];

/* 0x807943A0 (`.sdata`) - the message that assert panics with ("halt
"). */
extern const char NHTTPi_haltMessage[];

/* 0x80574CE8 (0x15 B, `.rodata`) - the 19-character code `NHTTPAddPostDataRaw` walks its request's
 * 18-byte code field up to, one character at a time, re-submitting after each step. */
extern const char NHTTPi_postDataRawCode[];

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_UNSPLIT_NHTTP_H */
