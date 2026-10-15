/*
 * DWCi/gt2Socket.c - the GameSpy GT2 socket core: the connection table of a GT2 socket (hash, compare, lookup),
 * socket and connection creation and teardown, the raw send and the per-socket connection timers.
 *
 * RANGE. `.text` 0x80511620..0x80512200 (14 functions); no data of its own. The left edge ends the GT2 message
 *   file (`DWCi/gt2Message.c`, whose last three functions address its `.sdata` `DWCi_protocolMagic`); the right edge
 *   is gt2Utility's first function (`DWCi/gt2Utility.c`). The GT2 files link in name order (`DWCi/gt2Auth.c` ..
 *   `DWCi/gt2Message.c`, then gt2Socket, gt2Utility).
 * FLAGS. `cflags_base` (-func_align 16): the retail functions start 16-byte aligned; per-row scores are unaffected
 *   by the lib's `cflags_dwc` (-func_align 4), the object layout is not.
 * NAMES. GUESSes from the bodies, kept from before the GT2 identification (`DWCi_createConnection` =
 *   gti2CreateSocket, `DWCi_createRequest` = gti2NewSocketConnection, ...). The GT2 records (`DWCiConn` = GTI2Socket,
 *   `DWCiReq` = GTI2Connection, `GTI2Buffer`) live in `DWCi/dwc_nasfunc.h` with the SDK's field names. The file name
 *   is the GameSpy SDK's `gt2Socket.c` (no `__FILE__` string).
 * RESIDUALS. none measured (every row matches).
 * SHAPES. The connection lookup keys on a whole `DWCiReq` on the stack (retail's 0xB0 frame) and `DWCi_findRequest`
 *   / `DWCi_removeRequest` are defined ahead of their callers, which expand them (`DWCi_createRequest` twice,
 *   `DWCi_sendTo`, `DWCi_requestTick`, `DWCi_connectionFlushRequests`); `DWCi_createConnection` returns the GT2
 *   result code and writes the socket only on success; the send error dispatch is an if/else chain ending in a
 *   shared `return 1`;
 *   `DWCi_removeRequest` leaves through one `||` early return (retail's `beq` / `bne` / `b` exit).
 */

#include "types.h"
#include "DWCi/natneg.h"            /* DWCi_GetStringLength: the owner's header (rule 2) */
#include "DWCi/gt2Socket.h"              /* this unit's own exported entry points */
#include "DWCi/gt2Utility.h"             /* DWCi_parseAddress */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/DWCi.h"                /* the band's unowned data and helpers (rule 2) */
#include "MSL_C/alloc.h"
#include "unsplit/SO.h"                  /* SOAddressToString / SOAddressToHostPort / SOHtoNs */
#include "SO/soi.h"
#include "unsplit/Runtime.PPCEABI.H.h"   /* sprintf / strchr / strlen / atoi */

/* The unit's own entry points, in address order. */
u32 DWCi_requestTableHash(DWCiAddrKey** key, u32 bucket);
u32 DWCi_requestTableCompare(DWCiAddrKey** a, DWCiAddrKey** b);
void DWCi_requestListRelease(DWCiReq** slot);
u32 DWCi_findRequest(DWCiConn* conn, u32 addr, u16 port);
s32 DWCi_createConnection(DWCiConn** out, char* host, u32 bufSize1, u32 bufSize2, u32 arg5, s32 mode);
void DWCi_destroyConnection(DWCiConn* conn);
void DWCi_setConnectionUserValue(DWCiConn* conn, u32 value);
void DWCi_removeRequest(DWCiReq* req);
u32 DWCi_sendTo(DWCiConn* conn, u32 addr, u16 port, void* buf, int len);
u32 DWCi_requestTick(void** slot, u32* now);
u32 DWCi_connectionTick(DWCiConn* conn);
void DWCi_connectionFlushRequests(DWCiConn* conn);
void DWCi_connectionShutdown(DWCiConn* conn);

/* 0x80511620 - the request table's hash: (key.addr * key.port) modulo the bucket count. */
u32 DWCi_requestTableHash(DWCiAddrKey** key, u32 bucket)
{
    DWCiAddrKey* k = *key;

    return (k->addr * k->port) % bucket;
}

/* 0x80511640 - the request table's ordering: the address difference, or the signed port difference
 * when the addresses are equal. */
u32 DWCi_requestTableCompare(DWCiAddrKey** a, DWCiAddrKey** b)
{
    DWCiAddrKey* ka = *a;
    DWCiAddrKey* kb = *b;

    if (ka->addr != kb->addr) {
        return ka->addr - kb->addr;
    }
    return (s16)(ka->port - kb->port);
}

/* 0x80511680 - the request list's "release one slot" hook. */
void DWCi_requestListRelease(DWCiReq** slot)
{
    DWCi_requestFree(*slot);
}

/* 0x80511690 - look a request up by `{addr, port}`. */
u32 DWCi_findRequest(DWCiConn* conn, u32 addr, u16 port)
{
    DWCiReq key;
    DWCiReq* pkey;
    DWCiReq** slot;

    key.addr = addr;
    key.port = port;
    pkey = &key;
    slot = TableLookup(conn->connections, &pkey);
    if (slot != 0) {
        return (u32)*slot;
    }
    return 0;
}

