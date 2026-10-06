/*
 * d_nhttp.c - the Revolution SDK NHTTP library's core TU, `.text` 0x80515774..0x8051B7FC
 * (108 functions, 24712 B): the connection/request/response objects and their list, the public
 * NHTTP calls over them, the comm thread that sends a request and receives its answer, the `NHTTPi_Soc*`
 * socket wrappers, and the string/URL/Base64 helpers.
 *
 * REGISTRATION.  Left edge 0x80515774 is the first referrer of the TU's `.data` strings (`"https://"`,
 * the base64 alphabet, `"CONNECT "`, the header literals) and the `.sdata` run jump 0x807943A0 ->
 * 0x807943A8 admits it; right edge 0x8051B7FC is `SSLNew`, the SSL library's start.  The source name is
 * `d_nhttp.c` (`NHTTPSetSystemProxy` panics with it, `.data:0x80630F04`).  One TU: the `.data` literals
 * appear in address order of their first use, from `createRequestObject` to `InitConnectionList`.
 *
 * FLAGS.  GC/3.0a5.2 with `cflags_nhttp` plus the unit's own `#pragma inline_max_size(8)` (below).
 *
 * BODY (sixth pass).  All 108 functions are written: 48 are byte-identical, 102 are >= 80 %,
 * the unit is 93.56 % fuzzy (13.84 % before this pass).  This pass wrote the request/response object
 * builder and teardown, the socket layer, the comm thread and its nine steps (connect, SSL tunnel,
 * request writer, three post-body senders, head/body receivers, header parser), the public calls and
 * `NHTTPi_Base64Encode`.
 *
 * NAMING (rule 7).  Every `fn_` row of the unit was named from its body and its callers, and the map
 * renamed with it; the SO resolver pair (`SOGetAddrInfo`/`SOFreeAddrInfo`) and the two 4-byte OS stubs
 * (`OSInitThreadQueueThunk`/`OSWakeupThreadThunk`) it calls were named too.  Evidence per family:
 *   - the public calls carry the SDK's own spelling where a `.data` string names it (`NHTTPAddHeaderField`,
 *     `NHTTPAddPostDataAscii`, `NHTTPSetProxy`), and the SDK's public vocabulary otherwise
 *     (`NHTTPCreateRequest`, `NHTTPSendRequestAsync`, `NHTTPCancelRequest`, `NHTTPDestroyResponse`,
 *     `NHTTPGetBodyAll`, `NHTTPGetResultCode`, `NHTTPSetVerifyOption`, `NHTTPClearRootCA`,
 *     `NHTTPClearClientCert`, `NHTTPSetSystemProxy`) - these are GUESSes: the body says what each does,
 *     the retail spelling is not in the image.
 *   - the internals are GUESSes read off the body: `NHTTPi_createRequestObject` (0x258-byte request +
 *     0x43C-byte response, URL parser), `NHTTPi_cancelRequestById`, `NHTTPi_probeSocket` (peek: 0 alive,
 *     -1 closed), `NHTTPi_openSocket`, `NHTTPi_SocConnect`, `NHTTPi_SocShutdown`, `NHTTPi_resolveHostName`,
 *     `NHTTPi_strToDec`, `NHTTPi_uintToStr`, `NHTTPi_isHeaderEnd`, `NHTTPi_findHeaderField`, and the
 *     comm-thread steps (`dequeueRequest`, `prepareTarget`, `connectSocket`, `negotiateSSL`,
 *     `sendRequest`, `sendHeaderFields`, `sendRawPostBody`, `sendMultipartBody`, `sendUrlEncodedBody`,
 *     `sendProxyConnect`, `recvProxyConnectReply`, `recvResponseHeaders`, `parseResponseHeaders`,
 *     `recvResponseBody`, `finishRequest`) whose behaviour is fixed by the strings they write.
 * Layouts: `NHTTPRequest` (bgnend.h), `NHTTPResponse`/`NHTTPConnection`/`NHTTPCommContext`/
 * `NHTTPProxyConfig` (d_nhttp.h) were completed from the field offsets the bodies use; the response IS the
 * receive ring `NHTTP_os_RVL.h` describes plus the caller's own word, and the sibling's `NHTTPSock`/
 * `conn` are that response and its request (see `NHTTPi_SocRecv` in d_nhttp.h).
 *
 * Load-bearing source shapes (each measured):
 *   - `#pragma inline_max_size(8)`: retail inlines only the one-line list wrappers (`Connection2Response`,
 *     `GetConnection`, ...) and keeps a `bl` to every bigger function; the default limit folds
 *     `NHTTPi_SocSend`, `NHTTPGetBodyBuffer`, `NHTTPi_GetResponseSize` into their callers (SaveBuf 46.5 ->
 *     95.4, GetBodyAll 28.2 -> 92.3).  8 and 10 are identical; 4 and 6 also stop the wrappers.
 *   - the file is in ADDRESS ORDER, so a callee defined below its caller stays a `bl` exactly as in retail
 *     (an out-of-order file inlined the request teardown into its users).
 *   - the four 4-12 byte shims (`NHTTPi_memcpy`/`strlen`/`strcmp`/`memclr`) are `asm`: MWCC inlines a C
 *     tail-call shim into every later caller, retail never does.
 *   - `static inline` for the two helpers retail inlines (`NHTTPi_toLower`, `NHTTPi_freeFieldList`,
 *     the proxy/basic-authorization writers): a plain `static` one is emitted as an extra symbol.
 *   - a range test is a source input: `c >= LO && c <= HI` folds into an unsigned compare retail almost
 *     never has.  A `&` pair in parentheses (`toLower`) or the constant on the LEFT (`'0' <= c`,
 *     `3 <= method`, `'0' > c`) keeps retail's two compares; `urlEncodedLength` is the one exception.
 *   - a `switch` on the post mode (`case 0: case 1:` before `case 2:`) reproduces retail's compare tree
 *     where `if (mode == 2) ... else if (mode < 2)` gave 64 % (sendPostField 64.4 -> 95.0).
 *   - a `for (;;) { ... if (x == 0) break; ... } return y;` puts the shared return at the loop's exit as
 *     retail does; locals declared uninitialised and set after the guard (`strToDec`) move the `li`.
 *   - `u8 previous` compared as `(s8)previous` keeps retail's re-extension of the byte at every test.
 *   - `NHTTPi_createRequestObject` is a `do { ... } while (0)` whose `break`s reach the shared cleanup
 *     (rule 8: no `goto`); the retail error blocks are separate per check, ours merge two of them.
 *   - kept from the earlier passes: `urlEncodedLength`/`...N` walk a separate pointer (`c = *p++`, not
 *     `*++s`, which fuses into `lbzu`); `encodeUrlChar` takes a `u8*` and stores the nibbles uncast;
 *     `NHTTPi_GetSystemInfoP` sits in `#pragma dont_inline on` so `InitThreadInfo` stays a `bl`;
 *     `containsString` walks the haystack with a pointer and tests `i < haystackLen - needleLen + 1`;
 *     `ControlConnectionList` declares its link slot inside the `else` and takes `mode` signed.
 *
 * CLAIMED DATA (rule 12).  The unit owns four runs in `splits.txt` and defines every object in them; the
 * `.bss`/`.sbss` ones are zero-filled, and the `.data`/`.sdata` pair is the version tag.
 *   - `.sbss 0x80795878..0x80795888` (16 B): two `OSRegisterVersion` lazy-init flags, the connection
 *     list's head and `NHTTPi_systemInfoP`.
 *   - `.bss 0x80762C20..0x80762C60` (0x40 B): the record `NHTTPi_NotifyCompletion` lazily initialises
 *     (`NHTTPi_completionSync`, name a GUESS; only 0x24 B are used, the tail stays padding).
 *   - `.sdata 0x80794410..0x80794418` and `.data 0x80630C50..0x80630D48`: the version tag (two pointer
 *     words and the three banners, incl. an unreferenced `UNOFFICIAL` one); byte-identical to the target.
 * NOT claimed, and why the literals below are local: the rest of the unit's strings sit in unowned runs
 * (`.data 0x80630B28..0x80630C50` and `0x80630D48..0x80630F88`, `.rodata 0x80574D00..0x80574DFC`,
 * `.sdata 0x807943A8..0x80794410` and the `":"` at 0x80794418); four requests are filed in
 * `tools/units/data-requests.json`.  Until they are claimed the unit's object carries its own literals
 * (`datagap`: ours-extra `.data` +657 B, `.rodata` 249 B, `.sdata` +104 B) and a string row's relocation
 * names its `@NN` literal where retail names `lbl_8063xxxx`.  Even a claim cannot make `.data` byte-equal:
 * retail's pool also holds the messages of four public calls the linker dropped (`NHTTPAddPostDataBinary`,
 * `NHTTPAddPostDataRaw`, `NHTTPSetPostDataEncoding`, `NHTTPDisableVerifyOptionForDebug`, at 0x80630DA8..),
 * so those bytes exist only in the target.  `-str reuse` also gives one symbol per literal where retail pools
 * several behind one base.
 *
 * Residuals (fuzzy, per function; built with GC/3.0a5.2, the lib's compiler since docs/network.md "SDK library
 * compilers"; some of the numbers below were taken under Wii/1.3):
 *   - `NHTTPi_Base64Encode` 48.8: retail divides by 3 with `li r0,3`/`divwu` and `groups * 3` with `mulli`
 *     (no reciprocal multiply, no shift-subtract) and schedules the two unrolled loops differently; no
 *     spelling of the division (`u32`, `3U`, a local, `opt size`, levels 2/3, peephole off) reproduces it.
 *   - `NHTTPi_urlEncodedLength` 59.64 / `NHTTPi_strtonum` 72.78 / `NHTTPi_containsString` 73.49 /
 *     `NHTTPi_strnicmp` 86.37 / `NHTTPi_compareToken` 94.22 /
 *     `NHTTPi_encodeUrlChar` 93.38: as recorded by the earlier passes - retail keeps a value in a
 *     callee-saved register (a frame ours does not need), a redundant terminator re-test in `strnicmp`,
 *     and merges two ranges in `urlEncodedLength` that the constant-left spelling would fuse.
 *   - `NHTTPi_AddConnection`/`NHTTPi_OmitConnectionList` 65.83: retail's fused `~(x | -x) >> 31`;
 *     `NHTTPi_ControlConnectionList` 92.91: retail's binary-search switch over modes 0-4.
 *   - `NHTTPi_dispatchConnectionCallback` 93.16 (retail's switch is a compare tree over 1-4 and holds the
 *     phase in r31), `NHTTPi_recvResponseBody` 92.99 / `NHTTPi_parseResponseHeaders` 87.10 /
 *     `NHTTPi_connectSocket` 88.64 / `NHTTPi_recvResponseHeaders` 91.01 / `NHTTPi_createRequestObject`
 *     91.92 / `NHTTPi_commThreadLoop` 91.94: register numbering of the longest-lived locals, and retail's
 *     `li r0,N; li r3,0; stw` (the return value loaded before the error store) where ours stores first -
 *     no scheduling pragma or return spelling moves it; the `.data` pool offsets (`addi r4,r28,0x88` off
 *     one base) are the unclaimed-literal effect above.
 *   - `NHTTPi_SocSSLConnect` 95.9 (retail's handshake result tree), `NHTTPi_SocRecv` 90.6 /
 *     `NHTTPi_SocSend` 95.3 / `NHTTPi_SocSend_sub` 97.0, `NHTTPi_cancelRequestById` 93.8 /
 *     `NHTTPi_cancelAllRequests` 92.7 (retail reloads `active->request` for the store), `NHTTP_SendRequestAsync`
 *     95.8 (retail stores the flag after the argument setup), `NHTTPi_GetResponseSize`/`BufferFull`/
 *     `Recv`/`PostSendCallback` (earlier passes): one to a few instructions of scheduling or allocation.
 *   - Relocation names that differ: `NHTTPi_createRequestObject` +0x21c, retail calls `NHTTPi_SetError` where ours
 *     inlines it; `NHTTPi_Base64Encode` reaches its table off a base where retail names `NHTTPi_base64Alphabet`;
 *     `NHTTPi_sendUrlEncodedBody` names `NHTTPi_urlEncodedHeader` where retail names the pool label `lbl_80630BDC`.
 *
 * Shared files touched (each minimal, all listed in the outbox): `NHTTPRequest` completed in
 * `NHTTP/NHTTP_bgnend.h` (+ `quitFlag`/`socket`/`sslHook` renames in `NHTTP_bgnend.c`), the request-info
 * word `active` in `NHTTP_os_RVL.c`, `NHTTPi_RecvBufFindUpper`/`FindSpace` prototypes in `NHTTP_os_RVL.h`,
 * the SO/SSL/OS calls in `unsplit/SO.h`, the new `unsplit/SSL.h` and `unsplit/OS.h`.
 */

#include "NHTTP/d_nhttp.h"
#include "NHTTP/NHTTP_bgnend.h"
#include "NHTTP/NHTTP_os_RVL.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "unsplit/SO.h"
#include "SO/soi.h"
#include "unsplit/SSL.h"

/* Retail inlines only the one-line list wrappers into this file's bodies and keeps a `bl` to every
 * bigger function (`NHTTPi_SocSend`, `NHTTPGetBodyBuffer`, `NHTTPi_GetResponseSize`, ...); an inline
 * limit of 8-10 reproduces that, where the default limit folds them all in. */
