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
 * BODY (2026-09-28, second/third/fourth pass).  29 of the 108 functions are byte-identical, 68 rows
 * are unwritten and the unit is 9.87 % fuzzy over its 24712 B.  The connection-list API
 * (0x8051B0D8..0x8051B79C), the three comm-thread flag accessors and the six public entry points at
 * 0x8051A300..0x8051A5CC are what they added, over the string helpers the first pass left.  The
 * fourth pass is the rule-12 data claim below, which turned two blocked rows into bodies:
 *
 *    100.00  NHTTPi_GetConnectionListLength (0x8051B358, 32 B)
 *     92.91  NHTTPi_ControlConnectionList  (272/264 B) - the list walk, see the residual
 *
 *    100.00  NHTTPi_StartRequest / GetConnectionStatus (0x8051A300 / 0x8051A49C)
 *    100.00  NHTTPDestroy / NHTTPGetError / NHTTPGetSSLError (0x8051A574 / A5A8 / A5CC)
 *    100.00  NHTTPi_Connection2Request / Connection2Response
 *    100.00  NHTTPi_Request2Connection / Response2Connection
 *    100.00  NHTTPi_GetConnection / GetRequest / GetResponse
 *    100.00  NHTTPi_CompleteCallback / SetSock / GetSock
 *    100.00  NHTTPi_InitThreadInfo / markCommThreadReady / isCommThreadReady
 *    100.00  NHTTPi_memcpy / memclr / strlen / strcmp and the Get*InfoP accessors
 *    100.00  NHTTPi_GetSystemInfoP
 *     97.38  NHTTPi_PostSendCallback    (232/232)
 *     92.93  NHTTPi_GetResponseSize     (116/108)
 *     92.26  NHTTPi_BufferFullCallback  (280/276)
 *     92.26  NHTTPi_RecvCallback        (280/276)
 *     67.38  NHTTPi_encodeUrlChar       (160/156)
 *     65.83  NHTTPi_AddConnection / OmitConnectionList (48/48)
 *     61.21  NHTTPi_urlEncodedLengthN   (116/116)
 *     59.64  NHTTPi_urlEncodedLength    (112/112)
 *     29.02  NHTTPi_strnicmp            (204/168)
 *
 * NAMING (rule 7).  Every function defined here already has a real map name (`NHTTPi_*`); the
 * `fn_` names it *references* were derived by their owners' lanes and are reached through the owner
 * headers: `0x805145B8` -> `NHTTPi_InitSystemInfo` (NHTTP_bgnend.h), `0x80514EA8` ->
 * `NHTTPi_InitMutexInfo` (NHTTP_bgnend.h), `0x80517614` -> `NHTTPi_InitThreadInfo` (this unit,
 * d_nhttp.h).  The `fn_` rows this unit has written bodies for were renamed in the map in the same
 * change: `0x80516CA8` -> `NHTTPi_strcmp`, `0x80516D84` -> `NHTTPi_urlEncodedLength`, `0x80516DF4`
 * -> `NHTTPi_urlEncodedLengthN`; (second pass) `0x8051B1E8` -> `NHTTPi_AddConnection`,
 * `0x8051B578` -> `NHTTPi_RecvCallback` (phase 3 of the callback set; the phase is what the body
 * reports, the receive half is a GUESS at which half that is), `0x8051B774` -> `NHTTPi_SetSock` and
 * `0x8051B784` -> `NHTTPi_GetSock` (from the +0x2C slot the `NHTTPi_SocRecv*` wrappers read).  No
 * deferral is needed here: every `fn_` this unit references is reached through its owner's header.
 *
 * Third-pass names, all GUESSes and all read off the body itself, since neither the map nor the
 * runtime dump names them (`dump=zz_051axxx_`):
 *   - `0x8051A300` -> `NHTTPi_StartRequest`: looks the connection up, requires its `+0x10` request,
 *     hands it to `NHTTP_SendRequestAsync`, stores that result at `+0x18` and sets the state `+0x00`
 *     to 1 when the send was accepted.  `NHTTP_SendRequestAsync` already has its real name in the
 *     map, which is what makes this the send path.
 *   - `0x8051A428` -> `NHTTPi_GetResponseSize`: answers `response->+0x438`, the value every phase
 *     callback `fn_8051A5F0` dispatches receives as its last argument.
 *   - `0x8051A49C` -> `NHTTPi_GetConnectionStatus`: answers the connection's `+0x04` word (0xF at
 *     creation, 8 once cancelled, the request's own error on completion), -1 when it is not listed.
 *   - `0x8051A574` -> `NHTTPDestroy`: hands the caller's callback straight to `NHTTPi_CleanupAsync`;
 *     `DWCi_authDataTask` is its caller and passes its own command callback, so this is the public
 *     teardown.  `NHTTPGetError`/`NHTTPGetSSLError` (`0x8051A5A8`/`0x8051A5CC`) are the same shape
 *     over `NHTTPi_GetError`/`NHTTPi_GetSSLError`, and the public `NHTTP*` spelling (against this
 *     unit's internal `NHTTPi_*`) is what the three already-named public entries use.
 *
 * The connection-record layout, read off these bodies and `NHTTPCreateConnection`: the records this
 * list links are the connection objects themselves, so `+0x00` is the state `fn_80518970` sets to 5
 * on completion, `+0x10` the request `fn_80515774` builds, `+0x14` the response it hangs off that
 * request's `+0x2C`, `+0x1C` the completion callback, `+0x20` the list link, `+0x2C` the socket the
 * `NHTTPi_SocRecv*` wrappers read (this unit's `NHTTPSock`), and `+0x40` the 0x8000-byte receive
 * buffer, which ends at `+0x8040`.
 *
 * The record's size (0x8060) has two independent measurements in `NHTTPCreateConnection`: the
 * allocation's own immediate (`lis r30,1` + `addi r3,r30,-32672` = 0x8060, `li r4,32` for 32-byte
 * alignment) and the whole-record `memclr(conn, 0x8060)` it runs before anything else.  The later
 * `memclr(conn+0x40, 0x8000)` is only the receive buffer's re-clear, so `0x40 + 0x8000` is not the
 * record: the buffer ends at 0x8040, the words at `+0x8040`/`+0x8044` are its read offset and its
 * unread count (`NHTTPi_SocRecv_sub` is what moves them), and the extent the bodies address, 0x8048,
 * is what that 32-byte alignment rounds up to 0x8060.
 *
 * Load-bearing source shapes (each measured):
 *   - `NHTTPi_urlEncodedLength`/`...N` walk a *separate* pointer (`const char* p = s; char c =
 *     *p++;`), not `c = *++s`: with the fused form MWCC emits `lbzu r0,1(r3)` where retail keeps the
 *     separate `lbz`/`addi` pair, and the row drops 48.9 -> 59.6.
 *   - `NHTTPi_encodeUrlChar` takes a `u8*` and stores the hex nibbles without a `char` cast; a
 *     `char*` plus `(char)` casts adds an `extsb` per store that retail does not have.
 *   - `NHTTPi_GetSystemInfoP` must keep its `bl NHTTPi_InitThreadInfo`: defining that function here
 *     is enough for the auto-inliner to fold its body in (`stw r0,0xb88(r31)` vs retail's
 *     `addi`+`bl`), so its row drops 100 -> 95.38.  `#pragma dont_inline on` around the singleton
 *     holds it, and is the shape this unit keeps.
 * CLAIMED DATA (rule 12, 2026-09-28).  The unit owns four runs in `splits.txt` and defines every object
 * in them; the `.bss`/`.sbss` ones are zero-filled, and the `.data`/`.sdata` pair is the version tag.
 *
 *   - `.sbss 0x80795878..0x80795888` (16 B) - the run this unit's own target object relocates
 *     (`objdump -r`: 0x80795878 and 0x8079587C are `li r0,1`/`stw` lazy-init flags, in
 *     `NHTTPi_RegisterCallbacks` and `fn_8051A8A4` respectively, each guarding one `OSRegisterVersion`
 *     call; 0x80795880 is the list head the three list bodies read) and which
 *     `auto_10_80795878_sbss.o` splits as one object.  Its fourth word is `NHTTPi_systemInfoP`, so the
 *     singleton slot is defined here now and its band declaration in `include/unsplit/NHTTP.h` is gone
 *     (rule 2's inversion: once the unit owns the range, a band declaration of it is the finding).
 *   - `.bss 0x80762C20..0x80762C60` (0x40 B) - the record `NHTTPi_NotifyCompletion` lazily initialises.
 *     0x40 is the symbol the map and the split give it (`symedit show NHTTPi_completionSync` ->
 *     `object size:0x40`), and 0x24 is the extent the body uses (+0x00 init flag, `OSInitMutex` at
 *     +0x04, the thread queue at +0x1C); a claim is sized by the target object's symbol, so 0x40 is
 *     the extent and the tail stays `pad_0x24`.  Sole referencer: this unit's own object.
 *   - `.sdata 0x80794410..0x80794418` (8 B) and `.data 0x80630C50..0x80630D48` (0xF8 B) - the version
 *     tag the two `OSRegisterVersion` blocks use: the two pointer words and the three banners they
 *     reach (the NHTTP one, an unreferenced `UNOFFICIAL` variant and the NHTTPCREATE one - see the
 *     definitions below for why all three are written and in that order).  Both sections come out
 *     byte-identical to the target object's (`objcopy -O binary --only-section` + `cmp`), which is
 *     playbook 23's test for a `.data` claim: no row moved and no `R_PPC_NONE` pool reloc was lost.
 *
 * Three names are read off what uses them; `NHTTPi_completionSync` is the one GUESS (its role - the
 * mutex and wake queue of the completion notification - is clear, its original spelling is not in the
 * image).  The two pooled banners keep the map's `lbl_80630C50`/`lbl_80630CF8` rows: our object's
 * symbols for them are MWCC's own `@NN` pool entries, so there is no name for this source to give
 * them (the same shape `NWC24/nwc24_msg.c` landed with for its version literal).
 *
 * Still blocked, and why:
 *   - `NHTTPi_NotifyCompletion` (124 B) needs the two 4-byte OS thunks `fn_804D2140` (a branch to
 *     `OSInitThreadQueue`) and `fn_804D2150` (a branch to the implementation the dump also calls
 *     `OSWakeupThread`, 0x804D4B20).  Naming those is a whole-DOL rename for the rename batch, not a
 *     guess this unit may make - the dump gives one name to two addresses.
 *   - `NHTTPi_InitConnectionList` (96 B) prints `.data 0x80630F48` (0x3A B), whose string ends in a
 *     CRLF-producing escape (playbook 45): MWCC on this host cannot write those bytes in-source, so
 *     the row stays unwritten.
 *   - `NHTTPi_RegisterCallbacks` (140 B) and `fn_8051A8A4` (132 B) are no longer blocked - the claim
 *     above gave them their version tag - but their bodies are not written yet.
 *
 * Residuals:
 *   - `NHTTPi_AddConnection`/`NHTTPi_OmitConnectionList` (65.83, 48 B each): the result is the same
 *     -1/0, but retail computes it as `neg`/`nor`/`srawi` (a fused `~(x | -x) >> 31`) where our
 *     build emits the `cntlzw`/`extrwi`/`neg` form.  Twelve spellings were measured in a scratch TU
 *     (`x == 0 ? -1 : 0`, `x ? 0 : -1`, `-(x == 0)`, `(x != 0) - 1`, `!!x - 1`, `(u32)x < 1`,
 *     `(x == 0) * -1`, a `BOOL` local, the signed and the unsigned compare, ...): every natural one
 *     gives `cntlzw`, and only a hand-written `~(x | -x) >> 31` reaches `nor`.  So the idiom is not
 *     reachable from a plain comparison and the two rows stop at 65.83.
 *   - `NHTTPi_GetResponseSize` (92.93, 116/108 B): retail keeps two `li r3,0` return blocks - the
 *     miss on the connection goes to the tail, the miss on the response returns inline - where ours
 *     merges them into the single tail block.  Both the `if/if/return` and the `if (conn == NULL)
 *     return 0` head-guard spellings were measured: 92.93 against 85.86 and 77.83.
 *   - `NHTTPi_PostSendCallback` (97.38) and the two phase-2/3 callbacks (92.26) share one residual:
 *     retail re-loads `connection->callback` for the indirect call (`lwz r12,0x1c(rNN)` again right
 *     before `mtctr`) where ours keeps the tested value in r12 and reuses it, and the register the
 *     connection itself lands in differs (retail r31, ours r29).  `x != NULL && cb != NULL` and the
 *     nested-if form measure identically, and moving the three exchanged words into a nested block
 *     (to change the allocator's web order) does not move either.
 *   - `NHTTPi_strnicmp` (29.02) additionally differs in the both-terminators test: retail emits a
 *     redundant re-test chain (`extsb. r12,r6` / `beq` / `cmpwi r31,0` / `bne` / `cmpwi r12,0` /
 *     `bne` / `cmpwi r31,0` / `bne`) where our `if (c1 == 0 && c2 == 0)` collapses to two `bne`s,
 *     and its `NHTTPi_toLower` stays branchless (`srawi`/`srwi`/`subfc`/`adde`) where ours folds.
 *   - The four partial string rows share one residual: our build folds every `c >= 'A' && c <= 'Z'`
 *     style range test into MWCC's unsigned `subi` + `clrlwi` + `cmplwi` idiom, where retail keeps
 *     the two `cmpwi`s (`cmpwi r0,0x30 / blt / cmpwi r0,0x39 / ble`).  `#pragma peephole off` around
 *     `NHTTPi_encodeUrlChar` was measured and costs 4.75 points (67.38 -> 62.63), so the fold is not
 *     the peephole pass.  It is the single reason `NHTTPi_encodeUrlChar` and both
 *     `urlEncodedLength` rows stop where they do.
 *   - `NHTTPi_ControlConnectionList` (92.91, 272/264 B): the frame, the registers and every arm match
 *     - retail keeps the walk's link slot in r4 inside the `else` branch (so the source declares it
 *     there) and its mode parameter is signed (`cmpwi`) - and the only difference left is the switch's
 *     compare chain: retail's is MWCC's binary search (`cmpwi r30,2` / `beq` / `bge`, then 4, then 0
 *     and 1), ours the linear chain 0,1,2,4 over the same four arms.  Five spellings were measured
 *     (link hoisted 71.09, link in the `else` 89.84, signed mode 92.91, a `default:` arm written first
 *     92.91, a five-case switch 92.91), so the lowering is not reachable from the source shape.
 *   - The remaining 68 rows are unwritten.  `d_nhttp.c` is a multi-wave unit: the connection /
 *     request / response state machine (`fn_80515774` 1576 B, `fn_8051900C` 1180 B, `fn_805199C4`
 *     1520 B, `fn_80518308` 932 B), the `NHTTPi_Soc*` socket wrappers and the Base64 encoder are
 *     still open.
 */

#include "NHTTP/d_nhttp.h"
#include "NHTTP/NHTTP_bgnend.h"
#include "NHTTP/NHTTP_os_RVL.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"

/* The data this unit owns (rule 12, claimed in `splits.txt`): the 16-byte `.sbss` run
 * 0x80795878..0x80795888 - the two version-registration flags, the connection list's head and the
 * system-info slot - and the 0x40-byte `.bss` completion record.  All of it is zero-filled, which is
 * why the objects are plain definitions with no initialiser.  `NHTTPi_systemInfoP` is the word the
 * band header `unsplit/NHTTP.h` used to declare; the other four names are derived from what uses them
 * (the unit header carries the evidence and the one GUESS). */
/* MWCC emits `.sbss` objects in reverse definition order, so the four words are listed back to front
 * for their addresses to come out ascending - the order the map gives them. */
NHTTPInfo* NHTTPi_systemInfoP;                /* 0x80795884 */
NHTTPConnection* NHTTPi_connectionListHead;   /* 0x80795880 */
u32 NHTTPi_createVersionRegistered;           /* 0x8079587C */
u32 NHTTPi_versionRegistered;                 /* 0x80795878 */
NHTTPiCompletionSync NHTTPi_completionSync;   /* 0x80762C20 */

/* The library's version banners (`.sdata` 0x80794410/0x80794414 and the `.data` 0x80630C50..0x80630D48
 * run behind them, rule 12 - claimed 2026-09-28).  `NHTTPi_RegisterCallbacks` and `fn_8051A8A4` each
 * hand one of them to `OSRegisterVersion`, once, behind the two lazy-init flags of the `.sbss` run.
 * The retail `.data` run holds the two NHTTP banners *and* an unreferenced `UNOFFICIAL` variant between
 * them, so all three are written here, in this order - the order is what makes MWCC's pool emit the
 * bytes at the offsets the target object has (A at +0x00, the variant at +0x4C, NHTTPCREATE at +0xA8).
 * The `UNOFFICIAL` object is dead in the retail image too (no relocation points into it), which is why
 * nothing in this unit reads it. */
const char* NHTTPi_versionString =
    "<< RVL_SDK - NHTTP \trelease build: May 12 2009 11:01:00 (0x4199_60831) >>";
char NHTTPi_unofficialVersionString[] =
    "<< RVL_SDK - NHTTP \trelease build: May 12 2009 11:01:00 (0x4199_60831) UNOFFICIAL >>";
const char* NHTTPi_createVersionString =
    "<< RVL_SDK - NHTTPCREATE \trelease build: May 12 2009 11:01:00 (0x4199_60831) >>";
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

/* The public NHTTP entry points 0x8051A300..0x8051A5CC (the map still carries their `fn_` rows;
 * these names are GUESSes derived from their bodies and their callers - the file header writes the
 * evidence for each). */

/* 0x8051A300 (0x8C): start the connection's request, answering -1 when the connection or its
 * request is missing and 0 once the send was accepted. */
s32 NHTTPi_StartRequest(NHTTPConnection* connection) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPConnection* conn = NHTTPi_GetConnection(mutex, connection);

    if (conn == NULL) {
        return -1;
    }
    if (conn->request == NULL) {
        return -1;
    }
    conn->field_0x18 = NHTTP_SendRequestAsync(info, conn->request);
    if (conn->field_0x18 >= 0) {
        conn->field_0x00 = 1;
    }
    return 0;
}

/* 0x8051A428 (0x74): the response's own size word (+0x438), or 0 when the connection or its
 * response is missing.  It is the last argument of every phase callback `fn_8051A5F0` dispatches. */
s32 NHTTPi_GetResponseSize(NHTTPConnection* connection) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHTTPConnection* conn = NHTTPi_GetConnection(mutex, connection);
    NHTTPResponse* response;

    if (conn != NULL) {
        response = NHTTPi_Connection2Response(mutex, conn);

        if (response != NULL) {
            return response->field_0x438;
        }
    }
    return 0;
}

