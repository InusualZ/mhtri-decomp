/*
 * DWCi/gt2Message.c - the GameSpy GT2 protocol messages: the reliable / unreliable message handlers, the datagram
 *   dispatcher and receive loop, and the senders of every protocol message (challenge, accept, reject, close,
 *   keep-alive, ack, nack, closed, resend, send).
 *
 * RANGE. `.text` 0x8050E2B0..0x80511620 (25 functions); `.bss` 0x807615C0..0x807625C0 (the receive buffer); `.sdata`
 *   0x80794348..0x80794358 (the protocol magic and the "time" ping tag). The left edge follows gt2Main's last
 *   function (`DWCi/dwc_nasfunc.c`); the right edge is gt2Socket's first (`DWCi/gt2Socket.c`). The GT2 files link in
 *   name order (gt2Auth, gt2Buffer, gt2Callback, gt2Connection, gt2Main, gt2Message, gt2Socket, gt2Utility).
 * FLAGS. `cflags_base` (-func_align 16): the retail functions start 16-byte aligned.
 * NAMES. GameSpy SDK names where the body is the SDK's (`gti2*`); the `DWCi_*` rows are GUESSes from their bodies,
 *   named before the GT2 identification: GUESS `DWCi_requestReconnect` (gti2ConnectionReset), GUESS
 *   `DWCi_requestRetry` (gti2ConnectionUnreachable), GUESS `DWCi_requestBlockState` (gti2BeginReliableMessage),
 *   GUESS `DWCi_requestFlush` (gti2SendAppUnreliable), GUESS `DWCi_sendControlFrame` (gti2SendClosed), GUESS
 *   `DWCi_appendTransfer` (gti2ResendMessage), GUESS `DWCi_flushRequest` (gti2Send), GUESS `DWCi_protocolMagic`,
 *   GUESS `DWCi_gt2ReceiveBuffer`. The file name is the GameSpy SDK's `gt2Message.c` (no `__FILE__` string).
 * RESIDUALS. Register colouring and block order remain in the larger message handlers
 *   (`gti2HandleReliableMessage`, `gti2HandleMessage`, `gti2ReceiveMessages`, `DWCi_requestBlockState`).
 *   `gti2HandleReliablePacket` re-tests the scan index after a delivered message where retail branches straight
 *   back to the scan, and colours the header length / header pointer and two delivery locals the other way round.
 *   `gti2HandleNack`'s inlined resend adds the buffer before the header offset where the standalone
 *   `DWCi_appendTransfer` (and retail) add the offsets first. `gti2HandleMessage` folds the delivery's 0/1 into
 *   a select where retail re-tests it, and `gti2ReceiveMessages` the same for the inlined `DWCi_requestRetry`
 *   (every if/return, ternary and nested spelling of the retry tail tried; none moves it without costing
 *   `DWCi_requestRetry` itself).
 * SHAPES. Message handling runs through 0/1 static inlines (one per message type, the delivery step) tested again by
 *   their callers (playbook idea 101); the dispatchers are if-chains, not switches. A 0/1 tail is spelled
 *   `return f(...) != 0;` where retail shows `neg/or/srwi`, and `if (!f(...)) return 0; ... return 1;` where retail
 *   branches to a shared `li r3,1`. The closed message, the end of a reliable message, the resend of a buffered
 *   message (`gti2ResendMessageInline`, also `DWCi_appendTransfer`'s whole body), the big-endian short write and
 *   the connection error path are static inlines (retail expands them in every caller); the message buffers are
 *   sized to the longest message (closed 5, ack 7, nack 9 bytes), which is what places them on retail's stack
 *   offsets. Buffered messages are delivered by a `for (;;)` around the backward scan, the delivery and the shift
 *   inside the scan reusing its index. `gti2HandleServerChallenge` and `gti2HandleClientResponse` sit in an
 *   `auto_inline off` block: defined ahead of `gti2HandleReliableMessage` in retail order, `-inline auto` would fold
 *   both into it (0x7BC -> 0xD54 B); `dont_inline` there also stops their own static inlines from folding.
 */
#include "DWCi/gt2Message.h"
#include "DWCi/dwc_nasfunc.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "MSL_C/alloc.h"
#include "MSL_C/strstr.h"
#include "DWCi/gt2Socket.h"

/* The GT2 protocol magic every protocol message starts with. */
u16 DWCi_protocolMagic = 0xFEFE;

/* The datagram buffer `gti2ReceiveMessages` reads into. */
u8 DWCi_gt2ReceiveBuffer[0x1000];


#define GTI2_MAGIC_LEN 2
#define GTI2_MSG_APP_RELIABLE 0
#define GTI2_MSG_CLIENT_CHALLENGE 1
#define GTI2_MSG_SERVER_CHALLENGE 2
#define GTI2_MSG_CLIENT_RESPONSE 3
#define GTI2_MSG_ACCEPT 4
#define GTI2_MSG_REJECT 5
#define GTI2_MSG_CLOSE 6
#define GTI2_MSG_KEEP_ALIVE 7
#define GTI2_MSG_ACK 100
#define GTI2_MSG_NACK 101
#define GTI2_MSG_PING 102
#define GTI2_MSG_PONG 103
#define GTI2_MSG_CLOSED 104
#define GTI2_VDP_PROTOCOL 2