#pragma inline_max_size(8)

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
 * run behind them, rule 12 - claimed 2026-09-28).  `NHTTPi_RegisterCallbacks` and `NHTTPCreateRequest` each
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

/* Free every node of a circular header-field list (the head's own `next` is the node after the
 * head; the head goes last).  MWCC inlines it at the four sites that empty a request's lists. */
static inline void NHTTPi_freeFieldList(NHTTPHeaderField* head) {
    while (head != NULL) {
        NHTTPHeaderField* second = head->next;

        if (head != second) {
            NHTTPHeaderField* rest = second->next;

            NHTTPi_free(second);
            head->next = rest;
        } else {
            NHTTPi_free(head);
            head = NULL;
        }
    }
}

/* Take the request object apart: its response, both field lists, the URL copies and the record. */
void NHTTP_DestroyRequest(NHTTPInfo* info, NHTTPRequest* request) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPConnection* connection = NHTTPi_Request2Connection(mutex, (s32)request);

    if (connection != NULL) {
        connection->response = NULL;
    }
    NHTTPi_free(request->response);
    connection = NHTTPi_Request2Connection(mutex, (s32)request);
    if (connection != NULL) {
        connection->request = NULL;
    }
    NHTTPi_freeFieldList(request->headerList);
    NHTTPi_freeFieldList(request->postDataList);
    NHTTPi_free(request->url);
    NHTTPi_free(request->host);
    NHTTPi_free(request);
}

/* The same teardown for a request that never got a response record: answers 1. */
s32 NHTTPi_destroyRequestObject(NHTTPMutexInfo* mutex, s32 request) {
    NHTTPRequest* req = (NHTTPRequest*)request;
    NHTTPConnection* connection = NHTTPi_Request2Connection(mutex, request);

    if (connection != NULL) {
        connection->request = NULL;
    }
    NHTTPi_freeFieldList(req->headerList);
    NHTTPi_freeFieldList(req->postDataList);
    NHTTPi_free(req->url);
    NHTTPi_free(req->host);
    NHTTPi_free(req);
    return 1;
}

/* Queue the request for the comm thread and wake it.  Answers the request's id in the list, or -1
 * when the request was already sent (error 11) or the list refused it (error 1). */
s32 NHTTP_SendRequestAsync(NHTTPInfo* info, NHTTPRequest* request) {
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    NHTTPThreadInfo* thread = NHTTPi_GetThreadInfoP(info);
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    s32 id;

    if (request->field_0x04 != 0) {
        NHTTPi_SetError(bgnend, 11);
        return -1;
    }
    NHTTPi_lockReqList(mutex);
    id = NHTTPi_insertRequest(NHTTPi_GetListInfoP(info), (s32)request);
    if (id >= 0) {
        request->field_0x04 = 1;
        NHTTPi_sendQuitMessage(&thread->queue);
    } else {
        NHTTPi_SetError(bgnend, 1);
    }
    NHTTPi_unlockReqList(mutex);
    return id;
}

/* Shut the socket of the request the comm thread is serving down, when that is request `id`; a
 * request still waiting in the list is cancelled instead.  Answers 1 once it found the request. */
s32 NHTTPi_cancelRequestById(NHTTPInfo* info, s32 id) {
    s32 found = 0;
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPRequestNode* active = requestInfo->active;

    NHTTPi_lockReqList(mutex);
    if (active != NULL) {
        if (active->id == id) {
            if (((NHTTPRequest*)active->request)->cancelled == 0) {
                ((NHTTPRequest*)active->request)->cancelled = 1;
                NHTTPi_SocShutdown(mutex, (NHTTPRequest*)active->request, active->state);
                found = 1;
            }
        }
    }
    if (found == 0) {
        found = NHTTPi_cancelRequest(NHTTPi_GetListInfoP(info), mutex, id);
    }
    NHTTPi_unlockReqList(mutex);
    return found;
}

/* Cancel the request the comm thread is serving and every request still waiting in the list. */
void NHTTPi_cancelAllRequests(NHTTPInfo* info) {
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPRequestList* listInfo = NHTTPi_GetListInfoP(info);
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPRequestNode* active = requestInfo->active;

    NHTTPi_lockReqList(mutex);
    if (active != NULL) {
        if (((NHTTPRequest*)active->request)->cancelled == 0) {
            ((NHTTPRequest*)active->request)->cancelled = 1;
            NHTTPi_SocShutdown(mutex, (NHTTPRequest*)active->request, active->state);
        }
    }
    NHTTPi_cancelConnectionRequests(listInfo, mutex);
    NHTTPi_unlockReqList(mutex);
}

/* Free the response's block list, its two aux buffers and the caller's buffer (through the free
 * callback), then the record itself. */
void NHTTP_DestroyResponse(NHTTPMutexInfo* mutex, NHTTPResponse* response) {
    NHTTPConnection* connection;

    while (response->blocks != NULL) {
        NHTTPRecvBlock* next = response->blocks->next;

        NHTTPi_free(response->blocks);
        response->blocks = next;
    }
    if (response->auxBufferA != NULL) {
        NHTTPi_free(response->auxBufferA);
    }
    if (response->auxBufferB != NULL) {
        NHTTPi_free(response->auxBufferB);
    }
    if (response->freeCallback != NULL) {
        response->freeCallback(response->userBuffer, NHTTPi_free, response->userData);
        response->userBuffer = 0;
        response->bufferSize = 0;
    }
    connection = NHTTPi_Response2Connection(mutex, (s32)response);
    if (connection != NULL) {
        connection->response = NULL;
    }
    NHTTPi_free(response);
}

/* Find header field `name` in the response head: answers the length of its value (-1 when there is
 * no such field, 0 for an empty one) and puts the value's start offset in `*offset`. */
s32 NHTTPi_findHeaderField(NHTTPRecvBuf* ring, const char* name, s32* offset) {
    s32 colon;
    s32 flag = 0;
    s32 valueEnd;
    s32 valueStart;
    s32 line;
    s32 next;

    line = NHTTPi_RecvBufFindLine(ring, 12, ring->capacity, &colon, &flag);

    while (line > 0) {
        next = NHTTPi_RecvBufFindLine(ring, line, ring->capacity, &colon, &flag);
        if (colon > 0 && NHTTPi_RecvBufFindUpper(ring, line, colon, name, 0) == 0) {
            if (colon + 1 < ring->capacity) {
                valueEnd = NHTTPi_RecvBufFindLine(ring, colon + 1, ring->capacity, NULL, &flag);
                if (valueEnd <= 0) {
                    valueEnd = ring->capacity;
                } else {
                    if (valueEnd < flag) {
                        return -1;
                    }
                    valueEnd -= flag;
                }
                valueStart = NHTTPi_RecvBufFindSpace(ring, colon + 1, valueEnd);
                if (valueStart < 0) {
                    valueStart = valueEnd;
                }
                *offset = valueStart;
                return valueEnd - valueStart;
            }
            return 0;
        }
        line = next;
    }
    return -1;
}

/* Probe whether an idle socket is still alive: 0 when a peek would block, -1 when it would not (the
 * peer closed or sent data nobody asked for) or when there is no socket. */
s32 NHTTPi_probeSocket(s32 fd) {
    s32 result = 0;
    u32 buffer;

    if (fd == -1) {
        result = -1;
    } else if (SORecv(fd, &buffer, 1, 4) != -6) {
        result = -1;
    }
    return result;
}

/* Open a stream socket and apply the request's buffer sizes. */
s32 NHTTPi_openSocket(NHTTPRequest* request) {
    s32 fd = __SOCreateSocket(2, 1, 0);
    u32 receiveSize = 0;
    u32 sendSize = 0;

    if (request != NULL) {
        receiveSize = request->recvBufferSize;
    }
    if (fd >= 0 && receiveSize != 0) {
        SOSetSockOpt(fd, 0xFFFF, 0x1002, &receiveSize, 4);
    }
    if (request != NULL) {
        sendSize = request->sendBufferSize;
    }
    if (fd >= 0 && sendSize != 0) {
        SOSetSockOpt(fd, 0xFFFF, 0x1001, &sendSize, 4);
    }
    return fd;
}

/* Close the request's SSL context (under the request-list lock), then the socket itself. */
s32 NHTTPi_SocClose(NHTTPMutexInfo* mutex, NHTTPRequest* request, s32 fd) {
    NHTTPi_lockReqList(mutex);
    if (request->sslHandle > 0) {
        SSLShutdown(request->sslHandle);
        request->sslHandle = -1;
    }
    NHTTPi_unlockReqList(mutex);
    return SOClose(fd);
}

/* Set the request's SSL context up (client certificate, root CA) and run the handshake to its end.
 * Answers 0 once it completed, -1005/-1004 when a certificate/CA is refused and -1001 on a
 * handshake failure. */
s32 NHTTPi_SocSSLConnect(NHTTPInfo* info, NHTTPMutexInfo* mutex, NHTTPRequest* request, s32 fd) {
    s32 done = 0;
    NHTTPConnection* connection;
    s32 result;

    request->sslHandle = SSLNew(request->verifyOption, request->host);
    if (info->sslHook != NULL && request->sslCallbackArg != 0) {
        info->sslHook(request->sslHandle, request->sslCallbackArg);
    }
    if (request->builtinClientCert == 1) {
        if (SSLSetBuiltinClientCert(request->sslHandle, request->builtinClientCertId) != 0) {
            return -1005;
        }
    } else if (request->clientCert != 0 && request->clientKey != 0) {
        if (SSLSetClientCert(request->sslHandle, request->clientCert, request->clientCertLength,
                             request->clientKey, request->clientKeyLength) != 0) {
            return -1005;
        }
    }
    if (request->rootCA != 0) {
        if (SSLSetRootCA(request->sslHandle, request->rootCA, request->rootCALength) != 0) {
            return -1004;
        }
    } else if (SSLSetBuiltinRootCA(request->sslHandle, request->builtinRootCAId) != 0) {
        return -1004;
    }
    if (SSLConnect(request->sslHandle, fd) < -1) {
        return -1001;
    }
    while (done == 0) {
        connection = NHTTPi_Request2Connection(mutex, (s32)request);
        result = SSLDoHandshake(request->sslHandle);
        NHTTPi_SetSSLError(info, result);
        if (connection != NULL) {
            connection->sslStatus = result;
        }
        switch (result) {
        case 0:
            done = 1;
            break;
        case -7:
        case -3:
        case -2:
            break;
        default:
            return -1001;
        }
    }
    return 0;
}

/* Receive from the connection's own 32 KB buffer, refilling it from the socket when it is empty.
 * Answers the bytes handed out, or what the socket answered when it had nothing (<= 0). */
s32 NHTTPi_SocRecv_sub(NHTTPConnection* connection, s32 fd, u8* buf, s32 length, s32 flags) {
    s32 got = 0;
    u8* recvBuffer = connection->recvBuffer;
    s32 want = length;
    s32 refill;

    if (length > 0) {
        if (connection->recvUnread == 0) {
            refill = SORecv(fd, recvBuffer, 0x8000, flags);
            if (refill <= 0) {
                return refill;
            }
            connection->recvUnread = refill;
            connection->recvOffset = 0;
        }
        if (connection->recvUnread != 0) {
            if ((u32)want > connection->recvUnread) {
                want = connection->recvUnread;
            }
            NHTTPi_memcpy(buf, recvBuffer + connection->recvOffset, want);
            connection->recvUnread -= want;
            if (connection->recvUnread == 0) {
                NHTTPi_memclr(recvBuffer, 0x8000);
                connection->recvOffset = 0;
            } else {
                connection->recvOffset += want;
            }
            got = want;
        }
    }
    return got;
}

/* Receive `length` bytes through SSL when the request holds a context, through the connection's
 * buffer otherwise.  A failure maps to -1002 for a cancelled request, 0 for a would-block, and -1001
 * for anything else.  (`handle` is the request-list mutex and `conn` the request - see the header.) */
s32 NHTTPi_SocRecv(s32 handle, NHTTPConnection* conn, s32 flags, u8* buf, s32 length, s32 arg) {
    s32 result;

    if (((NHTTPRequest*)conn)->sslHandle > 0) {
        result = SSLRead(((NHTTPRequest*)conn)->sslHandle, buf, length);
    } else {
        NHTTPConnection* connection = NHTTPi_Request2Connection((NHTTPMutexInfo*)handle, (s32)conn);

        if (connection != NULL) {
            result = NHTTPi_SocRecv_sub(connection, flags, buf, length, arg);
        } else {
            result = -1001;
        }
    }
    if (result < 0) {
        if (((NHTTPRequest*)conn)->cancelled != 0) {
            return -1002;
        }
        if (((NHTTPRequest*)conn)->sslHandle > 0) {
            if ((u32)(result + 7) <= 1U) {
                return 0;
            }
        } else if (result == -56) {
            return 0;
        }
        result = -1001;
    }
    return result;
}

/* Send a byte range with the SO layer's 32-byte alignment: the unaligned head and tail go through an
 * aligned stack copy, the aligned middle is sent in place.  Answers the bytes sent so far when a
 * part is short, or what the socket answered when it sent nothing at all. */
