/*
 * DWCi/gt2Connection.c - the GameSpy GT2 connection: outgoing and incoming connection setup, the connection attempt,
 *   accept / reject, the raw send, the per-connection timers (attempt timeout, keep-alive, resend, delayed ack), the
 *   close and the cleanup.
 *
 * RANGE. `.text` 0x8050D7A0..0x8050DEC0 (11 functions); no data of its own. The left edge follows gt2Callback's last
 *   function, the right edge is gt2Main's first (`DWCi/gt2Main.c`); the GT2 files link in name order.
 * FLAGS. `cflags_base` (-func_align 16): the retail functions start 16-byte aligned.
 * NAMES. GameSpy SDK names where the body is the SDK's (`gti2*`); GUESS `DWCi_requestFrame` (gti2ConnectionSendData),
 *   GUESS `DWCi_requestIsTimedOut` (gti2ConnectionThink), GUESS `DWCi_requestFree` (gti2ConnectionCleanup). The file
 *   name is the GameSpy SDK's `gt2Connection.c` (no `__FILE__` string).
 * RESIDUALS. `gti2StartConnectionAttempt` and `gti2AcceptConnection` copy the four callbacks with four loads then
 *   four stores where retail moves them as two load-pair / store-pair groups (the struct assignment, `memcpy` and an
 *   8-aligned callback record all leave it).
 * SHAPES. `gti2ConnectionClosed`'s body is a static inline its earlier callers expand (retail folds it into
 *   `gti2CheckTimeout` and `gti2CloseConnection`); the think step's keep-alive, resend and ack checks are 0/1 static
 *   inlines.
 */
#include "DWCi/gt2Connection.h"
#include "DWCi/dwc_nasfunc.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "DWCi/gt2Socket.h"
#include "DWCi/natneg.h"

/* Moves a connection into the closed state and onto its socket's closed list. */
static inline void gti2ConnectionClosedInline(DWCiReq* connection) {
    if (connection->state != GTI2_STATE_CLOSED) {
        connection->state = GTI2_STATE_CLOSED;
        TableRemove(connection->conn->connections, &connection);
        ArrayAppend(connection->conn->closedConnections, &connection);
    }
}

/* 0x8050D7A0 (0x54): a new connection this side initiates. */
s32 gti2NewOutgoingConnection(DWCiConn* socket, DWCiReq** connection, u32 ip, u16 port) {
    s32 result = DWCi_createRequest(socket, connection, ip, port);
    if (result != 0) {
        return result;
    }
    (*connection)->state = GTI2_STATE_AWAITING_SERVER_CHALLENGE;
    (*connection)->initiated = 1;
    return 0;
}

/* 0x8050D800 (0x54): a new connection the remote side initiated. */
s32 gti2NewIncomingConnection(DWCiConn* socket, DWCiReq** connection, u32 ip, u16 port) {
    s32 result = DWCi_createRequest(socket, connection, ip, port);
    if (result != 0) {
        return result;
    }
    (*connection)->state = GTI2_STATE_AWAITING_CLIENT_CHALLENGE;
    (*connection)->initiated = 0;
    return 0;
}

/* 0x8050D860 (0xD4): stores the initial message and callbacks and sends the client challenge. */
s32 gti2StartConnectionAttempt(DWCiReq* connection, u8* message, s32 len, const GT2ConnectionCallbacks* callbacks) {
    u8 challenge[0x20];

    DWCi_GetStringLength((void**)&message, (int*)&len);
    if (len > 0) {
        connection->initialMessage = (u8*)gsimalloc(len);
        if (connection->initialMessage == NULL) {
            return 1;
        }
        memcpy(connection->initialMessage, message, len);
        connection->initialMessageLength = len;
    }
    if (callbacks != NULL) {
        connection->callbacks = *callbacks;
    }
    gti2GetChallenge(challenge);
    gti2GetResponse(connection->response, challenge);
    gti2SendClientChallenge(connection, challenge);
    connection->state = GTI2_STATE_AWAITING_SERVER_CHALLENGE;
    return 0;
}

/* 0x8050D940 (0xA4): accepts a connection waiting for the decision; 0 when it cannot be accepted. */
s32 gti2AcceptConnection(DWCiReq* connection, const GT2ConnectionCallbacks* callbacks) {
    if (connection->freeAtAcceptReject) {
        connection->freeAtAcceptReject = 0;
        return 0;
    }
    connection->freeAtAcceptReject = 0;
    if (connection->state != GTI2_STATE_AWAITING_ACCEPT_REJECT) {
        return 0;
    }
    gti2SendAccept(connection);
    connection->state = GTI2_STATE_CONNECTED;
    if (callbacks != NULL) {
        connection->callbacks = *callbacks;
    }
    return 1;
}

/* 0x8050D9F0 (0x68): rejects a connection waiting for the decision. */
void gti2RejectConnection(DWCiReq* connection, u8* message, s32 len) {
    connection->freeAtAcceptReject = 0;
    if (connection->state == GTI2_STATE_AWAITING_ACCEPT_REJECT) {
        DWCi_GetStringLength((void**)&message, (int*)&len);
        gti2SendReject(connection, message, len);
        connection->state = GTI2_STATE_CLOSING;
    }
}

