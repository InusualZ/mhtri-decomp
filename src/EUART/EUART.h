/*
 * EUART/EUART.h - declarations of the symbols owned by `EUART/EUART.c` that other units call.
 */
#ifndef EUART_EUART_H
#define EUART_EUART_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

BOOL EUARTInit(void);
s32 InitializeUART(u32 baud_rate);
s32 WriteUARTN(char* buf, u32 len);

#ifdef __cplusplus
}
#endif

#endif