s32 NHTTPi_SocSend_sub(s32 fd, u8* buf, s32 length, s32 flags) {
    u8 aligned[32] __attribute__((aligned(32)));
    u32 head = ((u32)buf & 0x1F) != 0 ? 32 - ((u32)buf & 0x1F) : 0;
    s32 total = 0;
    s32 sent;
    u32 body;
    u32 tail;

    NHTTPi_memclr(aligned, 32);
    if (head != 0) {
        if (head > length) {
            head = length;
        }
        NHTTPi_memcpy(aligned, buf, head);
        sent = SOSend(fd, aligned, head, flags);
        if (sent <= 0) {
            return sent;
        }
        total = sent;
        if ((u32)sent < head) {
            return sent;
        }
        buf += sent;
        length -= sent;
    }
    if (length > 0 && (body = length & 0xFFFFFFE0) != 0) {
        sent = SOSend(fd, buf, body, flags);
        if (sent > 0) {
            total += sent;
            if ((u32)sent < body) {
                return total;
            }
            buf += sent;
            length -= sent;
        } else {
            if (total > 0) {
                return total;
            }
            return sent;
        }
    }
    if (length > 0 && (tail = length & 0x1F) != 0) {
        NHTTPi_memclr(aligned, 32);
        NHTTPi_memcpy(aligned, buf, tail);
        sent = SOSend(fd, aligned, tail, flags);
        if (sent > 0) {
            total += sent;
        } else {
            if (total > 0) {
                return total;
            }
            return sent;
        }
    }
    return total;
}

/* Send `length` bytes through SSL when the request holds a context, through the aligned SO send
 * otherwise, mapping a failure the way `NHTTPi_SocRecv` does. */
s32 NHTTPi_SocSend(NHTTPRequest* request, s32 fd, u8* buf, s32 length, s32 flags) {
    s32 result;

    if (request->sslHandle > 0) {
        result = SSLWrite(request->sslHandle, buf, length);
    } else {
        result = NHTTPi_SocSend_sub(fd, buf, length, flags);
    }
    if (result < 0) {
        if (request->cancelled != 0) {
            return -1002;
        }
        if (request->sslHandle > 0) {
            if ((u32)(result + 7) <= 1U) {
                return 0;
            }
        } else if (result == -56) {
            return 0;
        }
        result = -1001;
    }
    return result;
}

/* Shut the socket down for both directions under the request-list lock (a no-op for a negative
 * descriptor). */
void NHTTPi_SocShutdown(NHTTPMutexInfo* mutex, NHTTPRequest* request, s32 fd) {
    NHTTPi_lockReqList(mutex);
    if (fd >= 0) {
        SOShutdown(fd, 2);
    }
    NHTTPi_unlockReqList(mutex);
}

/* Connect the socket to `address`:`port` and, for an https request that is not going through a proxy,
 * run the SSL handshake.  Answers 0, -1002 for a cancelled request or -1001 when the connect failed. */
s32 NHTTPi_SocConnect(NHTTPInfo* info, NHTTPMutexInfo* mutex, NHTTPRequest* request, s32 fd, u32 address,
                      u32 port) {
    SOSockAddrIn addr;

    addr.len = 8;
    addr.family = 2;
    addr.port = SOHtoNs(port);
    addr.addr = address;
    if (SOConnect(fd, &addr) < 0) {
        s32 error = -1001;

        if (request->cancelled != 0) {
            error = -1002;
        }
        return error;
    }
    if (request->useSSL != 0 && request->useProxy == 0) {
        return NHTTPi_SocSSLConnect(info, mutex, request, fd);
    }
    return 0;
}

/* Resolve a host name to its first IPv4 address, or 0 when it does not resolve. */
u32 NHTTPi_resolveHostName(NHTTPRequest* request, const char* name) {
    SOAddrInfo* result;
    u32 address = 0;

    if (SOGetAddrInfo(name, NULL, NULL, &result) == 0) {
        NHTTPi_memcpy(&address, &result->addr->addr, 4);
        SOFreeAddrInfo(result);
    } else {
        address = 0;
    }
    return address;
}

/* 0x80516CA0 (4): tail shim to memcpy. */
asm void* NHTTPi_memcpy(void* dst, const void* src, u32 n) {
    nofralloc
    b memcpy
}

/* 0x80516CA4 (4): tail shim to strlen. */
asm u32 NHTTPi_strlen(const char* s) {
    nofralloc
    b strlen
}

/* 0x80516CA8 (4): tail shim to strcmp. */
asm int NHTTPi_strcmp(const char* a, const char* b) {
    nofralloc
    b strcmp
}

/* 0x80516CAC (0xC): zero-fill shim (memset(dst, 0, n)). */
asm void* NHTTPi_memclr(void* dst, u32 n) {
    nofralloc
    mr r5, r4
    li r4, 0
    b memset
}

/* The case fold `NHTTPi_strnicmp` and `NHTTPi_strToHex` both need.  MWCC inlines it at every call
 * site, so it has no symbol of its own. */
static inline s32 NHTTPi_toLower(char c) {
    return ((c >= 'A') & (c <= 'Z')) ? c + 32 : c;
}

/* 0x80516CB8 (0xCC): the bounded case-insensitive compare.  It answers 0 when both strings run
 * out inside `n` characters, and the number of characters still uncompared otherwise - not the
 * character difference a `strnicmp` would report. */
s32 NHTTPi_strnicmp(const char* s1, const char* s2, s32 n) {
    for (; n > 0; n--) {
        char c1 = *s1++;
        char c2 = *s2++;

        if (c1 == 0 || c2 == 0) {
            if (c1 == 0 && c2 == 0) {
                n = 0;
                break;
            }
        }
        if (NHTTPi_toLower(c1) != NHTTPi_toLower(c2)) {
            break;
        }
    }
    return n;
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
        if (('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z') ||
            c == ' ') {
            length += 1;
        } else {
            length += 3;
        }
        c = *p++;
    }
    return length;
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
    if (('0' <= c && c <= '9') || ('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')) {
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

/* 0x80516F08 (0x124): read at most `n` characters as a hexadecimal number.  Leading spaces are
 * skipped and a space or NUL after the first digit ends the number; answers -1 when the field holds
 * no hex digit, when a character is neither a hex digit, a space nor the end, and when eight
 * characters would carry the value into the sign bit. */
s32 NHTTPi_strToHex(const char* s, s32 n) {
    s32 value;
    BOOL seen;

    if (n > 8) {
        return -1;
    }
    if ((n == 8) & (*s > '7')) {
        return -1;
    }
    value = 0;
    seen = FALSE;
    for (; n > 0; n--) {
        char c = NHTTPi_toLower(*s);

        if ('0' <= c && c <= '9') {
            value = value * 16 + (c - '0');
            seen = TRUE;
        } else if ('a' <= c && c <= 'f') {
            value = value * 16 + (c - 'a' + 10);
            seen = TRUE;
        } else if (seen && (c == ' ' || c == 0)) {
            break;
        } else if (!seen && c == ' ') {
            /* a leading space is skipped */
        } else {
            return -1;
        }
        s++;
    }
    return value;
}

/* Read at most `n` characters as a decimal number: leading spaces are skipped, a space or NUL after
 * the first digit ends it.  Answers -1 for a character that is no digit, for more than ten
 * characters and for an overflow. */
s32 NHTTPi_strToDec(const char* s, s32 n) {
    s32 value;
    BOOL seen;
    s32 previous;

    if (n > 10) {
        return -1;
    }
    value = 0;
    seen = FALSE;
    for (; n > 0; n--) {
        char c = *s;

        if (seen && (c == ' ' || c == 0)) {
            break;
        }
        if (!(!seen && c == ' ')) {
            if ('0' > c || c > '9') {
                return -1;
            }
            previous = value;
            seen = TRUE;
            value = value * 10 + c - '0';
            if (previous > value) {
                return -1;
            }
        }
        s++;
    }
    return value;
}

/* Write the decimal digits of `value` at `buf` (no terminator; nine or ten digits at most) and answer
 * how many were written. */
s32 NHTTPi_uintToStr(char* buf, u32 value) {
    u32 powers[9] = {1000000000, 100000000, 10000000, 1000000, 100000, 10000, 1000, 100, 10};
    s32 length = 0;
    BOOL started = FALSE;
    char* out = buf;
    s32 i;

    for (i = 0; i < 9; i++) {
        u32 power = powers[i];

        if (value >= power) {
            u32 digit = value / power;

            started = TRUE;
            length++;
            *out++ = digit + '0';
            value -= digit * power;
        } else if (started) {
            *out++ = '0';
            length++;
        }
    }
    buf[length] = value + '0';
    return length + 1;
}

/* 0x80517248 (0xB4): compare two tokens case-insensitively.  Answers 0 once both have run out at a
 * NUL or a space, -1 as soon as a character differs. */
s32 NHTTPi_compareToken(const char* a, const char* b) {
    while (NHTTPi_toLower(*a) == NHTTPi_toLower(*b)) {
        if (*a == 0 || *a == ' ') {
            return 0;
        }
        a++;
        b++;
    }
    return -1;
}

/* 0x805172FC (0x90): read at most `n` characters as a decimal number, skipping spaces.  Answers -1
 * when the field held no digit at all, or when the number needs more than nine digits. */
s32 NHTTPi_strtonum(const char* s, s32 n) {
    char c;
    s32 value = 0;
    s32 digits = 0;

    for (; n != 0; n--) {
        c = *s;

        if (c != ' ') {
            if ((c >= '0') & (c <= '9')) {
                value = value * 10 + (c - '0');
                digits++;
                if (digits > 9) {
                    return -1;
                }
            }
        }
        s++;
    }
    if (digits == 0) {
        return -1;
    }
    return value;
}

/* 0x8051738C (0xAC): find `needle` inside the first `haystackLen` characters of `haystack`.
 * Answers 0 on a hit and -1 on a miss, or when the needle is the longer of the two. */
s32 NHTTPi_containsString(const char* haystack, s32 haystackLen, const char* needle, s32 needleLen) {
    const char* p;
    s32 i;
    s32 k;

    if (haystackLen < needleLen) {
        return -1;
    }
    p = haystack;
    for (i = 0; i < haystackLen - needleLen + 1; i++) {
        if (needle[0] == *p) {
            for (k = 1; k < needleLen; k++) {
                if (haystack[i + k] != needle[k]) {
                    break;
                }
            }
            if (k == needleLen) {
                return 0;
            }
        }
        p++;
    }
    return -1;
}

/* Base64-encode the NUL-terminated `src` into `dst` (with '=' padding and a terminator).  Answers the
 * encoded length. */
s32 NHTTPi_Base64Encode(char* dst, const char* src) {
    const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char* out = dst;
    s32 length = strlen(src);
    u32 groups = (u32)(length + 2) / 3;
    s32 covered = 0;
    u32 i;

    if (length > 0) {
        covered = groups * 3;
        for (i = 0; i < groups; i++) {
            out[0] = alphabet[src[0] >> 2];
            out[1] = alphabet[((src[0] << 4) & 0x30) + (src[1] >> 4)];
            out[2] = alphabet[((src[1] << 2) & 0x3C) + (src[2] >> 6)];
            out[3] = alphabet[src[2] & 0x3F];
            src += 3;
            out += 4;
        }
    }
    if (covered == length + 1) {
        out[-1] = '=';
    } else if (covered == length + 2) {
        out[-2] = '=';
        out[-1] = '=';
    }
    out[0] = 0;
    return strlen(dst);
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

/* Answers 1 when the last four bytes of the 4-byte ring `window` (`count` bytes were written into it)
 * end the response head: CR CR, LF LF or CR LF CR LF. */
s32 NHTTPi_isHeaderEnd(const char* window, s32 count) {
    u8 previous = window[(count - 2) & 3];

    if ((s8)previous == '\r' && window[(count - 1) & 3] == '\r') {
        return 1;
    }
    if ((s8)previous == '\n' && window[(count - 1) & 3] == '\n') {
        return 1;
    }
    if (window[(count - 4) & 3] == '\r' && window[(count - 3) & 3] == '\n' && (s8)previous == '\r' &&
        window[(count - 1) & 3] == '\n') {
        return 1;
    }
    return 0;
}

/* Copy `length` bytes into the 256-byte send buffer, sending the buffer whenever it fills.  Answers
 * `length`, -1 for a cancelled request, and the send's own answer when it sent nothing (<= 0). */
s32 NHTTPi_SaveBuf(NHTTPRequest* request, u8* buffer, s32 fd, s32* used, u8* data, s32 length) {
    s32 remaining = length;
    s32 chunk;
    s32 sent;

    while (remaining > 0) {
        if (request->cancelled != 0) {
            return -1;
        }
        chunk = remaining;
        if (chunk > 0x100 - *used) {
            chunk = 0x100 - *used;
        }
        NHTTPi_memcpy(buffer + *used, data, chunk);
        data += chunk;
        remaining -= chunk;
        *used += chunk;
        if (*used == 0x100) {
            sent = NHTTPi_SocSend(request, fd, buffer, 0x100, 0);
            if (sent <= 0) {
                return sent;
            }
            *used -= sent;
        }
    }
    return length;
}

/* Ask the request's send callback for the body of one field piece by piece and add up what it will
 * send: the raw size in modes 0/1, the url-encoded size in mode 2.  Answers 1 when the callback
 * ran dry, 0 when the request was cancelled or the callback failed. */
s32 NHTTPi_queryPostFieldSize(NHTTPMutexInfo* mutex, NHTTPRequest* request, const char* key, s32* total,
                              s32 mode) {
    s32 sent = 0;
    NHTTPConnection* connection = NHTTPi_Request2Connection(mutex, (s32)request);
    u32 chunk;
    u32 data;

    if (connection == NULL) {
        return 0;
    }
    connection->postData = 0;
    for (;;) {
        if (request->cancelled != 0) {
            return 0;
        }
        connection->postLength = 0;
        if (NHTTPi_PostSendCallback(mutex, connection, (u32)key, sent) < 0) {
            return 0;
        }
        chunk = connection->postLength;
        data = connection->postData;
        if (chunk == 0) {
            break;
        }
        if (data == 0) {
            return 0;
        }
        sent += chunk;
        switch (mode) {
        case 0:
        case 1:
            *total += chunk;
            break;
        case 2:
            *total += NHTTPi_urlEncodedLengthN((char*)data, chunk);
            break;
        }
    }
    return 1;
}

/* Send one field piece by piece through the send buffer: raw in modes 0/1, url-encoded in mode 2.
 * Answers 0 when the callback ran dry, 1 for a send error, 2 for a would-block and 3 when the
 * request was cancelled or the callback failed. */
s32 NHTTPi_sendPostField(NHTTPMutexInfo* mutex, NHTTPRequest* request, u8* buffer, const char* key, s32 fd,
                         s32* used, s32 mode) {
    s32 sent = 0;
    NHTTPConnection* connection = NHTTPi_Request2Connection(mutex, (s32)request);
    u32 chunk;
    u8* data;
    s32 result;
    u32 i;
    u8 encoded[3];

    if (connection == NULL) {
        return 3;
    }
    connection->postData = 0;
    for (;;) {
        if (request->cancelled != 0) {
            return 3;
        }
        connection->postLength = 0;
        if (NHTTPi_PostSendCallback(mutex, connection, (u32)key, sent) < 0) {
            return 3;
        }
        chunk = connection->postLength;
        data = (u8*)connection->postData;
        if (chunk == 0) {
            break;
        }
        if (data == NULL) {
            return 3;
        }
        sent += chunk;
        switch (mode) {
        case 0:
        case 1:
            result = NHTTPi_SaveBuf(request, buffer, fd, used, data, chunk);
            if (result < 0) {
                return 1;
            }
            if (result == 0) {
                return 2;
            }
            break;
        case 2:
            for (i = 0; i < chunk; i++, data++) {
                NHTTPi_memclr(encoded, 3);
                result = NHTTPi_SaveBuf(request, buffer, fd, used, encoded,
                                        NHTTPi_encodeUrlChar(encoded, *data));
                if (result < 0) {
                    return 1;
                }
                if (result == 0) {
                    return 2;
                }
            }
            break;
        }
    }
    return 0;
}

/* Make room in the response's receive buffer: when there is none, the caller's buffer-full callback
 * is asked for more.  Answers 1 when the buffer has space. */
s32 NHTTPi_ensureRecvSpace(NHTTPMutexInfo* mutex, NHTTPResponse* response) {
    s32 hasSpace = 0;
    BOOL full = NHTTPi_isRecvBufFull((NHTTPRecvBuf*)response, response->received);
    NHTTPConnection* connection;

    if (response->bufferSize == 0 || response->userBuffer == 0 || full != 0) {
        connection = NHTTPi_Response2Connection(mutex, (s32)response);
        if (connection != NULL) {
            NHTTPi_BufferFullCallback(mutex, connection);
            if (response->userBuffer != 0 && response->bufferSize != 0 &&
                NHTTPi_isRecvBufFull((NHTTPRecvBuf*)response, response->received) == 0) {
                hasSpace = 1;
            }
        }
    } else if (full == 0) {
        hasSpace = 1;
    }
    return hasSpace;
}

/* Queue bytes into the send buffer of the request the comm thread is serving.  Answers 0 when they
 * were queued, 1 for a send error and 2 for a would-block. */
s32 NHTTPi_appendSendData(NHTTPCommContext* context, const char* data, s32 length) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    NHTTPThreadInfo* thread = NHTTPi_GetThreadInfoP(info);
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    s32 result;

    if (length == 0) {
        return 0;
    }
    result = NHTTPi_SaveBuf(request, thread->sendBuffer, bgnend->socket, &context->sendUsed, (u8*)data, length);
    if (result < 0) {
        return 1;
    }
    return result == 0 ? 2 : 0;
}

/* The multipart pieces `NHTTPi_sendMultipartBody` writes around each field (the rest of retail's
 * `.rodata` run 0x80574D28.. that this unit reads). */
static const char NHTTPi_contentDisposition[] = "Content-Disposition: form-data; name=\"";
static const char NHTTPi_octetStreamHeaders[] =
    "Content-Type: application/octet-stream\r\nContent-Transfer-Encoding: binary\r\n";
static const char NHTTPi_urlEncodedHeader[] = "Content-Type: application/x-www-form-urlencoded\r\n";
static const char NHTTPi_multipartHeader[] = "Content-Type: multipart/form-data; boundary=";

/* Send the `Proxy-Authorization: Basic` header of the request being served, when it has credentials.
 * MWCC inlines it into the two senders below. */
static inline s32 NHTTPi_appendProxyAuthorization(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    s32 result;

    if (request->proxyAuthLength == 0) {
        return 0;
    }
    result = NHTTPi_appendSendData(context, "Proxy-Authorization: Basic ", 27);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, request->proxyAuth, request->proxyAuthLength);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "\r\n", 2);
    return result != 0 ? result : 0;
}

