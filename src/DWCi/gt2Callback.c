/*
 * DWCi/gt2Callback.c - the GameSpy GT2 callbacks: every application callback (socket error, connect attempt,
 *   connected, received, closed, ping, the send / receive filters, the dumps, unrecognised datagrams) runs with the
 *   socket's and the connection's nesting levels raised, and a socket closed meanwhile is freed afterwards.
 *
 * RANGE. `.text` 0x8050CEE0..0x8050D7A0 (10 functions); no data of its own. The left edge follows gt2Buffer's last
 *   function, the right edge is gt2Connection's first (`DWCi/gt2Connection.c`); the GT2 files link in name order.
 * FLAGS. `cflags_base` (-func_align 16): the retail functions start 16-byte aligned.
 * NAMES. GameSpy SDK names (`gti2*Callback`); GUESS `DWCi_socketConnect` (gti2SocketErrorCallback), GUESS
 *   `DWCi_requestSend` (gti2DumpCallback). The file name is the GameSpy SDK's `gt2Callback.c` (no `__FILE__`
 *   string).
 * RESIDUALS. none measured (every row matches).
 * SHAPES. The callback-done step (free a socket closed during the callback) is a static inline every callback
 *   expands.
 */
#include "DWCi/gt2Callback.h"
#include "DWCi/dwc_nasfunc.h"
#include "DWCi/gt2Socket.h"

/* The callback shapes GT2 calls through a connection's callback set. */
typedef void (*GTI2ConnectedFn)(DWCiReq* connection, s32 result, u8* message, s32 len);
typedef void (*GTI2ReceivedFn)(DWCiReq* connection, u8* message, s32 len, s32 reliable);
typedef void (*GTI2ClosedFn)(DWCiReq* connection, s32 reason);
typedef void (*GTI2PingFn)(DWCiReq* connection, s32 latency);
typedef void (*GTI2SocketErrorFn)(DWCiConn* socket);
typedef void (*GTI2ConnectAttemptFn)(DWCiConn* socket, DWCiReq* connection, u32 ip, u16 port, s32 latency,
                                     u8* message, s32 len);
typedef void (*GTI2DumpFn)(DWCiConn* socket, DWCiReq* connection, u32 ip, u16 port, s32 reset, u8* message,
                           s32 len);
typedef s32 (*GTI2UnrecognizedMessageFn)(DWCiConn* socket, u32 ip, u16 port, u8* message, s32 len);
typedef void (*GTI2FilterFn)(DWCiReq* connection, s32 filterId, u8* message, s32 len, s32 reliable);

/* Lowers the socket's nesting level after a callback and frees it when it was closed meanwhile (0 then). */
static inline s32 gti2SocketCallbackDone(DWCiConn* socket) {
    if (socket->close && socket->callbackLevel == 0) {
        DWCi_destroyConnection(socket);
        return 0;
    }
    return 1;
}

/* 0x8050CEE0 (0x94): the socket error callback. */
int DWCi_socketConnect(DWCiConn* conn) {
    if (conn == NULL) {
        return 1;
    }
    if (conn->socketErrorCallback == 0) {
        return 1;
    }
    conn->callbackLevel++;
    ((GTI2SocketErrorFn)conn->socketErrorCallback)(conn);
    conn->callbackLevel--;
    return gti2SocketCallbackDone(conn);
}

/* 0x8050CF80 (0xE8): the connect-attempt callback of a listening socket. */
s32 gti2ConnectAttemptCallback(DWCiConn* socket, DWCiReq* connection, u32 ip, u16 port, s32 latency, u8* message,
                               s32 len) {
    if (socket == NULL || connection == NULL) {
        return 1;
    }
    if (socket->connectAttemptCallback == 0) {
        return 1;
    }
    if (len == 0 || message == NULL) {
        message = NULL;
        len = 0;
    }
    socket->callbackLevel++;
    connection->callbackLevel++;
    ((GTI2ConnectAttemptFn)socket->connectAttemptCallback)(socket, connection, ip, port, latency, message, len);
    socket->callbackLevel--;
    connection->callbackLevel--;
    return gti2SocketCallbackDone(socket);
}

/* 0x8050D070 (0xDC): the connected callback (records the result first). */
s32 gti2ConnectedCallback(DWCiReq* connection, s32 result, u8* message, s32 len) {
    if (connection == NULL) {
        return 1;
    }
    connection->connectionResult = result;
    if (connection->callbacks.connected_00 == NULL) {
        return 1;
    }
    if (len == 0 || message == NULL) {
        message = NULL;
        len = 0;
    }
    connection->callbackLevel++;
    connection->conn->callbackLevel++;
    ((GTI2ConnectedFn)connection->callbacks.connected_00)(connection, result, message, len);
    connection->callbackLevel--;
    connection->conn->callbackLevel--;
    return gti2SocketCallbackDone(connection->conn);
}

