/*
 * MSL_C/signal.h - the signal dispatch entry point, owned by `MSL_C/signal.c`.
 */
#ifndef MSL_C_SIGNAL_H
#define MSL_C_SIGNAL_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8045F4AC (0xA8): runs the handler registered for `signal` (1..7); returns 0 or -1 for a bad number. */
s32 raise(s32 signal);

#ifdef __cplusplus
}
#endif

#endif