/* 0x805116E0 - build a connection around `host`: parse the address, allocate the 0x4C-byte record,
 * its request table and list, open the transport handle, then connect (or, in mode 3, only stamp
 * the address). */
s32 DWCi_createConnection(DWCiConn** out, char* host, u32 bufSize1, u32 bufSize2, u32 arg5, s32 mode)
{
    DWCiConn* conn;
    DWCiSockAddrIn sa;
    u32 addr;
    u16 port;
    int salen;

    SocketStartUp();
    if (bufSize2 == 0) {
        bufSize2 = 0x10000;
    }
    if (bufSize1 == 0) {
        bufSize1 = 0x10000;
    }
    if (DWCi_parseAddress(host, &addr, &port) == 0) {
        return 4;
    }
    conn = gsimalloc(0x4C);
    if (conn == 0) {
        return 1;
    }
    memset(conn, 0, 0x4C);
    conn->sock = -1;
    conn->incomingBufferSize = bufSize2;
    conn->outgoingBufferSize = bufSize1;
    conn->socketErrorCallback = arg5;
    conn->connections = TableNew2(4, 32, 2, (TableHashFn)DWCi_requestTableHash, (TableCompareFn)DWCi_requestTableCompare, NULL);
    if (conn->connections == 0) {
        gsifree(conn);
        return 1;
    }
    conn->closedConnections = ArrayNew(4, 4, (ArrayElementFreeFn)DWCi_requestListRelease);
    if (conn->closedConnections == 0) {
        TableFree(conn->connections);
        gsifree(conn);
        return 1;
    }
    conn->sock = DWCi_socketCreate(2, 2, 17);
    conn->protocolType = mode;
    if (mode == 3) {
        conn->protocolOffset = 0;
    } else {
        conn->protocolOffset = mode;
    }
    if (conn->sock == -1) {
        TableFree(conn->connections);
        ArrayFree(conn->closedConnections);
        gsifree(conn);
        return 3;
    }
    memset(&sa, 0, 8);
    sa.family = 2;
    sa.addr = addr;
    sa.port = SOHtoNs(port);
    if (mode != 3) {
        if (DWCi_socketBind(conn->sock, &sa, 8) == -1) {
            DWCi_socketClose(conn->sock);
            TableFree(conn->connections);
            ArrayFree(conn->closedConnections);
            gsifree(conn);
            return 3;
        }
    }
    salen = 8;
    DWCi_socketGetLocalName(conn->sock, &sa, &salen);
    conn->addr = sa.addr;
    conn->port = SOAddressToHostPort(sa.port);
    *out = conn;
    return 0;
}

/* 0x80511920 - close the connection object itself: mark "shutdown while busy", or release the
 * transport handle, the table, the list and the record. */
void DWCi_destroyConnection(DWCiConn* conn)
{
    if (conn->callbackLevel != 0) {
        conn->close = 1;
        return;
    }
    DWCi_socketClose(conn->sock);
    TableFree(conn->connections);
    ArrayFree(conn->closedConnections);
    gsifree(conn);
    SocketShutDown();
}

/* 0x80511990 - store the connection's user value. */
void DWCi_setConnectionUserValue(DWCiConn* conn, u32 value)
{
    conn->connectAttemptCallback = value;
}

