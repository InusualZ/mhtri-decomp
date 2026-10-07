/*
 * DWCi/fn_805113B0.c - the GameSpy GT2 socket core: the closed / resend / send message tail, the connection table
 * of a GT2 socket (hash, compare, lookup), socket and connection creation and teardown, the raw send, the
 * per-socket connection timers, and the address text helpers.
 *
 * RANGE. `.text` 0x805113B0..0x80512490 (20 functions); `.data` 0x806308A8..0x806308E8 (the stats host buffer);
 *   `.sdata` 0x80794358..0x80794368; `.sbss` 0x80795820..0x80795828.
 * FLAGS. the `DWCi` lib's `cflags_dwc` (the retail functions are 16-byte aligned; per-row scores are unaffected).
 * NAMES. GUESSes from the bodies, kept from before the GT2 identification (`DWCi_sendControlFrame` = gti2SendClosed,
 *   `DWCi_appendTransfer` = gti2ResendMessage, `DWCi_flushRequest` = gti2Send, `DWCi_createConnection` =
 *   gti2CreateSocket, `DWCi_createRequest` = gti2NewSocketConnection, ...). The GT2 records (`DWCiConn` = GTI2Socket,
 *   `DWCiReq` = GTI2Connection, `GTI2Buffer`) live in `DWCi/dwc_nasfunc.h` with the SDK's field names.
 * SHAPES. `DWCi_parseAddress` keeps nested ifs where retail jumps to shared exits (rule 8): 87.10 against an
 *   earlier flat shape's 82.30.
 * RESIDUALS. `DWCi_createRequest`/`DWCi_findRequest` keep smaller frames than retail (0x30/0x20 against 0x170/0xB0);
 *   the other open rows differ in register colouring and block order; `DWCi_appendTransfer` writes the header bytes
 *   with `srawi`+`stb` where retail uses `extrwi`+`stbx`. The first three rows are the tail of the GT2 message file
 *   that `DWCi/dwc_nasfunc.c` ends with (request net-c#13).
 *   `DWCi_createRequest` keeps no saved-register frame (retail `_savegpr_27`); `DWCi_parseAddress` saves from r27
 *   (retail r26). Flip blockers: `splits.txt` claims `.sbss` (0x8) and `.sdata` (0x10) the object does not emit.
 *   `DWCi_createRequest`: retail `_savegpr_27`/`_restgpr_27`, ours no frame; `DWCi_parseAddress`: ours
 *   `_savegpr_27`/`_restgpr_27`, retail `_savegpr_26`/`_restgpr_26`.
 */

#include "types.h"
#include "DWCi/DWCi_NatNeg.h"            /* DWCi_GetStringLength: the owner's header (rule 2) */
#include "DWCi/fn_805113B0.h"            /* this unit's own exported entry points */
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/DWCi.h"                /* the band's unowned data and helpers (rule 2) */
#include "MSL_C/alloc.h"
#include "unsplit/SO.h"                  /* SOAddressToString / SOAddressToHostPort / SOHtoNs */
#include "SO/soi.h"
#include "unsplit/Runtime.PPCEABI.H.h"   /* sprintf / strchr / strlen / atoi */

/* The GameSpy stats server host name, the unit's `.data` run; `DWC_Init` rewrites it for the server type. */
char DWCi_statsServerHostname[64] = "gamestats.gs.nintendowifi.net";

/* --------------------------------------------------------------------------------------------- */
/* Types                                                                                          */
/* --------------------------------------------------------------------------------------------- */

 /* size: 0x10 */

/* --------------------------------------------------------------------------------------------- */
/* The band data this unit loads.  It is owned by nobody and the registered ranges bracketing it   */
/* name different modules, so it is declared in `unsplit/DWCi.h` (rule 2) and never        */
/* defined here (playbook 29) - apart from `DWCi_addressRing`, which the NATNEG unit's `.bss` run   */
/* covers and whose declaration therefore lives in `DWCi/DWCi_NatNeg.h`.  The private       */
/* records those declarations reach stay below, next to the code that uses them.                   */
/* --------------------------------------------------------------------------------------------- */

/* The character-class record DWCi_parseAddress validates the port digits against: a header whose +0x38
 * pointer reaches the per-character u16 flags, bit 3 marking a decimal digit.  Declared, never
 * defined - the `.data` run belongs to the split (playbook 29). */
typedef struct DWCiCharClass {
    /* +0x00 */ u8 pad_0x00[0x8];
    /* +0x08 */ u16 bits[0x80];
} DWCiCharClass; /* size: 0x108 */