/* 0x8051A49C (0x4C): the connection's status word (+0x04 - 0xF at creation, 8 once cancelled),
 * or -1 when the connection is not in the list. */
s32 NHTTPi_GetConnectionStatus(NHTTPConnection* connection) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHTTPConnection* conn = NHTTPi_GetConnection(mutex, connection);

    if (conn != NULL) {
        return conn->field_0x04;
    }
    return -1;
}

/* 0x8051A574 (0x34): tear the HTTP layer down.  The caller's callback is handed straight to
 * `NHTTPi_CleanupAsync`, which is why the parameter is the callback and not a handle. */
void NHTTPDestroy(NHTTPCompletionCallback callback) {
    NHTTPi_CleanupAsync(NHTTPi_GetSystemInfoP(), callback);
}

/* 0x8051A5A8 (0x24): the last error the library recorded. */
s32 NHTTPGetError(void) {
    return NHTTPi_GetError(NHTTPi_GetSystemInfoP());
}

/* 0x8051A5CC (0x24): the last SSL error the library recorded. */
s32 NHTTPGetSSLError(void) {
    return NHTTPi_GetSSLError(NHTTPi_GetSystemInfoP());
}

/* 0x80517614 (0xC): clear the comm thread's ready flag. */
void NHTTPi_InitThreadInfo(NHTTPThreadInfo* info) {
    info->ready = 0;
}

