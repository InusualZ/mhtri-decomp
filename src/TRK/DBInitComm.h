/*
 * DBInitComm.h - the declaration of `DBInitComm`, owned by `TRK/exi2_comm.c`.
 */
#ifndef TRK_DBINITCOMM_H
#define TRK_DBINITCOMM_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8052237C - clears `*inputPendingPtrRef` and stores the channel handler. */
void DBInitComm(u32* inputPendingPtrRef, void (*handler)(void));

#ifdef __cplusplus
}
#endif

#endif