/* Sends the "closed" message to `ip:port` (the VDP length word first on a VDP socket). */
static inline s32 gti2SocketSendClosedInline(DWCiConn* socket, u32 ip, u16 port) {
    u8 buffer[5];
    s32 pos = 0;
    s16 vdpLength;

    if (socket->protocolType == GTI2_VDP_PROTOCOL) {
        vdpLength = 3;
        memcpy(buffer, &vdpLength, 2);
        pos = 2;
    }
    memcpy(&buffer[pos], &DWCi_protocolMagic, GTI2_MAGIC_LEN);
    pos += GTI2_MAGIC_LEN;
    buffer[pos++] = GTI2_MSG_CLOSED;
    return DWCi_sendTo(socket, ip, port, buffer, pos) != 0;
}

/* Sends the "closed" message to a connection's peer. */
static inline s32 gti2SendClosedInline(DWCiReq* connection) {
    return gti2SocketSendClosedInline(connection->conn, connection->addr, connection->port);
}

/* Ends a connection on a protocol error: an attempt reports `result`, an established connection `reason`. */
static inline s32 gti2ConnectionError(DWCiReq* connection, s32 result, s32 reason) {
    if (connection->state < GTI2_STATE_CONNECTED) {
        if (connection->initiated) {
            gti2ConnectionClosed(connection);
            if (!gti2ConnectedCallback(connection, result, NULL, 0)) {
                return 0;
            }
        } else {
            if (connection->state == GTI2_STATE_AWAITING_ACCEPT_REJECT) {
                connection->freeAtAcceptReject = 1;
            }
            gti2ConnectionClosed(connection);
        }
    } else if (connection->state != GTI2_STATE_CLOSED) {
        gti2ConnectionClosed(connection);
        if (!gti2ClosedCallback(connection, reason)) {
            return 0;
        }
    }
    return 1;
}

/* Sends the last message queued in the outgoing buffer and clears the pending ack. */
static inline s32 gti2EndReliableMessageInline(DWCiReq* connection) {
    DWCiXfer* message = (DWCiXfer*)ArrayNth(connection->outgoingBufferMessages,
                                            ArrayLength(connection->outgoingBufferMessages) - 1);
    if (!DWCi_requestFrame(connection, connection->outgoingBuffer.buffer + message->offset, message->length)) {
        return 0;
    }
    connection->pendingAck = 0;
    return 1;
}

/* 0x8050E2B0 (0x130): drops the outgoing messages the peer acknowledged up to `serialNumber` and compacts the
 * outgoing buffer. */
s32 gti2RemoveAckedMessages(DWCiReq* connection, u16 serialNumber) {
    s32 count;
    s32 i;
    s32 shift;
    DWCiXfer* message;

    count = ArrayLength(connection->outgoingBufferMessages);
    if (count == 0) {
        return 1;
    }
    for (i = 0; i < count; i++) {
        message = (DWCiXfer*)ArrayNth(connection->outgoingBufferMessages, i);
        if ((s16)(message->serialNumber - serialNumber) >= 0) {
            break;
        }
    }
    if (i == 0) {
        return 1;
    }
    while (i--) {
        ArrayDeleteAt(connection->outgoingBufferMessages, i);
    }
    count = ArrayLength(connection->outgoingBufferMessages);
    if (count == 0) {
        connection->outgoingBuffer.len = 0;
        return 1;
    }
    shift = ((DWCiXfer*)ArrayNth(connection->outgoingBufferMessages, 0))->offset;
    for (i = 0; i < count; i++) {
        message = (DWCiXfer*)ArrayNth(connection->outgoingBufferMessages, i);
        message->offset -= shift;
    }
    gti2BufferShorten(&connection->outgoingBuffer, 0, shift);
    return 1;
}

/* Sends the client response: the response to the server's challenge and the initial message. */
static inline s32 gti2SendClientResponseInline(DWCiReq* connection, const u8* response, const u8* message,
                                               s32 len) {
    s32 overflow;
    if (!DWCi_requestBlockState(connection, GTI2_MSG_CLIENT_RESPONSE, len + connection->conn->protocolOffset + 0x27,
                                (u32*)&overflow)) {
        return 0;
    }
    if (overflow) {
        return 1;
    }
    DWCi_bufferFrame(&connection->outgoingBuffer, response, 0x20);
    DWCi_bufferFrame(&connection->outgoingBuffer, message, len);
    if (!gti2EndReliableMessageInline(connection)) {
        return 0;
    }
    return 1;
}

/* Sends the server challenge: the response to the client's challenge and our own challenge. */
static inline s32 gti2SendServerChallengeInline(DWCiReq* connection, const u8* response, const u8* challenge) {
    s32 overflow;
    if (!DWCi_requestBlockState(connection, GTI2_MSG_SERVER_CHALLENGE, connection->conn->protocolOffset + 0x47,
                                (u32*)&overflow)) {
        return 0;
    }
    if (overflow) {
        return 1;
    }
    DWCi_bufferFrame(&connection->outgoingBuffer, response, 0x20);
    DWCi_bufferFrame(&connection->outgoingBuffer, challenge, 0x20);
    if (!gti2EndReliableMessageInline(connection)) {
        return 0;
    }
    connection->challengeTime = connection->lastSend;
    return 1;
}

/* The two handshake handlers are defined ahead of their caller, as retail lays them out, and kept out of line. */
#pragma push
#pragma auto_inline off
/* 0x8050E3E0 (0x334): the server's challenge: checks its response, answers its challenge with the initial
 * message and waits for the accept. */
