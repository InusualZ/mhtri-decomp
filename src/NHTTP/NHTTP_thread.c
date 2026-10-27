/*
 * NHTTP_thread.c - the Revolution SDK NHTTP library's comm-thread TU: the thread's ready flag, the request
 *   writer (send buffer, POST fields, header fields, the three body encodings, the proxy CONNECT), the
 *   response reader (head, header parse, body with chunked transfer) and the loop that serves a request.
 * RANGE. .text 0x80517614-0x8051A1B8 (25 functions), in address order; .rodata 0x80574D28-0x80574E00, .data
 *   0x80630B80-0x80630C50, .sdata 0x807943B0-0x80794410.  The pools are this TU's: `sendProxyConnect`,
 *   `sendRequest` and `parseResponseHeaders` address `.data` off 0x80630B80 and `sendMultipartBody` `.rodata`
 *   off 0x80574D28 (an anchor is the object's own section start), `.sdata` 0x807943B0 is this TU's `":"`
 *   (`d_nhttp.c` pools its own at 0x80794418), and 0x80630B80 sits behind a fill a single TU would not leave.
 *   The `.text` edges have no data pin: the left one is the first comm-thread helper, the right one the
 *   loop's end before the public `NHTTPCreateConnection`.
 * SOURCE FILE. GUESS `NHTTP_thread.c`: no `__FILE__` string; named for what the TU holds.
 * FLAGS. GC/3.0a5.2 with `cflags_nhttp`, plus `#pragma inline_max_size(8)` (as `d_nhttp.c`).
 * NAMES. GUESS, read off the bodies: `NHTTPi_isHeaderEnd`, the comm-thread steps (`dequeueRequest`,
 *   `prepareTarget`, `connectSocket`, `negotiateSSL`, `sendRequest`, `sendHeaderFields`, `sendRawPostBody`,
 *   `sendMultipartBody`, `sendUrlEncodedBody`, `sendProxyConnect`, `recvProxyConnectReply`,
 *   `recvResponseHeaders`, `parseResponseHeaders`, `recvResponseBody`, `finishRequest`) and the inline helper
 *   `NHTTPi_skipLine` (no symbol of its own).
 * SHAPES.
 *   - `NHTTPi_skipLine` (the chunk-extension and trailer skips of `NHTTPi_recvResponseBody`) is an inline
 *     helper whose early returns join in one register (playbook 101).
 *   - `volatile NHTTPResponse.headerLength` / `NHTTPRecvBuf.capacity` where retail re-reads them (playbook 130).
 *   - `NHTTPi_commThreadLoop`'s switches list `case 0:` with `default` (the compare tree pivots on it) and the
 *     bodies in retail's order (again, finish, send); `sendRequest` answering 1 finishes the request.
 *   - the send steps keep `u8* sendBuffer = thread->sendBuffer;` as a local (retail hoists the `addi`).
 *   - `NHTTPi_sendUrlEncodedBody` adds a field's encoded token length and its `=` as two statements
 *     (`total += len; total += 1;`), which gives retail's `add r3,r0,r3` operand order.
 *   - `NHTTPi_sendRequest` and `NHTTPi_recvProxyConnectReply` declare their locals bare and assign them below
 *     in call order: the declaration order (`urlLength` after `hostOffset`; `response` before `request`) is
 *     what moves the register colouring towards retail's.
 * RESIDUALS.
 *   - `NHTTPi_sendPostField`, `NHTTPi_recvResponseBody`, `NHTTPi_recvProxyConnectReply`: register colouring (a
 *     hill-climb over each one's declaration order, initialisers moved below the declarations, found nothing
 *     closer for the first two).
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
    s32 chunk;
    s32 remaining = length;
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
    s32 result;
    u32 i;
    u8* data;
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
        case 2: {
            u8* p = data;

            for (i = 0; i < chunk; i++, p++) {
                NHTTPi_memclr(encoded, 3);
                result = NHTTPi_SaveBuf(request, buffer, fd, used, encoded,
                                        NHTTPi_encodeUrlChar(encoded, *p));
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
    u8* sendBuffer = thread->sendBuffer;
    s32 result;

    if (length == 0) {
        return 0;
    }
    result = NHTTPi_SaveBuf(request, sendBuffer, bgnend->socket, &context->sendUsed, (u8*)data, length);
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
    NHTTPInfo* info;
    NHTTPRequestInfo* requestInfo;
    NHTTPThreadInfo* thread;
    NHTTPInfo* bgnend;
    NHTTPMutexInfo* mutex;
    NHTTPResponse* response;
    NHTTPRequest* request;
    u8* discard;
    s32 ok;
    s32 received;
    s32 count;
    char reply[0x200];
    s32 i;
    s32 blank;
    s32 result;

    info = NHTTPi_GetSystemInfoP();
    requestInfo = NHTTPi_GetReqInfoP(info);
    thread = NHTTPi_GetThreadInfoP(info);
    bgnend = NHTTPi_GetBgnEndInfoP(info);
    mutex = NHTTPi_GetMutexInfoP(info);
    request = (NHTTPRequest*)requestInfo->active->request;
    response = request->response;
    discard = thread->sendBuffer;
    ok = 0;
    received = 0;

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
    u8* sendBuffer = thread->sendBuffer;
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
            result = NHTTPi_sendPostField(mutex, request, sendBuffer, field->token, bgnend->socket,
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
    u8* sendBuffer = thread->sendBuffer;
    NHTTPHeaderField* field;
    char digits[12];
    s32 digitCount;
    s32 result;
    s32 i;

    for (field = request->postDataList; field != NULL; field = field->prev) {
        total += NHTTPi_urlEncodedLength(field->token);
        total += 1;
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
            result = NHTTPi_sendPostField(mutex, request, sendBuffer, field->token, bgnend->socket,
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
    NHTTPInfo* info;
    NHTTPRequestInfo* requestInfo;
    NHTTPRequest* request;
    NHTTPMutexInfo* mutex;
    NHTTPInfo* bgnend;
    NHTTPConnection* connection;
    NHTTPThreadInfo* thread;
    u8* sendBuffer;
    s32 result;
    s32 hostOffset;
    s32 urlLength;
    s32 sent;

    info = NHTTPi_GetSystemInfoP();
    requestInfo = NHTTPi_GetReqInfoP(info);
    request = (NHTTPRequest*)requestInfo->active->request;
    mutex = NHTTPi_GetMutexInfoP(info);
    bgnend = NHTTPi_GetBgnEndInfoP(info);
    connection = NHTTPi_Request2Connection(mutex, (s32)request);
    thread = NHTTPi_GetThreadInfoP(info);
    sendBuffer = thread->sendBuffer;
    urlLength = NHTTPi_strlen(request->url);
    result = 0;

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
    s32 lineEnd;
    s32 offset;

    if (NHTTPi_RecvBufCopy((NHTTPRecvBuf*)response, context->statusLine, 0, 14) == 0) {
        context->error = 7;
        return 0;
    }
    if (NHTTPi_strnicmp((char*)context->statusLine, "HTTP/", 5) != 0) {
        context->error = 7;
        return 0;
    }
    if ((s32)context->statusLine[8] != ' ') {
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
    } else {
        context->chunked = context->chunked > 0 ? NHTTPi_RecvBufFindUpper((NHTTPRecvBuf*)response, offset,
                                                                          offset + context->chunked, "chunked",
                                                                          ';') == 0
                                                : 0;
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

/* Reads and drops bytes one at a time up to the end of the current line (a CR read right after an LF
 * ends it, the order the two-byte window is tested in); the number of bytes dropped, or the failing
 * receive's result. */