/* 0x80517620 (0xC): set the comm thread's ready flag. */
void NHTTPi_markCommThreadReady(NHTTPThreadInfo* info) {
    info->ready = 1;
}

/* 0x8051762C (0x8): the comm thread's ready flag. */
s32 NHTTPi_isCommThreadReady(NHTTPThreadInfo* info) {
    return info->ready;
}

/* The connection list (`NHTTPi_ControlConnectionList`'s own chain).  The request-list handle is
 * stored as an `s32` by `NHTTPRequestNode`, so the two lookup wrappers re-type it for the shared
 * key parameter rather than change the sibling unit's record. */

/* 0x8051B0D8 (0x110): the one body that walks the connection list, under the request-list lock.  The
 * three get modes answer the connection the key finds - by the connection itself, by the request it
 * holds or by its response - add puts the connection at the head, and omit unlinks it from the chain.
 * Answers the connection the operation found, and null when it found none. */
/* untyped: opaque handle - the connection itself, or the request/response it holds */
NHTTPConnection* NHTTPi_ControlConnectionList(NHTTPMutexInfo* mutex, void* key, s32 mode) {
    NHTTPConnection* node;
    NHTTPConnection* result = NULL;

    NHTTPi_lockReqList(mutex);
    if (mode == NHTTPI_LIST_ADD) {
        result = key;
        ((NHTTPConnection*)key)->next = NHTTPi_connectionListHead;
        NHTTPi_connectionListHead = key;
    } else {
        /* The walk's link slot lives here, not at the top: retail materialises it inside this branch
         * (`li r4, NHTTPi_connectionListHead@sda21`), which is what keeps it in a volatile register
         * and the frame's saved set at four (r28..r31) instead of five. */
        NHTTPConnection** link = &NHTTPi_connectionListHead;

        while ((node = *link) != NULL) {
            switch (mode) {
            case NHTTPI_LIST_GET:
                if (node == key) {
                    result = node;
                }
                break;
            case NHTTPI_LIST_BY_REQUEST:
                if (node->request == key) {
                    result = node;
                }
                break;
            case NHTTPI_LIST_BY_RESPONSE:
                if (node->response == key) {
                    result = node;
                }
                break;
            case NHTTPI_LIST_OMIT:
                if (node == key) {
                    result = node;
                    *link = node->next;
                }
                break;
            }
            if (result != NULL) {
                break;
            }
            link = &node->next;
        }
    }
    NHTTPi_unlockReqList(mutex);
    return result;
}

