/*
 * TRK/serpoll.h - the MetroTRK serial input poller, owned by `TRK/serpoll.c`.
 */
#ifndef TRK_SERPOLL_H
#define TRK_SERPOLL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804698C0 (0x8): prepares the serial handler; returns 0. */
s32 TRKInitializeSerialHandler(void);

/* 0x804698C8 (0x8): releases the serial handler; returns 0. */
s32 TRKTerminateSerialHandler(void);

/* 0x80469788 (0xCC): reads a pending packet into a message buffer; returns its buffer id or -1 when none arrived. */
s32 TRKTestForPacket(void);

/* 0x80469854 (0x2C): checks the serial line for a complete packet and processes it. */
void TRKGetInput(void);

/* 0x80469880 (0x40): posts a request event for the message buffer `buffer_id`. */
void TRKProcessInput(s32 buffer_id);

#ifdef __cplusplus
}
#endif

#endif