/* 0x8050DA60 (0x5C): sends raw data to the connection's peer and stamps the send time. */
/* untyped: byte range - the datagram */
int DWCi_requestFrame(DWCiReq* req, void* p, u32 len) {
    if (!DWCi_sendTo(req->conn, req->addr, req->port, p, len)) {
        return 0;
    }
    req->lastSend = current_time();
    return 1;
}

/* 0x8050DAC0 (0xFC): times out a connection attempt (the initiator's own timeout, a minute for the other side). */
s32 gti2CheckTimeout(DWCiReq* connection, u32 now) {
    s32 timedOut;

    if (connection->state < GTI2_STATE_CONNECTED) {
        timedOut = 0;
        if (connection->initiated) {
            if (connection->timeout != 0 && now - connection->startTime > connection->timeout) {
                timedOut = 1;
            }
        } else if (connection->state < GTI2_STATE_AWAITING_ACCEPT_REJECT && now - connection->startTime > 60000) {
            timedOut = 1;
        }
        if (timedOut) {
            DWCi_sendControlFrame(connection);
            gti2ConnectionClosedInline(connection);
            if (!gti2ConnectedCallback(connection, 6, NULL, 0)) {
                return 0;
            }
        }
    }
    return 1;
}

/* 0x8050DBC0 (0x14C): runs a connection's timers: the attempt timeout, the keep-alive, the resends and the
 * delayed ack; 0 when the socket went away during a callback. */
/* Sends a keep-alive when the connection has been quiet for 30 s. */
static inline s32 gti2KeepAliveThink(DWCiReq* req, u32 now) {
    if (now - req->lastSend > 30000) {
        if (!gti2SendKeepAlive(req)) {
            return 0;
        }
    }
    return 1;
}

/* Resends every reliable message unacknowledged for a second. */
static inline s32 gti2ResendThink(DWCiReq* req, u32 now) {
    s32 count;
    s32 i;
    DWCiXfer* message;

    count = ArrayLength(req->outgoingBufferMessages);
    for (i = 0; i < count; i++) {
        message = (DWCiXfer*)ArrayNth(req->outgoingBufferMessages, i);
        if (now - message->stamp > 1000) {
            if (!DWCi_appendTransfer(req, message)) {
                return 0;
            }
        }
    }
    return 1;
}

/* Sends the pending acknowledgement once it is 100 ms old. */
static inline s32 gti2AckThink(DWCiReq* req, u32 now) {
    if (!req->pendingAck) {
        return 1;
    }
    if (now - req->pendingAckTime > 100) {
        if (!gti2SendAck(req)) {
            return 0;
        }
    }
    return 1;
}

int DWCi_requestIsTimedOut(DWCiReq* req, u32 now) {
    if (!gti2CheckTimeout(req, now)) {
        return 0;
    }
    if (!gti2KeepAliveThink(req, now)) {
        return 0;
    }
    if (!gti2ResendThink(req, now)) {
        return 0;
    }
    return gti2AckThink(req, now) != 0;
}

/* 0x8050DD10 (0xA0): closes a connection: hard (notify, free) or soft (send the close and wait). */
void gti2CloseConnection(DWCiReq* connection, s32 hard) {
    if (hard) {
        if (connection->state < GTI2_STATE_CLOSED) {
            gti2ConnectionClosedInline(connection);
            DWCi_sendControlFrame(connection);
            gti2ClosedCallback(connection, 0);
            DWCi_removeRequest(connection);
        }
    } else {
        connection->state = GTI2_STATE_CLOSING;
        gti2SendClose(connection);
    }
}

/* 0x8050DDB0 (0x5C): moves a connection into the closed state and onto its socket's closed list. */
void gti2ConnectionClosed(DWCiReq* connection) {
    gti2ConnectionClosedInline(connection);
}

/* 0x8050DE10 (0xA4): frees a connection and everything it owns. */
/* untyped: caller-owned payload - the connection record */
void DWCi_requestFree(void* p) {
    DWCiReq* connection = (DWCiReq*)p;
    if (connection->initialMessage != NULL) {
        gsifree(connection->initialMessage);
    }
    if (connection->incomingBuffer.buffer != NULL) {
        gsifree(connection->incomingBuffer.buffer);
    }
    if (connection->outgoingBuffer.buffer != NULL) {
        gsifree(connection->outgoingBuffer.buffer);
    }
    if (connection->incomingBufferMessages != NULL) {
        ArrayFree(connection->incomingBufferMessages);
    }
    if (connection->outgoingBufferMessages != NULL) {
        ArrayFree(connection->outgoingBufferMessages);
    }
    if (connection->sendFilters != NULL) {
        ArrayFree(connection->sendFilters);
    }
    if (connection->receiveFilters != NULL) {
        ArrayFree(connection->receiveFilters);
    }
    gsifree(connection);
}