/* Send the `CONNECT host:port` request a proxy needs for an https tunnel.  Answers 0, 1 for a send
 * error, 2 for a would-block. */
s32 NHTTPi_sendProxyConnect(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    NHTTPThreadInfo* thread = NHTTPi_GetThreadInfoP(info);
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    u8* sendBuffer = thread->sendBuffer;
    char portText[12];
    s32 portLength = NHTTPi_uintToStr(portText, request->port);
    s32 result;

    result = NHTTPi_appendSendData(context, "CONNECT ", 8);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, request->url + 8, request->hostEnd - 8);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, ":", 1);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, portText, portLength);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, " HTTP/1.1\r\n", 11);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "Host: ", 6);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, request->url + 8, request->hostEnd - 8);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, ":", 1);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, portText, portLength);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "Content-Length: 0\r\nPragma: no-cache\r\n", 37);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendProxyAuthorization(context);
    if (result != 0) {
        return result;
    }
    NHTTPi_appendSendData(context, "\r\n", 2);
    if (context->sendUsed > 0) {
        s32 sent = NHTTPi_SocSend(request, bgnend->socket, sendBuffer, context->sendUsed, 0);

        if (sent < 0) {
            return 1;
        }
        if (sent == 0) {
            return 2;
        }
    }
    context->sendUsed = 0;
    NHTTPi_memclr(sendBuffer, 0x100);
    return 0;
}

/* Read the proxy's answer to the CONNECT up to the blank line that ends it.  Answers 1 for a "200"
 * status, 0 for any other status or a receive error. */
s32 NHTTPi_recvProxyConnectReply(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPThreadInfo* thread = NHTTPi_GetThreadInfoP(info);
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    NHTTPResponse* response = request->response;
    u8* discard = thread->sendBuffer;
    s32 ok = 0;
    s32 received = 0;
    s32 count;
    char reply[0x200];
    s32 i;
    s32 blank;
    s32 result;

    for (;;) {
        count = NHTTPi_SocRecv((s32)mutex, (NHTTPConnection*)request, bgnend->socket,
                               (u8*)reply + received, 0x200 - received, 0);
        received += count;
        response->statusCode = NHTTPi_strToDec(reply + 9, 3);
        if (NHTTPi_strnicmp(reply, "HTTP/", 5) == 0 && reply[8] == ' ' && response->statusCode == 200) {
            ok = 1;
        }
        blank = 0;
        for (i = 0; i < received; i++) {
            if (i > 1 && reply[i - 1] == '\r' && reply[i] == '\r') {
                blank = 1;
            } else if (i > 1 && reply[i - 1] == '\n' && reply[i] == '\n') {
                blank = 1;
            } else if (i > 3 && reply[i - 3] == '\r' && reply[i - 2] == '\n' && reply[i - 1] == '\r' &&
                       reply[i] == '\n') {
                blank = 1;
            }
        }
        if (blank != 0) {
            return ok != 0;
        }
        if (count < 0) {
            return 0;
        }
        if (received >= 0x200) {
            result = NHTTPi_SocRecv((s32)mutex, (NHTTPConnection*)request, bgnend->socket, discard, 1, 0);
            if (result < 0) {
                return 0;
            }
            if (result != 0) {
                return 0;
            }
        }
    }
}

/* Send the request's header fields one by one ("token: value" CR LF) and free each node.  Answers
 * the first non-zero result of the sends. */
s32 NHTTPi_sendHeaderFields(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    NHTTPHeaderField* field = NHTTPi_RemoveNode(&request->headerList);
    s32 result;

    while (field != NULL) {
        result = NHTTPi_appendSendData(context, field->token, NHTTPi_strlen(field->token));
        if (result != 0) {
            return result;
        }
        result = NHTTPi_appendSendData(context, ": ", 2);
        if (result != 0) {
            return result;
        }
        result = NHTTPi_appendSendData(context, field->value, NHTTPi_strlen(field->value));
        if (result != 0) {
            return result;
        }
        result = NHTTPi_appendSendData(context, "\r\n", 2);
        if (result != 0) {
            return result;
        }
        NHTTPi_free(field);
        field = NHTTPi_RemoveNode(&request->headerList);
    }
    return 0;
}

/* Send a raw (non-field) post body: the length header, the blank line, then either the body the
 * caller gave or the pieces its send callback hands out.  Answers 0, 1 (send error), 2 (would block)
 * or 3 (cancelled or the callback failed). */
s32 NHTTPi_sendRawPostBody(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    NHTTPThreadInfo* thread = NHTTPi_GetThreadInfoP(info);
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    s32 length = 0;
    u8* sendBuffer = thread->sendBuffer;
    char digits[12];
    s32 digitCount;
    s32 result;

    if (request->rawBody == NULL) {
        if (NHTTPi_queryPostFieldSize(mutex, request, NULL, &length, 0) == 0) {
            return 3;
        }
    } else {
        length = request->rawBodyLength;
    }
    digitCount = NHTTPi_uintToStr(digits, length);
    result = NHTTPi_appendSendData(context, "Content-Length: ", 16);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, digits, digitCount);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    if (request->rawBody == NULL) {
        result = NHTTPi_sendPostField(mutex, request, sendBuffer, NULL, bgnend->socket, &context->sendUsed, 0);
        if (result != 0) {
            return result;
        }
    } else {
        result = NHTTPi_appendSendData(context, request->rawBody, request->rawBodyLength);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}

/* Send the post fields as a multipart/form-data body: the content type with the boundary, the total
 * length, then each field between boundaries.  Answers as `NHTTPi_sendRawPostBody`. */
s32 NHTTPi_sendMultipartBody(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    NHTTPThreadInfo* thread = NHTTPi_GetThreadInfoP(info);
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    s32 total = 0;
    NHTTPHeaderField* field;
    char digits[12];
    s32 digitCount;
    s32 result;

    for (field = request->postDataList; field != NULL; field = field->prev) {
        total += 0x16;
        total += NHTTPi_strlen(field->token) + 0x29;
        if (field->field_0x14 != 0) {
            total += 0x4B;
        }
        total += 2;
        if (field->value == NULL) {
            if (NHTTPi_queryPostFieldSize(mutex, request, field->token, &total, 1) == 0) {
                return 3;
            }
        } else {
            total += field->field_0x10;
        }
        total += 2;
        if (field == request->postDataList->next) {
            break;
        }
    }
    total += 0x18;
    digitCount = NHTTPi_uintToStr(digits, total);
    result = NHTTPi_appendSendData(context, NHTTPi_multipartHeader, 44);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, request->code, 0x12);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "Content-Length: ", 16);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, digits, digitCount);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    for (field = request->postDataList; field != NULL; field = field->prev) {
        result = NHTTPi_appendSendData(context, (char*)&request->boundaryDashes, 0x14);
        if (result != 0) {
            return result;
        }
        result = NHTTPi_appendSendData(context, "\r\n", 2);
        if (result != 0) {
            return result;
        }
        result = NHTTPi_appendSendData(context, NHTTPi_contentDisposition, 38);
        if (result != 0) {
            return result;
        }
        result = NHTTPi_appendSendData(context, field->token, NHTTPi_strlen(field->token));
        if (result != 0) {
            return result;
        }
        result = NHTTPi_appendSendData(context, "\"\r\n", 3);
        if (result != 0) {
            return result;
        }
        if (field->field_0x14 != 0) {
            result = NHTTPi_appendSendData(context, NHTTPi_octetStreamHeaders, 75);
            if (result != 0) {
                return result;
            }
        }
        result = NHTTPi_appendSendData(context, "\r\n", 2);
        if (result != 0) {
            return result;
        }
        if (field->value == NULL) {
            result = NHTTPi_sendPostField(mutex, request, thread->sendBuffer, field->token, bgnend->socket,
                                          &context->sendUsed, 1);
            if (result != 0) {
                return result;
            }
        } else {
            result = NHTTPi_appendSendData(context, field->value, field->field_0x10);
            if (result != 0) {
                return result;
            }
        }
        result = NHTTPi_appendSendData(context, "\r\n", 2);
        if (result != 0) {
            return result;
        }
        if (field == request->postDataList->next) {
            break;
        }
    }
    result = NHTTPi_appendSendData(context, (char*)&request->boundaryDashes, 0x14);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "--\r\n", 4);
    return result != 0 ? result : 0;
}

