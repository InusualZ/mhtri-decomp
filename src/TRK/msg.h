/*
 * TRK/msg.h - the framed MetroTRK message `TRK_MessageSend` writes, owned by `TRK/msg.c`.
 */
#ifndef TRK_MSG_H
#define TRK_MSG_H

#include "types.h"
#include "TRK/msgbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8046A26C (0x64): stamps the next message id and writes the buffer's frame to the UART; returns 0. */
s32 TRK_MessageSend(TRKBuffer* buffer);

#ifdef __cplusplus
}
#endif

#endif
