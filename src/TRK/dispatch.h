/*
 * TRK/dispatch.h - the MetroTRK command dispatcher, owned by `TRK/dispatch.c`.
 */
#ifndef TRK_DISPATCH_H
#define TRK_DISPATCH_H

#include "types.h"
#include "TRK/msgbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80468C2C (0x120): runs the handler for the command byte of `message`; returns its result or 0x500 for an unknown one. */
s32 TRKDispatchMessage(TRKBuffer* message);

#ifdef __cplusplus
}
#endif

#endif