/* Send the post fields as an application/x-www-form-urlencoded body ("token=value" pairs joined by
 * '&', every character percent-encoded as needed).  Answers as `NHTTPi_sendRawPostBody`. */
s32 NHTTPi_sendUrlEncodedBody(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    NHTTPThreadInfo* thread = NHTTPi_GetThreadInfoP(info);
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    s32 total = 0;
    NHTTPHeaderField* field;
    char digits[12];
    s32 digitCount;
    s32 result;
    s32 i;

    for (field = request->postDataList; field != NULL; field = field->prev) {
        total = total + NHTTPi_urlEncodedLength(field->token) + 1;
        if (field->value == NULL) {
            if (NHTTPi_queryPostFieldSize(mutex, request, field->token, &total, 2) == 0) {
                return 3;
            }
        } else {
            total += NHTTPi_urlEncodedLength(field->value);
        }
        if (field == request->postDataList->next) {
            break;
        }
        total += 1;
    }
    digitCount = NHTTPi_uintToStr(digits, total);
    result = NHTTPi_appendSendData(context, NHTTPi_urlEncodedHeader, 49);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "Content-Length: ", 16);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, digits, digitCount);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    for (field = request->postDataList; field != NULL; field = field->prev) {
        for (i = 0; field->token[i] != 0; i++) {
            result = NHTTPi_appendSendData(context, digits, NHTTPi_encodeUrlChar((u8*)digits, field->token[i]));
            if (result != 0) {
                return result;
            }
        }
        result = NHTTPi_appendSendData(context, "=", 1);
        if (result != 0) {
            return result;
        }
        if (field->value == NULL) {
            result = NHTTPi_sendPostField(mutex, request, thread->sendBuffer, field->token, bgnend->socket,
                                          &context->sendUsed, 2);
            if (result != 0) {
                return result;
            }
        } else {
            for (i = 0; field->value[i] != 0; i++) {
                result = NHTTPi_appendSendData(context, digits, NHTTPi_encodeUrlChar((u8*)digits, field->value[i]));
                if (result != 0) {
                    return result;
                }
            }
        }
        if (field == request->postDataList->next) {
            break;
        }
        result = NHTTPi_appendSendData(context, "&", 1);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}

/* Wrap the served request up: close the socket unless it is kept, record the error on the request's
 * response and connection, free the active node and the request, and tell the connection's callback
 * and the waiters. */
void NHTTPi_finishRequest(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    NHTTPResponse* response = request->response;
    NHTTPConnection* connection = NHTTPi_Request2Connection(mutex, (s32)request);

    if (request->cancelled != 0) {
        context->error = 8;
        context->connected = 0;
    }
    if (context->connected == 0 && bgnend->socket >= 0) {
        if (NHTTPi_SocClose(mutex, request, bgnend->socket) < 0) {
            context->error = 10;
        }
        bgnend->socket = -1;
    }
    if (context->error == 0) {
        response->completed = 1;
    } else {
        response->completed = 0;
        NHTTPi_SetError(bgnend, context->error);
        if ((u8*)response->userBuffer == context->recvScratch) {
            response->userBuffer = 0;
            response->bufferSize = 0;
        }
    }
    if (connection != NULL) {
        connection->status = context->error;
    }
    NHTTPi_lockReqList(mutex);
    NHTTPi_free(requestInfo->active);
    requestInfo->active = NULL;
    NHTTPi_unlockReqList(mutex);
    NHTTPi_destroyRequestObject(mutex, (s32)request);
    if (connection != NULL && response->completed != 0) {
        connection->state = 5;
    }
    NHTTPi_CompleteCallback(mutex, connection);
    if (connection != NULL) {
        NHTTPi_NotifyCompletion(connection);
    }
}

/* Take the next queued request off the list and make it the active one.  Answers 1 when there is one;
 * otherwise waits for the quit/wake message and answers 0. */
s32 NHTTPi_dequeueRequest(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPRequestNode* node;

    NHTTPi_lockReqList(mutex);
    node = (NHTTPRequestNode*)NHTTPi_RemoveHeaderField(NHTTPi_GetListInfoP(info));
    if (node != NULL) {
        NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);

        context->requestId = node->id;
        requestInfo->active = node;
    } else {
        context->requestId = -1;
    }
    NHTTPi_unlockReqList(mutex);
    if (context->requestId < 0) {
        NHTTPThreadInfo* thread = NHTTPi_GetThreadInfoP(info);

        NHTTPi_receiveMessage(&thread->queue);
        return 0;
    }
    return 1;
}

/* Resolve the host (or the proxy) of the active request and decide whether the connection kept from the
 * last request can be reused.  Answers 1, or 0 with the context's error set (4, or 12 for a proxy). */
s32 NHTTPi_prepareTarget(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    char* host = request->host;

    if (request->useProxy != 0) {
        host = request->proxyHost;
    }
    if (NHTTPi_strlen(host) == 0 || NHTTPi_strcmp(host, context->host) != 0) {
        context->address = NHTTPi_resolveHostName(request, host);
        if (context->address == 0) {
            if (request->useProxy != 0) {
                context->error = 12;
                return 0;
            }
            context->error = 4;
            return 0;
        }
    } else {
        context->address = context->lastAddress;
    }
    NHTTPi_memclr(context->host, 0x100);
    NHTTPi_memcpy(context->host, host, NHTTPi_strlen(host));
    context->port = request->port;
    if (request->useProxy != 0) {
        context->port = request->proxyPort;
    }
    if (context->address != context->lastAddress || context->port != context->lastPort ||
        request->useSSL == 1) {
        context->connected = 0;
    }
    context->lastAddress = context->address;
    context->lastPort = context->port;
    return 1;
}

/* Open (or keep) the socket the active request is sent on and connect it.  Answers 1 when the socket
 * is ready, 0 with the context's error set otherwise. */
s32 NHTTPi_connectSocket(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    s32 peerClosed = 0;

    if (context->connected == 1 && NHTTPi_probeSocket(bgnend->socket) == -1) {
        peerClosed = 1;
        context->connected = 0;
    }
    if (context->connected == 0) {
        if (bgnend->socket >= 0 && NHTTPi_SocClose(mutex, request, bgnend->socket) < 0 && peerClosed == 0) {
            bgnend->socket = -1;
            context->error = 10;
            return 0;
        }
        bgnend->socket = NHTTPi_openSocket(request);
        if (bgnend->socket < 0) {
            context->error = 3;
            return 0;
        }
        NHTTPi_lockReqList(mutex);
        requestInfo->active->state = bgnend->socket;
        NHTTPi_unlockReqList(mutex);
        if (request->cancelled != 0) {
            return 0;
        }
        if (NHTTPi_SocConnect(bgnend, mutex, request, bgnend->socket, context->address, context->port) < 0) {
            if (request->useProxy != 0) {
                context->error = 13;
                return 0;
            }
            if (NHTTPi_GetSSLError(bgnend) != 0) {
                context->error = 14;
                return 0;
            }
            context->error = 5;
            return 0;
        }
    } else {
        NHTTPi_lockReqList(mutex);
        requestInfo->active->state = bgnend->socket;
        NHTTPi_unlockReqList(mutex);
    }
    return 1;
}

/* For an https request through a proxy: send the CONNECT, read the proxy's answer and run the SSL
 * handshake over the tunnel.  Answers 0 to go on, 1 on failure (with the context's error set to the
 * SSL cause) and what the sends answered otherwise. */
s32 NHTTPi_negotiateSSL(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    s32 result;

    context->error = 10;
    context->sendUsed = 0;
    if (request->useSSL != 0 && request->useProxy != 0) {
        result = NHTTPi_sendProxyConnect(context);
        if (result != 0) {
            return result;
        }
        if (NHTTPi_recvProxyConnectReply(context) == 0) {
            return 1;
        }
        result = NHTTPi_SocSSLConnect(bgnend, mutex, request, bgnend->socket);
        if (result != 0) {
            if (result == -1004) {
                if (NHTTPi_GetSSLError(bgnend) != 0) {
                    context->error = 16;
                }
                return 1;
            }
            if (result == -1005) {
                if (NHTTPi_GetSSLError(bgnend) != 0) {
                    context->error = 17;
                }
                return 1;
            }
            if (NHTTPi_GetSSLError(bgnend) != 0) {
                context->error = 14;
            }
            return 1;
        }
    }
    return 0;
}

/* The same for the request's own `Authorization: Basic` credentials. */
static inline s32 NHTTPi_appendBasicAuthorization(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    s32 result;

    if (request->basicAuthLength == 0) {
        return 0;
    }
    result = NHTTPi_appendSendData(context, "Authorization: Basic ", 21);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, request->basicAuth, request->basicAuthLength);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "\r\n", 2);
    return result != 0 ? result : 0;
}

/* Write the whole request of the active connection: the request line, Host and the auth headers, the
 * caller's header fields, and the body for a POST.  Answers 0 when it went out, 1 on a send error,
 * 2 for a would-block and 3 when the body callback failed. */
s32 NHTTPi_sendRequest(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    NHTTPConnection* connection = NHTTPi_Request2Connection(mutex, (s32)request);
    NHTTPThreadInfo* thread = NHTTPi_GetThreadInfoP(info);
    u8* sendBuffer = thread->sendBuffer;
    s32 urlLength = NHTTPi_strlen(request->url);
    s32 result = 0;
    s32 hostOffset;
    s32 sent;

    context->error = 10;
    if (connection != NULL) {
        connection->state = 2;
    }
    context->sendUsed = 0;
    switch (request->method) {
    case 0:
        result = NHTTPi_appendSendData(context, "GET ", 4);
        break;
    case 1:
        result = NHTTPi_appendSendData(context, "POST ", 5);
        break;
    case 2:
        result = NHTTPi_appendSendData(context, "HEAD ", 5);
        break;
    }
    if (result != 0) {
        return result;
    }
    if (request->useProxy != 0 && request->useSSL == 0) {
        result = NHTTPi_appendSendData(context, request->url, urlLength);
        if (result != 0) {
            return result;
        }
    } else if (urlLength > request->pathStart) {
        result = NHTTPi_appendSendData(context, request->url + request->pathStart, urlLength - request->pathStart);
        if (result != 0) {
            return result;
        }
    } else {
        result = NHTTPi_appendSendData(context, "/", 1);
        if (result != 0) {
            return result;
        }
    }
    result = NHTTPi_appendSendData(context, " HTTP/1.1\r\n", 11);
    if (result != 0) {
        return result;
    }
    hostOffset = (request->useSSL != 0) + 7;
    result = NHTTPi_appendSendData(context, "Host: ", 6);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, request->url + hostOffset, request->hostEnd - hostOffset);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_appendSendData(context, "\r\n", 2);
    if (result != 0) {
        return result;
    }
    if (request->useProxy != 0 && request->useSSL == 0) {
        result = NHTTPi_appendProxyAuthorization(context);
        if (result != 0) {
            return result;
        }
    }
    result = NHTTPi_appendBasicAuthorization(context);
    if (result != 0) {
        return result;
    }
    result = NHTTPi_sendHeaderFields(context);
    if (result != 0) {
        return result;
    }
    if (request->method == 1) {
        if (request->field_0x10 != 0) {
            result = NHTTPi_sendRawPostBody(context);
        } else {
            BOOL multipart;

            if (request->postType == 0) {
                NHTTPHeaderField* field;

                multipart = FALSE;
                for (field = request->postDataList; field != NULL; field = field->prev) {
                    if (field->field_0x14 != 0) {
                        multipart = TRUE;
                        break;
                    }
                    if (field == request->postDataList->next) {
                        break;
                    }
                }
            } else {
                multipart = request->postType - 2 == 0;
            }
            if (multipart != 0) {
                result = NHTTPi_sendMultipartBody(context);
            } else {
                result = NHTTPi_sendUrlEncodedBody(context);
            }
        }
        if (result != 0) {
            if (result == 3) {
                context->error = 3;
                return result;
            }
            return result;
        }
    } else {
        result = NHTTPi_appendSendData(context, "\r\n", 2);
        if (result != 0) {
            return result;
        }
    }
    result = 0;
    if (context->sendUsed > 0) {
        sent = NHTTPi_SocSend(request, bgnend->socket, sendBuffer, context->sendUsed, 0);
        context->sendUsed = 0;
        NHTTPi_memclr(sendBuffer, 0x100);
        if (sent < 0) {
            result = 1;
        }
        if (sent == 0) {
            result = 2;
        }
    }
    context->sendUsed = 0;
    NHTTPi_memclr(sendBuffer, 0x100);
    return result;
}

/* Receive the response head a byte at a time (into the response's ring, then its block list) until the
 * blank line.  Answers 1 when the head is complete, 0 with the context's error set otherwise. */