/* 0x8051B1E8 (0x30): put the connection at the head of the list, answering 0 when it went in and -1
 * when it did not. */
s32 NHTTPi_AddConnection(NHTTPMutexInfo* mutex, NHTTPConnection* connection) {
    return NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_ADD) == NULL ? -1 : 0;
}

/* 0x8051B218 (0x30): take the connection out of the list, answering 0 when it was in it and -1 when
 * it was not. */
s32 NHTTPi_OmitConnectionList(NHTTPMutexInfo* mutex, NHTTPConnection* connection) {
    return NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_OMIT) == NULL ? -1 : 0;
}

/* 0x8051B248 (0x38): the request the connection holds, or null when it is not in the list. */
struct NHTTPRequest* NHTTPi_Connection2Request(NHTTPMutexInfo* mutex, NHTTPConnection* connection) {
    NHTTPConnection* node = NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_GET);

    return node != NULL ? node->request : NULL;
}

/* 0x8051B280 (0x38): the response the connection holds, or null when it is not in the list. */
NHTTPResponse* NHTTPi_Connection2Response(NHTTPMutexInfo* mutex, NHTTPConnection* connection) {
    NHTTPConnection* node = NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_GET);

    return node != NULL ? node->response : NULL;
}

/* 0x8051B2B8 (0x8): the connection that serves the request.  `NHTTPi_cancelRequest` reads the handle
 * out of `NHTTPRequestNode.request`, which is an `s32`. */
