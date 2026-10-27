/*
 * d_nhttp.c - the Revolution SDK NHTTP library's public-API TU: the connection object and the public
 *   NHTTP calls over requests, responses and connections, the library's version banners and its registration
 *   with the DWCi runtime.
 * RANGE. .text 0x8051A1B8-0x8051B05C (25 functions), in address order; .data 0x80630C50-0x80630F48, .sdata
 *   0x80794410-0x80794420, .sbss 0x80795878-0x80795880.  `NHTTPSetProxy`/`NHTTPSetSystemProxy` address `.data`
 *   off 0x80630C50 (an anchor is the object's own section start) and `.sdata` 0x80794418 is this TU's own
 *   `":"`.  The `.text` edges have no data pin: the left one is the first public call after the comm-thread
 *   loop, the right one the last public call before `NHTTPi_NotifyCompletion`.
 * SOURCE FILE. `NHTTPSetSystemProxy` panics with `"d_nhttp.c"` (`.data` 0x80630F04).
 * FLAGS. GC/3.0a5.2 with `cflags_nhttp`, plus `#pragma inline_max_size(8)` below: retail inlines only the
 *   one-line list wrappers and keeps a `bl` to every bigger function (the default limit folds them in).
 * NAMES. Public calls carry the SDK's spelling where a `.data` message names it (`NHTTPAddHeaderField`,
 *   `NHTTPAddPostDataAscii`, `NHTTPSetProxy`); GUESS: `NHTTPCreateRequest`, `NHTTPSendRequestAsync`,
 *   `NHTTPCancelRequest`, `NHTTPDestroyResponse`, `NHTTPGetBodyAll`, `NHTTPGetResultCode`, `NHTTPSetVerifyOption`,
 *   `NHTTPClearRootCA`, `NHTTPClearClientCert`, `NHTTPSetSystemProxy` (the SDK vocabulary; the body says what
 *   each does), and the inline helpers `NHTTPi_callSendCallback`, `NHTTPi_callBufferFullCallback`,
 *   `NHTTPi_callCompleteCallback` (no symbol of their own).  The four public calls the linker strips
 *   (`NHTTPAddPostDataBinary`, `NHTTPAddPostDataRaw`, `NHTTPSetPostDataEncoding`,
 *   `NHTTPDisableVerifyOptionForDebug`) take their names from their pooled `__FUNCTION__` strings; their
 *   parameters and everything past the "called before NHTTPStartConnection" guard are GUESSes.
 * LAYOUT. `NHTTPRequest` (NHTTP_bgnend.h), `NHTTPResponse`/`NHTTPConnection`/`NHTTPCommContext`/`NHTTPProxyConfig`
 *   (d_nhttp.h) come from the field offsets the bodies use; the response IS the receive ring `NHTTP_os_RVL.h`
 *   describes.  d_nhttp.h is the library's shared type header; each sibling TU declares its own calls.
 * SHAPES. Each measured against the alternatives:
 *   - the three phase handlers of `NHTTPi_dispatchConnectionCallback` are inline helpers whose early returns
 *     join in one register (playbook 101).
 *   - `r ? 0 : -1` (not `r == 0 ? -1 : 0`) for retail's `cntlzw`/`srwi`/`neg`.
 *   - `NHTTPSetProxy` spells the password's address `&credentials[userLength + 1]` (no CSE with the `":"` store).
 *   - `NHTTPAddHeaderField`/`NHTTPAddPostDataAscii` report through `__FUNCTION__`: retail's pool holds each
 *     name ahead of the shared message, the order a function-local name object gives (a literal argument
 *     pools after the format).
 *   - the four stripped public calls are written in their pool places (playbook 147): their names and the
 *     "(exclusive fucntion)" warning fill `.data` 0x80630DA8..0x80630E48 and 0x80630F20..0x80630F41.
 *   - `NHTTPCreateConnection` declares `connection` ahead of `info`/`bgnend`/`mutex` and assigns them after
 *     the declarations: the order gives retail's register colouring (`connection` takes `info`'s r29).
 * RESIDUALS. none in `.text`/`.data`; the trial flip links byte-identical.
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

/* The two version-registration flags (`.sbss` 0x80795878..0x80795880).  MWCC emits `.sbss` objects in
 * reverse definition order, so they are listed back to front. */
u32 NHTTPi_createVersionRegistered;           /* 0x8079587C */
u32 NHTTPi_versionRegistered;                 /* 0x80795878 */