typedef struct DWCiCType {
    /* +0x00 */ u8 pad_0x00[0x38];
    /* +0x38 */ DWCiCharClass* cls;
    /* +0x3C */ u8 pad_0x3c[0xC];
} DWCiCType; /* size: 0x48 */



/* The unit's own entry points, in address order. */
u32 DWCi_sendControlFrame(DWCiReq* req);
u32 DWCi_appendTransfer(DWCiReq* req, DWCiXfer* xfer);
u32 DWCi_flushRequest(DWCiReq* req, u32 a, u32 b, u32 flag);
u32 DWCi_requestTableHash(DWCiAddrKey** key, u32 bucket);
u32 DWCi_requestTableCompare(DWCiAddrKey** a, DWCiAddrKey** b);
void DWCi_requestListRelease(DWCiReq** slot);
u32 DWCi_findRequest(DWCiConn* conn, u32 addr, u16 port);
void DWCi_createConnection(DWCiConn** out, char* host, u32 bufSize1, u32 bufSize2, u32 arg5, u32 mode);
void DWCi_destroyConnection(DWCiConn* conn);
void DWCi_setConnectionUserValue(DWCiConn* conn, u32 value);
void DWCi_removeRequest(DWCiReq* req);
u32 DWCi_sendTo(DWCiConn* conn, u32 addr, u16 port, void* buf, int len);
u32 DWCi_requestTick(void** slot, u32* now);
u32 DWCi_connectionTick(DWCiConn* conn);
void DWCi_connectionFlushRequests(DWCiConn* conn);
void DWCi_connectionShutdown(DWCiConn* conn);
u16 DWCi_htons(u16 port);
char* DWCi_formatAddress(u32 addr, u16 port, char* buf);
u32 DWCi_parseAddress(char* str, u32* outAddr, u16* outPort);

/* 0x805113B0 - the handshake write: { 0x0003 } when the connection is in mode 2, then the shared
 * 0xFEFE constant and the 0x68 terminator, handed to the send path. */
u32 DWCi_sendControlFrame(DWCiReq* req)
{
    DWCiConn* conn;
    u16 port;
    u32 addr;
    u8 buf[32];
    u16 word;
    int n;

    conn = req->conn;
    port = req->port;
    addr = req->addr;
    n = 0;
    if (conn->protocolType == 2) {
        word = 3;
        memcpy(buf, &word, 2);
        n = 2;
    }
    memcpy(buf + n, &DWCi_protocolMagic, 2);
    n += 2;
    buf[n] = 0x68;
    return DWCi_sendTo(conn, addr, port, buf, n + 1) != 0;
}

/* 0x80511470 - the send-buffer append: stamp the two header bytes at the connection's header offset,
 * hand the payload to the framer, then stamp the transfer's timestamp and, for a mode-2 request,
 * the "stamped" mark. */
u32 DWCi_appendTransfer(DWCiReq* req, DWCiXfer* xfer)
{
    DWCiConn* conn;
    u16 value;
    u8* buf;
    u32 off;

    conn = req->conn;
    value = req->expectedSerialNumber;
    buf = req->outgoingBuffer.buffer;
    off = xfer->offset + conn->protocolOffset;
    buf[off + 5] = (u8)(value >> 8);
    buf[off + 6] = (u8)value;
    if (DWCi_requestFrame(req, req->outgoingBuffer.buffer + xfer->offset, xfer->length) == 0) {
        return 0;
    }
    xfer->stamp = req->lastSend;
    if (req->outgoingBuffer.buffer[xfer->offset + req->conn->protocolOffset + 2] == 2) {
        req->challengeTime = req->lastSend;
    }
    return 1;
}

/* 0x80511530 - the flush path: when the flag is clear, hand the request to DWCi_requestFlush; otherwise
 * look the block up by the size byte, and if it is not already complete, append it and re-frame the
 * last queue entry. */
u32 DWCi_flushRequest(DWCiReq* req, u32 a, u32 b, u32 flag)
{
    u32 out;
    u32 count;
    u32* item;
    u32 ret;

    if (flag != 0) {
        if (DWCi_requestBlockState(req, 0, b + 7, &out) == 0) {
            return 0;
        }
        if (out != 0) {
            return 1;
        }
        DWCi_bufferFrame(&req->outgoingBuffer, (const u8*)a, b);
        count = ArrayLength(req->outgoingBufferMessages);
        item = ArrayNth(req->outgoingBufferMessages, count - 1);
        ret = DWCi_requestFrame(req, req->outgoingBuffer.buffer + item[0], item[1]);
        if (ret == 0) {
            return 0;
        }
        req->pendingAck = 0;
        return 1;
    }
    return DWCi_requestFlush(req, (u8*)a, b);
}

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
    return (u16)(ka->port - kb->port);
}