NHTTPConnection* NHTTPi_Request2Connection(NHTTPMutexInfo* mutex, s32 request) {
    return NHTTPi_ControlConnectionList(mutex, (void*)(u32)request, NHTTPI_LIST_BY_REQUEST);
}

/* 0x8051B2C0 (0x8): the connection that serves the response. */
NHTTPConnection* NHTTPi_Response2Connection(NHTTPMutexInfo* mutex, s32 response) {
    return NHTTPi_ControlConnectionList(mutex, (void*)(u32)response, NHTTPI_LIST_BY_RESPONSE);
}

/* 0x8051B2C8 (0x8): the connection itself, or null when it is not in the list. */
NHTTPConnection* NHTTPi_GetConnection(NHTTPMutexInfo* mutex, NHTTPConnection* connection) {
    return NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_GET);
}

/* 0x8051B2D0 (0x44): the request the connection holds.  The list's own key is handed back when the
 * connection is not in it, so both ends of the lookup are the same handle. */
/* untyped: opaque handle - the record the connection holds, or the key handed back on a miss */
void* NHTTPi_GetRequest(NHTTPMutexInfo* mutex, NHTTPConnection* key) {
    NHTTPConnection* node = NHTTPi_ControlConnectionList(mutex, key, NHTTPI_LIST_GET);

    return node != NULL ? node->request : (void*)key;
}