s32 NHTTPi_recvResponseHeaders(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    NHTTPResponse* response = request->response;
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPConnection* connection = NHTTPi_Request2Connection(mutex, (s32)request);
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    char window[4] = {0, 0, 0, 0};
    NHTTPRecvBlock* block;
    s32 received;
    s32 index;

    if (connection != NULL) {
        connection->state = 3;
    }
    response->headerLength = 0;
    NHTTPi_memclr(context->statusLine, 0xE);
    block = (NHTTPRecvBlock*)response->blocks;
    context->recvCount = 0;
    for (;;) {
        if (request->cancelled != 0) {
            return 0;
        }
        if (context->recvCount < 0x400) {
            received = NHTTPi_SocRecv((s32)mutex, (NHTTPConnection*)request, bgnend->socket,
                                      response->data + context->recvCount, 1, 0);
            window[context->recvCount & 3] = response->data[context->recvCount];
        } else {
            index = context->recvCount & 0x1FF;
            if (index == 0) {
                if (block != NULL) {
                    block->next = NHTTPi_alloc(0x204, 4);
                    block = block->next;
                } else {
                    block = NHTTPi_alloc(0x204, 4);
                    response->blocks = block;
                }
                if (block == NULL) {
                    context->error = 1;
                    return 0;
                }
                block->next = NULL;
            }
            received = NHTTPi_SocRecv((s32)mutex, (NHTTPConnection*)request, bgnend->socket,
                                      block->data + index, 1, 0);
            window[context->recvCount & 3] = block->data[index];
        }
        if (received <= 0) {
            context->error = 10;
            return 0;
        }
        context->recvCount += received;
        if (NHTTPi_isHeaderEnd(window, context->recvCount) != 0) {
            response->headerLength = context->recvCount;
            if (response->headerLength == 0) {
                context->error = 7;
                return 0;
            }
            return 1;
        }
    }
}

/* Read the received head: the status line, Content-Length, Connection (keep-alive unless `Close`) and
 * Transfer-Encoding (chunked).  Answers 1 once it is parsed; 0 with the context's error set (7 for a
 * malformed head, 0 for an empty body length) otherwise. */
s32 NHTTPi_parseResponseHeaders(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    NHTTPResponse* response = request->response;
    NHTTPThreadInfo* thread = NHTTPi_GetThreadInfoP(info);
    u8* scratch = thread->sendBuffer;
    s32 offset;
    s32 lineEnd;

    if (NHTTPi_RecvBufCopy((NHTTPRecvBuf*)response, context->statusLine, 0, 14) == 0) {
        context->error = 7;
        return 0;
    }
    if (NHTTPi_strnicmp((char*)context->statusLine, "HTTP/", 5) != 0) {
        context->error = 7;
        return 0;
    }
    if (context->statusLine[8] != ' ') {
        context->error = 7;
        return 0;
    }
    response->statusCode = NHTTPi_strToDec((char*)&context->statusLine[9], 3);
    if (response->statusCode < 0) {
        context->error = 7;
        return 0;
    }
    if (NHTTPi_RecvBufFindLine((NHTTPRecvBuf*)response, 12, response->headerLength, &lineEnd, NULL) < 0) {
        context->error = 7;
        return 0;
    }
    context->contentRemaining = NHTTPi_findHeaderField((NHTTPRecvBuf*)response, "Content-Length", &offset);
    if (context->contentRemaining == 0) {
        context->error = 0;
        return 0;
    }
    if (context->contentRemaining > 256) {
        context->error = 7;
        return 0;
    }
    if (context->contentRemaining > 0) {
        if (NHTTPi_RecvBufCopy((NHTTPRecvBuf*)response, scratch, offset, context->contentRemaining) == 0) {
            context->error = 7;
            return 0;
        }
        context->contentRemaining = NHTTPi_strToDec((char*)scratch, context->contentRemaining);
        if (context->contentRemaining < 0) {
            context->error = 7;
            return 0;
        }
        response->contentLength = context->contentRemaining;
    } else {
        response->contentLength = -1;
    }
    if (request->useSSL != 0) {
        context->connected = 0;
    } else {
        s32 length = NHTTPi_findHeaderField((NHTTPRecvBuf*)response, "Connection", &offset);

        if (length == 0) {
            context->error = 7;
            context->connected = 0;
            return 0;
        }
        if (NHTTPi_strnicmp((char*)context->statusLine, "HTTP/1.1", 8) == 0) {
            context->connected = 1;
        } else {
            context->connected = 0;
        }
        if (length <= 256 && length > 0) {
            if (NHTTPi_RecvBufFindUpper((NHTTPRecvBuf*)response, offset, offset + length, "Keep-Alive", 0) ==
                0) {
                context->connected = 1;
            }
            if (NHTTPi_RecvBufFindUpper((NHTTPRecvBuf*)response, offset, offset + length, "Close", 0) == 0) {
                context->connected = 0;
            }
        }
    }
    context->chunked = NHTTPi_findHeaderField((NHTTPRecvBuf*)response, "Transfer-Encoding", &offset);
    if (context->chunked == 0) {
        context->error = 7;
        return 0;
    }
    if (context->chunked > 256) {
        context->chunked = 0;
    } else if (context->chunked > 0) {
        context->chunked = NHTTPi_RecvBufFindUpper((NHTTPRecvBuf*)response, offset, offset + context->chunked,
                                                   "chunked", ';') == 0;
    } else {
        context->chunked = 0;
    }
    context->error = 0;
    response->headersParsed = 1;
    return 1;
}

/* `NHTTPi_SocRecvOffsetRange` with the request-list mutex and the request spelled as what they are
 * (the sibling unit's prototype still types them `s32` and `NHTTPConnection*`). */
static inline s32 NHTTPi_recvIntoResponse(NHTTPMutexInfo* mutex, NHTTPRequest* request, s32 fd, s32 offset,
                                          s32 length) {
    return NHTTPi_SocRecvOffsetRange((s32)mutex, (NHTTPConnection*)request, fd, offset, length, 0);
}

/* Receive the response body into the caller's buffer (the scratch buffer once the caller's is full),
 * through the content length, the chunked framing, or until the peer closes - whichever the head
 * announced.  Answers 1 when the body is over (the context's error says how), 0 on a receive error. */
s32 NHTTPi_recvResponseBody(NHTTPCommContext* context) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);
    NHTTPRequest* request = (NHTTPRequest*)requestInfo->active->request;
    NHTTPResponse* response = request->response;
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPConnection* connection = NHTTPi_Request2Connection(mutex, (s32)request);
    NHTTPThreadInfo* thread = NHTTPi_GetThreadInfoP(info);
    u8* buffer = thread->sendBuffer;
    s32 received;
    s32 chunkSize;
    s32 index;
    s32 skipped;
    s32 count;
    u8 sizeWindow[2];
    u8 extensionWindow[2];
    u8 trailerWindow[2];
    u8 byte;

    if (request->method == 2 || response->statusCode == 204 || response->statusCode == 304 ||
        (response->statusCode >= 100 && response->statusCode < 200)) {
        return 1;
    }
    NHTTPi_SetSock(connection, NULL);
    if (connection != NULL) {
        connection->state = 4;
    }
    if (context->contentRemaining >= 0) {
        NHTTPi_SetSock(connection, (NHTTPSock*)context->contentRemaining);
        while (context->contentRemaining > 0) {
            if (context->error != 6 && NHTTPi_ensureRecvSpace(mutex, response) == 0) {
                context->error = 6;
                response->userBuffer = (u32)context->recvScratch;
                response->bufferSize = 0x200;
            }
            if (context->error == 6) {
                received = NHTTPi_recvIntoResponse(mutex, request, bgnend->socket, 0, context->contentRemaining);
            } else {
                received = NHTTPi_recvIntoResponse(mutex, request, bgnend->socket, response->received,
                                                   context->contentRemaining);
            }
            if (received < 0) {
                return 0;
            }
            if (received == 0) {
                break;
            }
            if (context->error != 6) {
                response->received += received;
                response->receivedTotal += received;
            }
            context->contentRemaining -= received;
        }
        if (context->error != 6) {
            if (context->contentRemaining != 0) {
                context->error = NHTTPi_isRecvBufFull((NHTTPRecvBuf*)response, response->received) != 0 ? 6 : 10;
            } else {
                context->error = 0;
            }
        }
    } else {
        context->error = 10;
        if (context->chunked != 0) {
            chunkSize = -1;
            for (;;) {
                sizeWindow[0] = 0;
                sizeWindow[1] = 0;
                context->recvCount = 0;
                while (context->recvCount < 256) {
                    if (NHTTPi_SocRecv((s32)mutex, (NHTTPConnection*)request, bgnend->socket,
                                       buffer + context->recvCount, 1, 0) < 0) {
                        return 0;
                    }
                    index = context->recvCount;
                    byte = buffer[index];
                    sizeWindow[index & 1] = byte;
                    if ((s8)byte == ';' || ((s8)byte == '\n' && (s8)sizeWindow[(index - 1) & 1] == '\r')) {
                        if ((s8)byte == '\n') {
                            index -= 1;
                        } else {
                            s32 fd = bgnend->socket;

                            skipped = 0;
                            count = 0;
                            extensionWindow[0] = 0;
                            extensionWindow[1] = 0;
                            while (extensionWindow[count & 1] != '\r' ||
                                   (s8)extensionWindow[(count - 1) & 1] != '\n') {
                                received = NHTTPi_SocRecv((s32)mutex, (NHTTPConnection*)request, fd,
                                                          &extensionWindow[count & 1], 1, 0);
                                if (received <= 0) {
                                    break;
                                }
                                skipped += received;
                                count++;
                            }
                            if (received <= 0) {
                                return 0;
                            }
                        }
                        if (index == 0) {
                            return 0;
                        }
                        chunkSize = NHTTPi_strToHex((char*)buffer, index);
                        if (chunkSize < 0) {
                            return 0;
                        }
                        break;
                    }
                    context->recvCount = context->recvCount + 1;
                }
                if (context->recvCount == 256) {
                    context->error = 7;
                    return 0;
                }
                if (chunkSize > 0) {
                    NHTTPi_SetSock(connection, (NHTTPSock*)chunkSize);
                    while (chunkSize > 0) {
                        if (context->error != 6 && NHTTPi_ensureRecvSpace(mutex, response) == 0) {
                            context->error = 6;
                            response->userBuffer = (u32)context->recvScratch;
                            response->bufferSize = 0x200;
                        }
                        if (context->error == 6) {
                            received = NHTTPi_recvIntoResponse(mutex, request, bgnend->socket, 0, chunkSize);
                        } else {
                            received = NHTTPi_recvIntoResponse(mutex, request, bgnend->socket, response->received,
                                                               chunkSize);
                        }
                        if (received <= 0) {
                            return 0;
                        }
                        chunkSize -= received;
                        response->received += received;
                        response->receivedTotal += received;
                        if (chunkSize == 0 &&
                            NHTTPi_SocRecv((s32)mutex, (NHTTPConnection*)request, bgnend->socket, buffer, 2, 0) <= 0) {
                            return 0;
                        }
                    }
                } else {
                    s32 fd = bgnend->socket;

                    count = 0;
                    trailerWindow[0] = 0;
                    trailerWindow[1] = 0;
                    while (trailerWindow[count & 1] != '\r' || (s8)trailerWindow[(count - 1) & 1] != '\n') {
                        if (NHTTPi_SocRecv((s32)mutex, (NHTTPConnection*)request, fd, &trailerWindow[count & 1], 1, 0) <=
                            0) {
                            break;
                        }
                        count++;
                    }
                    context->error = 0;
                    break;
                }
            }
        } else {
            for (;;) {
                if (NHTTPi_ensureRecvSpace(mutex, response) == 0) {
                    context->error = 6;
                    response->userBuffer = (u32)context->recvScratch;
                    response->bufferSize = 0x200;
                }
                if (context->error == 6) {
                    received = NHTTPi_SocRecvFromOffset((s32)mutex, (NHTTPConnection*)request, bgnend->socket, 0, 0);
                } else {
                    received = NHTTPi_SocRecvFromOffset((s32)mutex, (NHTTPConnection*)request, bgnend->socket,
                                                        response->received, 0);
                }
                if (received < 0) {
                    return 0;
                }
                if (received == 0) {
                    if (context->error != 6) {
                        context->error = 0;
                    }
                    break;
                }
                response->received += received;
                response->receivedTotal += received;
            }
        }
    }
    connection = NHTTPi_Response2Connection(mutex, (s32)response);
    if (context->error == 0 && connection != NULL) {
        NHTTPi_RecvCallback(mutex, connection);
    }
    return 1;
}

/* The comm thread's body: serve queued requests one at a time until the quit flag is raised - dequeue,
 * resolve, connect, (tunnel), send, receive the head and the body, then wrap the request up.  A
 * would-block answer (2) keeps the request for the next round. */
void NHTTPi_commThreadLoop(void) {
    NHTTPCommContext context;
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    NHTTPRequestInfo* requestInfo = NHTTPi_GetReqInfoP(info);

    context.requestId = -1;
    NHTTPi_memclr(context.host, 0x100);
    NHTTPi_memclr(context.recvScratch, 0x200);
    context.address = -1;
    context.lastAddress = -1;
    context.sendUsed = 0;
    context.connected = 0;
    context.chunked = 0;
    context.contentRemaining = 0;
    context.again = 0;
    context.error = 0;
    while (bgnend->quitFlag == 0) {
        if (context.again == 0) {
            if (NHTTPi_dequeueRequest(&context) == 0) {
                continue;
            }
            if (((NHTTPRequest*)requestInfo->active->request)->cancelled != 0) {
                NHTTPi_finishRequest(&context);
                continue;
            }
            if (NHTTPi_prepareTarget(&context) == 0) {
                NHTTPi_finishRequest(&context);
                continue;
            }
        }
        if (context.again == 1) {
            context.again = 0;
        }
        if (NHTTPi_connectSocket(&context) == 0) {
            NHTTPi_finishRequest(&context);
        } else {
            switch (NHTTPi_negotiateSSL(&context)) {
            case 2:
                context.again = 1;
                break;
            case 1:
                NHTTPi_finishRequest(&context);
                break;
            default:
                switch (NHTTPi_sendRequest(&context)) {
                case 2:
                    context.again = 1;
                    break;
                case 3:
                    NHTTPi_finishRequest(&context);
                    break;
                default:
                    if (((NHTTPRequest*)requestInfo->active->request)->cancelled != 0) {
                        NHTTPi_finishRequest(&context);
                    } else if (NHTTPi_recvResponseHeaders(&context) == 0) {
                        NHTTPi_finishRequest(&context);
                    } else if (NHTTPi_parseResponseHeaders(&context) == 0) {
                        NHTTPi_finishRequest(&context);
                    } else {
                        NHTTPi_recvResponseBody(&context);
                        NHTTPi_finishRequest(&context);
                    }
                    break;
                }
                break;
            }
        }
    }
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
    conn->requestId = NHTTP_SendRequestAsync(info, conn->request);
    if (conn->requestId >= 0) {
        conn->state = 1;
    }
    return 0;
}

