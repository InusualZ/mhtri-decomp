/*
 * NHTTP/NHTTP_list.h - the calls and data `src/NHTTP/NHTTP_list.c` defines (.text 0x8051B05C..0x8051B7FC): the
 * completion notice, the connection list, the three request/response callbacks and the system-info
 * singleton.  The types are the library's shared ones in `NHTTP/d_nhttp.h`.
 */
#ifndef MHTRI_NHTTP_NHTTP_LIST_H
#define MHTRI_NHTTP_NHTTP_LIST_H

#include "types.h"
#include "NHTTP/d_nhttp.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The data this unit owns: the `.sbss` words 0x80795880/0x80795884 and the `.bss` run 0x80762C20..0x80763900. */
extern NHTTPConnection* NHTTPi_connectionListHead;   /* 0x80795880: the connection list's head */
extern NHTTPInfo* NHTTPi_systemInfoP;                /* 0x80795884: the lazily-set info-block slot */
extern NHTTPiCompletionSync NHTTPi_completionSync;   /* 0x80762C20: the completion mutex/queue */
extern struct NHTTPInfo NHTTPi_systemInfo;           /* 0x80762C60: the block `NHTTPi_systemInfoP` points at */

void NHTTPi_NotifyCompletion(NHTTPConnection* connection);

/* The connection-list API (0x8051B0D8..0x8051B378).  The list is one singly-linked chain of
 * `NHTTPConnection` records, and the five `mode`s below say what an operation does to it.
 *
 * `NHTTPi_ControlConnectionList` is the one body that walks it; every other entry point is a
 * one-call wrapper over it.  The key it looks a record up by is the connection itself in the get,
 * add and omit modes and the request/response record the connection holds in the two lookup modes,
 * so it stays an opaque handle here (`NHTTPi_Request2Connection` is handed the request list's own
 * `s32` handle and the `NHTTPRequestNode.request` field it is read out of).
 */
#define NHTTPI_LIST_GET         0  /* find the connection `key` is */
#define NHTTPI_LIST_BY_REQUEST  1  /* find the connection whose request is `key` */
#define NHTTPI_LIST_BY_RESPONSE 2  /* find the connection whose response is `key` */
#define NHTTPI_LIST_ADD         3  /* put the connection at the head of the list */
#define NHTTPI_LIST_OMIT        4  /* take the connection out of the list */

/* untyped: opaque handle - the connection itself, or the request/response it holds */
NHTTPConnection* NHTTPi_ControlConnectionList(NHTTPMutexInfo* mutex, void* key, s32 mode);
s32 NHTTPi_AddConnection(NHTTPMutexInfo* mutex, NHTTPConnection* connection);
s32 NHTTPi_OmitConnectionList(NHTTPMutexInfo* mutex, NHTTPConnection* connection);
struct NHTTPRequest* NHTTPi_Connection2Request(NHTTPMutexInfo* mutex, NHTTPConnection* connection);
NHTTPResponse* NHTTPi_Connection2Response(NHTTPMutexInfo* mutex, NHTTPConnection* connection);
NHTTPConnection* NHTTPi_Request2Connection(NHTTPMutexInfo* mutex, s32 request);
NHTTPConnection* NHTTPi_Response2Connection(NHTTPMutexInfo* mutex, s32 response);
NHTTPConnection* NHTTPi_GetConnection(NHTTPMutexInfo* mutex, NHTTPConnection* connection);
/* The two get modes look the connection up by itself, so both take it typed; only the *return* is
 * untyped - it is the record the connection holds, or that same key handed back when the lookup
 * misses (the miss signal). */
/* untyped: opaque handle - the record the connection holds, or the key handed back on a miss */
void* NHTTPi_GetRequest(NHTTPMutexInfo* mutex, NHTTPConnection* key);
/* untyped: opaque handle - the record the connection holds, or the key handed back on a miss */
void* NHTTPi_GetResponse(NHTTPMutexInfo* mutex, NHTTPConnection* key);
s32 NHTTPi_GetConnectionListLength(void);

/* The three request/response callbacks `NHTTPCreateConnection` can install (phases 1, 2/3 and 4).
 * `phase` 3's body is the map's `NHTTPi_RecvCallback` and is named for the receive half it reports. */
s32 NHTTPi_PostSendCallback(NHTTPMutexInfo* mutex, NHTTPConnection* connection, u32 arg1, u32 arg2);
void NHTTPi_BufferFullCallback(NHTTPMutexInfo* mutex, NHTTPConnection* connection);
void NHTTPi_RecvCallback(NHTTPMutexInfo* mutex, NHTTPConnection* connection);
void NHTTPi_CompleteCallback(NHTTPMutexInfo* mutex, NHTTPConnection* connection);

NHTTPInfo* NHTTPi_GetSystemInfoP(void);
void* NHTTPi_GetBgnEndInfoP(NHTTPInfo* info); /* untyped: opaque handle */
void* NHTTPi_GetListInfoP(NHTTPInfo* info); /* untyped: opaque handle */
void* NHTTPi_GetReqInfoP(NHTTPInfo* info); /* untyped: opaque handle */
void* NHTTPi_GetThreadInfoP(NHTTPInfo* info); /* untyped: opaque handle */
void* NHTTPi_GetMutexInfoP(NHTTPInfo* info); /* untyped: opaque handle */
void NHTTPi_SetSock(NHTTPConnection* connection, NHTTPSock* sock);
NHTTPSock* NHTTPi_GetSock(NHTTPConnection* connection);
void NHTTPi_InitConnectionList(void);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_NHTTP_NHTTP_LIST_H */