/* 0x8051B314 (0x44): the response the connection holds, on the same terms as the request. */
/* untyped: opaque handle - the record the connection holds, or the key handed back on a miss */
void* NHTTPi_GetResponse(NHTTPMutexInfo* mutex, NHTTPConnection* key) {
    NHTTPConnection* node = NHTTPi_ControlConnectionList(mutex, key, NHTTPI_LIST_GET);

    return node != NULL ? node->response : (void*)key;
}

/* 0x8051B358 (0x20): how many connections the list holds.  It walks the chain without taking the
 * request-list lock - the count is never more than a length. */
s32 NHTTPi_GetConnectionListLength(void) {
    NHTTPConnection* node = NHTTPi_connectionListHead;
    s32 count = 0;

    while (node != NULL) {
        node = node->next;
        count++;
    }
    return count;
}

/* 0x8051B378 (0xE8): report the send phase to the connection's callback (phase 1) and put the two
 * words it may have updated back.  Answers the callback's own value, or -1 when there is nothing to
 * report. */
s32 NHTTPi_PostSendCallback(NHTTPMutexInfo* mutex, NHTTPConnection* connection, u32 arg1, u32 arg2) {
    NHTTPCallbackArgs args;
    NHTTPConnection* node;
    NHTTPResponse* response;
    s32 result = -1;

    if (NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_GET) != NULL) {
        node = NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_GET);
        response = node != NULL ? node->response : NULL;
        if (response != NULL && connection->callback != NULL) {
            args.field_0x00 = arg1;
            args.field_0x04 = connection->field_0x24;
            args.field_0x08 = connection->field_0x28;
            args.field_0x0C = arg2;
            result = connection->callback(connection, 1, &args);
            arg1 = args.field_0x04;
            arg2 = args.field_0x08;
            node = NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_GET);
            if (node != NULL) {
                node->field_0x24 = arg1;
                node->field_0x28 = arg2;
            }
        }
    }
    return result;
}