/* Answers the request handle the send hook of `NHTTPCreateRequest` was given, or null when the
 * connection or its request could not be made. */
NHTTPRequest* __NHTTPCreateRequestEx(const char* url, s32 method, u32 buffer, u32 bufferSize,
                                     NHTTPCompleteCallback completeCallback, u32 userData,
                                     NHTTPBufferFullCallback bufferFullCallback,
                                     NHTTPFreeBufferCallback freeCallback) {
    NHTTPConnection* connection = NHTTPCreateConnection(url, method, buffer, bufferSize,
                                                        NHTTPi_dispatchConnectionCallback, userData);
    NHTTPInfo* info;
    NHTTPMutexInfo* mutex;
    NHTTPRequest* request;

    if (connection != NULL) {
        info = NHTTPi_GetSystemInfoP();
        mutex = NHTTPi_GetMutexInfoP(info);
        connection = NHTTPi_GetConnection(mutex, connection);
        request = NHTTPi_Connection2Request(mutex, connection);
        if (request != NULL) {
            if (request->response != NULL) {
                connection->completeCallback = completeCallback;
                request->response->bufferFullCallback = bufferFullCallback;
                request->response->freeCallback = freeCallback;
                return request;
            }
            NHTTP_DestroyRequest(info, request);
            NHTTPi_OmitConnectionList(mutex, connection);
            NHTTPi_free(connection);
        }
    }
    return NULL;
}

/* Allocate a connection and its request, register the connection in the list and set it up in the
 * "created" state (status 0xF, pending). */
NHTTPConnection* NHTTPCreateConnection(const char* url, s32 method, u32 buffer, u32 bufferSize,
                                       NHTTPConnectionCallback callback, u32 userData) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPConnection* connection = NHTTPi_alloc(0x8060, 0x20);

    if (connection == NULL) {
        NHTTPi_SetError(bgnend, 1);
        return NULL;
    }
    NHTTPi_memclr(connection, 0x8060);
    connection->request = NHTTPi_createRequestObject(bgnend, url, method, buffer, bufferSize, userData,
                                                     NULL, NULL);
    if (connection->request == NULL) {
        NHTTPi_free(connection);
        return NULL;
    }
    connection->response = connection->request->response;
    connection->state = 0;
    connection->callback = callback;
    connection->postData = 0;
    connection->postLength = 0;
    connection->requestId = -1;
    NHTTPi_AddConnection(mutex, connection);
    connection->status = 0xF;
    connection->sslStatus = 0;
    connection->pending = 1;
    NHTTPi_SetSock(connection, NULL);
    connection->completeCallback = NULL;
    connection->recvOffset = 0;
    connection->recvUnread = 0;
    NHTTPi_memclr(connection->recvBuffer, 0x8000);
    return connection;
}

/* Answers the body window of the response's connection: the buffer and its size go out through
 * `buffer`/`size`, and the byte count received so far is the result; -1 when the response has no
 * connection. */
s32 NHTTPGetBodyBuffer(NHTTPConnection* connection, u32* buffer, u32* size) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHTTPConnection* node = NHTTPi_GetConnection(mutex, connection);
    NHTTPResponse* response;

    if (node != NULL) {
        response = NHTTPi_Connection2Response(mutex, node);
        if (response != NULL) {
            *buffer = response->userBuffer;
            *size = response->bufferSize;
            return response->received;
        }
        return -1;
    }
    return -1;
}

/* 0x8051A428 (0x74): the response's own size word (+0x438), or 0 when the connection or its
 * response is missing.  It is the last argument of every phase callback `NHTTPi_dispatchConnectionCallback` dispatches. */
s32 NHTTPi_GetResponseSize(NHTTPConnection* connection) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHTTPConnection* conn = NHTTPi_GetConnection(mutex, connection);
    NHTTPResponse* response;

    if (conn != NULL) {
        response = NHTTPi_Connection2Response(mutex, conn);
        if (response != NULL) {
            return response->userData;
        }
        return 0;
    }
    return 0;
}

/* 0x8051A49C (0x4C): the connection's status word (+0x04 - 0xF at creation, 8 once cancelled),
 * or -1 when the connection is not in the list. */
s32 NHTTPi_GetConnectionStatus(NHTTPConnection* connection) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHTTPConnection* conn = NHTTPi_GetConnection(mutex, connection);

    if (conn != NULL) {
        return conn->status;
    }
    return -1;
}

/* 0x8051A4E8 (0x8C): bring the HTTP layer up with the caller's two command callbacks and one
 * command id.  The library's version banner is registered once, behind its own lazy-init flag, and
 * all three arguments go on to `NHTTPi_Startup`; answers 0 when it came up and -1 when it did not. */
s32 NHTTPi_RegisterCallbacks(void (*commandCallback)(u32), void (*commandCallbackEx)(u32), u32 command) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();

    if (NHTTPi_versionRegistered == 0) {
        OSRegisterVersion(NHTTPi_versionString);
        NHTTPi_versionRegistered = 1;
    }
    return !NHTTPi_Startup(info, (NHTTPAllocFn)commandCallback, (NHTTPFreeFn)commandCallbackEx,
                           command)
               ? -1
               : 0;
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

/* The connection callback every created connection carries: it routes the four phases to what the
 * caller registered (the request's send hook, the response's buffer-full callback, the connection's
 * completion hook).  Phase 1 answers the hook's own value, -1 when there is none. */
s32 NHTTPi_dispatchConnectionCallback(NHTTPConnection* connection, s32 phase, NHTTPCallbackArgs* args) {
    NHTTPMutexInfo* mutex;
    s32 result;

    mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    connection = NHTTPi_GetConnection(mutex, connection);
    result = 0;
    switch (phase) {
    case 3:
        break;
    case 1: {
        NHTTPRequest* request;
        s32 (*sendCallback)(u32 arg0, u32* arg1, u32* arg2, u32 arg3, s32 size);

        if (connection != NULL && (request = NHTTPi_Connection2Request(mutex, connection)) != NULL &&
            (sendCallback = request->sendCallback) != NULL) {
            result = sendCallback(args->field_0x00, &args->field_0x04, &args->field_0x08, args->field_0x0C,
                                  NHTTPi_GetResponseSize(connection));
        } else {
            result = -1;
        }
        break;
    }
    case 2:
        if (connection != NULL) {
            NHTTPResponse* response = NHTTPi_Connection2Response(mutex, connection);

            if (response != NULL) {
                NHTTPBufferFullCallback bufferFullCallback = response->bufferFullCallback;

                if (bufferFullCallback != NULL) {
                    u32 buffer = args->field_0x00;
                    NHTTPSock* sock = NHTTPi_GetSock(connection);
                    s32 value = bufferFullCallback(&buffer, &args->field_0x04, sock, NHTTPi_alloc,
                                                   NHTTPi_free, NHTTPi_GetResponseSize(connection));

                    args->field_0x00 = value;
                    if (value != 0 && buffer != 0) {
                        args->field_0x08 = 0;
                    }
                }
            }
        }
        result = 0;
        break;
    case 4:
        if (connection != NULL) {
            NHTTPCompleteCallback completeCallback = connection->completeCallback;

            if (completeCallback != NULL) {
                NHTTPResponse* response = NHTTPi_Connection2Response(mutex, connection);

                if (response != NULL) {
                    s32 status = NHTTPi_GetConnectionStatus(connection);

                    completeCallback(status, response, NHTTPi_GetResponseSize(connection));
                }
            }
        }
        result = 0;
        break;
    }
    return result;
}

/* Append a header field to the request before it is sent; -1 (after a report) once it was. */
s32 NHTTPAddHeaderField(NHTTPRequest* handle, const char* token, const char* value) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPRequest* request = NHTTPi_GetRequest(mutex, (NHTTPConnection*)handle);
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);

    if (request == NULL) {
        return -1;
    }
    if (request->field_0x04 != 0) {
        printf("%s can be called before NHTTPStartConnection()\n", "NHTTPAddHeaderField");
        return -1;
    }
    return NHTTP_AddHeaderField(request, bgnend, token, value) == 0 ? -1 : 0;
}

/* The same for one url-encoded post field. */
s32 NHTTPAddPostDataAscii(NHTTPRequest* handle, const char* token, const char* value) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPRequest* request = NHTTPi_GetRequest(mutex, (NHTTPConnection*)handle);
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);

    if (request == NULL) {
        return -1;
    }
    if (request->field_0x04 != 0) {
        printf("%s can be called before NHTTPStartConnection()\n", "NHTTPAddPostDataAscii");
        return -1;
    }
    return NHTTPAddPostDataRaw(request, bgnend, token, value) == 0 ? -1 : 0;
}

/* Start the request, answering the id the send returned (or -1 when it could not be started). */
s32 NHTTPSendRequestAsync(NHTTPRequest* request) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHTTPConnection* connection = NHTTPi_Request2Connection(mutex, (s32)request);

    if (connection != NULL && NHTTPi_StartRequest(connection) == 0) {
        return connection->requestId;
    }
    return -1;
}

/* Create a request for `url`, wrapped as a connection: the caller's completion hook is stored, and the
 * NHTTPCREATE version is registered once. */
NHTTPRequest* NHTTPCreateRequest(const char* url, s32 method, u32 buffer, u32 bufferSize,
                                 NHTTPCompleteCallback completeCallback, u32 userData) {
    if (NHTTPi_createVersionRegistered == 0) {
        OSRegisterVersion(NHTTPi_createVersionString);
        NHTTPi_createVersionRegistered = 1;
    }
    return __NHTTPCreateRequestEx(url, method, buffer, bufferSize, completeCallback, userData, NULL, NULL);
}

/* Cancel request `id`; answers 0 when it was found and -1 when not. */
s32 NHTTPCancelRequest(s32 id) {
    return NHTTPi_cancelRequestById(NHTTPi_GetSystemInfoP(), id) == 0 ? -1 : 0;
}

/* Destroy the response the handle names together with its connection (and its list entry). */
void NHTTPDestroyResponse(NHTTPResponse* response) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHTTPConnection* connection = NHTTPi_Response2Connection(mutex, (s32)response);

    if (connection != NULL) {
        if (NHTTPi_Connection2Response(mutex, connection) != NULL) {
            NHTTP_DestroyResponse(mutex, connection->response);
        }
        NHTTPi_OmitConnectionList(mutex, connection);
        NHTTPi_free(connection);
    }
}

/* Answers the body window of the response's connection through `NHTTPGetBodyBuffer`, -1 when the
 * response has none. */
s32 NHTTPGetBodyAll(NHTTPResponse* response, u32* buffer) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHTTPConnection* connection = NHTTPi_Response2Connection(mutex, (s32)response);
    u32 size;

    if (connection != NULL) {
        size = 0;
        return NHTTPGetBodyBuffer(connection, buffer, &size);
    }
    return -1;
}

/* The response's HTTP status code, -1 until the head was parsed. */
s32 NHTTPGetResultCode(NHTTPResponse* response) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHTTPResponse* found = NHTTPi_GetResponse(mutex, (NHTTPConnection*)response);

    if (found == NULL) {
        return -1;
    }
    if (found->headersParsed != 0) {
        return found->statusCode;
    }
    return -1;
}

/* Store the SSL verification option in the request; -1 when the handle names no request. */
s32 NHTTPSetVerifyOption(NHTTPRequest* handle, u32 option) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHTTPRequest* request = NHTTPi_GetRequest(mutex, (NHTTPConnection*)handle);

    if (request == NULL) {
        return -1;
    }
    request->verifyOption = option;
    return 0;
}

/* Give the request an HTTP proxy: the host (at most 256 characters) and port, plus an optional
 * user/password pair (32 characters each) that is sent base64-encoded.  Answers 0, or -1 when the
 * handle names no request or a limit is exceeded. */