s32 gti2HandleServerChallenge(DWCiReq* connection, u8* message, s32 len) {
    u8 response[0x20];

    if (connection->state != GTI2_STATE_AWAITING_SERVER_CHALLENGE) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    if (len < 0x40) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    if (!gti2CheckResponse(message, connection->response)) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    gti2GetResponse(response, message + 0x20);
    if (!gti2SendClientResponseInline(connection, response, connection->initialMessage,
                                      connection->initialMessageLength)) {
        return 0;
    }
    if (connection->initialMessage != NULL) {
        gsifree(connection->initialMessage);
        connection->initialMessage = NULL;
    }
    connection->state = 1;
    return 1;
}

/* 0x8050E720 (0x308): the client's response: checks it, then refuses (no listener) or asks the listener. */
s32 gti2HandleClientResponse(DWCiReq* connection, u8* message, s32 len) {
    DWCiConn* socket;

    if (connection->state != 3) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    if (len < 0x20) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    if (!gti2CheckResponse(message, connection->response)) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    socket = connection->conn;
    if (socket->connectAttemptCallback == 0) {
        if (!gti2SocketSendClosedInline(socket, connection->addr, connection->port)) {
            return 0;
        }
        gti2ConnectionClosed(connection);
        return 1;
    }
    connection->state = GTI2_STATE_AWAITING_ACCEPT_REJECT;
    return gti2ConnectAttemptCallback(connection->conn, connection, connection->addr, connection->port,
                                      current_time() - connection->challengeTime, message + 0x20, len - 0x20) != 0;
}
#pragma pop

/* Delivers reliable application data (a connection not yet or no longer up fails instead). */
static inline s32 gti2HandleAppReliableInline(DWCiReq* connection, u8* message, s32 len) {
    s32 state = connection->state;

    if (state != GTI2_STATE_CONNECTED && state != GTI2_STATE_CLOSING) {
        if (!gti2ConnectionError(connection, 7, 2)) {
            return 0;
        }
    } else if (ArrayLength(connection->receiveFilters) != 0) {
        if (!gti2ReceiveFilterCallback(connection, 0, message, len, 1)) {
            return 0;
        }
    } else if (!gti2ReceivedCallback(connection, message, len, 1)) {
        return 0;
    }
    return 1;
}

/* Answers the client's challenge with ours and its response. */
static inline s32 gti2HandleClientChallengeInline(DWCiReq* connection, u8* message, s32 len) {
    u8 response[0x20];
    u8 challenge[0x20];

    if (connection->state != GTI2_STATE_AWAITING_CLIENT_CHALLENGE) {
        if (!gti2ConnectionError(connection, 7, 2)) {
            return 0;
        }
    } else if (len < 0x20) {
        if (!gti2ConnectionError(connection, 7, 2)) {
            return 0;
        }
    } else {
        gti2GetResponse(response, message);
        gti2GetChallenge(challenge);
        gti2GetResponse(connection->response, challenge);
        if (!gti2SendServerChallengeInline(connection, response, challenge)) {
            return 0;
        }
        connection->state = 3;
    }
    return 1;
}

/* The server accepted our attempt. */
static inline s32 gti2HandleAcceptInline(DWCiReq* connection) {
    if (connection->state != 1) {
        if (!gti2ConnectionError(connection, 7, 2)) {
            return 0;
        }
        return 1;
    }
    connection->state = GTI2_STATE_CONNECTED;
    return gti2ConnectedCallback(connection, 0, NULL, 0) != 0;
}

/* The server rejected our attempt. */
static inline s32 gti2HandleRejectInline(DWCiReq* connection, u8* message, s32 len) {
    if (connection->state != 1) {
        if (!gti2ConnectionError(connection, 7, 2)) {
            return 0;
        }
        return 1;
    }
    gti2ConnectionClosed(connection);
    if (!gti2SendClosedInline(connection)) {
        return 0;
    }
    return gti2ConnectedCallback(connection, 2, message, len) != 0;
}

/* The peer closes the connection. */
static inline s32 gti2HandleCloseInline(DWCiReq* connection) {
    if (!gti2SendClosedInline(connection)) {
        return 0;
    }
    return gti2ConnectionError(connection, 2, connection->state != GTI2_STATE_CLOSING) != 0;
}

/* 0x8050EA30 (0x7BC): one reliable message, in order: the application data or a handshake / close step. */
s32 gti2HandleReliableMessage(DWCiReq* connection, s32 type, u8* message, s32 len) {
    connection->expectedSerialNumber++;
    if (type == GTI2_MSG_APP_RELIABLE) {
        if (!gti2HandleAppReliableInline(connection, message, len)) {
            return 0;
        }
    } else if (type == GTI2_MSG_CLIENT_CHALLENGE) {
        if (!gti2HandleClientChallengeInline(connection, message, len)) {
            return 0;
        }
    } else if (type == GTI2_MSG_SERVER_CHALLENGE) {
        if (!gti2HandleServerChallenge(connection, message, len)) {
            return 0;
        }
    } else if (type == GTI2_MSG_CLIENT_RESPONSE) {
        if (!gti2HandleClientResponse(connection, message, len)) {
            return 0;
        }
    } else if (type == GTI2_MSG_ACCEPT) {
        if (!gti2HandleAcceptInline(connection)) {
            return 0;
        }
    } else if (type == GTI2_MSG_REJECT) {
        if (!gti2HandleRejectInline(connection, message, len)) {
            return 0;
        }
    } else if (type == GTI2_MSG_CLOSE) {
        if (!gti2HandleCloseInline(connection)) {
            return 0;
        }
    }
    return 1;
}