/* 0x8051B460 (0x118): report the receive-buffer-full phase (2) to the connection's callback and put
 * the response's three words back.  The three words are read out before the callback so that the two
 * list lookups below cannot overwrite them. */
void NHTTPi_BufferFullCallback(NHTTPMutexInfo* mutex, NHTTPConnection* connection) {
    NHTTPCallbackArgs args;
    NHTTPConnection* node;
    NHTTPResponse* response;

    if (NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_GET) != NULL) {
        response = NHTTPi_Connection2Response(mutex, connection);
        if (response != NULL) {
            if (connection->callback != NULL) {
                args.field_0x00 = response->field_0x28;
                args.field_0x04 = response->field_0x1C;
                args.field_0x08 = response->field_0x04;
                connection->callback(connection, 2, &args);
                {
                    u32 field_0x28 = args.field_0x00;
                    u32 field_0x1C = args.field_0x04;
                    u32 field_0x04 = args.field_0x08;

                    node = NHTTPi_GetConnection(mutex, connection);
                    if (node != NULL) {
                        response = NHTTPi_Connection2Response(mutex, node);
                        if (response != NULL) {
                            response->field_0x28 = field_0x28;
                            response->field_0x1C = field_0x1C;
                            response->field_0x04 = field_0x04;
                        }
                    }
                }
            }
        }
    }
}