/* 0x8050D150 (0xD8): the received callback. */
s32 gti2ReceivedCallback(DWCiReq* connection, u8* message, s32 len, s32 reliable) {
    if (connection == NULL) {
        return 1;
    }
    if (connection->callbacks.received_04 == NULL) {
        return 1;
    }
    if (len == 0 || message == NULL) {
        message = NULL;
        len = 0;
    }
    connection->callbackLevel++;
    connection->conn->callbackLevel++;
    ((GTI2ReceivedFn)connection->callbacks.received_04)(connection, message, len, reliable);
    connection->callbackLevel--;
    connection->conn->callbackLevel--;
    return gti2SocketCallbackDone(connection->conn);
}

/* 0x8050D230 (0xBC): the closed callback. */
s32 gti2ClosedCallback(DWCiReq* connection, s32 reason) {
    if (connection == NULL) {
        return 1;
    }
    if (connection->callbacks.closed_08 == NULL) {
        return 1;
    }
    connection->callbackLevel++;
    connection->conn->callbackLevel++;
    ((GTI2ClosedFn)connection->callbacks.closed_08)(connection, reason);
    connection->callbackLevel--;
    connection->conn->callbackLevel--;
    return gti2SocketCallbackDone(connection->conn);
}

/* 0x8050D2F0 (0xBC): the ping callback. */
s32 gti2PingCallback(DWCiReq* connection, s32 latency) {
    if (connection == NULL) {
        return 1;
    }
    if (connection->callbacks.ping_0C == NULL) {
        return 1;
    }
    connection->callbackLevel++;
    connection->conn->callbackLevel++;
    ((GTI2PingFn)connection->callbacks.ping_0C)(connection, latency);
    connection->callbackLevel--;
    connection->conn->callbackLevel--;
    return gti2SocketCallbackDone(connection->conn);
}

/* 0x8050D3B0 (0x108): runs send filter `filterId` on a message. */
s32 gti2SendFilterCallback(DWCiReq* connection, s32 filterId, u8* message, s32 len, s32 reliable) {
    GTI2FilterFn* filter;
    if (connection == NULL) {
        return 1;
    }
    filter = (GTI2FilterFn*)ArrayNth(connection->sendFilters, filterId);
    if (filter == NULL) {
        return 1;
    }
    if (len == 0 || message == NULL) {
        message = NULL;
        len = 0;
    }
    connection->callbackLevel++;
    connection->conn->callbackLevel++;
    (*filter)(connection, filterId, message, len, reliable);
    connection->callbackLevel--;
    connection->conn->callbackLevel--;
    return gti2SocketCallbackDone(connection->conn);
}

/* 0x8050D4C0 (0x108): runs receive filter `filterId` on a message. */
s32 gti2ReceiveFilterCallback(DWCiReq* connection, s32 filterId, u8* message, s32 len, s32 reliable) {
    GTI2FilterFn* filter;
    if (connection == NULL) {
        return 1;
    }
    filter = (GTI2FilterFn*)ArrayNth(connection->receiveFilters, filterId);
    if (filter == NULL) {
        return 1;
    }
    if (len == 0 || message == NULL) {
        message = NULL;
        len = 0;
    }
    connection->callbackLevel++;
    connection->conn->callbackLevel++;
    (*filter)(connection, filterId, message, len, reliable);
    connection->callbackLevel--;
    connection->conn->callbackLevel--;
    return gti2SocketCallbackDone(connection->conn);
}

/* 0x8050D5D0 (0xFC): the send / receive dump callback. */
/* untyped: byte range - the datagram */
int DWCi_requestSend(DWCiConn* conn, DWCiReq* req, u32 addr, u16 port, u32 a, void* buf, int len, u32 flag) {
    GTI2DumpFn dump;
    u8* message = (u8*)buf;

    if (conn == NULL) {
        return 1;
    }
    if (flag) {
        dump = (GTI2DumpFn)conn->sendDumpCallback;
    } else {
        dump = (GTI2DumpFn)conn->receiveDumpCallback;
    }
    if (dump == NULL) {
        return 1;
    }
    if (len == 0 || message == NULL) {
        message = NULL;
        len = 0;
    }
    conn->callbackLevel++;
    if (req != NULL) {
        req->callbackLevel++;
    }
    dump(conn, req, addr, port, a, message, len);
    conn->callbackLevel--;
    if (req != NULL) {
        req->callbackLevel--;
    }
    return gti2SocketCallbackDone(conn);
}

/* 0x8050D6D0 (0xCC): the unrecognised-message callback; `*handled` gets its answer. */
s32 gti2UnrecognizedMessageCallback(DWCiConn* socket, u32 ip, u16 port, u8* message, s32 len, s32* handled) {
    *handled = 0;
    if (socket == NULL) {
        return 1;
    }
    if (socket->unrecognizedMessageCallback == 0) {
        return 1;
    }
    if (len == 0 || message == NULL) {
        message = NULL;
        len = 0;
    }
    socket->callbackLevel++;
    *handled = ((GTI2UnrecognizedMessageFn)socket->unrecognizedMessageCallback)(socket, ip, port, message, len);
    socket->callbackLevel--;
    return gti2SocketCallbackDone(socket);
}