/* A reliable message buffered out of order: its place in the incoming buffer, its type and serial number. */
typedef struct GTI2IncomingBufferMessage {
    /* +0x00 */ s32 start;
    /* +0x04 */ s32 len;
    /* +0x08 */ s32 type;
    /* +0x0C */ u16 serialNumber;
    /* +0x0E */ u8 pad_0x0e[2];
} GTI2IncomingBufferMessage; /* size: 0x10 */

/* 0x8050F1F0 (0x14): orders buffered incoming messages by serial number. */
/* untyped: caller-owned payload - the two buffered message records */
int gti2IncomingBufferMessageCompare(const void* elem1, const void* elem2) {
    return (s16)(((const GTI2IncomingBufferMessage*)elem1)->serialNumber -
                 ((const GTI2IncomingBufferMessage*)elem2)->serialNumber);
}

/* 0x8050F210 (0x1AC): keeps an out-of-order reliable message until the gap before it is filled, asking the peer
 * for the missing ones; `*overflow` is set when it does not fit. */
s32 gti2BufferIncomingMessage(DWCiReq* connection, s32 type, u16 serialNumber, u8* message, s32 len,
                              s32* overflow) {
    GTI2IncomingBufferMessage newMessage;
    GTI2IncomingBufferMessage* bufferedMessage;
    s32 count;
    s32 i;

    count = ArrayLength(connection->incomingBufferMessages);
    for (i = 0; i < count; i++) {
        bufferedMessage = (GTI2IncomingBufferMessage*)ArrayNth(connection->incomingBufferMessages, i);
        if (bufferedMessage->serialNumber == serialNumber) {
            *overflow = 0;
            return 1;
        }
        if ((s16)(bufferedMessage->serialNumber - serialNumber) > 0) {
            break;
        }
    }
    if (gti2GetBufferFreeSpace(&connection->incomingBuffer) < len) {
        *overflow = 1;
        return 1;
    }
    newMessage.start = connection->incomingBuffer.len;
    newMessage.len = len;
    newMessage.type = type;
    newMessage.serialNumber = serialNumber;
    ArrayInsertSorted(connection->incomingBufferMessages, &newMessage, gti2IncomingBufferMessageCompare);
    if (count + 1 != ArrayLength(connection->incomingBufferMessages)) {
        *overflow = 1;
        return 1;
    }
    DWCi_bufferFrame(&connection->incomingBuffer, message, len);
    if (count == 0) {
        if (!gti2SendNack(connection, connection->expectedSerialNumber, (u16)(serialNumber - 1))) {
            return 0;
        }
    } else if (((GTI2IncomingBufferMessage*)ArrayNth(connection->incomingBufferMessages, count))->serialNumber ==
               serialNumber) {
        u16 previous =
            ((GTI2IncomingBufferMessage*)ArrayNth(connection->incomingBufferMessages, count - 1))->serialNumber;
        if ((u16)(serialNumber - previous) > 1 &&
            !gti2SendNack(connection, (u16)(previous + 1), (u16)(serialNumber - 1))) {
            return 0;
        }
    }
    *overflow = 0;
    return 1;
}

/* Delivers the buffered messages that are next in order. */
static inline s32 gti2DeliverBufferedMessagesInline(DWCiReq* connection) {
    s32 i;
    GTI2IncomingBufferMessage* bufferedMessage;

    for (;;) {
        for (i = ArrayLength(connection->incomingBufferMessages) - 1; i >= 0; i--) {
            bufferedMessage = (GTI2IncomingBufferMessage*)ArrayNth(connection->incomingBufferMessages, i);
            if (bufferedMessage->serialNumber == connection->expectedSerialNumber) {
                if (!gti2HandleReliableMessage(connection, bufferedMessage->type,
                                               connection->incomingBuffer.buffer + bufferedMessage->start,
                                               bufferedMessage->len)) {
                    return 0;
                }
                {
                    GTI2IncomingBufferMessage* other;
                    s32 shiftEnd = 0;
                    s32 end;
                    s32 count;
                    s32 len = bufferedMessage->len;
                    s32 start = bufferedMessage->start;

                    ArrayDeleteAt(connection->incomingBufferMessages, i);
                    count = ArrayLength(connection->incomingBufferMessages);
                    for (i = 0; i < count; i++) {
                        other = (GTI2IncomingBufferMessage*)ArrayNth(connection->incomingBufferMessages, i);
                        if (other->start > start) {
                            other->start -= len;
                            end = other->start + other->len;
                            if (shiftEnd > end) {
                                end = shiftEnd;
                            }
                            shiftEnd = end;
                        }
                    }
                    gti2BufferShorten(&connection->incomingBuffer, start, len);
                }
                break;
            }
        }
        if (i < 0) {
            return 1;
        }
    }
}

/* 0x8050F3C0 (0x450): a reliable packet: acknowledges our messages, then delivers it in order, buffers it or
 * drops a duplicate. */
