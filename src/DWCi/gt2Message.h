/*
 * DWCi/gt2Message.h - the declarations `src/DWCi/gt2Message.c` owns (docs/plan.md 6.5 rule 2): the GT2 protocol
 *   message handlers and senders the connection, main and socket files call.  The GT2 records come from
 *   `DWCi/dwc_nasfunc.h`.
 */
#ifndef MHTRI_DWCI_GT2MESSAGE_H
#define MHTRI_DWCI_GT2MESSAGE_H

#include "types.h"
#include "DWCi/dwc_nasfunc.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80794348 - the 2-byte 0xFEFE GT2 protocol magic every protocol message starts with. */
extern u16 DWCi_protocolMagic;

/* 0x807615C0 (.bss, 0x1000 B) - the datagram buffer `gti2ReceiveMessages` reads into. */
extern u8 DWCi_gt2ReceiveBuffer[0x1000];

s32 gti2RemoveAckedMessages(DWCiReq* connection, u16 serialNumber);
s32 gti2HandleServerChallenge(DWCiReq* connection, u8* message, s32 len);
s32 gti2HandleClientResponse(DWCiReq* connection, u8* message, s32 len);
s32 gti2HandleReliableMessage(DWCiReq* connection, s32 type, u8* message, s32 len);
/* untyped: caller-owned payload - the two buffered message records */
int gti2IncomingBufferMessageCompare(const void* elem1, const void* elem2);
s32 gti2BufferIncomingMessage(DWCiReq* connection, s32 type, u16 serialNumber, u8* message, s32 len,
                              s32* overflow);
s32 gti2HandleReliablePacket(DWCiReq* connection, s32 type, u8* message, s32 len);
s32 gti2HandleNack(DWCiReq* connection, u8* message, s32 len);
s32 gti2HandleUnreliableMessage(DWCiReq* connection, s32 type, u8* message, s32 len);
s32 gti2HandleMessage(DWCiConn* socket, u8* message, s32 len, u32 ip, u16 port);
int DWCi_requestReconnect(struct DWCiConn* conn, u32 addr, u16 port);
int DWCi_requestRetry(struct DWCiConn* conn, u32 addr, u16 port, u32 flag);
s32 gti2ReceiveMessages(DWCiConn* socket);
int DWCi_requestBlockState(struct DWCiReq* req, u32 a, u32 b, u32* out);
s32 gti2SendClientChallenge(DWCiReq* connection, const u8* challenge);
s32 gti2SendAccept(DWCiReq* connection);
s32 gti2SendReject(DWCiReq* connection, const u8* message, s32 len);
s32 gti2SendClose(DWCiReq* connection);
s32 gti2SendKeepAlive(DWCiReq* connection);
int DWCi_requestFlush(struct DWCiReq* req, u8* message, s32 len);
s32 gti2SendAck(DWCiReq* connection);
s32 gti2SendNack(DWCiReq* connection, u16 start, u16 end);
/* 0x805113B0 - sends the GT2 closed message; 0x80511470 - re-sends a buffered reliable message;
 * 0x80511530 - sends a message (reliable through the outgoing buffer). */
u32 DWCi_sendControlFrame(DWCiReq* req);
u32 DWCi_appendTransfer(DWCiReq* req, struct DWCiXfer* xfer);
u32 DWCi_flushRequest(DWCiReq* req, u32 a, u32 b, u32 flag);

#ifdef __cplusplus
}
#endif

#endif /* MHTRI_DWCI_GT2MESSAGE_H */
