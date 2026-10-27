/*
 * NHTTP/NHTTP_thread.h - the calls `src/NHTTP/NHTTP_thread.c` defines (.text 0x80517614..0x8051A1B8): the comm
 * thread's ready flag, the request writer, the response reader and the loop.  The types are the library's
 * shared ones in `NHTTP/d_nhttp.h`.
 */
#ifndef MHTRI_NHTTP_NHTTP_THREAD_H
#define MHTRI_NHTTP_NHTTP_THREAD_H

#include "types.h"
#include "NHTTP/d_nhttp.h"

#ifdef __cplusplus
extern "C" {
#endif

void NHTTPi_InitThreadInfo(NHTTPThreadInfo* info);
void NHTTPi_markCommThreadReady(NHTTPThreadInfo* info);
s32 NHTTPi_isCommThreadReady(NHTTPThreadInfo* info);
s32 NHTTPi_isHeaderEnd(const char* window, s32 count);

/* The request writer (0x805176FC..0x80518970). */
s32 NHTTPi_SaveBuf(struct NHTTPRequest* request, u8* buffer, s32 fd, s32* used, u8* data, s32 length);
s32 NHTTPi_queryPostFieldSize(NHTTPMutexInfo* mutex, struct NHTTPRequest* request, const char* key, s32* total,
                              s32 mode);
s32 NHTTPi_sendPostField(NHTTPMutexInfo* mutex, struct NHTTPRequest* request, u8* buffer, const char* key,
                         s32 fd, s32* used, s32 mode);
s32 NHTTPi_ensureRecvSpace(NHTTPMutexInfo* mutex, NHTTPResponse* response);
s32 NHTTPi_appendSendData(NHTTPCommContext* context, const char* data, s32 length);

/* The comm thread's steps (0x80517BE8..0x80519FB4): each takes the working state the loop keeps. */
s32 NHTTPi_sendProxyConnect(NHTTPCommContext* context);
s32 NHTTPi_recvProxyConnectReply(NHTTPCommContext* context);
s32 NHTTPi_sendHeaderFields(NHTTPCommContext* context);
s32 NHTTPi_sendRawPostBody(NHTTPCommContext* context);
s32 NHTTPi_sendMultipartBody(NHTTPCommContext* context);
s32 NHTTPi_sendUrlEncodedBody(NHTTPCommContext* context);
void NHTTPi_finishRequest(NHTTPCommContext* context);
s32 NHTTPi_dequeueRequest(NHTTPCommContext* context);
s32 NHTTPi_prepareTarget(NHTTPCommContext* context);
s32 NHTTPi_connectSocket(NHTTPCommContext* context);
s32 NHTTPi_negotiateSSL(NHTTPCommContext* context);
s32 NHTTPi_sendRequest(NHTTPCommContext* context);
s32 NHTTPi_recvResponseHeaders(NHTTPCommContext* context);
s32 NHTTPi_parseResponseHeaders(NHTTPCommContext* context);
s32 NHTTPi_recvResponseBody(NHTTPCommContext* context);
void NHTTPi_commThreadLoop(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NHTTP_NHTTP_THREAD_H */