/* 0x80511680 - the request list's "release one slot" hook. */
void DWCi_requestListRelease(DWCiReq** slot)
{
    DWCi_requestFree(*slot);
}

/* 0x80511690 - look a request up by `{addr, port}`. */
u32 DWCi_findRequest(DWCiConn* conn, u32 addr, u16 port)
{
    DWCiAddrKey key;
    DWCiAddrKey* pkey;
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
void DWCi_createConnection(DWCiConn** out, char* host, u32 bufSize1, u32 bufSize2, u32 arg5, u32 mode)
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
        *out = (DWCiConn*)4;
        return;
    }
    conn = gsimalloc(0x4C);
    if (conn == 0) {
        *out = (DWCiConn*)1;
        return;
    }
    memset(conn, 0, 0x4C);
    conn->sock = -1;
    conn->incomingBufferSize = bufSize2;
    conn->outgoingBufferSize = bufSize1;
    conn->socketErrorCallback = arg5;
    conn->connections = TableNew2(4, 32, 2, (TableHashFn)DWCi_requestTableHash, (TableCompareFn)DWCi_requestTableCompare, NULL);
    if (conn->connections == 0) {
        gsifree(conn);
        *out = (DWCiConn*)1;
        return;
    }
    conn->closedConnections = ArrayNew(4, 4, (ArrayElementFreeFn)DWCi_requestListRelease);
    if (conn->closedConnections == 0) {
        TableFree(conn->connections);
        gsifree(conn);
        *out = (DWCiConn*)1;
        return;
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
        *out = (DWCiConn*)3;
        return;
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
            *out = (DWCiConn*)3;
            return;
        }
    }
    salen = 8;
    DWCi_socketGetLocalName(conn->sock, &sa, &salen);
    conn->addr = sa.addr;
    conn->port = SOAddressToHostPort(sa.port);
    *out = conn;
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
    DWCiAddrKey key;
    DWCiAddrKey* pkey;
    DWCiReq** slot;
    DWCiReq* req;
    DWCiReq* entry;
    DWCiReq** found;

    key.addr = addr;
    key.port = port;
    pkey = &key;
    found = TableLookup(conn->connections, &pkey);
    req = 0;
    if (found != 0) {
        req = *found;
    }
    if (req != 0) {
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
                                key.addr = addr;
                                key.port = port;
                                pkey = &key;
                                slot = TableLookup(conn->connections, &pkey);
                                entry = 0;
                                if (slot != 0) {
                                    entry = *slot;
                                }
                                *out = entry;
                                if (entry != 0) {
                                    return 0;
                                }
                            }
                        }
                    }
                }
            }
        }
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
    DWCiReq* key;
    u32 count;
    u32 i;

    key = req;
    if (req->freeAtAcceptReject != 0) {
        return;
    }
    if (req->callbackLevel != 0) {
        return;
    }
    if (req->state == 7) {
        count = ArrayLength(req->conn->closedConnections);
        for (i = 0; i < count; i++) {
            if (*(DWCiReq**)ArrayNth(req->conn->closedConnections, i) == req) {
                ArrayDeleteAt(req->conn->closedConnections, i);
                return;
            }
        }
    } else {
        TableRemove(req->conn->connections, &key);
    }
}

/* 0x80511CF0 - the send path: default the buffer/length pair, refuse while the transport is not
 * usable, address the peer, hand the frame to the socket layer and dispatch the socket error to the
 * reconnect/retry entry points. */
u32 DWCi_sendTo(DWCiConn* conn, u32 addr, u16 port, void* buf, int len)
{
    DWCiSockAddrIn sa;
    DWCiAddrKey key;
    DWCiAddrKey* pkey;
    DWCiReq** slot;
    DWCiReq* req;
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
            if (DWCi_requestReconnect(conn, addr, port) != 0) {
                return 1;
            }
            return 0;
        }
        if (err == -23) {
            if (DWCi_requestRetry(conn, addr, port, 1) != 0) {
                return 1;
            }
            return 0;
        }
        if (err == -42 || err == -6) {
            return 1;
        }
        if (err == -35) {
            return 1;
        }
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
    if (conn->sendDumpCallback != 0) {
        key.addr = addr;
        key.port = port;
        pkey = &key;
        slot = TableLookup(conn->connections, &pkey);
        req = 0;
        if (slot != 0) {
            req = *slot;
        }
        if (DWCi_requestSend(conn, req, addr, port, 0, buf, len, 1) == 0) {
            return 0;
        }
    }
    return 1;
}