/* The public NHTTP entry points 0x8051A300..0x8051A5CC (the map still carries their `fn_` rows;
 * these names are GUESSes derived from their bodies and their callers - the file header writes the
 * evidence for each). */

/* The library's version banners (`.sdata` 0x80794410/0x80794414 and the `.data` 0x80630C50..0x80630D48
 * run behind them).  `NHTTPi_RegisterCallbacks` and `NHTTPCreateRequest` each hand one of them to
 * `OSRegisterVersion`, once, behind the two lazy-init flags of the `.sbss` run.  They sit here, after the
 * comm-thread steps' strings (`"Transfer-Encoding"`, `"chunked"`) and before the public calls' messages,
 * which is their place in the retail pools.  The `.data` run also holds an unreferenced `UNOFFICIAL` variant
 * between the two banners (no relocation points into it), so all three are written, in this order. */
const char* NHTTPi_versionString =
    "<< RVL_SDK - NHTTP \trelease build: May 12 2009 11:01:00 (0x4199_60831) >>";
char NHTTPi_unofficialVersionString[] =
    "<< RVL_SDK - NHTTP \trelease build: May 12 2009 11:01:00 (0x4199_60831) UNOFFICIAL >>";
const char* NHTTPi_createVersionString =
    "<< RVL_SDK - NHTTPCREATE \trelease build: May 12 2009 11:01:00 (0x4199_60831) >>";

/* Allocate a connection and its request, register the connection in the list and set it up in the
 * "created" state (status 0xF, pending). */
NHTTPConnection* NHTTPCreateConnection(const char* url, s32 method, u32 buffer, u32 bufferSize,
                                       NHTTPConnectionCallback callback, u32 userData) {
    NHTTPConnection* connection;
    NHTTPInfo* info;
    NHTTPInfo* bgnend;
    NHTTPMutexInfo* mutex;

    info = NHTTPi_GetSystemInfoP();
    bgnend = NHTTPi_GetBgnEndInfoP(info);
    mutex = NHTTPi_GetMutexInfoP(info);
    connection = NHTTPi_alloc(0x8060, 0x20);

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

/* Hands the connection's request to its send callback with the phase-1 arguments and the response
 * size; -1 when the connection, its request or the callback is missing. */
static inline s32 NHTTPi_callSendCallback(NHTTPMutexInfo* mutex, NHTTPConnection* connection, NHTTPCallbackArgs* args) {
    NHTTPRequest* request;
    s32 (*sendCallback)(u32 arg0, u32* arg1, u32* arg2, u32 arg3, s32 size);

    if (connection != NULL && (request = NHTTPi_Connection2Request(mutex, connection)) != NULL &&
        (sendCallback = request->sendCallback) != NULL) {
        return sendCallback(args->field_0x00, &args->field_0x04, &args->field_0x08, args->field_0x0C,
                            NHTTPi_GetResponseSize(connection));
    }
    return -1;
}

/* Hands a full receive buffer to the response's buffer-full callback (the phase-2 arguments, the socket,
 * the allocator pair and the response size) and stores what it answers back into the arguments. */
static inline void NHTTPi_callBufferFullCallback(NHTTPMutexInfo* mutex, NHTTPConnection* connection,
                                                 NHTTPCallbackArgs* args) {
    if (connection != NULL) {
        NHTTPResponse* response = NHTTPi_Connection2Response(mutex, connection);

        if (response != NULL) {
            NHTTPBufferFullCallback bufferFullCallback = response->bufferFullCallback;

            if (bufferFullCallback != NULL) {
                u32 buffer = args->field_0x00;
                NHTTPSock* sock = NHTTPi_GetSock(connection);
                s32 value = bufferFullCallback(&buffer, &args->field_0x04, sock, NHTTPi_alloc, NHTTPi_free,
                                               NHTTPi_GetResponseSize(connection));

                args->field_0x00 = value;
                if (value != 0 && buffer != 0) {
                    args->field_0x08 = 0;
                }
            }
        }
    }
}

/* Runs the connection's completion callback with its status, the response and the response size. */
static inline void NHTTPi_callCompleteCallback(NHTTPMutexInfo* mutex, NHTTPConnection* connection) {
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
    case 1:
        result = NHTTPi_callSendCallback(mutex, connection, args);
        break;
    case 2:
        NHTTPi_callBufferFullCallback(mutex, connection, args);
        result = 0;
        break;
    case 4:
        NHTTPi_callCompleteCallback(mutex, connection);
        result = 0;
        break;
    }
    return result;
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
    NHTTPRequest* request = NULL;

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
            request = NULL;
        }
    }
    return request;
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
        printf("%s can be called before NHTTPStartConnection()\n", __FUNCTION__);
        return -1;
    }
    return NHTTP_AddHeaderField(request, bgnend, token, value) ? 0 : -1;
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
        printf("%s can be called before NHTTPStartConnection()\n", __FUNCTION__);
        return -1;
    }
    return NHTTP_AddPostDataAscii(request, bgnend, token, value) ? 0 : -1;
}