u32 DWCi_createRequest(DWCiConn* conn, DWCiReq** out, u32 addr, u16 port)
{
    DWCiReq* req = NULL;

    if (DWCi_findRequest(conn, addr, port) != 0) {
        return 5;
    }
    req = gsimalloc(0xA0);
    if (req != 0) {
        memset(req, 0, 0xA0);
        req->addr = addr;
        req->port = port;
        req->conn = conn;
        req->startTime = current_time();
        req->lastSend = req->startTime;
        req->serialNumber = 0;
        req->expectedSerialNumber = 0;
        if (DWCi_bufferAlloc(&req->incomingBuffer, conn->incomingBufferSize) != 0) {
            if (DWCi_bufferAlloc(&req->outgoingBuffer, conn->outgoingBufferSize) != 0) {
                req->incomingBufferMessages = ArrayNew(16, 64, 0);
                if (req->incomingBufferMessages != 0) {
                    req->outgoingBufferMessages = ArrayNew(16, 64, 0);
                    if (req->outgoingBufferMessages != 0) {
                        req->sendFilters = ArrayNew(4, 2, 0);
                        if (req->sendFilters != 0) {
                            req->receiveFilters = ArrayNew(4, 2, 0);
                            if (req->receiveFilters != 0) {
                                TableEnter(conn->connections, &req);
                                *out = (DWCiReq*)DWCi_findRequest(conn, addr, port);
                                if (*out != 0) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    if (req != NULL) {
        gsifree(req->incomingBuffer.buffer);
        gsifree(req->outgoingBuffer.buffer);
        if (req->incomingBufferMessages != 0) {
            ArrayFree(req->incomingBufferMessages);
        }
        if (req->outgoingBufferMessages != 0) {
            ArrayFree(req->outgoingBufferMessages);
        }
        if (req->sendFilters != 0) {
            ArrayFree(req->sendFilters);
        }
        if (req->receiveFilters != 0) {
            ArrayFree(req->receiveFilters);
        }
        gsifree(req);
    }
    return 1;
}

/* 0x80511C20 - drop a request from its owner: nothing to do while it is still flagged busy, else
 * remove it from the request list (state 7) or from the request table, by its own key. */
void DWCi_removeRequest(DWCiReq* req)
{
    int count;
    int i;

    if (req->freeAtAcceptReject != 0 || req->callbackLevel != 0) {
        return;
    }
    if (req->state == 7) {
        count = ArrayLength(req->conn->closedConnections);
        for (i = 0; i < count; i++) {
            if (req == *(DWCiReq**)ArrayNth(req->conn->closedConnections, i)) {
                ArrayDeleteAt(req->conn->closedConnections, i);
                return;
            }
        }
    } else {
        TableRemove(req->conn->connections, &req);
    }
}

/* 0x80511CF0 - the send path: default the buffer/length pair, refuse while the transport is not
 * usable, address the peer, hand the frame to the socket layer and dispatch the socket error to the
 * reconnect/retry entry points. */
u32 DWCi_sendTo(DWCiConn* conn, u32 addr, u16 port, void* buf, int len)
{
    DWCiSockAddrIn sa;
    int err;

    DWCi_GetStringLength(&buf, &len);
    if (conn->protocolType != 3 && DWCi_socketIsUsable(conn->sock) == 0) {
        return 1;
    }
    memset(&sa, 0, 8);
    sa.family = 2;
    sa.addr = addr;
    sa.port = SOHtoNs(port);
    if (DWCi_socketSendTo(conn->sock, buf, len, 0, &sa, 8) == -1) {
        err = DWCi_socketGetLastError(conn->sock);
        if (err == -15) {
            if (DWCi_requestReconnect(conn, addr, port) == 0) {
                return 0;
            }
        } else if (err == -23) {
            if (DWCi_requestRetry(conn, addr, port, 1) == 0) {
                return 0;
            }
        } else {
            if (err == -42 || err == -6) {
                return 1;
            }
            if (err != -35) {
                if (conn->error == 0) {
                    conn->error = 1;
                    DWCi_socketShutdown(conn);
                    if (DWCi_socketConnect(conn) != 0) {
                        if (conn->callbackLevel != 0) {
                            conn->close = 1;
                        } else {
                            DWCi_socketClose(conn->sock);
                            TableFree(conn->connections);
                            ArrayFree(conn->closedConnections);
                            gsifree(conn);
                            SocketShutDown();
                        }
                    }
                }
                return 0;
            }
        }
    } else if (conn->sendDumpCallback != 0) {
        if (DWCi_requestSend(conn, (DWCiReq*)DWCi_findRequest(conn, addr, port), addr, port, 0, buf, len, 1) == 0) {
            return 0;
        }
    }
    return 1;
}

/* 0x80511F10 - the table walk's per-request step: skip a request that is neither terminal nor
 * timed out, then drop the terminal ones through the same remove path DWCi_removeRequest uses. */
u32 DWCi_requestTick(void** slot, u32* now)
{
    DWCiReq* req = *slot;
    u32 time = *now;

    if (req->state != 7) {
        if (DWCi_requestIsTimedOut(req, time) == 0) {
            return 0;
        }
    }
    if (req->state == 7 && req->freeAtAcceptReject == 0 && req->callbackLevel == 0) {
        DWCi_removeRequest(req);
    }
    return 1;
}

/* 0x80512030 - the periodic walk: hand the current tick to the table's per-request step. */
u32 DWCi_connectionTick(DWCiConn* conn)
{
    u32 now;

    now = current_time();
    return TableMapSafe2(conn->connections, (TableMapFn2)DWCi_requestTick, &now) == 0;
}

/* 0x80512080 - the teardown walk: every request of the owner's list that is terminal is removed
 * from itself, back to front. */
void DWCi_connectionFlushRequests(DWCiConn* conn)
{
    int i;

    for (i = ArrayLength(conn->closedConnections) - 1; i >= 0; i--) {
        DWCi_removeRequest(*(DWCiReq**)ArrayNth(conn->closedConnections, i));
    }
}

/* 0x80512170 - the single-shot teardown: run the transport's shutdown hooks once, then either mark
 * "shutdown while busy" or release the connection. */
void DWCi_connectionShutdown(DWCiConn* conn)
{
    if (conn->error != 0) {
        return;
    }
    conn->error = 1;
    DWCi_socketShutdown(conn);
    if (DWCi_socketConnect(conn) == 0) {
        return;
    }
    if (conn->callbackLevel != 0) {
        conn->close = 1;
        return;
    }
    DWCi_socketClose(conn->sock);
    TableFree(conn->connections);
    ArrayFree(conn->closedConnections);
    gsifree(conn);
    SocketShutDown();
}