/* 0x80511F10 - the table walk's per-request step: skip a request that is neither terminal nor
 * timed out, then drop the terminal ones through the same remove path DWCi_removeRequest uses. */
u32 DWCi_requestTick(void** slot, u32* now)
{
    DWCiReq* req;
    DWCiReq* key;
    u32 count;
    u32 i;

    req = *slot;
    if (req->state != 7 && DWCi_requestIsTimedOut(req, *now) == 0) {
        return 0;
    }
    if (req->state == 7 && req->freeAtAcceptReject == 0 && req->callbackLevel == 0) {
        key = req;
        if (req->state == 7 && req->freeAtAcceptReject == 0 && req->callbackLevel == 0) {
            if (req->state == 7) {
                count = ArrayLength(req->conn->closedConnections);
                for (i = 0; i < count; i++) {
                    if (*(DWCiReq**)ArrayNth(req->conn->closedConnections, i) == req) {
                        ArrayDeleteAt(req->conn->closedConnections, i);
                        return 1;
                    }
                }
            } else {
                TableRemove(req->conn->connections, &key);
            }
        }
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
        DWCiReq* req = *(DWCiReq**)ArrayNth(conn->closedConnections, i);
        DWCiReq* key;

        if (req->freeAtAcceptReject != 0) {
            continue;
        }
        if (req->callbackLevel != 0) {
            continue;
        }
        if (req->state == 7) {
            u32 count = ArrayLength(req->conn->closedConnections);
            u32 j;

            for (j = 0; j < count; j++) {
                if (*(DWCiReq**)ArrayNth(req->conn->closedConnections, j) == req) {
                    ArrayDeleteAt(req->conn->closedConnections, j);
                    break;
                }
            }
        } else {
            key = req;
            TableRemove(req->conn->connections, &key);
        }
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

/* 0x80512200 - the host-to-network short wrapper the transport clients call. */
u16 DWCi_htons(u16 port)
{
    return SOAddressToHostPort(port);
}

/* 0x80512210 - format an address for the log/text layer: "addr:port", "addr", ":port" or the empty
 * string, into the caller's buffer or into the two-buffer ring when the caller passes none. */
char* DWCi_formatAddress(u32 addr, u16 port, char* buf)
{
    if (buf == 0) {
        DWCi_addressRingIndex ^= 1;
        buf = (char*)(DWCi_addressRing + DWCi_addressRingIndex * 0x16);
    }
    if (addr != 0) {
        u32 withPort;
        u32 withoutPort;

        if (port != 0) {
            withPort = addr;
            sprintf(buf, DWCi_addressFormat, SOAddressToString(&withPort), port);
        } else {
            withoutPort = addr;
            sprintf(buf, DWCi_addressFormatHost, SOAddressToString(&withoutPort));
        }
    } else {
        if (port != 0) {
            sprintf(buf, DWCi_addressFormatPort, port);
        } else {
            *buf = 0;
        }
    }
    return buf;
}

/* 0x80512300 - parse "host:port": split at the colon, validate the port digits against the band's
 * character-class table, convert with atoi, resolve the host name and hand both halves back. */
u32 DWCi_parseAddress(char* str, u32* outAddr, u16* outPort)
{
    char buf[0x108];
    char* colon;
    char* p;
    u32 addr;
    u16 port;
    u32 value;

    if (str == 0 || *str == 0) {
        addr = 0;
        port = 0;
    } else {
        colon = strchr(str, ':');
        if (colon == 0) {
            port = 0;
        } else {
            if (colon == str) {
                str = 0;
                addr = 0;
            } else {
                memcpy(buf, str, colon - str);
                buf[colon - str] = 0;
                str = buf;
            }
            p = colon + 1;
            while (*p != 0) {
                if ((u32)(s8)*p > 0xFFu ||
                    (_current_locale.cls->bits[(u8)*p] & 8) == 0) {
                    return 0;
                }
                p++;
            }
            value = atoi(colon + 1);
            if (value > 0xFFFF) {
                return 0;
            }
            port = (u16)value;
        }
        if (str != 0) {
            addr = DWCi_socketResolveAddress(str);
            if (addr == (u32)-1) {
                DWCiHostEntry* entry = DWCi_socketLookupHost(str);

                if (entry == 0) {
                    return 0;
                }
                addr = (*entry->hosts)->addr;
            }
        }
    }
    if (outAddr != 0) {
        *outAddr = addr;
    }
    if (outPort != 0) {
        *outPort = port;
    }
    return 1;
}

/* 0x80512490 moved to `DWCi/DWCi_NatNeg.c` with the seam (see the header): the body was
 * byte-identical here and is kept there verbatim, so this unit now ends at 0x80512490. */
