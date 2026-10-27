/*
 * NHTTP_request.c - the Revolution SDK NHTTP library's request TU: the request object (build, destroy,
 *   queue, cancel), the response's destroy and header-field lookup, and the `NHTTPi_Soc*` socket layer.
 * RANGE. .text 0x80515774-0x80516CA0 (19 functions), in address order; .rodata 0x80574CE8-0x80574D00 (the
 *   multipart boundary), .data 0x80630B28-0x80630B38 (`"https://"`), .sdata 0x807943A8-0x807943B0
 *   (`"http://"`).  Left edge: `NHTTP_os_RVL.c`'s end and the first referrer of this TU's `.data`.  Right
 *   edge: the `.data` 7-byte fill 0x80630B31..0x80630B38 starts the next TU (`NHTTPi_base64Alphabet`, 0x41 B,
 *   would sit at +4 inside one object); the `.text` cut inside 0x80515774..0x80517438 has no data pin, so it
 *   is placed where the socket layer ends and the string helpers begin.
 * SOURCE FILE. GUESS `NHTTP_request.c`: no `__FILE__` string; the name follows the siblings' `NHTTP_` scheme
 *   and what the TU holds.
 * FLAGS. GC/3.0a5.2 with `cflags_nhttp`, plus `#pragma inline_max_size(8)` (as `d_nhttp.c`).
 * NAMES. GUESS, read off the bodies: `NHTTPi_createRequestObject`, `NHTTPi_cancelRequestById`,
 *   `NHTTPi_probeSocket`, `NHTTPi_openSocket`, `NHTTPi_SocConnect`, `NHTTPi_SocShutdown`,
 *   `NHTTPi_resolveHostName`, `NHTTPi_findHeaderField`.
 * SHAPES.
 *   - `NHTTPi_createRequestObject` is a `do { ... } while (0)` whose `break`s reach the shared cleanup (rule 8).
 *   - `volatile NHTTPSock.length` where retail re-reads it (playbook 130); `if (r > 0) { ... } else { return r; }`
 *     after `SORecv`/`SOSend` (playbook 131); `NHTTPi_SocRecv` returns -1001 directly when the request has no
 *     connection.
 * RESIDUALS.
 *   - `NHTTPi_createRequestObject`: retail calls `NHTTPi_SetError` and jumps to the cleanup from inside the URL
 *     scan loop, a two-level exit rule 8 forbids; plus register colouring.
 *   - `NHTTPi_findHeaderField` (prologue copy order), `NHTTPi_SocSend` (the `sslHandle` load scheduled above the
 *     `r31` save): register colouring or scheduling.
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

/* The multipart boundary every new request starts from (`.rodata` 0x80574CE8); `NHTTP_bgnend.c` steps a
 * request's copy until no posted value contains it. */
const char NHTTPi_postDataRawCode[] = "--t9Sf4yfjf1RtvDu3AA";

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
    s32 out;
    s32 sawSlash;
    s32 rest;
    char* restPtr;
    s32 i;
    s32 escapes;
    NHTTPRequest* request = NULL;
    s32 prefix;
    char* p;
    s32 state;
    s32 urlLength;
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
    s32 valueStart;
    s32 line;
    s32 next;
    s32 valueEnd;

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
                valueStart = NHTTPi_RecvBufSkipSpaces(ring, colon + 1, valueEnd);
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
    u8* recvBuffer = connection->recvBuffer;
    s32 got = 0;
    s32 refill;

    if (length > 0) {
        if (connection->recvUnread == 0) {
            refill = SORecv(fd, recvBuffer, 0x8000, flags);
            if (refill > 0) {
                connection->recvUnread = refill;
                connection->recvOffset = 0;
            } else {
                return refill;
            }
        }
        if (connection->recvUnread != 0) {
            if ((u32)length > connection->recvUnread) {
                length = connection->recvUnread;
            }
            NHTTPi_memcpy(buf, recvBuffer + connection->recvOffset, length);
            connection->recvUnread -= length;
            if (connection->recvUnread == 0) {
                NHTTPi_memclr(recvBuffer, 0x8000);
                connection->recvOffset = 0;
            } else {
                connection->recvOffset += length;
            }
            got = length;
        }
    }
    return got;
}

/* Receive `length` bytes through SSL when the request holds a context, through the connection's
 * buffer otherwise; -1001 at once when the request has no connection.  A failed receive maps to -1002
 * for a cancelled request, 0 for a would-block, and -1001 for anything else.  (`handle` is the request-list mutex and `conn` the request - see the header.) */
s32 NHTTPi_SocRecv(s32 handle, NHTTPConnection* conn, s32 flags, u8* buf, s32 length, s32 arg) {
    s32 result;

    if (((NHTTPRequest*)conn)->sslHandle > 0) {
        result = SSLRead(((NHTTPRequest*)conn)->sslHandle, buf, length);
    } else {
        NHTTPConnection* connection = NHTTPi_Request2Connection((NHTTPMutexInfo*)handle, (s32)conn);

        if (connection != NULL) {
            result = NHTTPi_SocRecv_sub(connection, flags, buf, length, arg);
        } else {
            return -1001;
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
        if (sent > 0) {
            total = sent;
            if ((u32)sent < head) {
                return sent;
            }
            buf += sent;
            length -= sent;
        } else {
            return sent;
        }
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

/* Resolve a host name to its first IPv4 address, or 0 when it does not resolve. */
u32 NHTTPi_resolveHostName(NHTTPRequest* request, const char* name) {
    u32 address = 0;
    SOAddrInfo* result;

    if (SOGetAddrInfo(name, NULL, NULL, &result) == 0) {
        NHTTPi_memcpy(&address, &result->addr->addr, 4);
        SOFreeAddrInfo(result);
    } else {
        address = 0;
    }
    return address;
}
