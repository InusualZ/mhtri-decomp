/*
 * d_nhttp.c - the Revolution SDK NHTTP library's core TU, `.text` 0x80515774..0x8051B7FC
 * (108 functions, 24712 B).
 *
 * REGISTRATION (recon lane, 2026-09-27).  Left edge 0x80515774 (`0x80515774`) is the first referrer
 * of the TU's `.data` fragment 0x80630B28 (`"https://"`, `"CONNECT  "`, `" HTTP/1.1"`, the Base64
 * alphabet, `"Proxy-Authorization: Basic "`), and the `.sdata` run jump `0x807943A0 -> 0x807943A8`
 * (cuts [18686,18739]) admits it.  Right edge 0x8051B7FC is `SSLNew` - the NHTTP block's end and the
 * SSL library's start (`NHTTPi_InitConnectionList` 0x8051B79C..0x8051B7FC is the last function).
 *
 * SOURCE FILE.  Real name recovered from the pool: `0x8051A9EC` loads `.data:0x80630F04` =
 * `"d_nhttp.c"` (group 0x80630D90, with the `"NHTTPAddPostDataAscii"`/`"NHTTPSetProxy"` asserts).
 * The TU is the HTTP core: the connection/request/response objects and their state machine, the
 * `NHTTPi_Soc*` socket wrappers, the URL/Base64/hex string helpers, `NHTTPCreateConnection`,
 * `NHTTPGetBodyBuffer`, `__NHTTPCreateRequestEx`, `NHTTPSetProxy`, the `NHTTPi_*ConnectionList`
 * accessors and the completion callbacks.
 *
 * LIBRARY.  `NHTTP` - `0x8051A9EC`'s own `__FILE__` string, and its callees are all NHTTPi symbols
 * plus the REX/SO platform (`SOClose`).
 *
 * NOTE ON THE SPLIT.  `tudiscover`'s internal strong pool evidence for this span is only weakly
 * pinned (all three NHTTP cuts are weak signals); the split here follows the `.data` fragment order
 * and the three real `__FILE__` names, which is the strongest independent evidence available.  If a
 * later pass recovers the SDK's object list, re-cut at the real file boundaries and re-measure.
 *
 * SECTIONS.  `.text` only.  The `.data`/`.bss`/`.sbss`/`.sdata` the unit loads are declared, never
 * defined.
 *
 * FLAGS.  `cflags_nhttp`, as its two siblings.
 *
 * NAMING (rule 7).  Every function defined here already has a real map name (`NHTTPi_*`); the three
 * `fn_` names it *references* were derived by their owners' lanes and are reached through the owner
 * headers: `0x805145B8` -> `NHTTPi_InitSystemInfo` (NHTTP_bgnend.h), `0x80514EA8` ->
 * `NHTTPi_InitMutexInfo` (NHTTP_bgnend.h), `0x80517614` -> `NHTTPi_InitThreadInfo` (this unit,
 * d_nhttp.h).  No deferral is needed here.
 *
 * BODY (probe).  8 of the 108 functions: the memory shims, the five `*InfoP` accessors and the
 * `NHTTPi_GetSystemInfoP` lazy singleton - all byte-identical at 100 %.  `NHTTPi_InitThreadInfo`
 * (0x80517614) stays a *reference* on purpose: retail calls it from `NHTTPi_GetSystemInfoP`, so it
 * lives in a different original object than the singleton and must not be inlined into it.
 */

#include "NHTTP/d_nhttp.h"
#include "NHTTP/NHTTP_bgnend.h"
#include "NHTTP/NHTTP_os_RVL.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* 0x80516CA0 (4): tail shim to memcpy. */
void* NHTTPi_memcpy(void* dst, const void* src, u32 n) {
    return memcpy(dst, src, n);
}

/* 0x80516CAC (0xC): zero-fill shim (memset(dst, 0, n)). */
void* NHTTPi_memclr(void* dst, u32 n) {
    return memset(dst, 0, n);
}

/* 0x8051B750 (4): the bgnend info slot is the base pointer itself. */
void* NHTTPi_GetBgnEndInfoP(NHTTPInfo* info) {
    return info;
}

/* 0x8051B754 (8): the list-info slot. */
void* NHTTPi_GetListInfoP(NHTTPInfo* info) {
    return &info->list;
}

/* 0x8051B75C (8): the request-info slot. */
void* NHTTPi_GetReqInfoP(NHTTPInfo* info) {
    return &info->request;
}

/* 0x8051B764 (8): the thread-info slot. */
void* NHTTPi_GetThreadInfoP(NHTTPInfo* info) {
    return &info->thread;
}

/* 0x8051B76C (8): the mutex-info slot. */
void* NHTTPi_GetMutexInfoP(NHTTPInfo* info) {
    return &info->mutex;
}

/* 0x8051B6E8 (0x68): the lazily-initialised NHTTP system-info singleton. */
NHTTPInfo* NHTTPi_GetSystemInfoP(void) {
    if (NHTTPi_systemInfoP == 0) {
        NHTTPInfo* pInfo = &NHTTPi_systemInfo;
        NHTTPi_systemInfoP = pInfo;
        NHTTPi_InitSystemInfo(pInfo);
        NHTTPi_InitListInfo(&pInfo->list);
        NHTTPi_InitRequestInfo(&pInfo->request);
        NHTTPi_InitMutexInfo(&pInfo->mutex);
        NHTTPi_InitThreadInfo(&pInfo->thread);
    }
    return NHTTPi_systemInfoP;
}
