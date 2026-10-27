/*
 * NHTTP/NHTTP_request.h - the calls and data `src/NHTTP/NHTTP_request.c` defines (.text 0x80515774..0x80516CA0):
 * the request object, the response's destroy and header lookup, and the socket layer.  The types are the
 * library's shared ones in `NHTTP/d_nhttp.h`.
 */
#ifndef MHTRI_NHTTP_NHTTP_REQUEST_H
#define MHTRI_NHTTP_NHTTP_REQUEST_H

#include "types.h"
#include "NHTTP/d_nhttp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80574CE8 (0x15 B, `.rodata`) - the 19-character code `NHTTP_AddPostDataAscii` walks its request's
 * 18-byte code field up to, one character at a time, re-submitting after each step. */
extern const char NHTTPi_postDataRawCode[];

/* The request and response objects (0x80515774..0x805161A4) and the public API over them. */
struct NHTTPRequest* NHTTPi_createRequestObject(NHTTPInfo* info, const char* url, s32 method, u32 buffer,
                                                u32 bufferSize, u32 userData,
                                                NHTTPBufferFullCallback bufferFullCallback,
                                                NHTTPFreeBufferCallback freeCallback);
void NHTTP_DestroyRequest(NHTTPInfo* info, struct NHTTPRequest* request);
s32 NHTTPi_destroyRequestObject(NHTTPMutexInfo* mutex, s32 request);
s32 NHTTP_SendRequestAsync(NHTTPInfo* info, struct NHTTPRequest* request);
s32 NHTTPi_cancelRequestById(NHTTPInfo* info, s32 id);
void NHTTPi_cancelAllRequests(NHTTPInfo* info);
void NHTTP_DestroyResponse(NHTTPMutexInfo* mutex, NHTTPResponse* response);
struct NHTTPRecvBuf;
s32 NHTTPi_findHeaderField(struct NHTTPRecvBuf* ring, const char* name, s32* offset);

/* The socket layer (0x805163A4..0x80516CA0) - the raw SO wrappers with the request's SSL handle in
 * front: a send/receive goes through SSL when the request holds a handle, and the error mapping is
 * shared by both directions. */
s32 NHTTPi_probeSocket(s32 fd);
s32 NHTTPi_openSocket(struct NHTTPRequest* request);
s32 NHTTPi_SocClose(NHTTPMutexInfo* mutex, struct NHTTPRequest* request, s32 fd);
s32 NHTTPi_SocConnect(NHTTPInfo* info, NHTTPMutexInfo* mutex, struct NHTTPRequest* request, s32 fd,
                      u32 address, u32 port);
s32 NHTTPi_SocSSLConnect(NHTTPInfo* info, NHTTPMutexInfo* mutex, struct NHTTPRequest* request, s32 fd);
s32 NHTTPi_SocRecv_sub(NHTTPConnection* connection, s32 fd, u8* buf, s32 length, s32 flags);
/* The raw socket receive the RVL wrappers end in.  The sibling `NHTTP_os_RVL.c` declared the six
 * parameters as (handle, conn, flags, ...); the body reads them as the request-list mutex, the
 * REQUEST (its +0xAC SSL handle and +0x00 cancel flag), the socket, the buffer, its length and the
 * receive flags, so `conn` is a request and `handle` a mutex there (a shared-file request). */
s32 NHTTPi_SocRecv(s32 handle, NHTTPConnection* conn, s32 flags, u8* buf, s32 length, s32 arg);
s32 NHTTPi_SocSend_sub(s32 fd, u8* buf, s32 length, s32 flags);
s32 NHTTPi_SocSend(struct NHTTPRequest* request, s32 fd, u8* buf, s32 length, s32 flags);
void NHTTPi_SocShutdown(NHTTPMutexInfo* mutex, struct NHTTPRequest* request, s32 fd);
u32 NHTTPi_resolveHostName(struct NHTTPRequest* request, const char* name);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NHTTP_NHTTP_REQUEST_H */