static inline s32 NHTTPi_skipLine(NHTTPMutexInfo* mutex, NHTTPRequest* request, s32 fd) {
    s32 received;
    s32 skipped;
    s32 count;
    char window[2];

    window[0] = 0;
    window[1] = 0;
    skipped = 0;
    count = 0;
    while (window[count & 1] != '\r' || (s8)window[(count - 1) & 1] != '\n') {
        received = NHTTPi_SocRecv((s32)mutex, (NHTTPConnection*)request, fd, (u8*)&window[count & 1], 1, 0);
        if (received <= 0) {
            return received;
        }
        skipped += received;
        count++;
    }
    return skipped;
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
    u8 sizeWindow[2];
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
                        } else if (NHTTPi_skipLine(mutex, request, bgnend->socket) <= 0) {
                            return 0;
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
                    NHTTPi_skipLine(mutex, request, bgnend->socket);
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
 * would-block answer (2) keeps the request for the next round; a failed step (1, or 3 from the send)
 * finishes it. */
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
    context.again = 0;
    context.contentRemaining = 0;
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
            case 0:
            default:
                switch (NHTTPi_sendRequest(&context)) {
                case 2:
                    context.again = 1;
                    break;
                case 1:
                case 3:
                    NHTTPi_finishRequest(&context);
                    break;
                case 0:
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
