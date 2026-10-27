/*
 * NHTTP_list.c - the Revolution SDK NHTTP library's connection-list TU: the completion notice, the
 *   connection list and its lookups, the three request/response callbacks, the system-info singleton and
 *   its sub-record accessors, and the list's teardown.
 * RANGE. .text 0x8051B05C-0x8051B7FC (24 functions), in address order; .data 0x80630F48-0x80630F88 (the
 *   teardown's warning), .bss 0x80762C20-0x80763900, .sbss 0x80795880-0x80795888.  Left edge: the `.data`
 *   7-byte fill 0x80630F41..F48 starts this TU; the `.text` cut inside 0x8051AF8C..0x8051B0D8 is placed after
 *   the last public `NHTTP` call.  Right edge: `SSLNew`, the SSL library's start.
 * SOURCE FILE. GUESS `NHTTP_list.c`: no `__FILE__` string; named for what the TU holds.
 * FLAGS. GC/3.0a5.2 with `cflags_nhttp`, plus `#pragma inline_max_size(8)` (as `d_nhttp.c`).
 * NAMES. GUESS: `NHTTPiCompletionSync`/`NHTTPi_completionSync` (the record's name), `NHTTPi_connectionListHead`.
 * LAYOUT. `NHTTPi_systemInfo` is one 0xCA0-byte `NHTTPInfo` (the map splits it at 0x8076317A, a label no
 *   relocation names).
 * SHAPES.
 *   - `(u32)mode == NHTTPI_LIST_ADD` for the one unsigned compare in `NHTTPi_ControlConnectionList`, whose walk
 *     steps `link = &(*link)->next`.
 *   - `NHTTPi_GetSystemInfoP` sits in `#pragma dont_inline on`.
 *   - `NHTTPi_AddConnection`/`NHTTPi_OmitConnectionList` spell `found ? 0 : -1` as `~(found | -found) >> 31`:
 *     only that gives retail's `neg`/`nor`/`srawi` (a ternary, `-(r == 0)`, `-!r`, `(r != 0) - 1`, an `if`
 *     and a `BOOL` local all give `cntlzw` or `or`/`srwi`/`subi`).
 *   - `NHTTPi_BufferFullCallback`/`NHTTPi_RecvCallback` declare the three words in field order and read them
 *     back to front, which gives retail's register colouring.
 * RESIDUALS.
 *   - `NHTTPi_PostSendCallback`: register colouring (`result`/`arg1` take each other's r29/r31; reading the
 *     two words back to front gains a little, no declaration order more).
 *   - flip blocker: `NHTTPi_PostSendCallback` (8 bytes); flipcheck does not list the map row `lbl_8076317A`.
 */

#include "NHTTP/d_nhttp.h"
#include "NHTTP/NHTTP_request.h"
#include "NHTTP/NHTTP_util.h"
#include "NHTTP/NHTTP_thread.h"
#include "NHTTP/NHTTP_list.h"
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

/* The data this unit owns: the `.sbss` words 0x80795880..0x80795888 (the connection list's head and the
 * system-info slot) and the `.bss` run 0x80762C20..0x80763900 (the 0x40-byte completion record and the
 * 0xCA0-byte system-info block), all zero-filled.  MWCC emits `.sbss` objects in reverse definition
 * order, so the two words are listed back to front. */
NHTTPInfo* NHTTPi_systemInfoP;                /* 0x80795884 */
NHTTPConnection* NHTTPi_connectionListHead;   /* 0x80795880 */
NHTTPInfo NHTTPi_systemInfo;                  /* 0x80762C60 */
NHTTPiCompletionSync NHTTPi_completionSync;   /* 0x80762C20 */

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
    if ((u32)mode == NHTTPI_LIST_ADD) {
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
            link = &(*link)->next;
        }
    }
    NHTTPi_unlockReqList(mutex);
    return result;
}

/* 0x8051B1E8 (0x30): put the connection at the head of the list, answering 0 when it went in and -1
 * when it did not. */
s32 NHTTPi_AddConnection(NHTTPMutexInfo* mutex, NHTTPConnection* connection) {
    s32 found = (s32)NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_ADD);

    return ~(found | -found) >> 31;
}

/* 0x8051B218 (0x30): take the connection out of the list, answering 0 when it was in it and -1 when
 * it was not. */
s32 NHTTPi_OmitConnectionList(NHTTPMutexInfo* mutex, NHTTPConnection* connection) {
    s32 found = (s32)NHTTPi_ControlConnectionList(mutex, connection, NHTTPI_LIST_OMIT);

    return ~(found | -found) >> 31;
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
            arg2 = args.field_0x08;
            arg1 = args.field_0x04;
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
                    u32 userBuffer;
                    u32 bufferSize;
                    u32 received;

                    received = args.field_0x08;
                    bufferSize = args.field_0x04;
                    userBuffer = args.field_0x00;

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
                    u32 userBuffer;
                    u32 bufferSize;
                    u32 received;

                    received = args.field_0x08;
                    bufferSize = args.field_0x04;
                    userBuffer = args.field_0x00;

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
