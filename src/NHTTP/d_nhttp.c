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
 * BODY (2026-09-28 pass).  10 of the 108 functions are byte-identical and four more are partly
 * reconstructed; the unit is 1.90 % fuzzy over its 24712 B.
 *
 *    100.00  NHTTPi_memcpy / NHTTPi_memclr / NHTTPi_strlen / NHTTPi_strcmp   (the four shims)
 *    100.00  NHTTPi_GetBgnEndInfoP / GetListInfoP / GetReqInfoP / GetThreadInfoP / GetMutexInfoP
 *    100.00  NHTTPi_GetSystemInfoP
 *     67.38  NHTTPi_encodeUrlChar        (160/156)
 *     61.21  NHTTPi_urlEncodedLengthN    (116/116)
 *     59.64  NHTTPi_urlEncodedLength     (112/112)
 *     29.02  NHTTPi_strnicmp            (204/168)
 *
 * NAMING (rule 7).  Every function defined here already has a real map name (`NHTTPi_*`); the
 * `fn_` names it *references* were derived by their owners' lanes and are reached through the owner
 * headers: `0x805145B8` -> `NHTTPi_InitSystemInfo` (NHTTP_bgnend.h), `0x80514EA8` ->
 * `NHTTPi_InitMutexInfo` (NHTTP_bgnend.h), `0x80517614` -> `NHTTPi_InitThreadInfo` (this unit,
 * d_nhttp.h).  The four `fn_` rows this pass wrote bodies for were renamed in the map in the same
 * change: `0x80516CA8` -> `NHTTPi_strcmp`, `0x80516D84` -> `NHTTPi_urlEncodedLength`, `0x80516DF4`
 * -> `NHTTPi_urlEncodedLengthN`.
 *
 * Load-bearing source shapes (each measured):
 *   - `NHTTPi_urlEncodedLength`/`...N` walk a *separate* pointer (`const char* p = s; char c =
 *     *p++;`), not `c = *++s`: with the fused form MWCC emits `lbzu r0,1(r3)` where retail keeps the
 *     separate `lbz`/`addi` pair, and the row drops 48.9 -> 59.6.
 *   - `NHTTPi_encodeUrlChar` takes a `u8*` and stores the hex nibbles without a `char` cast; a
 *     `char*` plus `(char)` casts adds an `extsb` per store that retail does not have.
 *
 * Residuals:
 *   - The four partial rows share one residual: our build folds every `c >= 'A' && c <= 'Z'` style
 *     range test into MWCC's unsigned `subi` + `clrlwi` + `cmplwi` idiom, where retail keeps the two
 *     `cmpwi`s (`cmpwi r0,0x30 / blt / cmpwi r0,0x39 / ble`).  `#pragma peephole off` around
 *     `NHTTPi_encodeUrlChar` was measured and costs 4.75 points (67.38 -> 62.63), so the fold is not
 *     the peephole pass.  It is the single reason `NHTTPi_encodeUrlChar` and both
 *     `urlEncodedLength` rows stop where they do.
 *   - `NHTTPi_strnicmp` (29.02) additionally differs in the both-terminators test: retail emits a
 *     redundant re-test chain (`extsb. r12,r6` / `beq` / `cmpwi r31,0` / `bne` / `cmpwi r12,0` /
 *     `bne` / `cmpwi r31,0` / `bne`) where our `if (c1 == 0 && c2 == 0)` collapses to two `bne`s,
 *     and its `NHTTPi_toLower` stays branchless (`srawi`/`srwi`/`subfc`/`adde`) where ours folds.
 *   - `NHTTPi_InitThreadInfo` (0x80517614) stays a *reference* on purpose: retail calls it from
 *     `NHTTPi_GetSystemInfoP`, so it lives in a different original object than the singleton and
 *     must not be inlined into it.
 *   - The remaining 94 rows are unwritten.  `d_nhttp.c` is a multi-wave unit: the connection /
 *     request / response state machine (`fn_80515774` 1576 B, `fn_8051900C` 1180 B, `fn_805199C4`
 *     1520 B, `fn_80518308` 932 B), the `NHTTPi_Soc*` socket wrappers and the Base64/URL encoders
 *     are all still open.
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

/* 0x80516CA4 (4): tail shim to strlen. */
u32 NHTTPi_strlen(const char* s) {
    return strlen(s);
}

/* 0x80516CA8 (4): tail shim to strcmp. */
int NHTTPi_strcmp(const char* a, const char* b) {
    return strcmp(a, b);
}

/* The case fold `NHTTPi_strnicmp` and `NHTTPi_strToHex` both need.  MWCC inlines it at every call
 * site, so it has no symbol of its own. */
static char NHTTPi_toLower(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

/* 0x80516CAC (0xC): zero-fill shim (memset(dst, 0, n)). */
void* NHTTPi_memclr(void* dst, u32 n) {
    return memset(dst, 0, n);
}

/* 0x80516CB8 (0xCC): the bounded case-insensitive compare.  It answers 0 when both strings run
 * out inside `n` characters, and the number of characters still uncompared otherwise - not the
 * character difference a `strnicmp` would report. */
s32 NHTTPi_strnicmp(const char* s1, const char* s2, s32 n) {
    for (; n > 0; n--) {
        char c1 = *s1++;
        char c2 = *s2++;

        if (c1 == 0 && c2 == 0) {
            n = 0;
            break;
        }
        if (NHTTPi_toLower(c1) != NHTTPi_toLower(c2)) {
            break;
        }
    }
    return n;
}

/* 0x80516E68 (0xA0): write one URL-encoded character at `dst`.  A space becomes `+`, an
 * alphanumeric is copied through, and anything else becomes `%XX` in upper-case hex.  Answers the
 * number of bytes written. */
s32 NHTTPi_encodeUrlChar(u8* dst, char c) {
    u8 value = (u8)c;
    u8 high;
    u8 low;

    if (c == ' ') {
        dst[0] = '+';
        return 1;
    }
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
        dst[0] = value;
        return 1;
    }
    high = value >> 4;
    low = value & 0xF;
    dst[0] = '%';
    dst[1] = high < 10 ? high + '0' : high + '7';
    dst[2] = low < 10 ? low + '0' : low + '7';
    return 3;
}

/* 0x80516D84 (0x70): the URL-encoded length of a NUL-terminated string. */
s32 NHTTPi_urlEncodedLength(const char* s) {
    const char* p = s;
    char c = *p++;
    s32 length = 0;

    while (c != 0) {
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            c == ' ') {
            length += 1;
        } else {
            length += 3;
        }
        c = *p++;
    }
    return length;
}

/* 0x80516DF4 (0x74): the same for at most `n` characters. */
s32 NHTTPi_urlEncodedLengthN(const char* s, s32 n) {
    const char* p = s;
    char c = *p++;
    s32 length = 0;

    for (; n > 0; n--) {
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            c == ' ') {
            length += 1;
        } else {
            length += 3;
        }
        c = *p++;
    }
    return length;
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