s32 NHTTPSetProxy(NHTTPRequest* handle, const char* host, s32 port, const char* user, const char* password) {
    NHTTPRequest* request;
    s32 hostLength;
    s32 userLength;
    s32 passwordLength;
    char credentials[0x41];

    request = NHTTPi_GetRequest(NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP()), (NHTTPConnection*)handle);
    if (request == NULL || host == NULL) {
        return -1;
    }
    hostLength = NHTTPi_strlen(host);
    if (hostLength > 0x100) {
        printf("proxy-address exceeded 256 characters\n");
        return -1;
    }
    NHTTPi_memcpy(request->proxyHost, host, hostLength);
    request->proxyPort = port;
    if (user != NULL && password != NULL) {
        userLength = NHTTPi_strlen(user);
        passwordLength = NHTTPi_strlen(password);
        if (userLength > 0x20) {
            printf("username exceeded 32 characters\n");
            return -1;
        }
        if (passwordLength > 0x20) {
            printf("password exceeded 32 characters\n");
            return -1;
        }
        NHTTPi_memclr(credentials, 0x41);
        NHTTPi_memcpy(credentials, user, userLength);
        NHTTPi_memcpy(credentials + userLength, ":", 1);
        NHTTPi_memcpy(credentials + userLength + 1, password, passwordLength);
        request->proxyAuthLength = NHTTPi_Base64Encode(request->proxyAuth, credentials);
    }
    request->useProxy = 1;
    return 0;
}

/* Apply the proxy the network settings hold (the https one for an https request) to the request,
 * logging what it uses.  Answers 0 when a proxy was set and -1 when the settings hold none; a proxy
 * the request refuses is fatal. */
s32 NHTTPSetSystemProxy(NHTTPRequest* handle) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPRequest* request = NHTTPi_GetRequest(NHTTPi_GetMutexInfoP(info), (NHTTPConnection*)handle);
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);
    NHTTPProxyConfig* proxy;
    char* user;
    char* password;
    s32 result;

    if (request->useSSL != 0) {
        proxy = &bgnend->httpsProxy;
    } else {
        proxy = &bgnend->httpProxy;
    }
    if (proxy->enabled == 1) {
        user = NULL;
        password = NULL;
        if (proxy->authEnabled == 1) {
            user = proxy->user;
            password = proxy->password;
        }
        OSReport("Using proxy server %s:%d (%s/%s)\n", proxy->host, proxy->port,
                 user != NULL ? user : "[no-auth]", password != NULL ? password : "[no-auth]");
        result = NHTTPSetProxy(request, proxy->host, proxy->port, user, password);
        if (result < 0) {
            OSReport("NHTTPSetProxy failed.(%d)\n", result);
            OSPanic("d_nhttp.c", 988, "NHTTPSetProxy");
        } else {
            return 0;
        }
    }
    return -1;
}

/* Forget the request's custom root CA and built-in CA id. */
s32 NHTTPClearRootCA(NHTTPRequest* handle) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHTTPRequest* request = NHTTPi_GetRequest(mutex, (NHTTPConnection*)handle);
    s32 result;

    if (request == NULL) {
        result = -1;
    } else {
        request->builtinRootCAId = 0;
        request->rootCA = 0;
        request->rootCALength = 0;
        result = 0;
    }
    return result;
}

/* Forget the request's client certificate (and select the built-in one). */
s32 NHTTPClearClientCert(NHTTPRequest* handle) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHTTPRequest* request = NHTTPi_GetRequest(mutex, (NHTTPConnection*)handle);
    s32 result;

    if (request == NULL) {
        result = -1;
    } else {
        request->builtinClientCertId = 0;
        request->builtinClientCert = 1;
        request->clientCert = 0;
        request->clientCertLength = 0;
        request->clientKey = 0;
        request->clientKeyLength = 0;
        result = 0;
    }
    return result;
}

/* Lazily set up the completion record, then clear the connection's pending flag under its mutex and
 * wake whoever waits on it. */
void NHTTPi_NotifyCompletion(NHTTPConnection* connection) {
    NHTTPiCompletionSync* sync = &NHTTPi_completionSync;

    if (sync->initialized == 0) {
        OSInitMutex(&sync->mutex);
        OSInitThreadQueueThunk(&sync->queue);
        sync->initialized = 1;
    }
    OSLockMutex(&sync->mutex);
    connection->pending = 0;
    OSWakeupThreadThunk(&sync->queue);
    OSUnlockMutex(&sync->mutex);
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
    return (NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_ADD) != NULL) - 1;
}

/* 0x8051B218 (0x30): take the connection out of the list, answering 0 when it was in it and -1 when
 * it was not. */
s32 NHTTPi_OmitConnectionList(NHTTPMutexInfo* mutex, NHTTPConnection* connection) {
    return (NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_OMIT) != NULL) - 1;
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
            args.field_0x04 = connection->postData;
            args.field_0x08 = connection->postLength;
            args.field_0x0C = arg2;
            result = connection->callback(connection, 1, &args);
            arg1 = args.field_0x04;
            arg2 = args.field_0x08;
            node = NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_GET);
            if (node != NULL) {
                node->postData = arg1;
                node->postLength = arg2;
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
                args.field_0x00 = response->userBuffer;
                args.field_0x04 = response->bufferSize;
                args.field_0x08 = response->received;
                connection->callback(connection, 2, &args);
                {
                    u32 userBuffer = args.field_0x00;
                    u32 bufferSize = args.field_0x04;
                    u32 received = args.field_0x08;

                    node = NHTTPi_GetConnection(mutex, connection);
                    if (node != NULL) {
                        response = NHTTPi_Connection2Response(mutex, node);
                        if (response != NULL) {
                            response->userBuffer = userBuffer;
                            response->bufferSize = bufferSize;
                            response->received = received;
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
                args.field_0x00 = response->userBuffer;
                args.field_0x04 = response->bufferSize;
                args.field_0x08 = response->received;
                connection->callback(connection, 3, &args);
                {
                    u32 userBuffer = args.field_0x00;
                    u32 bufferSize = args.field_0x04;
                    u32 received = args.field_0x08;

                    node = NHTTPi_GetConnection(mutex, connection);
                    if (node != NULL) {
                        response = NHTTPi_Connection2Response(mutex, node);
                        if (response != NULL) {
                            response->userBuffer = userBuffer;
                            response->bufferSize = bufferSize;
                            response->received = received;
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

/* Drop the connection list: warn when connections were still in it. */
void NHTTPi_InitConnectionList(void) {
    NHTTPConnection* node = NHTTPi_connectionListHead;
    s32 count;

    if (node != NULL) {
        count = 0;
        while (node != NULL) {
            node = node->next;
            count++;
        }
        if (count != 0) {
            printf("*warning: %d connections rests! Please free connections.\n", count);
        }
    }
    NHTTPi_connectionListHead = NULL;
}

/* Build the request object for `url`: validate the method, copy the URL (decoding %XX escapes, except
 * inside the path), read out the scheme (http / https), host and port, and set up the response record
 * that hangs off it.  Answers the request, or null with the info block's error set (11 bad method, 1 no
 * memory, 4 bad URL). */
NHTTPRequest* NHTTPi_createRequestObject(NHTTPInfo* info, const char* url, s32 method, u32 buffer,
                                         u32 bufferSize, u32 userData,
                                         NHTTPBufferFullCallback bufferFullCallback,
                                         NHTTPFreeBufferCallback freeCallback) {
    char urlBuffer[0x100];
    char* urlCopy = urlBuffer;
    NHTTPRequest* request = NULL;
    s32 urlLength;
    s32 prefix;
    s32 rest;
    char* restPtr;
    char* p;
    s32 i;
    s32 out;
    s32 escapes;
    s32 state;
    s32 sawSlash;
    s32 hostLength;
    s32 size;
    s32 number;
    s8 decoded;
    u8 c;
    u32 escaped;
    char* hostStart;

    do {
        if (3 <= method || method < 0) {
            NHTTPi_SetError(info, 11);
            break;
        }
        request = NHTTPi_alloc(0x258, 4);
        if (request == NULL) {
            NHTTPi_SetError(info, 1);
            break;
        }
        NHTTPi_memclr(request, 0x258);
        request->response = NHTTPi_alloc(0x43C, 4);
        if (request->response == NULL) {
            NHTTPi_SetError(info, 1);
            break;
        }
        NHTTPi_memclr(request->response, 0x43C);
        request->response->userBuffer = buffer;
        request->response->bufferSize = bufferSize;
        request->response->bufferFullCallback = bufferFullCallback;
        request->response->freeCallback = freeCallback;
        urlLength = NHTTPi_strlen(url);
        if (urlLength <= 7) {
            NHTTPi_SetError(info, 4);
            break;
        }
        if (urlLength + 1 > 0x100) {
            urlCopy = NHTTPi_alloc(urlLength + 1, 4);
            if (urlCopy == NULL) {
                NHTTPi_SetError(info, 4);
                break;
            }
        }
        NHTTPi_memclr(urlCopy, urlLength);
        NHTTPi_memcpy(urlCopy, url, urlLength);
        request->port = 80;
        prefix = 7;
        if (NHTTPi_strnicmp(urlCopy, "http://", 7) != 0) {
            if (NHTTPi_strnicmp(urlCopy, "https://", 8) != 0) {
                NHTTPi_SetError(info, 4);
                break;
            }
            request->useSSL = 1;
            prefix = 8;
            request->port = 443;
        }
        rest = urlLength - prefix;
        restPtr = urlCopy + prefix;
        if (rest <= 0) {
            NHTTPi_SetError(info, 4);
            break;
        }
        p = restPtr;
        i = 0;
        escapes = 0;
        state = 0;
        decoded = 0;
        while (i < rest && *p != '/') {
            if (state == 2) {
                state -= 1;
            } else if (state == 1) {
                decoded = NHTTPi_strToHex(restPtr + i - 1, 2);
                state -= 1;
                if (decoded < 0) {
                    break;
                }
                if (decoded == '/') {
                    escapes -= 1;
                    break;
                }
            } else if ((s8)*p == '%') {
                state = 2;
                escapes += 1;
            }
            i++;
            p++;
        }
        if (decoded < 0 || state != 0) {
            NHTTPi_SetError(info, 4);
            break;
        }
        size = prefix + rest - escapes * 2 + 1;
        request->url = NHTTPi_alloc(size, 4);
        if (request->url == NULL) {
            NHTTPi_SetError(info, 1);
            break;
        }
        NHTTPi_memclr(request->url, size);
        NHTTPi_memcpy(request->url, urlCopy, prefix);
        p = restPtr;
        i = 0;
        out = 0;
        state = 0;
        sawSlash = 0;
        while (i < rest) {
            if (state == 2) {
                state -= 1;
            } else if (state == 1) {
                decoded = NHTTPi_strToHex(restPtr + i - 1, 2);
                state -= 1;
                request->url[out + prefix - 1] = decoded;
                if (decoded == '/') {
                    sawSlash = 1;
                }
            } else {
                c = *p;
                if ((s8)c == '/') {
                    sawSlash = 1;
                }
                escaped = (sawSlash == 0) & ((s8)c == '%');
                if (escaped != 0) {
                    state = 2;
                } else {
                    request->url[out + prefix] = c;
                }
                out += 1;
            }
            i++;
            p++;
        }
        request->url[prefix + out] = 0;
        hostStart = request->url + prefix;
        p = hostStart;
        i = 0;
        for (size = out; size > 0; size--) {
            if (*p == '/' || *p == ':') {
                request->hostEnd = i + prefix;
                break;
            }
            i++;
            p++;
        }
        if (i == out) {
            request->hostEnd = i + prefix;
            request->pathStart = request->hostEnd;
        } else if ((s8)hostStart[i] == '/') {
            request->pathStart = request->hostEnd;
        } else if ((s8)hostStart[i] == ':') {
            p = hostStart + i;
            for (size = out - i; size > 0; size--) {
                if (*p == '/') {
                    request->pathStart = i + prefix;
                    break;
                }
                i++;
                p++;
            }
            if (i == out) {
                request->pathStart = i + prefix;
            } else {
                number = NHTTPi_strtonum(request->url + request->hostEnd + 1,
                                         request->pathStart - (request->hostEnd + 1));
                if (number < 0) {
                    number = request->port;
                } else if (number > 0xFFFF) {
                    NHTTPi_SetError(info, 4);
                    break;
                }
                request->port = (u16)number;
            }
        }
        hostLength = request->hostEnd - ((request->useSSL != 0) + 7);
        request->host = NHTTPi_alloc(hostLength + 1, 4);
        if (request->host == NULL) {
            NHTTPi_SetError(info, 1);
            break;
        }
        NHTTPi_memclr(request->host, hostLength + 1);
        NHTTPi_memcpy(request->host, request->url + ((request->useSSL != 0) + 7), hostLength);
        NHTTPi_memcpy(&request->boundaryDashes, NHTTPi_postDataRawCode, 0x14);
        request->method = method;
        request->sslHandle = 0;
        request->clientCert = 0;
        request->clientCertLength = 0;
        request->clientKey = 0;
        request->clientKeyLength = 0;
        request->rootCA = 0;
        request->rootCALength = 0;
        request->builtinClientCert = 0;
        request->verifyOption = 0;
        request->sslCallbackArg = 0;
        request->response->userData = userData;
        request->useProxy = 0;
        request->recvBufferSize = 0;
        request->sendBufferSize = 0;
        if (urlCopy != NULL && urlCopy != urlBuffer) {
            NHTTPi_free(urlCopy);
        }
        return request;
    } while (0);
    if (request != NULL) {
        if (request->url != NULL) {
            NHTTPi_free(request->url);
        }
        if (request->host != NULL) {
            NHTTPi_free(request->host);
        }
        if (request->response != NULL) {
            NHTTPi_free(request->response);
        }
        NHTTPi_free(request);
    }
    if (urlCopy != NULL && urlCopy != urlBuffer) {
        NHTTPi_free(urlCopy);
    }
    return NULL;
}