/* 0x8051B578 (0x118): the same report for the receive phase (3). */
void NHTTPi_RecvCallback(NHTTPMutexInfo* mutex, NHTTPConnection* connection) {
    NHTTPCallbackArgs args;
    NHTTPConnection* node;
    NHTTPResponse* response;

    if (NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_GET) != NULL) {
        response = NHTTPi_Connection2Response(mutex, connection);
        if (response != NULL) {
            if (connection->callback != NULL) {
                args.field_0x00 = response->field_0x28;
                args.field_0x04 = response->field_0x1C;
                args.field_0x08 = response->field_0x04;
                connection->callback(connection, 3, &args);
                {
                    u32 field_0x28 = args.field_0x00;
                    u32 field_0x1C = args.field_0x04;
                    u32 field_0x04 = args.field_0x08;

                    node = NHTTPi_GetConnection(mutex, connection);
                    if (node != NULL) {
                        response = NHTTPi_Connection2Response(mutex, node);
                        if (response != NULL) {
                            response->field_0x28 = field_0x28;
                            response->field_0x1C = field_0x1C;
                            response->field_0x04 = field_0x04;
                        }
                    }
                }
            }
        }
    }
}

/* 0x8051B690 (0x58): report the completion phase (4) to the connection's callback.  Nothing is
 * exchanged, so the callback is handed no argument block. */
void NHTTPi_CompleteCallback(NHTTPMutexInfo* mutex, NHTTPConnection* connection) {
    if (NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_GET) != NULL) {
        if (connection->callback != NULL) {
            connection->callback(connection, 4, NULL);
        }
    }
}

/* 0x8051B774 (0x10): the socket handle the connection's `NHTTPi_SocRecv*` wrappers read. */
void NHTTPi_SetSock(NHTTPConnection* connection, NHTTPSock* sock) {
    if (connection != NULL) {
        connection->sock = sock;
    }
}

/* 0x8051B784 (0x18): the same slot, null for a null connection. */
NHTTPSock* NHTTPi_GetSock(NHTTPConnection* connection) {
    if (connection != NULL) {
        return connection->sock;
    }
    return NULL;
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

/* 0x8051B6E8 (0x68): the lazily-initialised NHTTP system-info singleton.  `NHTTPi_InitThreadInfo`
 * is defined in this file, so the four initialisers above must stay calls: with the auto-inliner
 * free to take it, its body is folded in here and the row loses the `bl` retail keeps. */
#pragma dont_inline on
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
#pragma dont_inline off
