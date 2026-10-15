/*
 * DWCi/gt2Main.h - the declarations `src/DWCi/gt2Main.c` owns (docs/plan.md 6.5 rule 2): the GameSpy GT2 public API
 *   the game's GameSpy interface calls.  The GT2 handles and records come from `DWCi/dwc_nasfunc.h`.
 */
#ifndef MHTRI_DWCI_GT2MAIN_H
#define MHTRI_DWCI_GT2MAIN_H

#include "types.h"
#include "DWCi/dwc_nasfunc.h"

#ifdef __cplusplus
extern "C" {
#endif

s32 gt2CreateSocket(GT2Socket* socket, const char* localAddress, u32 outgoingBufferSize, u32 incomingBufferSize,
                    NetworkCallback socketErrorCallback);
void gt2CloseSocket(GT2Socket socket);
void gt2Think(GT2Socket socket);
void gt2Listen(GT2Socket socket, NetworkCallback connectAttemptCallback);
s32 gt2Accept(GT2Connection connection, const GT2ConnectionCallbacks* callbacks);
/* untyped: byte range - the reject message */
void gt2Reject(GT2Connection connection, const char* message, s32 length);
/* untyped: byte range - the connect message */
s32 gt2Connect(GT2Socket socket, u32* connection, const char* remoteAddress, const void* message, u32 length,
               u32 timeout, const GT2ConnectionCallbacks* callbacks, s32 blocking);
/* untyped: byte range - the datagram */
s32 gt2Send(GT2Connection connection, const void* message, u32 length, s32 reliable);
void gt2CloseConnection(GT2Connection connection);
/* untyped: caller-owned payload - the table element and the client data */
void gti2CloseAllConnectionsHardMap(void* elem, void* clientData);
void DWCi_socketShutdown(struct DWCiConn* conn);
s32 gt2GetSocketSOCKET(GT2Socket socket);
void gt2SetUnrecognizedMessageCallback(GT2Socket socket, NetworkCallback callback);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_GT2MAIN_H */
