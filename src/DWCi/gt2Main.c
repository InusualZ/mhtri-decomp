/*
 * DWCi/gt2Main.c - the GameSpy GT2 public API: socket create / close / think / listen, connection accept / reject /
 *   connect / send / close, and the socket accessors.
 *
 * RANGE. `.text` 0x8050DEC0..0x8050E2B0 (13 functions); no data of its own. The left edge follows gt2Connection's
 *   last function, the right edge is gt2Message's first (`DWCi/gt2Message.c`); the GT2 files link in name order.
 * FLAGS. `cflags_base` (-func_align 16): the retail functions start 16-byte aligned.
 * NAMES. GameSpy SDK names (`gt2*`, `gti2CloseAllConnectionsHardMap`); GUESS `DWCi_socketShutdown` (the GT2
 *   close-all-connections helper, gt2CloseAllConnectionsHard). The file name is the GameSpy SDK's `gt2Main.c` (no
 *   `__FILE__` string). GUESS: the map's `gt2CloseConnection` passes `hard` = 1, the SDK's
 *   `gt2CloseConnectionHard` (rename request net-nasfunc#1; the game's GameSpy interface calls it).
 * RESIDUALS. `gt2Send` colours the connection r31 / `reliable` r30 (retail r30 / r31) and folds its 0-or-10 result
 *   into a branchless select where retail branches (early-return, result-variable and ternary spellings all
 *   select; the if/else spelling keeps the size).
 * SHAPES. The blocking `gt2Connect` loop repeats `gt2Think`'s body in place (retail calls the receive and think
 *   steps directly).
 */
#include "DWCi/gt2Main.h"
#include "DWCi/dwc_nasfunc.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "DWCi/gt2Socket.h"
#include "DWCi/gt2Utility.h"
#include "DWCi/natneg.h"

/* 0x8050DEC0 (0x8): creates a GT2 socket bound to `localAddress` (UDP protocol). */
s32 gt2CreateSocket(GT2Socket* socket, const char* localAddress, u32 outgoingBufferSize, u32 incomingBufferSize,
                    NetworkCallback socketErrorCallback) {
    return DWCi_createConnection((DWCiConn**)socket, (char*)localAddress, outgoingBufferSize, incomingBufferSize,
                          (u32)socketErrorCallback, 0);
}

/* 0x8050DED0 (0x44): hard-closes every connection of the socket, then the socket. */
void gt2CloseSocket(GT2Socket socket) {
    TableMapSafe(((DWCiConn*)socket)->connections, gti2CloseAllConnectionsHardMap, NULL);
    DWCi_destroyConnection((DWCiConn*)socket);
}

/* 0x8050DF20 (0x4C): receives, runs every connection's timers and frees the closed connections. */
void gt2Think(GT2Socket socket) {
    if (gti2ReceiveMessages((DWCiConn*)socket) && DWCi_connectionTick((DWCiConn*)socket)) {
        DWCi_connectionFlushRequests((DWCiConn*)socket);
    }
}

/* 0x8050DF70 (0x4): starts accepting connection attempts through `callback`. */
void gt2Listen(GT2Socket socket, NetworkCallback connectAttemptCallback) {
    DWCi_setConnectionUserValue((DWCiConn*)socket, (u32)connectAttemptCallback);
}

/* 0x8050DF80 (0x4): accepts a pending connection with `callbacks`. */
s32 gt2Accept(GT2Connection connection, const GT2ConnectionCallbacks* callbacks) {
    return gti2AcceptConnection((DWCiReq*)connection, callbacks);
}

/* 0x8050DF90 (0x4): rejects a pending connection with `message`. */
/* untyped: byte range - the reject message */
void gt2Reject(GT2Connection connection, const char* message, s32 length) {
    gti2RejectConnection((DWCiReq*)connection, (u8*)message, length);
}

