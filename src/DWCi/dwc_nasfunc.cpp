/*
 * DWCi/dwc_nasfunc.cpp - the DWC NAS login / service locator front end and the GameSpy common code the DWC library
 * links behind it: the auth request callback, the dynamic array and hash table, the socket and platform wrappers,
 * the memory hooks, the availability probe, the GT2 transport (API, connection, callbacks, buffer, messages).
 *
 * RANGE. `.text` 0x80509DB0..0x805113B0 (117 functions); `.data` 0x806307F0..0x806308A8; `.bss`
 *   0x807613B8..0x807614D8; `.sdata` 0x807942FC..0x80794358; `.sbss` 0x807957F8..0x80795820.
 * FLAGS. the `DWCi` lib's `cflags_dwc`. The retail functions start 16-byte aligned (`gap_*` words between them, and
 *   `gti2CheckResponse`'s loop-alignment `nop`), which `-func_align 4` does not reproduce.
 * NAMES. GameSpy SDK names where the body is the SDK's (darray `Array*`, hashtable `Table*`, gsMemory `gsi*`,
 *   gsPlatform `current_time`/`msleep`/`SocketStartUp`/`SocketShutDown`, GT2 `gt2*`/`gti2*`); `DWCi_Auth_EndProcess`
 *   is the function's own report string; the remaining `DWCi_*` names (socket wrappers, request/buffer rows named
 *   before the GT2 identification, the auth callback, `DWCi_Auth_CheckNandResult`) are GUESSes from their bodies.
 * SHAPES. The retail object is twelve SDK files (auth callback | nas | svl | darray | hashtable | socket | platform
 *   | memory | available | gt2 auth | gt2 callback/connection/main/message | buffer). MWCC inlines any function
 *   defined earlier in one TU, so the source is ordered callers-first across those file boundaries (the auth
 *   callback, then GT2 main, connection, message, the exported `gti2ConnectionClosed`/`gti2ConnectionSendData`
 *   shapes, callbacks, buffer; the availability probe ahead of the socket layer) and the hashtable sits in a
 *   `dont_inline` block; same-file helpers the retail inlines are `static inline` (`ArrayNthInline`,
 *   `gti2ConnectionError`, `gti2SendClosedInline`, `gti2EndReliableMessageInline`, ...). Socket results go through
 *   `DWCi_socketResult`; POST/answer fields through `DWCI_AUTH_DECODE`.
 * RESIDUALS. `DWCi_Auth_RequestCallback` addresses `DWCi/DWCi_Np_CPUCopyFast.c`'s string pool: the retail auth TU
 *   extends to 0x8050A710, so this unit's first row belongs to it (not re-cut) and its strings land in this object's
 *   `.data`. `DWC_SVLProcess` copies the 0x174-byte
 *   result member-wise because the unit compiles as C++ (request net-c#10). The availability probe's three objects
 *   live in `DWCi/DWCi_NatNeg.c`'s `.bss`, so `DWCi_natProbeStart` cannot share one base register for them.
 *   Register colouring and block order remain in the larger GT2 message handlers and the socket resolver.
 */
#include "DWCi/dwc_nasfunc.h"
#include "DWCi/dwc_error.h"
#include "DWCi/DWCi_Np_CPUCopyFast.h"
#include "NAND/nand.h"
#include "unsplit/OS.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "Runtime.PPCEABI.H/memset.h"
#include "unsplit/Runtime.PPCEABI.H.h"
#include "MSL_C/alloc.h"
#include "MSL_C/strstr.h"
#include "nw4r/DWCi_Base64Encode.h"
#include "DWCi/fn_805113B0.h"

