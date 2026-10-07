/*
 * TRK/msg.h - the framed MetroTRK message `TRK_MessageSend` writes, owned by `TRK/msg.c`.
 */
#ifndef TRK_MSG_H
#define TRK_MSG_H

#include "types.h"

/* The leading fields of a message buffer (approximate: only the fields `TRK_MessageSend` touches are named). */
typedef struct TRKMessage {
    /* +0x00 */ u8 pad_0x00[4];
    /* +0x04 */ u32 length;        /* bytes written after the 0xC-byte header */
    /* +0x08 */ u8 pad_0x08[4];
    /* +0x0C */ u8 header[6];      /* first bytes of the frame */
    /* +0x12 */ u16 message_id;    /* sequence number stamped on send */
} TRKMessage; /* size: 0x14 (approximate: the fields past +0x14 are not touched here) */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8046A26C - stamps the next message id and writes the frame to the UART; returns 0. */
s32 TRK_MessageSend(TRKMessage* msg);

#ifdef __cplusplus
}
#endif

#endif