/* 0x8050DFA0 (0x1A8): connects to `remoteAddress` ("host:port"), optionally blocking until the attempt ends. */
/* untyped: byte range - the connect message */
s32 gt2Connect(GT2Socket socket, u32* connection, const char* remoteAddress, const void* message, u32 length,
               u32 timeout, const GT2ConnectionCallbacks* callbacks, s32 blocking) {
    DWCiReq* newConnection;
    u32 ip;
    u16 port;
    s32 result;
    s32 done;

    if (!DWCi_parseAddress((char*)remoteAddress, &ip, &port) || ip == 0 || port == 0) {
        return 4;
    }
    if ((DWCi_socketTokenFromPeer(ip) & 0xE0000000) == 0xE0000000) {
        return 4;
    }
    result = gti2NewOutgoingConnection((DWCiConn*)socket, &newConnection, ip, port);
    if (result != 0) {
        return result;
    }
    newConnection->timeout = timeout;
    result = gti2StartConnectionAttempt(newConnection, (u8*)message, length, callbacks);
    if (result != 0) {
        DWCi_removeRequest(newConnection);
        return result;
    }
    if (!blocking) {
        if (connection != NULL) {
            *connection = (u32)newConnection;
        }
        return 0;
    }
    newConnection->callbackLevel++;
    do {
        if (gti2ReceiveMessages((DWCiConn*)socket) && DWCi_connectionTick((DWCiConn*)socket)) {
            DWCi_connectionFlushRequests((DWCiConn*)socket);
        }
        done = newConnection->state >= GTI2_STATE_CONNECTED;
        if (!done) {
            msleep(1);
        }
    } while (!done);
    newConnection->callbackLevel--;
    if (newConnection->state == GTI2_STATE_CONNECTED) {
        *connection = (u32)newConnection;
    }
    return newConnection->connectionResult;
}

/* 0x8050E150 (0xFC): sends `message` on a connected connection (through the send filters when there are any). */
/* untyped: byte range - the datagram */
s32 gt2Send(GT2Connection connection, const void* message, u32 length, s32 reliable) {
    DWCiReq* conn = (DWCiReq*)connection;
    u16 vdpLength;

    if (conn->state != GTI2_STATE_CONNECTED) {
        return 8;
    }
    DWCi_GetStringLength((void**)&message, (int*)&length);
    if (reliable && conn->conn->protocolType == 2) {
        memcpy(&vdpLength, message, 2);
        if ((s32)length != (s32)(vdpLength + conn->conn->protocolOffset)) {
            return 9;
        }
    }
    if (ArrayLength(conn->sendFilters) != 0) {
        gti2SendFilterCallback(conn, 0, (u8*)message, length, reliable);
        return 0;
    }
    if (DWCi_flushRequest(conn, (u32)message, length, reliable) == 0) {
        return 10;
    } else {
        return 0;
    }
}

/* 0x8050E250 (0x8): hard-closes the connection (notify and free at once). */
void gt2CloseConnection(GT2Connection connection) {
    gti2CloseConnection((DWCiReq*)connection, 1);
}

/* 0x8050E260 (0xC): the `TableMapSafe` callback that hard-closes one connection. */
/* untyped: caller-owned payload - the table element and the client data */
void gti2CloseAllConnectionsHardMap(void* elem, void* clientData) {
    gti2CloseConnection(*(DWCiReq**)elem, 1);
}

/* 0x8050E270 (0x14): hard-closes every connection of the socket. */
void DWCi_socketShutdown(DWCiConn* conn) {
    TableMapSafe(conn->connections, gti2CloseAllConnectionsHardMap, NULL);
}

/* 0x8050E290 (0x8): the SO socket under a GT2 socket. */
s32 gt2GetSocketSOCKET(GT2Socket socket) {
    return ((DWCiConn*)socket)->sock;
}

/* 0x8050E2A0 (0x8): installs the callback for datagrams GT2 does not recognise. */
void gt2SetUnrecognizedMessageCallback(GT2Socket socket, NetworkCallback callback) {
    ((DWCiConn*)socket)->unrecognizedMessageCallback = (u32)callback;
}