extern "C" {




/* The unit's GT2 functions, declared ahead of the GT2 block (it is defined callers first, see below). */
s32 gti2GetBufferFreeSpace(const GTI2Buffer* buffer);
void gti2BufferWriteByte(GTI2Buffer* buffer, u8 b);
void gti2BufferWriteUShort(GTI2Buffer* buffer, u16 s);
void gti2BufferShorten(GTI2Buffer* buffer, s32 start, s32 shortenBy);
s32 gti2ConnectAttemptCallback(DWCiConn* socket, DWCiReq* connection, u32 ip, u16 port, s32 latency, u8* message,
                               s32 len);
s32 gti2ConnectedCallback(DWCiReq* connection, s32 result, u8* message, s32 len);
s32 gti2ReceivedCallback(DWCiReq* connection, u8* message, s32 len, s32 reliable);
s32 gti2ClosedCallback(DWCiReq* connection, s32 reason);
s32 gti2PingCallback(DWCiReq* connection, s32 latency);
s32 gti2SendFilterCallback(DWCiReq* connection, s32 filterId, u8* message, s32 len, s32 reliable);
s32 gti2ReceiveFilterCallback(DWCiReq* connection, s32 filterId, u8* message, s32 len, s32 reliable);
s32 gti2UnrecognizedMessageCallback(DWCiConn* socket, u32 ip, u16 port, u8* message, s32 len, s32* handled);
s32 gti2NewOutgoingConnection(DWCiConn* socket, DWCiReq** connection, u32 ip, u16 port);
s32 gti2NewIncomingConnection(DWCiConn* socket, DWCiReq** connection, u32 ip, u16 port);
s32 gti2StartConnectionAttempt(DWCiReq* connection, u8* message, s32 len, const GT2ConnectionCallbacks* callbacks);
s32 gti2AcceptConnection(DWCiReq* connection, const GT2ConnectionCallbacks* callbacks);
void gti2RejectConnection(DWCiReq* connection, u8* message, s32 len);
s32 gti2CheckTimeout(DWCiReq* connection, u32 now);
void gti2CloseConnection(DWCiReq* connection, s32 hard);
void gti2ConnectionClosed(DWCiReq* connection);
/* untyped: caller-owned payload - the table element and the client data */
void gti2CloseAllConnectionsHardMap(void* elem, void* clientData);
s32 gti2ReceiveMessages(DWCiConn* socket);
s32 gti2SendClientChallenge(DWCiReq* connection, const u8* challenge);
s32 gti2SendAccept(DWCiReq* connection);
s32 gti2SendReject(DWCiReq* connection, const u8* message, s32 len);
s32 gti2SendClose(DWCiReq* connection);
s32 gti2SendKeepAlive(DWCiReq* connection);
s32 gti2SendAck(DWCiReq* connection);
u8* gti2GetChallenge(u8* buffer);
s32 gti2RemoveAckedMessages(DWCiReq* connection, u16 serialNumber);
s32 gti2HandleServerChallenge(DWCiReq* connection, u8* message, s32 len);
s32 gti2HandleClientResponse(DWCiReq* connection, u8* message, s32 len);
/* untyped: caller-owned payload - the two buffered message records */
int gti2IncomingBufferMessageCompare(const void* elem1, const void* elem2);
s32 gti2HandleReliableMessage(DWCiReq* connection, s32 type, u8* message, s32 len);
s32 gti2BufferIncomingMessage(DWCiReq* connection, s32 type, u16 serialNumber, u8* message, s32 len,
                              s32* overflow);
s32 gti2HandleReliablePacket(DWCiReq* connection, s32 type, u8* message, s32 len);
s32 gti2HandleNack(DWCiReq* connection, u8* message, s32 len);
s32 gti2HandleUnreliableMessage(DWCiReq* connection, s32 type, u8* message, s32 len);
s32 gti2HandleMessage(DWCiConn* socket, u8* message, s32 len, u32 ip, u16 port);
s32 gti2SendNack(DWCiReq* connection, u16 start, u16 end);
u8* gti2GetResponse(u8* buffer, const u8* challenge);
int gti2CheckResponse(const u8* response1, const u8* response2);

/* ---- gsMemory: the four callbacks `gsiMemoryCallbacksSet` installs ------------------------------------ */

/* The GameSpy allocator hooks (`gsiMemoryCallbacksSet` stores, `gsimalloc`/`gsifree`/`gsirealloc` call). */
typedef struct GSIMemoryCallbacks {
    /* +0x00 */ GSIMallocFn mallocFn;
    /* +0x04 */ GSIFreeFn freeFn;
    /* +0x08 */ GSIReallocFn reallocFn;
    /* +0x0C */ GSIMemalignFn memalignFn;
} GSIMemoryCallbacks; /* size: 0x10 */

GSIMemoryCallbacks gsiMemoryCallbacks;

s32 DWCi_nasLoginState;
DWCSvlResult* DWCi_svlResult;

/* The resolver cache (31 slots of `DWCiHostCacheEntry`), the empty alias list of the local host record and the
 * last socket error. */
struct DWCiHostCacheEntry** DWCi_hostCache;
char* DWCi_localHostAliases;
s32 gsiSocketError;

/* The local host record `DWCi_socketGetLocalHostEntry` builds: the record, its address pointers and the up to 16
 * 12-byte interface address records SO answers. */
DWCiHostEntry DWCi_localHostEntry;
DWCiHostAddr* DWCi_localAddressList[16];
u8 DWCi_localAddresses[16][12];

/* The GT2 challenge secret `gti2GetResponse` mixes into every response. */
char DWCi_gt2SecretKey[] = "3b8dd8995f7c40a9a5c5b7dd5b481341";

/* ---- the auth request callback (retail: the auth interface's TU, see the unit header) --------------- */

/* Base64-decodes the value after `key` in `token` into `value` (NUL-terminated); answers the decoded length. */
#define DWCI_AUTH_DECODE(key)                                                                     \
    (decoded = DWCi_Base64Decode(&token[strlen(key)], strlen(token) - strlen(key), value, 0x100), \
     value[decoded] = 0, decoded)

/* 0x80509DB0 (0x960): the completion callback of the NAS request: keeps the response, then parses the
 * "key=value" lines (each value base64) into the state block and sets the request status. */
void DWCi_Auth_RequestCallback(s32 result, struct NHTTPResponse* response, s32 size) {
    DWCiStateBlock* state = (DWCiStateBlock*)DWCi_stateBlock;
    char returnCode[8];
    char* body;
    char* token;
    OSCalendarTime calendar;
    char value[0x100];
    s32 decoded;
    s32 haveReturnCode;
    s64 serverTime;
    s32 code;

    if (state->response != NULL) {
        DWC_Printf(0x1000000, DWCi_reportHttpDestroy);
        NHTTPDestroyResponse(state->response);
    }
    state->response = response;
    DWC_Printf(0x1000000, " request_callback = %d\n", result);
    if (result == 8) {
        DWC_Printf(0x1000000, " nhttp canceled(%d)\n", result);
        state->status = 2;
        return;
    }
    if (result != 0) {
        if (result == 14) {
            DWC_Printf(0x1000000, " ssl error(%d)\n", NHTTPGetSSLError());
        }
        DWC_Printf(0x1000000, " nhttp error(%d)\n", result);
        state->status = -20100;
        state->saveImage.requestFailed = 1;
        return;
    }
    code = NHTTPGetResultCode(response);
    if (code != 200) {
        DWC_Printf(0x1000000, " status code is not 200, but %d\n", code);
        state->status = -23000 - code;
        if (NHTTPGetBodyAll(response, (u32*)&body) > 0) {
            DWC_Printf(0x1000000, "%s\n", body);
        } else {
            DWC_Printf(0x1000000, "no body\n", body);
        }
        return;
    }
    haveReturnCode = 0;
    if (NHTTPGetBodyAll(response, (u32*)&body) > 0) {
        DWC_Printf(0x1000000, " %s\n", body);
        for (token = strtok(body, "&\r\n"); token != NULL; token = strtok(NULL, "&\r\n")) {
            if (strncmp(token, "retry=", strlen("retry=")) == 0) {
                DWC_Printf(0x1000000, " (%d) retry=%s\n", DWCI_AUTH_DECODE("retry="), value);
            } else if (strncmp(token, "returncd=", strlen("returncd=")) == 0) {
                decoded = DWCI_AUTH_DECODE("returncd=");
                strcpy(returnCode, value);
                DWC_Printf(0x1000000, " (%d) returncd=%s\n", decoded, value);
                haveReturnCode = 1;
            } else if (strncmp(token, "datetime=", strlen("datetime=")) == 0) {
                decoded = DWCI_AUTH_DECODE("datetime=");
                if (sscanf(value, "%04d%02d%02d%02d%02d%02d", &calendar.year, &calendar.mon, &calendar.mday,
                           &calendar.hour, &calendar.min, &calendar.sec) != 6) {
                    DWC_Printf(0x1000000, " cannot parse datetime: %s\n", value);
                    serverTime = 0;
                } else {
                    calendar.mon -= 1;
                    calendar.wday = 0;
                    calendar.yday = 0;
                    calendar.msec = 0;
                    calendar.usec = 0;
                    serverTime = OSCalendarTimeToTicks(&calendar);
                }
                state->serverTimeOffset = serverTime - OSGetTime();
                DWC_Printf(0x1000000, " (%d) datetime=%s\n", decoded, value);
            } else if (strncmp(token, "locator=", strlen("locator=")) == 0) {
                decoded = DWCI_AUTH_DECODE("locator=");
                strcpy(state->locator, value);
                DWC_Printf(0x1000000, " (%d) locator=%s\n", decoded, value);
            } else if (strncmp(token, "token=", strlen("token=")) == 0) {
                decoded = DWCI_AUTH_DECODE("token=");
                strcpy(state->token, value);
                DWC_Printf(0x1000000, " (%d) token=%s\n", decoded, value);
            } else if (strncmp(token, "challenge=", strlen("challenge=")) == 0) {
                decoded = DWCI_AUTH_DECODE("challenge=");
                strcpy(state->challenge, value);
                DWC_Printf(0x1000000, " (%d) challenge=%s\n", decoded, value);
            } else if (strncmp(token, "userid=", strlen("userid=")) == 0) {
                decoded = DWCI_AUTH_DECODE("userid=");
                sscanf(value, "%llu", &state->userId);
                DWC_Printf(0x1000000, " (%d) userid=%llu\n", decoded);
                state->saveImage.userId = state->userId;
            } else if (strncmp(token, "svchost=", strlen("svchost=")) == 0) {
                decoded = DWCI_AUTH_DECODE("svchost=");
                strcpy(state->svl.svlhost, value);
                DWC_Printf(0x1000000, " (%d) svlhost=%s\n", decoded, value);
            } else if (strncmp(token, "servicetoken=", strlen("servicetoken=")) == 0) {
                decoded = DWCI_AUTH_DECODE("servicetoken=");
                strcpy(state->svl.svltoken, value);
                DWC_Printf(0x1000000, " (%d) servicetoken=%s\n", decoded, value);
            } else if (strncmp(token, "statusdata=", strlen("statusdata=")) == 0) {
                decoded = DWCI_AUTH_DECODE("statusdata=");
                if (value[0] == 'Y') {
                    state->svl.status = 1;
                } else {
                    state->svl.status = 0;
                }
                DWC_Printf(0x1000000, " (%d) statusdata=%s\n", decoded, value);
            } else if (strncmp(token, "prwords=", strlen("prwords=")) == 0) {
                DWCI_AUTH_DECODE("prwords=");
                strcpy(state->saveImage.profileWords, value);
            } else {
                DWC_Printf(0x1000000, " unknown token : %s\n", token);
            }
        }
    }
    if (!haveReturnCode) {
        DWC_Printf(0x1000000, " no return code.\n");
        state->status = -20101;
        return;
    }
    code = strtol(returnCode, NULL, 10);
    if (code == 0 && DWCi_runtime->mode_0x59C8 != 3) {
        DWC_Printf(0x1000000, " cannot parse returncd(%s)\n", returnCode);
        state->status = -20101;
        return;
    }
    if (code == 1 && DWCi_runtime->mode_0x59C8 == 3) {
        DWC_Printf(0x1000000, " prof server maintenance(%s)\n", returnCode);
        state->status = -33001;
        return;
    }
    if (code >= 100) {
        if (DWCi_runtime->mode_0x59C8 == 3) {
            DWC_Printf(0x1000000, " prof server retruns error (%d) but ignored by DWC library\n", code);
        } else {
            DWC_Printf(0x1000000, " server retruns error (%d)\n", code);
            state->status = -20000 - code;
            return;
        }
    }
    if (code == 40) {
        state->loginKind = 2;
    } else {
        state->loginKind = 1;
    }
    state->status = 1;
}

/* ---- GT2 (the GameSpy reliable UDP transport): the public API, the connection, the callbacks, the buffer -- */
/* The retail GT2 files are separate TUs that call each other and the darray/challenge code; they are defined
 * here ahead of their callees so MWCC does not inline across those TU boundaries. */

#define GTI2_STATE_AWAITING_SERVER_CHALLENGE 0
#define GTI2_STATE_AWAITING_CLIENT_CHALLENGE 2
#define GTI2_STATE_AWAITING_ACCEPT_REJECT 4
#define GTI2_STATE_CONNECTED 5
#define GTI2_STATE_CLOSING 6
#define GTI2_STATE_CLOSED 7

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

/* 0x8050DEC0 (0x8): creates a GT2 socket bound to `localAddress` (UDP protocol). */
s32 gt2CreateSocket(GT2Socket* socket, const char* localAddress, u32 outgoingBufferSize, u32 incomingBufferSize,
                    NetworkCallback socketErrorCallback) {
    DWCi_createConnection((DWCiConn**)socket, (char*)localAddress, outgoingBufferSize, incomingBufferSize,
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
    u8* data = (u8*)message;
    s32 len = length;
    u16 vdpLength;

    if (conn->state != GTI2_STATE_CONNECTED) {
        return 8;
    }
    DWCi_GetStringLength((void**)&data, (int*)&len);
    if (reliable && conn->conn->protocolType == 2) {
        memcpy(&vdpLength, data, 2);
        if (len != vdpLength + conn->conn->protocolOffset) {
            return 9;
        }
    }
    if (ArrayLength(conn->sendFilters) != 0) {
        gti2SendFilterCallback(conn, 0, data, len, reliable);
        return 0;
    }
    return DWCi_flushRequest(conn, (u32)data, len, reliable) ? 0 : 10;
}

/* 0x8050E250 (0x8): starts a soft close of the connection. */
void gt2CloseConnection(GT2Connection connection) {
    gti2CloseConnection((DWCiReq*)connection, 0);
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

/* ---- gt2Connection ------------------------------------------------------------------------------------ */

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
int DWCi_requestIsTimedOut(DWCiReq* req, u32 now) {
    s32 count;
    s32 i;
    DWCiXfer* message;

    if (!gti2CheckTimeout(req, now)) {
        return 0;
    }
    if (now - req->lastSend > 30000 && !gti2SendKeepAlive(req)) {
        return 0;
    }
    count = ArrayLength(req->outgoingBufferMessages);
    for (i = 0; i < count; i++) {
        message = (DWCiXfer*)ArrayNth(req->outgoingBufferMessages, i);
        if (now - message->stamp > 1000 && !DWCi_appendTransfer(req, message)) {
            return 0;
        }
    }
    if (req->pendingAck && now - req->pendingAckTime > 100 && !gti2SendAck(req)) {
        return 0;
    }
    return 1;
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

/* ---- gt2Message: the GT2 protocol messages ---------------------------------------------------------- */

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
    u8 buffer[8];
    s32 pos = 0;
    s16 vdpLength;

    if (socket->protocolType == GTI2_VDP_PROTOCOL) {
        vdpLength = 3;
        memcpy(buffer, &vdpLength, 2);
        pos = 2;
    }
    memcpy(&buffer[pos], &DWCi_protocolMagic, GTI2_MAGIC_LEN);
    buffer[pos + 2] = GTI2_MSG_CLOSED;
    return DWCi_sendTo(socket, ip, port, buffer, pos + 3) != 0;
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
        if ((s16)(message->field_0x08 - serialNumber) >= 0) {
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
    return gti2EndReliableMessageInline(connection);
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

/* 0x8050EA30 (0x7BC): one reliable message, in order: the application data or a handshake / close step. */
s32 gti2HandleReliableMessage(DWCiReq* connection, s32 type, u8* message, s32 len) {
    u8 response[0x20];
    u8 challenge[0x20];
    s32 state;

    connection->expectedSerialNumber++;
    if (type == GTI2_MSG_APP_RELIABLE) {
        state = connection->state;
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
    switch (type) {
    case GTI2_MSG_CLIENT_CHALLENGE:
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
        break;
    case GTI2_MSG_SERVER_CHALLENGE:
        if (!gti2HandleServerChallenge(connection, message, len)) {
            return 0;
        }
        break;
    case GTI2_MSG_CLIENT_RESPONSE:
        if (!gti2HandleClientResponse(connection, message, len)) {
            return 0;
        }
        break;
    case GTI2_MSG_ACCEPT:
        if (connection->state != 1) {
            if (!gti2ConnectionError(connection, 7, 2)) {
                return 0;
            }
        } else {
            connection->state = GTI2_STATE_CONNECTED;
            if (!gti2ConnectedCallback(connection, 0, NULL, 0)) {
                return 0;
            }
        }
        break;
    case GTI2_MSG_REJECT:
        if (connection->state != 1) {
            if (!gti2ConnectionError(connection, 7, 2)) {
                return 0;
            }
        } else {
            gti2ConnectionClosed(connection);
            if (!gti2SendClosedInline(connection)) {
                return 0;
            }
            if (!gti2ConnectedCallback(connection, 2, message, len)) {
                return 0;
            }
        }
        break;
    case GTI2_MSG_CLOSE:
        if (!gti2SendClosedInline(connection)) {
            return 0;
        }
        if (!gti2ConnectionError(connection, 2, connection->state != GTI2_STATE_CLOSING)) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x8050E3E0 (0x334): the server's challenge: checks its response, answers its challenge with the initial
 * message and waits for the accept. */
s32 gti2HandleServerChallenge(DWCiReq* connection, u8* message, s32 len) {
    u8 response[0x20];

    if (connection->state != GTI2_STATE_AWAITING_SERVER_CHALLENGE) {
        return gti2ConnectionError(connection, 7, 2);
    }
    if (len < 0x40) {
        return gti2ConnectionError(connection, 7, 2);
    }
    if (!gti2CheckResponse(message, connection->response)) {
        return gti2ConnectionError(connection, 7, 2);
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
        return gti2ConnectionError(connection, 7, 2);
    }
    if (len < 0x20) {
        return gti2ConnectionError(connection, 7, 2);
    }
    if (!gti2CheckResponse(message, connection->response)) {
        return gti2ConnectionError(connection, 7, 2);
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
    s32 j;
    s32 count;
    s32 start;
    s32 len;
    s32 end;
    s32 shiftEnd;
    GTI2IncomingBufferMessage* bufferedMessage;
    GTI2IncomingBufferMessage* other;

    for (;;) {
        for (i = ArrayLength(connection->incomingBufferMessages) - 1; i >= 0; i--) {
            bufferedMessage = (GTI2IncomingBufferMessage*)ArrayNth(connection->incomingBufferMessages, i);
            if (bufferedMessage->serialNumber == connection->expectedSerialNumber) {
                break;
            }
        }
        if (i < 0) {
            return 1;
        }
        if (!gti2HandleReliableMessage(connection, bufferedMessage->type,
                                       connection->incomingBuffer.buffer + bufferedMessage->start,
                                       bufferedMessage->len)) {
            return 0;
        }
        start = bufferedMessage->start;
        len = bufferedMessage->len;
        shiftEnd = 0;
        ArrayDeleteAt(connection->incomingBufferMessages, i);
        count = ArrayLength(connection->incomingBufferMessages);
        for (j = 0; j < count; j++) {
            other = (GTI2IncomingBufferMessage*)ArrayNth(connection->incomingBufferMessages, j);
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
}

/* 0x8050F3C0 (0x450): a reliable packet: acknowledges our messages, then delivers it in order, buffers it or
 * drops a duplicate. */
s32 gti2HandleReliablePacket(DWCiReq* connection, s32 type, u8* message, s32 len) {
    DWCiConn* socket = connection->conn;
    s32 headerLength = socket->protocolOffset + 7;
    u8* header = message + socket->protocolOffset;
    u16 serialNumber;
    u8* data;
    s32 dataLength;
    s32 overflow;

    if (len < headerLength) {
        return gti2ConnectionError(connection, 7, 2);
    }
    serialNumber = (header[3] << 8) | header[4];
    if (socket->protocolType == GTI2_VDP_PROTOCOL && type == GTI2_MSG_APP_RELIABLE) {
        header[5] = message[0];
        message[connection->conn->protocolOffset + 6] = message[1];
        data = message + (headerLength - connection->conn->protocolOffset);
        dataLength = len - (headerLength - connection->conn->protocolOffset);
    } else {
        data = message + headerLength;
        dataLength = len - headerLength;
    }
    if (!gti2RemoveAckedMessages(connection, (header[5] << 8) | header[6])) {
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
        return gti2DeliverBufferedMessagesInline(connection);
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
        if (!gti2SendClosedInline(connection) || !gti2ConnectionError(connection, 1, 4)) {
            return 0;
        }
    }
    return 1;
}

/* 0x8050F810 (0x1E8): the peer's nack: resends the messages in the reported serial range. */
s32 gti2HandleNack(DWCiReq* connection, u8* message, s32 len) {
    u16 start;
    u16 end;
    s32 count;
    s32 i;
    s32 pos;
    DWCiXfer* outgoing;

    start = (message[0] << 8) | message[1];
    if (len == 2) {
        end = start;
    } else if (len == 4) {
        end = (message[2] << 8) | message[3];
    } else {
        return gti2ConnectionError(connection, 7, 2);
    }
    count = ArrayLength(connection->outgoingBufferMessages);
    for (i = 0; i < count; i++) {
        outgoing = (DWCiXfer*)ArrayNth(connection->outgoingBufferMessages, i);
        if ((s16)(outgoing->field_0x08 - start) >= 0 && (s16)(outgoing->field_0x08 - end) <= 0) {
            pos = outgoing->offset + connection->conn->protocolOffset + 5;
            connection->outgoingBuffer.buffer[pos] = (u8)(connection->expectedSerialNumber >> 8);
            connection->outgoingBuffer.buffer[pos + 1] = (u8)connection->expectedSerialNumber;
            if (!DWCi_requestFrame(connection, connection->outgoingBuffer.buffer + outgoing->offset,
                                   outgoing->length)) {
                return 0;
            }
            outgoing->stamp = connection->lastSend;
            if (connection->outgoingBuffer.buffer[outgoing->offset + connection->conn->protocolOffset + 2] ==
                GTI2_MSG_SERVER_CHALLENGE) {
                connection->challengeTime = connection->lastSend;
            }
        }
    }
    return 1;
}

/* 0x8050FA00 (0x2C8): an unreliable protocol message: ack, nack, ping, pong or closed. */
s32 gti2HandleUnreliableMessage(DWCiReq* connection, s32 type, u8* message, s32 len) {
    s32 headerLength = connection->conn->protocolOffset + 3;
    u8* data = message + headerLength;
    s32 dataLength = len - headerLength;
    u32 time;

    switch (type) {
    case GTI2_MSG_ACK:
        if (dataLength != 2) {
            if (!gti2ConnectionError(connection, 7, 2)) {
                return 0;
            }
        } else if (!gti2RemoveAckedMessages(connection, (data[0] << 8) | data[1])) {
            return 0;
        }
        break;
    case GTI2_MSG_NACK:
        if (!gti2HandleNack(connection, data, dataLength)) {
            return 0;
        }
        break;
    case GTI2_MSG_PING:
        message[2] = GTI2_MSG_PONG;
        if (!DWCi_requestFrame(connection, message, len)) {
            return 0;
        }
        break;
    case GTI2_MSG_PONG:
        if (connection->callbacks.ping_0C != NULL && (u32)dataLength == 8 && memcmp(data, "time", 4) == 0) {
            memcpy(&time, data + 4, 4);
            if (!gti2PingCallback(connection, current_time() - time)) {
                return 0;
            }
        }
        break;
    case GTI2_MSG_CLOSED:
        if (connection->state != GTI2_STATE_CLOSED &&
            !gti2ConnectionError(connection, 2, connection->state == GTI2_STATE_CLOSING)) {
            return 0;
        }
        break;
    }
    return 1;
}

/* 0x8050FCD0 (0x48C): one datagram from `ip:port`: a new connection attempt, a protocol message of a known
 * connection, or application data. */
s32 gti2HandleMessage(DWCiConn* socket, u8* message, s32 len, u32 ip, u16 port) {
    u8* header = message + socket->protocolOffset;
    s32 headerLength = len - socket->protocolOffset;
    DWCiReq* connection;
    s32 handled;
    s32 isProtocol;
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
        len -= 2;
        message += 2;
        message[2] = message[0];
    }
    if (!isProtocol) {
        if (connection->state < GTI2_STATE_CONNECTED) {
            if (!gti2UnrecognizedMessageCallback(socket, ip, port, message, len, &handled)) {
                return 0;
            }
            return 1;
        }
        if (connection->state == GTI2_STATE_CONNECTED || connection->state == GTI2_STATE_CLOSING) {
            if (ArrayLength(connection->receiveFilters) != 0) {
                if (!gti2ReceiveFilterCallback(connection, 0, message, len, 0)) {
                    return 0;
                }
            } else if (!gti2ReceivedCallback(connection, message, len, 0)) {
                return 0;
            }
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
        return gti2ConnectionError(connection, 6, 1);
    }
    return gti2ConnectionError(connection, 2, 1);
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
    return gti2ConnectionError(connection, 6, 1);
}

/* 0x805104B0 (0x3FC): reads every waiting datagram and handles it (a reset or unreachable peer is reported to its
 * connection, any other socket error shuts the socket down). */
s32 gti2ReceiveMessages(DWCiConn* socket) {
    SOSockAddrIn from;
    int fromLength;
    int len;
    u16 port;

    while (DWCi_socketHasData(socket->sock)) {
        fromLength = 8;
        len = DWCi_socketRecvFrom(socket->sock, DWCi_gt2ReceiveBuffer, 0x1000, 0, &from, &fromLength);
        if (len == -1) {
            switch (DWCi_socketGetLastError(socket->sock)) {
            case -15:
                port = SOAddressToHostPort(from.port);
                if (!DWCi_requestReconnect(socket, from.addr, port)) {
                    return 0;
                }
                break;
            case -23:
                port = SOAddressToHostPort(from.port);
                if (!DWCi_requestRetry(socket, from.addr, port, 0)) {
                    return 0;
                }
                break;
            case -35:
                break;
            default:
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
    message.field_0x08 = serialNumber;
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
    return gti2EndReliableMessageInline(connection) != 0;
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
    return gti2EndReliableMessageInline(connection) != 0;
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
    return gti2EndReliableMessageInline(connection) != 0;
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
    return gti2EndReliableMessageInline(connection) != 0;
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
    return gti2EndReliableMessageInline(connection) != 0;
}

/* 0x805110B0 (0x12C): sends application data unreliably, escaping a payload that starts with the magic. */
int DWCi_requestFlush(DWCiReq* req, u8* message, s32 len) {
    u8* start;

    if (len < 2 || memcmp(message + req->conn->protocolOffset, &DWCi_protocolMagic, GTI2_MAGIC_LEN) != 0) {
        if (!DWCi_requestFrame(req, message, len)) {
            return 0;
        }
        return 1;
    }
    if (gti2GetBufferFreeSpace(&req->outgoingBuffer) < len + 2) {
        return 1;
    }
    start = req->outgoingBuffer.buffer + req->outgoingBuffer.len;
    if (req->conn->protocolType == GTI2_VDP_PROTOCOL) {
        DWCi_bufferFrame(&req->outgoingBuffer, message, 2);
    }
    DWCi_bufferFrame(&req->outgoingBuffer, (const u8*)&DWCi_protocolMagic, GTI2_MAGIC_LEN);
    DWCi_bufferFrame(&req->outgoingBuffer, message + req->conn->protocolOffset, len - req->conn->protocolOffset);
    if (!DWCi_requestFrame(req, start, len + 2)) {
        return 0;
    }
    gti2BufferShorten(&req->outgoingBuffer, -1, len + 2);
    return 1;
}

/* 0x805111E0 (0xC4): acknowledges everything received so far. */
s32 gti2SendAck(DWCiReq* connection) {
    u8 buffer[8];
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
    buffer[pos++] = (u8)(connection->expectedSerialNumber >> 8);
    buffer[pos++] = (u8)connection->expectedSerialNumber;
    if (!DWCi_requestFrame(connection, buffer, pos)) {
        return 0;
    }
    connection->pendingAck = 0;
    return 1;
}

/* 0x805112B0 (0xF4): asks the peer to resend the serial range `start`..`end`. */
s32 gti2SendNack(DWCiReq* connection, u16 start, u16 end) {
    u8 buffer[10];
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
    buffer[pos++] = (u8)(start >> 8);
    buffer[pos++] = (u8)start;
    if (start != end) {
        buffer[pos++] = (u8)(end >> 8);
        buffer[pos++] = (u8)end;
    }
    if (!DWCi_requestFrame(connection, buffer, pos)) {
        return 0;
    }
    return 1;
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

/* 0x8050DA60 (0x5C): sends raw data to the connection's peer and stamps the send time. */
/* untyped: byte range - the datagram */
int DWCi_requestFrame(DWCiReq* req, void* p, u32 len) {
    if (!DWCi_sendTo(req->conn, req->addr, req->port, p, len)) {
        return 0;
    }
    req->lastSend = current_time();
    return 1;
}

/* 0x8050DDB0 (0x5C): moves a connection into the closed state and onto its socket's closed list. */
void gti2ConnectionClosed(DWCiReq* connection) {
    gti2ConnectionClosedInline(connection);
}

/* ---- gt2Callback: every callback runs with the nesting levels raised; a socket closed meanwhile is freed -- */

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

/* ---- gt2Buffer ---------------------------------------------------------------------------------------- */

/* 0x8050CD20 (0x58): allocates `size` bytes of buffer storage. */
int DWCi_bufferAlloc(GTI2Buffer* buffer, u32 size) {
    buffer->buffer = (u8*)gsimalloc(size);
    if (buffer->buffer == NULL) {
        return 0;
    }
    buffer->size = size;
    return 1;
}

/* 0x8050CD80 (0x10): the free bytes left in the buffer. */
s32 gti2GetBufferFreeSpace(const GTI2Buffer* buffer) {
    return buffer->size - buffer->len;
}

/* 0x8050CD90 (0x18): appends one byte. */
void gti2BufferWriteByte(GTI2Buffer* buffer, u8 b) {
    buffer->buffer[buffer->len++] = b;
}

/* 0x8050CDB0 (0x2C): appends a big-endian 16-bit value. */
void gti2BufferWriteUShort(GTI2Buffer* buffer, u16 s) {
    buffer->buffer[buffer->len++] = (u8)(s >> 8);
    buffer->buffer[buffer->len++] = (u8)s;
}

/* 0x8050CDE0 (0x8C): appends `len` bytes (-1: a NUL-terminated string without its NUL). */
void DWCi_bufferFrame(GTI2Buffer* buffer, const u8* data, s32 len) {
    if (data != NULL && len != 0) {
        if (len == -1) {
            len = strlen((const char*)data);
        }
        memcpy(buffer->buffer + buffer->len, data, len);
        buffer->len += len;
    }
}

/* 0x8050CE70 (0x6C): removes `shortenBy` bytes at `start` (-1: the end of the buffer). */
void gti2BufferShorten(GTI2Buffer* buffer, s32 start, s32 shortenBy) {
    if (start == -1) {
        start = buffer->len - shortenBy;
    }
    memmove(buffer->buffer + start, buffer->buffer + start + shortenBy, buffer->len - start - shortenBy);
    buffer->len -= shortenBy;
}

/* 0x8050A710 (0x58): the NHTTP teardown callback: reports, tells the host the session is over and parks the
 * state machine in its terminal state. */
void DWCi_Auth_EndProcess(void) {
    DWC_Printf(0x1000000, "DWCi_Auth_EndProcess()\n");
    DWCi_runtime->commandCallback(0, (u32)DWCi_runtime, 0);
    DWCi_runtime = NULL;
    DWCi_state = 25;
}

/* 0x8050A770 (0x128): moves the auth state machine on after a NAND call completed. */
void DWCi_Auth_CheckNandResult(s32 success, s32 retry, s32 noExist, s32 access) {
    s32 result;

    DWCi_runtime->resultFlag = 0;
    if (retry != 27 && (s32)DWCi_runtime->resultValue == -3 && DWCi_runtime->retryCount < 5) {
        DWCi_runtime->retryCount++;
        DWCi_state = retry;
        return;
    }
    DWCi_runtime->retryCount = 0;
    result = DWCi_runtime->resultValue;
    if (result == 0) {
        DWCi_state = success;
    } else if (noExist != 27 && result == -12) {
        DWCi_state = noExist;
    } else if (access != 27 && result == -1) {
        DWCi_state = access;
    } else {
        DWC_Printf(0x1000000, " NAND access failed.[%d]\n", result);
        *(volatile s32*)DWCi_stateBlock = ((s32)DWCi_runtime->resultValue == -4 ? -1 : 0) - 29000;
        DWCi_Auth_EndProcess();
    }
}


/* ---- dwc_nas / dwc_svl ------------------------------------------------------------------------------ */

/* 0x8050A8A0 (0x2C): starts the NAS login (the auth interface with empty names and the DWC allocator). */
s32 DWC_NASLoginAsync(void) {
    DWCi_nasLoginState = 4;
    return DWCi_initRuntime((char*)L"", "", 0, (DWCiAllocCallback)DWC_Alloc, (DWCiCommandCallback)DWC_Free);
}

/* 0x8050A8D0 (0xB4): steps the NAS login: 2 while it runs, 3 on success, 4 on an error (recorded), 5 cancelled. */
s32 DWC_NASLoginProcess(void) {
    switch (DWCi_nasLoginState) {
    case 1:
        return 3;
    case 2:
        return 5;
    case 3:
        return 4;
    case 4:
        DWCi_authDataTask();
        if (DWCi_AdvanceStatus() != 0) {
            if (DWCi_IsStatusReady() != 0) {
                DWCi_nasLoginState = 1;
                return 3;
            }
            DWCi_nasLoginState = 3;
            DWCi_SetError(2, DWCi_GetStatus());
            return 4;
        }
        return 2;
    }
    return 4;
}

/* 0x8050A990 (0x10): begins a service-locator session. */
s32 DWC_SVLBegin(void) {
    DWCi_svlResult = NULL;
    return 1;
}

/* 0x8050A9A0 (0xC): ends the service-locator session. */
void DWC_SVLEnd(void) {
    DWCi_svlResult = NULL;
}

/* 0x8050A9B0 (0x18): requests a token for the service `svl`; the answer lands in `result`. */
s32 DWC_SVLGetTokenAsync(const char* svl, DWCSvlResult* result) {
    DWCi_svlResult = result;
    return DWCi_npSetup((char*)svl, (DWCiAllocCallback)DWC_Alloc, (DWCiCommandCallback)DWC_Free);
}

/* 0x8050A9D0 (0x11C): steps the token request: 2 while it runs, 3 with a token, 4 on an error (recorded with the
 * server's code mapped into the DWC ranges). */
s32 DWC_SVLProcess(void) {
    DWCSvlResult* result;
    s32 code;

    if (DWCi_AdvanceStatus() != 0) {
        if (DWCi_IsStatusReady() != 0) {
            result = DWCi_svlResult;
            if (result != NULL) {
                *result = *(DWCSvlResult*)DWCi_GetWorkBuffer();
            }
            if (strlen(result->svltoken) == 0) {
                DWCi_SetError(15, -24101);
                return 4;
            }
            return 3;
        }
        code = DWCi_GetStatus();
        if ((u32)(code + 20110) <= 10) {
            code -= 4000;
        } else if ((u32)(code + 20999) <= 888) {
            code -= 4000;
        } else if ((u32)(code + 23999) <= 999) {
            code -= 2000;
        } else {
            DWC_Printf(0x1000000, "[svl] Unknown Error Code:%d\n", code);
        }
        DWCi_SetError(15, code);
        return 4;
    }
    DWCi_authDataTask();
    return 2;
}

/* ---- darray (the GameSpy dynamic array) ------------------------------------------------------------ */

/* The address of element `n`, or NULL outside the array. */
/* untyped: caller-owned payload - the element */
static inline void* ArrayNthInline(DArray array, int n) {
    if (!(n >= 0 && n < array->count)) {
        return NULL;
    }
    return (char*)array->list + array->elemsize * n;
}

/* 0x8050AAF0 (0x9C): a new array of `elemSize`-byte elements growing by `numElemsToAllocate` (8 for 0). */
DArray ArrayNew(int elemSize, int numElemsToAllocate, ArrayElementFreeFn elemFreeFn) {
    DArray array = (DArray)gsimalloc(sizeof(struct DArrayImplementation));
    if (numElemsToAllocate == 0) {
        numElemsToAllocate = 8;
    }
    array->count = 0;
    array->capacity = numElemsToAllocate;
    array->elemsize = elemSize;
    array->growby = numElemsToAllocate;
    array->elemfreefn = elemFreeFn;
    if (array->capacity != 0) {
        array->list = gsimalloc(array->capacity * array->elemsize);
    } else {
        array->list = NULL;
    }
    return array;
}

/* 0x8050AB90 (0x94): frees every element through the array's free function, then the array. */
void ArrayFree(DArray array) {
    int i;
    for (i = 0; i < array->count; i++) {
        if (array->elemfreefn != NULL) {
            array->elemfreefn(ArrayNthInline(array, i));
        }
    }
    gsifree(array->list);
    gsifree(array);
}

/* 0x8050AC30 (0x8): the element count. */
int ArrayLength(DArray array) {
    return array->count;
}

/* 0x8050AC40 (0x30): the address of element `n`, or NULL outside the array. */
/* untyped: caller-owned payload - the element */
void* ArrayNth(DArray array, int n) {
    if (n < 0 || n >= array->count) {
        return NULL;
    }
    return (char*)array->list + array->elemsize * n;
}

/* Inserts a copy of `newElem` before element `n`, growing the array when it is full. */
/* untyped: caller-owned payload - the element copied in */
static inline void ArrayInsertAt(DArray array, const void* newElem, int n) {
    if (array->count == array->capacity) {
        array->capacity += array->growby;
        array->list = gsirealloc(array->list, array->capacity * array->elemsize);
    }
    array->count++;
    if (n < array->count - 1) {
        memmove(ArrayNthInline(array, n + 1), ArrayNthInline(array, n), (array->count - 1 - n) * array->elemsize);
    }
    memcpy(ArrayNthInline(array, n), newElem, array->elemsize);
}

/* 0x8050AC70 (0x124): appends a copy of `newElem`. */
/* untyped: caller-owned payload - the element copied in */
void ArrayAppend(DArray array, const void* newElem) {
    if (array != NULL) {
        ArrayInsertAt(array, newElem, array->count);
    }
}

/* 0x8050ADA0 (0x180): inserts a copy of `newElem` at its sorted place (binary search with `comparator`). */
/* untyped: caller-owned payload - the element copied in */
void ArrayInsertSorted(DArray array, const void* newElem, ArrayCompareFn comparator) {
    char* base = (char*)array->list;
    int elemsize = array->elemsize;
    int low = 0;
    int high = array->count - 1;
    int mid;
    int result;
    int n;

    while (low <= high) {
        mid = (low + high) >> 1;
        result = comparator(base + mid * elemsize, newElem);
        if (result < 0) {
            low = mid + 1;
        }
        if (result >= 0) {
            high = mid - 1;
        }
    }
    n = (base + low * elemsize - (char*)array->list) / array->elemsize;
    ArrayInsertAt(array, newElem, n);
}

/* 0x8050AF20 (0xAC): removes element `n` without freeing it. */
void ArrayRemoveAt(DArray array, int n) {
    if (n < array->count - 1) {
        memmove(ArrayNthInline(array, n), ArrayNthInline(array, n + 1), (array->count - 1 - n) * array->elemsize);
    }
    array->count--;
}

/* 0x8050AFD0 (0xF0): frees element `n` through the array's free function and removes it. */
void ArrayDeleteAt(DArray array, int n) {
    if (array->elemfreefn != NULL) {
        array->elemfreefn(ArrayNthInline(array, n));
    }
    ArrayRemoveAt(array, n);
}

/* 0x8050B0C0 (0xB8): frees element `n` and overwrites it with a copy of `newElem`. */
/* untyped: caller-owned payload - the element copied in */
void ArrayReplaceAt(DArray array, const void* newElem, int n) {
    if (array->elemfreefn != NULL) {
        array->elemfreefn(ArrayNthInline(array, n));
    }
    memcpy(ArrayNthInline(array, n), newElem, array->elemsize);
}

/* 0x8050B180 (0x198): the index of the element matching `key` from `fromIndex` on (binary search when
 * `isSorted`), or -1. */
/* untyped: caller-owned payload - the key */
int ArraySearch(DArray array, const void* key, ArrayCompareFn comparator, int fromIndex, int isSorted) {
    char* res;
    int found = 1;
    int count;

    if (array == NULL || array->count == 0) {
        return -1;
    }
    if (isSorted) {
        char* base = (char*)ArrayNthInline(array, fromIndex);
        int elemsize = array->elemsize;
        int low = 0;
        int high = array->count - fromIndex - 1;
        int mid;
        int result;

        found = 0;
        while (low <= high) {
            mid = (low + high) >> 1;
            result = comparator(base + mid * elemsize, key);
            if (result == 0) {
                found = 1;
            }
            if (result < 0) {
                low = mid + 1;
            }
            if (result >= 0) {
                high = mid - 1;
            }
        }
        res = base + low * elemsize;
    } else {
        char* base = (char*)ArrayNthInline(array, fromIndex);
        int elemsize = array->elemsize;
        int i;

        count = array->count - fromIndex;
        res = NULL;
        for (i = 0; i < count; i++) {
            if (comparator(key, base + elemsize * i) == 0) {
                res = base + elemsize * i;
                break;
            }
        }
    }
    if (res != NULL && found) {
        return (res - (char*)array->list) / array->elemsize;
    }
    return -1;
}

/* 0x8050B320 (0x94): calls `fn` on every element, last first. */
/* untyped: caller-owned payload - the client data */
void ArrayMapBackwards(DArray array, ArrayMapFn fn, void* clientData) {
    int i;
    for (i = array->count - 1; i >= 0; i--) {
        fn(ArrayNthInline(array, i), clientData);
    }
}

/* 0x8050B3C0 (0x9C): calls `fn` on every element, last first, and returns the first one it answers 0 for. */
/* untyped: caller-owned payload - the client data and the element returned */
void* ArrayMapBackwards2(DArray array, ArrayMapFn2 fn, void* clientData) {
    int i;
    void* elem;
    for (i = array->count - 1; i >= 0; i--) {
        elem = ArrayNthInline(array, i);
        if (!fn(elem, clientData)) {
            return elem;
        }
    }
    return NULL;
}

/* ---- hashtable (the GameSpy hash table over darray buckets) ---------------------------------------- */

/* The retail hashtable is its own TU and calls the darray functions; in this merged object MWCC would inline them. */
#pragma push
#pragma dont_inline on

/* 0x8050B460 (0xA4): a table of `nBuckets` arrays of `elemSize`-byte elements. */
HashTable TableNew2(int elemSize, int nBuckets, int nChains, TableHashFn hashFn, TableCompareFn compFn,
                    TableElementFreeFn freeFn) {
    HashTable table;
    int i;

    table = (HashTable)gsimalloc(sizeof(struct HashImplementation));
    table->buckets = (DArray*)gsimalloc(nBuckets * sizeof(DArray));
    for (i = 0; i < nBuckets; i++) {
        table->buckets[i] = ArrayNew(elemSize, nChains, freeFn);
    }
    table->nbuckets = nBuckets;
    table->freefn = freeFn;
    table->compfn = compFn;
    table->hashfn = hashFn;
    return table;
}

/* 0x8050B510 (0x7C): frees every bucket and the table. */
void TableFree(HashTable table) {
    int i;
    if (table != NULL) {
        for (i = 0; i < table->nbuckets; i++) {
            ArrayFree(table->buckets[i]);
        }
        gsifree(table->buckets);
        gsifree(table);
    }
}

/* 0x8050B590 (0xA8): adds a copy of `newElem`, replacing an equal element. */
/* untyped: caller-owned payload - the element copied in */
void TableEnter(HashTable table, const void* newElem) {
    int hash;
    int itempos;
    if (table != NULL) {
        hash = table->hashfn(newElem, table->nbuckets);
        itempos = ArraySearch(table->buckets[hash], newElem, table->compfn, 0, 0);
        if (itempos == -1) {
            ArrayAppend(table->buckets[hash], newElem);
        } else {
            ArrayReplaceAt(table->buckets[hash], newElem, itempos);
        }
    }
}

/* 0x8050B640 (0xA4): removes the element equal to `delElem`; 1 when one was found. */
/* untyped: caller-owned payload - the key */
int TableRemove(HashTable table, const void* delElem) {
    int hash;
    int itempos;
    if (table == NULL) {
        return 0;
    }
    hash = table->hashfn(delElem, table->nbuckets);
    itempos = ArraySearch(table->buckets[hash], delElem, table->compfn, 0, 0);
    if (itempos == -1) {
        return 0;
    }
    ArrayDeleteAt(table->buckets[hash], itempos);
    return 1;
}

/* 0x8050B6F0 (0xA0): the element equal to `elemKey`, or NULL. */
/* untyped: caller-owned payload - the key and the element returned */
void* TableLookup(HashTable table, const void* elemKey) {
    int hash;
    int itempos;
    if (table == NULL) {
        return NULL;
    }
    hash = table->hashfn(elemKey, table->nbuckets);
    itempos = ArraySearch(table->buckets[hash], elemKey, table->compfn, 0, 0);
    if (itempos == -1) {
        return NULL;
    }
    return ArrayNth(table->buckets[hash], itempos);
}

/* 0x8050B790 (0x6C): calls `fn` on every element (each bucket last first). */
/* untyped: caller-owned payload - the client data */
void TableMapSafe(HashTable table, TableMapFn fn, void* clientData) {
    int i;
    for (i = 0; i < table->nbuckets; i++) {
        ArrayMapBackwards(table->buckets[i], fn, clientData);
    }
}

/* 0x8050B800 (0x7C): calls `fn` on every element and returns the first one it answers 0 for. */
/* untyped: caller-owned payload - the client data and the element returned */
void* TableMapSafe2(HashTable table, TableMapFn2 fn, void* clientData) {
    int i;
    void* elem;
    for (i = 0; i < table->nbuckets; i++) {
        elem = ArrayMapBackwards2(table->buckets[i], fn, clientData);
        if (elem != NULL) {
            return elem;
        }
    }
    return NULL;
}

#pragma pop

/* The retail objects below are separate TUs that call the socket layer; defining them ahead of it keeps MWCC
 * from inlining those calls into them. */
/* ---- gsAvailable: the "is the game's backend up" probe --------------------------------------------- */

/* Resolves `host` (a dotted address or a name) into `addr` with `port`; 0 when the name cannot be resolved. */
static inline int DWCi_getSockAddrIn(char* host, u16 port, SOSockAddrIn* addr) {
    DWCiHostEntry* entry;
    addr->family = 2;
    addr->port = SOHtoNs(port);
    addr->addr = DWCi_socketResolveAddress(host);
    if (addr->addr == (u32)-1) {
        entry = DWCi_socketLookupHost(host);
        if (entry == NULL) {
            return 0;
        }
        addr->addr = entry->hosts[0]->addr;
    }
    return 1;
}

/* 0x8050C5F0 (0x17C): opens the probe socket and sends the availability query for `gameName` to
 * "<gameName>.available.gs.nintendowifi.net" (or the override host) port 27900. */
void DWCi_natProbeStart(const char* gameName) {
    char hostName[0x40];
    char overrideHost;
    int length;

    strcpy(DWCi_availableNames.gameName, gameName);
    DWCi_availableCheck.sock = -1;
    SocketStartUp();
    overrideHost = DWCi_availableNames.hostName[0];
    if (overrideHost == 0) {
        sprintf(hostName, "%s.available.gs.nintendowifi.net", gameName);
    }
    if (DWCi_getSockAddrIn(overrideHost != 0 ? DWCi_availableNames.hostName : hostName, 27900,
                           &DWCi_availableCheck.address)) {
        DWCi_availableCheck.sock = DWCi_socketCreate(2, 2, 17);
        if (DWCi_availableCheck.sock != -1) {
            DWCi_availableCheck.packet[0] = 9;
            length = strlen(gameName);
            memcpy(&DWCi_availableCheck.packet[5], gameName, length + 1);
            DWCi_availableCheck.packetLength = length + 6;
            DWCi_socketSendTo(DWCi_availableCheck.sock, DWCi_availableCheck.packet,
                              DWCi_availableCheck.packetLength, 0, &DWCi_availableCheck.address, 8);
            DWCi_availableCheck.sendTime = current_time();
            DWCi_availableCheck.resendCount = 0;
        }
    }
}

/* Records and answers the probe result. */
static inline s32 DWCi_natProbeResult(s32 result) {
    DWCi_natProbeStatus = result;
    return result;
}

/* 0x8050C770 (0x1D4): polls the probe: 0 while waiting, 1 available, 2 unavailable, 3 temporarily unavailable
 * (also answered 1 when the socket could not be opened or nothing came back after one resend). */
s32 DWCi_natProbePoll(void) {
    __attribute__((aligned(32))) u8 packet[0x40];
    SOSockAddrIn from;
    int fromLength;
    int error;
    u32 flags;

    fromLength = 8;
    if (DWCi_availableCheck.sock == -1) {
        return DWCi_natProbeResult(1);
    }
    if (DWCi_socketHasData(DWCi_availableCheck.sock)) {
        if (DWCi_socketRecvFrom(DWCi_availableCheck.sock, packet, 0x40, 0, &from, &fromLength) < 7) {
            error = 1;
        } else if (memcmp(&from.addr, &DWCi_availableCheck.address.addr, 4) != 0) {
            error = 1;
        } else if (from.port != DWCi_availableCheck.address.port) {
            error = 1;
        } else if (memcmp(packet, "\xFE\xFD\x09", 3) != 0) {
            error = 1;
        } else {
            error = 0;
            flags = (packet[3] << 24) | (packet[4] << 16) | (packet[5] << 8) | packet[6];
        }
        if (!error) {
            DWCi_socketClose(DWCi_availableCheck.sock);
            if (flags & 1) {
                return DWCi_natProbeResult(2);
            }
            if (flags & 2) {
                return DWCi_natProbeResult(3);
            }
            return DWCi_natProbeResult(1);
        }
    }
    if (current_time() > DWCi_availableCheck.sendTime + 2000) {
        if (DWCi_availableCheck.resendCount == 1) {
            DWCi_socketClose(DWCi_availableCheck.sock);
            return DWCi_natProbeResult(1);
        }
        DWCi_socketSendTo(DWCi_availableCheck.sock, DWCi_availableCheck.packet, DWCi_availableCheck.packetLength, 0,
                          &DWCi_availableCheck.address, 8);
        DWCi_availableCheck.sendTime = current_time();
        DWCi_availableCheck.resendCount++;
    }
    return 0;
}

/* ---- gt2Auth: the GT2 connection challenge ------------------------------------------------------------ */

#define GTI2_CHALLENGE_LEN 32
#define GTI2_RANDOM_CHAR() (u8)(rand() % 93 + 33)

/* 0x8050C950 (0x110): fills `buffer` with a fresh 32-byte challenge (each byte's parity is a function of the
 * ones before it). */
u8* gti2GetChallenge(u8* buffer) {
    int i;
    int odd;

    srand(current_time());
    odd = 0;
    buffer[0] = GTI2_RANDOM_CHAR();
    for (i = 1; i < GTI2_CHALLENGE_LEN; i++) {
        odd = (buffer[0] & 1) ^ odd ^ ((i ^ buffer[i - 1]) & 1) ^ (buffer[0] < 79) ^ (buffer[i - 1] < buffer[0]);
        buffer[i] = GTI2_RANDOM_CHAR();
        if ((odd && !(buffer[i] & 1)) || (!odd && (buffer[i] & 1) == 1)) {
            buffer[i]++;
        }
    }
    return buffer;
}

/* 0x8050CA60 (0x1F0): the response to `challenge` (random bytes when the challenge is not a valid one). */
u8* gti2GetResponse(u8* buffer, const u8* challenge) {
    int keyLength = strlen(DWCi_gt2SecretKey);
    int odd;
    int i;
    int valid;
    int c;
    int value;

    odd = 0;
    for (i = 1; i < GTI2_CHALLENGE_LEN; i++) {
        odd = (challenge[0] & 1) ^ odd ^ ((i ^ challenge[i - 1]) & 1) ^ (challenge[0] < 79) ^ (challenge[i - 1] < challenge[0]);
        if ((odd && !(challenge[i] & 1)) || (!odd && (challenge[i] & 1) == 1)) {
            break;
        }
    }
    valid = (i == GTI2_CHALLENGE_LEN);
    for (i = 0; i < GTI2_CHALLENGE_LEN; i++) {
        if (!valid || i == 0 || i == 13) {
            buffer[i] = GTI2_RANDOM_CHAR();
        } else {
            if (i == 1 || i == 14) {
                c = challenge[i];
            } else {
                c = challenge[i - 1];
            }
            value = challenge[(DWCi_gt2SecretKey[(i + challenge[i]) % keyLength] + i * challenge[i]) % GTI2_CHALLENGE_LEN] ^
                    DWCi_gt2SecretKey[(c * i * 17991) % keyLength];
            if (value < 0) {
                value = -value;
            }
            buffer[i] = value % 93 + 33;
        }
    }
    return buffer;
}

/* 0x8050CC50 (0xC8): non-zero when the two responses agree (bytes 0 and 13 are random). */
int gti2CheckResponse(const u8* response1, const u8* response2) {
    int i;
    for (i = 0; i < GTI2_CHALLENGE_LEN; i++) {
        if (i != 0 && i != 13 && response1[i] != response2[i]) {
            return 0;
        }
    }
    return 1;
}
/* ---- the socket layer over SO (GameSpy's `gsSocket` for this platform) --------------------------------- */

/* The SO result as the socket API answers it: the value, or -1 with the error recorded. */
static inline int DWCi_socketResult(int result) {
    if (result >= 0) {
        return result;
    }
    gsiSocketError = result;
    return -1;
}

/* 0x8050B880 (0x38): a new socket; -1 with the error recorded on failure. */
int DWCi_socketCreate(int domain, int type, int protocol) {
    return DWCi_socketResult(SOSocket(domain, type, 0));
}

/* 0x8050B8C0 (0x34): closes a socket. */
int DWCi_socketClose(int sock) {
    int result = SOClose(sock);
    return DWCi_socketResult(result);
}

/* 0x8050B900 (0x78): binds a socket to `addr` (nothing to do for port 0). */
int DWCi_socketBind(int sock, const SOSockAddrIn* addr, int len) {
    SOSockAddrIn local;
    int result;

    if (addr->port == 0) {
        return 0;
    }
    memcpy(&local, addr, sizeof(SOSockAddrIn));
    local.len = len;
    result = SOBind(sock, &local);
    return DWCi_socketResult(result);
}

/* 0x8050B980 (0x60): receives one datagram without blocking. */
int DWCi_socketRecvFrom(int sock, u8* buf, int len, u32 flags, SOSockAddrIn* from, int* fromLen) {
    int result;

    from->len = *fromLen;
    result = SORecvFrom(sock, buf, len, 4, from);
    *fromLen = from->len;
    return DWCi_socketResult(result);
}

/* 0x8050B9E0 (0x80): sends one datagram to `to`. */
/* untyped: byte range - the datagram */
int DWCi_socketSendTo(int sock, const void* buf, int len, u32 flags, const SOSockAddrIn* to, int toLen) {
    SOSockAddrIn local;
    int result;

    memcpy(&local, to, sizeof(SOSockAddrIn));
    local.len = toLen;
    result = SOSendTo(sock, (const u8*)buf, len, flags, &local);
    return DWCi_socketResult(result);
}

/* 0x8050BA60 (0x5C): the address a socket is bound to. */
int DWCi_socketGetLocalName(int sock, SOSockAddrIn* addr, int* len) {
    int result;

    addr->len = *len;
    result = SOGetSockName(sock, addr);
    *len = addr->len;
    return DWCi_socketResult(result);
}

/* 0x8050BAC0 (0x38): a dotted address as a network-order word, or -1. */
u32 DWCi_socketResolveAddress(char* host) {
    u32 addr;
    if (SOInetAtoN(host, (u8*)&addr) == 0) {
        return (u32)-1;
    }
    return addr;
}

/* 0x8050BB00 (0x8): the last socket error. */
int DWCi_socketGetLastError(int sock) {
    return gsiSocketError;
}

/* 0x8050BB10 (0x128): polls one socket for reading, writing and errors (any of the flag pointers may be NULL). */
int DWCi_socketSelect(int sock, int* readFlag, int* writeFlag, int* exceptFlag) {
    SOPollFD fd;
    int result;

    fd.fd = sock;
    fd.events = 0;
    if (readFlag != NULL) {
        fd.events |= 1;
    }
    if (writeFlag != NULL) {
        fd.events |= 8;
    }
    fd.revents = 0;
    result = SOPoll(&fd, 1, 0);
    if (result < 0) {
        return -1;
    }
    if (readFlag != NULL) {
        if (result > 0 && (fd.revents & 0x41)) {
            *readFlag = 1;
        } else {
            *readFlag = 0;
        }
    }
    if (writeFlag != NULL) {
        if (result > 0 && (fd.revents & 8)) {
            *writeFlag = 1;
        } else {
            *writeFlag = 0;
        }
    }
    if (exceptFlag != NULL) {
        if (result > 0 && (fd.revents & 0x20)) {
            *exceptFlag = 1;
        } else {
            *exceptFlag = 0;
        }
    }
    return result;
}

/* The GameSpy allocator hooks called in place (the retail socket TU expands `gsimalloc`/`gsifree` inline). */
/* untyped: caller-owned payload - a raw block */
static inline void* gsiMallocInline(u32 size) {
    return gsiMemoryCallbacks.mallocFn(size);
}

/* untyped: caller-owned payload - a raw block */
static inline void gsiFreeInline(void* block) {
    if (block != NULL) {
        gsiMemoryCallbacks.freeFn(block);
    }
}

/* The cache record of one resolved host: the resolver's `hostent` fields, then the host name it was asked for. */
typedef struct DWCiHostCacheEntry {
    /* +0x00 */ char* name;
    /* +0x04 */ char** aliases;
    /* +0x08 */ s16 addrType;
    /* +0x0A */ s16 length;
    /* +0x0C */ char** addrList;
    /* +0x10 */ char* hostName;
} DWCiHostCacheEntry; /* size: 0x14 */

#define DWCI_HOST_CACHE_SIZE 31

/* 0x8050BC40 (0x584): resolves `host` through a 31-slot cache of resolver answers (quadratic probing); the
 * name "clear" frees the cache instead. */
DWCiHostEntry* DWCi_socketLookupHost(char* host) {
    u32 i;
    int j;
    int hash;
    u32 probe;
    int slot;
    int count;
    int length;
    DWCiHostCacheEntry* entry;
    DWCiHostEntry* result;

    if (strncmp("clear", host, strlen("clear")) == 0) {
        if (DWCi_hostCache != NULL) {
            for (i = 0; i < DWCI_HOST_CACHE_SIZE; i++) {
                if (DWCi_hostCache[i] != NULL) {
                    for (j = 0; DWCi_hostCache[i]->addrList[j] != NULL; j++) {
                        gsiFreeInline(DWCi_hostCache[i]->addrList[j]);
                    }
                    gsiFreeInline(DWCi_hostCache[i]->addrList[j]);
                    gsiFreeInline(DWCi_hostCache[i]->addrList);
                    gsiFreeInline(DWCi_hostCache[i]->hostName);
                }
                gsiFreeInline(DWCi_hostCache[i]);
            }
            gsiFreeInline(DWCi_hostCache);
            DWCi_hostCache = NULL;
        }
        return NULL;
    }
    if (DWCi_hostCache == NULL) {
        DWCi_hostCache = (DWCiHostCacheEntry**)gsiMallocInline(DWCI_HOST_CACHE_SIZE * sizeof(DWCiHostCacheEntry*));
        memset(DWCi_hostCache, 0, DWCI_HOST_CACHE_SIZE * sizeof(DWCiHostCacheEntry*));
    }
    length = strlen(host);
    hash = 0;
    for (i = 0; i < (u32)length; i++) {
        hash += host[i] << ((i * 4) & 0x1C);
    }
    hash = (u32)hash % DWCI_HOST_CACHE_SIZE;
    for (probe = 0; probe < 15; probe++) {
        entry = DWCi_hostCache[(u32)(hash + probe * probe) % DWCI_HOST_CACHE_SIZE];
        if (entry != NULL && strcmp(entry->hostName, host) == 0) {
            return (DWCiHostEntry*)entry;
        }
    }
    if (DWCi_hostCache[hash] != NULL) {
        for (probe = 1; probe < 15; probe++) {
            slot = (u32)(hash + probe * probe) % DWCI_HOST_CACHE_SIZE;
            if (DWCi_hostCache[slot] == NULL) {
                hash = slot;
                break;
            }
        }
        if (probe == 15) {
            return SOGetHostByName(host);
        }
    }
    result = SOGetHostByName(host);
    if (result == NULL) {
        return NULL;
    }
    count = 0;
    while (result->hosts[count] != NULL) {
        count++;
    }
    DWCi_hostCache[hash] = (DWCiHostCacheEntry*)gsiMallocInline(sizeof(DWCiHostCacheEntry));
    DWCi_hostCache[hash]->addrType = 2;
    DWCi_hostCache[hash]->length = result->length;
    DWCi_hostCache[hash]->name = NULL;
    DWCi_hostCache[hash]->aliases = NULL;
    DWCi_hostCache[hash]->addrList = (char**)gsiMallocInline((count + 1) * sizeof(char*));
    for (j = 0; result->hosts[j] != NULL; j++) {
        DWCi_hostCache[hash]->addrList[j] = (char*)gsiMallocInline(result->length);
        memcpy(DWCi_hostCache[hash]->addrList[j], result->hosts[j], result->length);
    }
    DWCi_hostCache[hash]->addrList[j] = NULL;
    DWCi_hostCache[hash]->hostName = (char*)gsiMallocInline(strlen(host) + 1);
    strcpy(DWCi_hostCache[hash]->hostName, host);
    return result;
}

/* 0x8050C1D0 (0x48): non-zero when the socket has a datagram waiting. */
int DWCi_socketHasData(int sock) {
    int readFlag = 0;
    if (DWCi_socketSelect(sock, &readFlag, NULL, NULL) == 1) {
        return readFlag;
    }
    return 0;
}

/* 0x8050C220 (0x48): non-zero when the socket can be written to. */
int DWCi_socketIsUsable(int sock) {
    int writeFlag = 0;
    if (DWCi_socketSelect(sock, NULL, &writeFlag, NULL) == 1) {
        return writeFlag;
    }
    return 0;
}

/* 0x8050C270 (0x1E0): the resolver record of this machine ("localhost" with every interface address), or NULL
 * when the interface count cannot be read. */
DWCiHostEntry* DWCi_socketGetLocalHostEntry(void) {
    s32 count;
    s32 length;
    s32 i;

    length = 4;
    SOGetInterfaceOpt(NULL, 0xFFFE, 0x4002, &count, &length);
    if ((u32)(count - 1) > 15) {
        return NULL;
    }
    memset(DWCi_localAddresses, 0xBE, count * 12);
    length = count * 12;
    SOGetInterfaceOpt(NULL, 0xFFFE, 0x4003, DWCi_localAddresses, &length);
    DWCi_localHostEntry.name = "localhost";
    DWCi_localHostEntry.aliases = &DWCi_localHostAliases;
    DWCi_localHostEntry.addrType = 2;
    DWCi_localHostEntry.length = count;
    for (i = 0; i < count; i++) {
        DWCi_localAddressList[i] = (DWCiHostAddr*)DWCi_localAddresses[i];
    }
    DWCi_localAddressList[i] = NULL;
    DWCi_localHostEntry.hosts = DWCi_localAddressList;
    return &DWCi_localHostEntry;
}

/* 0x8050C450 (0x74): non-zero when the address is a private one (10/8, 172.16/12, 192.168/16). */
int DWCi_hostAddressIsPrivate(DWCiHostAddr* host) {
    u32 addr = DWCi_socketTokenFromPeer(host->addr);
    u32 first = addr >> 24;
    u32 second = (addr >> 16) & 0xFF;

    if (first == 10) {
        return 1;
    }
    if (first == 172 && second >= 16 && second <= 31) {
        return 1;
    }
    if (first == 192 && second == 168) {
        return 1;
    }
    return 0;
}

/* ---- gsPlatform -------------------------------------------------------------------------------------- */

/* 0x8050C4D0 (0x4): the socket layer start-up (nothing to do on this platform). */
void SocketStartUp(void) {
}

/* 0x8050C4E0 (0x4): the socket layer shut-down (nothing to do on this platform). */
void SocketShutDown(void) {
}

/* 0x8050C4F0 (0x48): the time base in milliseconds. */
u32 current_time(void) {
    return (u32)(OSGetTime() / (OS_BUS_CLOCK / 4 / 1000));
}

/* 0x8050C540 (0x3C): sleeps the calling thread for `msec` milliseconds. */
void msleep(u32 msec) {
    OSSleepTicks((s64)msec * (s64)(OS_BUS_CLOCK / 4 / 1000));
}


/* ---- gsMemory ---------------------------------------------------------------------------------------- */

/* 0x8050C580 (0x1C): installs the allocator callbacks. */
void gsiMemoryCallbacksSet(GSIMallocFn mallocFn, GSIFreeFn freeFn, GSIReallocFn reallocFn, GSIMemalignFn memalignFn) {
    gsiMemoryCallbacks.mallocFn = mallocFn;
    gsiMemoryCallbacks.freeFn = freeFn;
    gsiMemoryCallbacks.reallocFn = reallocFn;
    gsiMemoryCallbacks.memalignFn = memalignFn;
}

/* 0x8050C5A0 (0x10): allocates through the installed callback. */
/* untyped: caller-owned payload - a raw block */
void* gsimalloc(u32 size) {
    return gsiMemoryCallbacks.mallocFn(size);
}

/* 0x8050C5B0 (0x14): reallocates through the installed callback. */
/* untyped: caller-owned payload - a raw block */
void* gsirealloc(void* block, u32 size) {
    return gsiMemoryCallbacks.reallocFn(block, size);
}

/* 0x8050C5D0 (0x20): frees through the installed callback (NULL is ignored). */
/* untyped: caller-owned payload - a raw block */
void gsifree(void* block) {
    if (block == NULL) {
        return;
    }
    gsiMemoryCallbacks.freeFn(block);
}

}
