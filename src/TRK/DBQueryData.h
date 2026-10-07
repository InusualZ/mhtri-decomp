/*
 * DBQueryData.h - the declaration of `DBQueryData`, owned by `TRK/exi2_comm.c`.
 */
#ifndef TRK_DBQUERYDATA_H
#define TRK_DBQUERYDATA_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8052242C - the number of bytes waiting on the debugger channel. */
s32 DBQueryData(void);

#ifdef __cplusplus
}
#endif

#endif