/* Adds one binary post field (sent as an octet-stream part).  The linker strips it: only its pooled name
 * is in retail, so the body past the guard is a GUESS. */
s32 NHTTPAddPostDataBinary(NHTTPRequest* handle, const char* token, const char* value, u32 length) {
    NHTTPInfo* info = NHTTPi_GetSystemInfoP();
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(info);
    NHTTPRequest* request = NHTTPi_GetRequest(mutex, (NHTTPConnection*)handle);
    NHTTPInfo* bgnend = NHTTPi_GetBgnEndInfoP(info);

    if (request == NULL) {
        return -1;
    }
    if (request->field_0x04 != 0) {
        printf("%s can be called before NHTTPStartConnection()\n", __FUNCTION__);
        return -1;
    }
    if (NHTTPi_SetHeaderField(&request->postDataList, bgnend, token, value) == 0) {
        return -1;
    }
    request->postDataList->next->field_0x10 = length;
    request->postDataList->next->field_0x14 = 1;
    return 0;
}

/* Sets the whole post body as one raw block, refused once a post field was added.  The linker strips it:
 * only its pooled name and warning are in retail, so the body past the two guards is a GUESS. */
s32 NHTTPAddPostDataRaw(NHTTPRequest* handle, char* data, s32 length) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHTTPRequest* request = NHTTPi_GetRequest(mutex, (NHTTPConnection*)handle);

    if (request == NULL) {
        return -1;
    }
    if (request->field_0x04 != 0) {
        printf("%s can be called before NHTTPStartConnection()\n", __FUNCTION__);
        return -1;
    }
    if (request->postDataList != NULL) {
        printf("already called NHTTPAddPostDataAscii or NHTTPAddPostDataBinary (exclusive fucntion)\n");
        return -1;
    }
    request->field_0x10 = 1;
    request->rawBody = data;
    request->rawBodyLength = length;
    return 0;
}

/* Chooses how the post fields are encoded (`postType`).  The linker strips it: only its pooled name is in
 * retail, so the body past the guard is a GUESS. */
s32 NHTTPSetPostDataEncoding(NHTTPRequest* handle, s32 type) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHTTPRequest* request = NHTTPi_GetRequest(mutex, (NHTTPConnection*)handle);

    if (request == NULL) {
        return -1;
    }
    if (request->field_0x04 != 0) {
        printf("%s can be called before NHTTPStartConnection()\n", __FUNCTION__);
        return -1;
    }
    request->postType = type;
    return 0;
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

/* Cancel request `id`; answers 0 when it was found and -1 when not. */
s32 NHTTPCancelRequest(s32 id) {
    return NHTTPi_cancelRequestById(NHTTPi_GetSystemInfoP(), id) ? 0 : -1;
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
        NHTTPi_memcpy(&credentials[userLength + 1], password, passwordLength);
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
        OSReport("Using proxy server %s:%d (%s/%s).\n", proxy->host, proxy->port,
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

/* Clears SSL verification options of the request.  The linker strips it: only its pooled name is in
 * retail, so the body past the guard is a GUESS. */
s32 NHTTPDisableVerifyOptionForDebug(NHTTPRequest* handle, u32 option) {
    NHTTPMutexInfo* mutex = NHTTPi_GetMutexInfoP(NHTTPi_GetSystemInfoP());
    NHTTPRequest* request = NHTTPi_GetRequest(mutex, (NHTTPConnection*)handle);

    if (request == NULL) {
        return -1;
    }
    if (request->field_0x04 != 0) {
        printf("%s can be called before NHTTPStartConnection()\n", __FUNCTION__);
        return -1;
    }
    request->verifyOption &= ~option;
    return 0;
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
