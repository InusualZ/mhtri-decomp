/*
 * TRK/main_TRK.h - the MetroTRK entry points, owned by `TRK/main_TRK.c`.
 */
#ifndef TRK_MAIN_TRK_H
#define TRK_MAIN_TRK_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804688A0 (0x3C): runs the nub (init, banner, main loop, terminate); returns the termination result. */
s32 TRK_main(void);

/* 0x804688DC (0xEC): pumps nub events and serial input until a shutdown event arrives. */
void TRKNubMainLoop(void);

#ifdef __cplusplus
}
#endif

#endif