s32 gti2HandleReliablePacket(DWCiReq* connection, s32 type, u8* message, s32 len) {
    DWCiConn* socket = connection->conn;
    s32 headerLength = socket->protocolOffset + 7;
    u8* header;
    u16 serialNumber;
    u16 ackSerialNumber;
    s32 dataLength;
    u8* data;
    s32 overflow;

    if (len < headerLength) {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    header = message + socket->protocolOffset;
    serialNumber = (header[3] << 8) | header[4];
    ackSerialNumber = (header[5] << 8) | header[6];
    if (socket->protocolType == GTI2_VDP_PROTOCOL && type == GTI2_MSG_APP_RELIABLE) {
        header[5] = message[0];
        message[connection->conn->protocolOffset + 6] = message[1];
        data = message + (headerLength - connection->conn->protocolOffset);
        dataLength = len - (headerLength - connection->conn->protocolOffset);
    } else {
        data = message + headerLength;
        dataLength = len - headerLength;
    }
    if (!gti2RemoveAckedMessages(connection, ackSerialNumber)) {
        return 0;
    }
    if (serialNumber == connection->expectedSerialNumber) {
        if (!connection->pendingAck) {
            connection->pendingAck = 1;
            connection->pendingAckTime = current_time();
        }
        if (!gti2HandleReliableMessage(connection, type, data, dataLength)) {
            return 0;
        }
        if (!gti2DeliverBufferedMessagesInline(connection)) {
            return 0;
        }
        return 1;
    }
    if ((s16)(serialNumber - connection->expectedSerialNumber) < 0) {
        if (!connection->pendingAck) {
            connection->pendingAck = 1;
            connection->pendingAckTime = current_time();
        }
        return 1;
    }
    if (!gti2BufferIncomingMessage(connection, type, serialNumber, data, dataLength, &overflow)) {
        return 0;
    }
    if (overflow) {
        if (!gti2SendClosedInline(connection)) {
            return 0;
        }
        if (!gti2ConnectionError(connection, 1, 4)) {
            return 0;
        }
    }
    return 1;
}

/* Writes `s` big-endian at `buffer[pos]`. */
static inline void gti2WriteUShortAt(u8* buffer, int pos, u16 s) {
    buffer[pos++] = (u8)((s >> 8) & 0xFF);
    buffer[pos] = (u8)(s & 0xFF);
}

/* Resends a buffered reliable message with the current ack number and stamps its send time (a resent server
 * challenge restarts the challenge clock); 0 when the send fails. */
static inline s32 gti2ResendMessageInline(DWCiReq* req, DWCiXfer* xfer) {
    int pos;
    int type;

    pos = xfer->offset + req->conn->protocolOffset + 5;
    gti2WriteUShortAt(req->outgoingBuffer.buffer, pos, req->expectedSerialNumber);
    if (!DWCi_requestFrame(req, req->outgoingBuffer.buffer + xfer->offset, xfer->length)) {
        return 0;
    }
    xfer->stamp = req->lastSend;
    pos = xfer->offset + req->conn->protocolOffset + 2;
    type = req->outgoingBuffer.buffer[pos];
    if (type == GTI2_MSG_SERVER_CHALLENGE) {
        req->challengeTime = req->lastSend;
    }
    return 1;
}

/* 0x8050F810 (0x1E8): the peer's nack: resends the messages in the reported serial range. */
s32 gti2HandleNack(DWCiReq* connection, u8* message, s32 len) {
    u16 start;
    u16 end;
    s32 count;
    s32 i;
    DWCiXfer* outgoing;

    start = (message[0] << 8) | message[1];
    if (len == 2) {
        end = start;
    } else if (len == 4) {
        end = (message[2] << 8) | message[3];
    } else {
        return gti2ConnectionError(connection, 7, 2) != 0;
    }
    count = ArrayLength(connection->outgoingBufferMessages);
    for (i = 0; i < count; i++) {
        outgoing = (DWCiXfer*)ArrayNth(connection->outgoingBufferMessages, i);
        if ((s16)(outgoing->serialNumber - start) >= 0 && (s16)(outgoing->serialNumber - end) <= 0) {
            if (!gti2ResendMessageInline(connection, outgoing)) {
                return 0;
            }
        }
    }
    return 1;
}

/* 0x8050FA00 (0x2C8): an unreliable protocol message: ack, nack, ping, pong or closed. */
/* Handles an acknowledgement: drops the messages it covers (a malformed one fails the connection). */
static inline s32 gti2HandleAckInline(DWCiReq* connection, u8* data, s32 dataLength) {
    if (dataLength != 2) {
        if (!gti2ConnectionError(connection, 7, 2)) {
            return 0;
        }
        return 1;
    }
    return gti2RemoveAckedMessages(connection, (data[0] << 8) | data[1]) != 0;
}

/* Handles a pong: reports the round trip of the "time" stamp it echoes. */
static inline s32 gti2HandlePongInline(DWCiReq* connection, u8* data, s32 dataLength) {
    u32 time;

    if (connection->callbacks.ping_0C == NULL) {
        return 1;
    }
    if ((u32)dataLength != 8) {
        return 1;
    }
    if (memcmp(data, "time", 4) != 0) {
        return 1;
    }
    memcpy(&time, data + 4, 4);
    return gti2PingCallback(connection, current_time() - time) != 0;
}

/* Handles the peer's closed notice. */
static inline s32 gti2HandleClosedInline(DWCiReq* connection) {
    s32 localClose;

    if (connection->state == GTI2_STATE_CLOSED) {
        return 1;
    }
    localClose = connection->state == GTI2_STATE_CLOSING;
    return gti2ConnectionError(connection, 2, !localClose) != 0;
}

s32 gti2HandleUnreliableMessage(DWCiReq* connection, s32 type, u8* message, s32 len) {
    s32 headerLength = connection->conn->protocolOffset + 3;
    u8* data = message + headerLength;
    s32 dataLength = len - headerLength;

    if (type == GTI2_MSG_ACK) {
        if (!gti2HandleAckInline(connection, data, dataLength)) {
            return 0;
        }
    } else if (type == GTI2_MSG_NACK) {
        if (!gti2HandleNack(connection, data, dataLength)) {
            return 0;
        }
    } else if (type == GTI2_MSG_PING) {
        message[2] = GTI2_MSG_PONG;
        if (!DWCi_requestFrame(connection, message, len)) {
            return 0;
        }
    } else if (type == GTI2_MSG_PONG) {
        if (!gti2HandlePongInline(connection, data, dataLength)) {
            return 0;
        }
    } else if (type == GTI2_MSG_CLOSED) {
        if (!gti2HandleClosedInline(connection)) {
            return 0;
        }
    }
    return 1;
}

/* 0x8050FCD0 (0x48C): one datagram from `ip:port`: a new connection attempt, a protocol message of a known
 * connection, or application data. */
/* Hands an application message to the receive filters or the received callback. */
static inline s32 gti2DeliverInline(DWCiReq* connection, u8* message, s32 len) {
    if (connection->state == GTI2_STATE_CONNECTED || connection->state == GTI2_STATE_CLOSING) {
        if (ArrayLength(connection->receiveFilters) != 0) {
            if (!gti2ReceiveFilterCallback(connection, 0, message, len, 0)) {
                return 0;
            }
            return 1;
        }
        return gti2ReceivedCallback(connection, message, len, 0) != 0;
    }
    return 1;
}

s32 gti2HandleMessage(DWCiConn* socket, u8* message, s32 len, u32 ip, u16 port) {
    DWCiReq* connection;
    s32 handled;
    s32 isProtocol;
    s32 headerLength = len - socket->protocolOffset;
    u8* header = message + socket->protocolOffset;
    s32 result;
    s32 type;

    connection = (DWCiReq*)DWCi_findRequest(socket, ip, port);
    if (socket->receiveDumpCallback != 0 &&
        !DWCi_requestSend(socket, connection, ip, port, 0, message, len, 0)) {
        return 0;
    }
    isProtocol = 0;
    if (headerLength > 2 && memcmp(header, &DWCi_protocolMagic, GTI2_MAGIC_LEN) == 0) {
        isProtocol = 1;
    }
    if (connection == NULL) {
        if (!gti2UnrecognizedMessageCallback(socket, ip, port, message, len, &handled)) {
            return 0;
        }
        if (handled) {
            return 1;
        }
        if (!isProtocol || header[2] != GTI2_MSG_CLIENT_CHALLENGE) {
            if (!isProtocol || header[2] != GTI2_MSG_CLOSED) {
                if (!gti2SocketSendClosedInline(socket, ip, port)) {
                    return 0;
                }
            }
            return 1;
        }
        if (socket->connectAttemptCallback == 0) {
            return 1;
        }
        result = gti2NewIncomingConnection(socket, &connection, ip, port);
        if (result != 0) {
            if (result != 5) {
                if (!gti2SocketSendClosedInline(socket, ip, port)) {
                    return 0;
                }
            }
            return 1;
        }
    }
    if (connection->state == GTI2_STATE_CLOSED) {
        if (!isProtocol || header[2] != GTI2_MSG_CLOSED) {
            if (!gti2SendClosedInline(connection)) {
                return 0;
            }
        }
        return 1;
    }
    if (isProtocol && headerLength >= 4 && memcmp(header + 2, &DWCi_protocolMagic, GTI2_MAGIC_LEN) == 0) {
        isProtocol = 0;
        header += 2;
        message[3] = message[1];
        message[2] = message[0];
        len -= 2;
        message += 2;
    }
    if (!isProtocol) {
        if (connection->state < GTI2_STATE_CONNECTED) {
            if (!gti2UnrecognizedMessageCallback(socket, ip, port, message, len, &handled)) {
                return 0;
            }
            return 1;
        }
        if (!gti2DeliverInline(connection, message, len)) {
            return 0;
        }
        return 1;
    }
    type = header[2];
    if (type < 8) {
        return gti2HandleReliablePacket(connection, type, message, len) != 0;
    }
    return gti2HandleUnreliableMessage(connection, type, message, len) != 0;
}

/* 0x80510160 (0x218): the peer reset the connection (an attempt that is still being retried times out). */
int DWCi_requestReconnect(DWCiConn* conn, u32 addr, u16 port) {
    DWCiReq* connection = (DWCiReq*)DWCi_findRequest(conn, addr, port);

    if (conn->receiveDumpCallback != 0 && !DWCi_requestSend(conn, connection, addr, port, 1, NULL, 0, 0)) {
        return 0;
    }
    if (connection == NULL) {
        return 1;
    }
    if (connection->state == GTI2_STATE_AWAITING_SERVER_CHALLENGE) {
        if (connection->timeout == 0 || current_time() - connection->startTime < connection->timeout) {
            return 1;
        }
        if (!gti2ConnectionError(connection, 6, 1)) {
            return 0;
        }
    } else {
        if (!gti2ConnectionError(connection, 2, 1)) {
            return 0;
        }
    }
    return 1;
}

/* 0x80510380 (0x130): the peer is unreachable. */
int DWCi_requestRetry(DWCiConn* conn, u32 addr, u16 port, u32 flag) {
    DWCiReq* connection = (DWCiReq*)DWCi_findRequest(conn, addr, port);

    if (conn->receiveDumpCallback != 0 && !DWCi_requestSend(conn, connection, addr, port, 1, NULL, 0, flag)) {
        return 0;
    }
    if (connection == NULL) {
        return 1;
    }
    return gti2ConnectionError(connection, 6, 1) != 0;
}

/* 0x805104B0 (0x3FC): reads every waiting datagram and handles it (a reset or unreachable peer is reported to its
 * connection, any other socket error shuts the socket down). */
s32 gti2ReceiveMessages(DWCiConn* socket) {
    SOSockAddrIn from;
    int fromLength;
    int len;
    u16 port;
    s32 error;

    while (DWCi_socketHasData(socket->sock)) {
        fromLength = 8;
        len = DWCi_socketRecvFrom(socket->sock, DWCi_gt2ReceiveBuffer, 0x1000, 0, &from, &fromLength);
        if (len == -1) {
            error = DWCi_socketGetLastError(socket->sock);
            if (error == -15) {
                port = SOAddressToHostPort(from.port);
                if (!DWCi_requestReconnect(socket, from.addr, port)) {
                    return 0;
                }
            } else if (error == -23) {
                port = SOAddressToHostPort(from.port);
                if (!DWCi_requestRetry(socket, from.addr, port, 0)) {
                    return 0;
                }
            } else if (error != -35) {
                DWCi_connectionShutdown(socket);
                return 0;
            }
        } else if (!gti2HandleMessage(socket, DWCi_gt2ReceiveBuffer, len, from.addr, SOAddressToHostPort(from.port))) {
            return 0;
        }
    }
    return 1;
}

/* Ends the connection when its outgoing buffer is full: closed to the peer, out-of-memory locally. */
static inline s32 gti2OutgoingBufferFullInline(DWCiReq* connection) {
    if (!gti2SendClosedInline(connection)) {
        return 0;
    }
    return gti2ConnectionError(connection, 1, 4);
}

/* 0x805108B0 (0x394): starts a reliable message of `type` and `len` bytes in the outgoing buffer (header and
 * bookkeeping); `*overflow` is set when it does not fit. */
int DWCi_requestBlockState(DWCiReq* req, u32 type, u32 len, u32* overflow) {
    DWCiXfer message;
    s16 vdpLength = len - req->conn->protocolOffset;
    s32 count;
    u16 serialNumber;

    if (gti2GetBufferFreeSpace(&req->outgoingBuffer) < (s32)len) {
        if (!gti2OutgoingBufferFullInline(req)) {
            return 0;
        }
        *overflow = 1;
        return 1;
    }
    serialNumber = req->serialNumber;
    memset(&message, 0, sizeof(message));
    message.offset = req->outgoingBuffer.len;
    message.length = len;
    message.serialNumber = serialNumber;
    message.stamp = current_time();
    count = ArrayLength(req->outgoingBufferMessages);
    ArrayAppend(req->outgoingBufferMessages, &message);
    if (count + 1 != ArrayLength(req->outgoingBufferMessages)) {
        if (!gti2OutgoingBufferFullInline(req)) {
            return 0;
        }
        *overflow = 1;
        return 1;
    }
    if (req->conn->protocolType == GTI2_VDP_PROTOCOL) {
        DWCi_bufferFrame(&req->outgoingBuffer, (const u8*)&vdpLength, req->conn->protocolOffset);
    }
    DWCi_bufferFrame(&req->outgoingBuffer, (const u8*)&DWCi_protocolMagic, GTI2_MAGIC_LEN);
    gti2BufferWriteByte(&req->outgoingBuffer, type);
    gti2BufferWriteUShort(&req->outgoingBuffer, req->serialNumber++);
    gti2BufferWriteUShort(&req->outgoingBuffer, req->expectedSerialNumber);
    *overflow = 0;
    return 1;
}

/* 0x80510C50 (0xE4): sends the client challenge. */
s32 gti2SendClientChallenge(DWCiReq* connection, const u8* challenge) {
    s32 overflow;
    if (!DWCi_requestBlockState(connection, GTI2_MSG_CLIENT_CHALLENGE, connection->conn->protocolOffset + 0x27,
                                (u32*)&overflow)) {
        return 0;
    }
    if (overflow) {
        return 1;
    }
    DWCi_bufferFrame(&connection->outgoingBuffer, challenge, 0x20);
    if (!gti2EndReliableMessageInline(connection)) {
        return 0;
    }
    return 1;
}

/* 0x80510D40 (0xC8): sends the accept. */
s32 gti2SendAccept(DWCiReq* connection) {
    s32 overflow;
    if (!DWCi_requestBlockState(connection, GTI2_MSG_ACCEPT, connection->conn->protocolOffset + 7, (u32*)&overflow)) {
        return 0;
    }
    if (overflow) {
        return 1;
    }
    if (!gti2EndReliableMessageInline(connection)) {
        return 0;
    }
    return 1;
}

/* 0x80510E10 (0xF4): sends the reject with its message. */
s32 gti2SendReject(DWCiReq* connection, const u8* message, s32 len) {
    s32 overflow;
    if (!DWCi_requestBlockState(connection, GTI2_MSG_REJECT, len + connection->conn->protocolOffset + 7,
                                (u32*)&overflow)) {
        return 0;
    }
    if (overflow) {
        return 1;
    }
    DWCi_bufferFrame(&connection->outgoingBuffer, message, len);
    if (!gti2EndReliableMessageInline(connection)) {
        return 0;
    }
    return 1;
}

/* 0x80510F10 (0xC8): sends the close. */
s32 gti2SendClose(DWCiReq* connection) {
    s32 overflow;
    if (!DWCi_requestBlockState(connection, GTI2_MSG_CLOSE, connection->conn->protocolOffset + 7, (u32*)&overflow)) {
        return 0;
    }
    if (overflow) {
        return 1;
    }
    if (!gti2EndReliableMessageInline(connection)) {
        return 0;
    }
    return 1;
}

/* 0x80510FE0 (0xC8): sends a keep-alive. */
s32 gti2SendKeepAlive(DWCiReq* connection) {
    s32 overflow;
    if (!DWCi_requestBlockState(connection, GTI2_MSG_KEEP_ALIVE, connection->conn->protocolOffset + 7,
                                (u32*)&overflow)) {
        return 0;
    }
    if (overflow) {
        return 1;
    }
    if (!gti2EndReliableMessageInline(connection)) {
        return 0;
    }
    return 1;
}

/* 0x805110B0 (0x12C): sends application data unreliably, escaping a payload that starts with the magic. */
int DWCi_requestFlush(DWCiReq* req, u8* message, s32 len) {
    s32 total;
    u8* start;

    if (len < 2 || memcmp(message + req->conn->protocolOffset, &DWCi_protocolMagic, GTI2_MAGIC_LEN) != 0) {
        if (!DWCi_requestFrame(req, message, len)) {
            return 0;
        }
        return 1;
    }
    total = len + 2;
    if (gti2GetBufferFreeSpace(&req->outgoingBuffer) < total) {
        return 1;
    }
    start = req->outgoingBuffer.buffer + req->outgoingBuffer.len;
    if (req->conn->protocolType == GTI2_VDP_PROTOCOL) {
        DWCi_bufferFrame(&req->outgoingBuffer, message, 2);
    }
    DWCi_bufferFrame(&req->outgoingBuffer, (const u8*)&DWCi_protocolMagic, GTI2_MAGIC_LEN);
    DWCi_bufferFrame(&req->outgoingBuffer, message + req->conn->protocolOffset, len - req->conn->protocolOffset);
    if (!DWCi_requestFrame(req, start, total)) {
        return 0;
    }
    gti2BufferShorten(&req->outgoingBuffer, -1, total);
    return 1;
}

/* 0x805111E0 (0xC4): acknowledges everything received so far. */
s32 gti2SendAck(DWCiReq* connection) {
    u8 buffer[7];
    s32 pos = 0;
    s16 vdpLength;

    if (connection->conn->protocolType == GTI2_VDP_PROTOCOL) {
        vdpLength = 5;
        memcpy(buffer, &vdpLength, 2);
        pos = 2;
    }
    memcpy(&buffer[pos], &DWCi_protocolMagic, GTI2_MAGIC_LEN);
    pos += GTI2_MAGIC_LEN;
    buffer[pos++] = GTI2_MSG_ACK;
    gti2WriteUShortAt(buffer, pos, connection->expectedSerialNumber);
    pos += 2;
    if (!DWCi_requestFrame(connection, buffer, pos)) {
        return 0;
    }
    connection->pendingAck = 0;
    return 1;
}

/* 0x805112B0 (0xF4): asks the peer to resend the serial range `start`..`end`. */
s32 gti2SendNack(DWCiReq* connection, u16 start, u16 end) {
    u8 buffer[9];
    s32 pos = 0;
    s16 vdpLength;

    if (connection->conn->protocolType == GTI2_VDP_PROTOCOL) {
        vdpLength = 7;
        memcpy(buffer, &vdpLength, 2);
        pos = 2;
    }
    memcpy(&buffer[pos], &DWCi_protocolMagic, GTI2_MAGIC_LEN);
    pos += GTI2_MAGIC_LEN;
    buffer[pos++] = GTI2_MSG_NACK;
    gti2WriteUShortAt(buffer, pos, start);
    pos += 2;
    if (start != end) {
        gti2WriteUShortAt(buffer, pos, end);
        pos += 2;
    }
    if (!DWCi_requestFrame(connection, buffer, pos)) {
        return 0;
    }
    return 1;
}

/* 0x805113B0 (0xB8): sends the "closed" message to the connection's peer. */
u32 DWCi_sendControlFrame(DWCiReq* req)
{
    return gti2SendClosedInline(req);
}

/* 0x80511470 (0xBC): resends a buffered reliable message. */
u32 DWCi_appendTransfer(DWCiReq* req, DWCiXfer* xfer)
{
    return gti2ResendMessageInline(req, xfer);
}

/* 0x80511530 (0xF0): sends a message: reliably through the outgoing buffer, or unreliably through
 * `DWCi_requestFlush`. */
u32 DWCi_flushRequest(DWCiReq* req, u32 a, u32 b, u32 flag)
{
    u32 overflow;

    if (flag != 0) {
        if (DWCi_requestBlockState(req, 0, b + 7, &overflow) == 0) {
            return 0;
        }
        if (overflow != 0) {
            return 1;
        }
        DWCi_bufferFrame(&req->outgoingBuffer, (const u8*)a, b);
        return gti2EndReliableMessageInline(req) != 0;
    }
    return DWCi_requestFlush(req, (u8*)a, b);
}
