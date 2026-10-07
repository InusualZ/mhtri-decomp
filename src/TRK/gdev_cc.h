/*
 * TRK/gdev_cc.h - the debugger channel functions owned by `TRK/gdev_cc.c`, which `TRK/dolphin_trk.c` installs in its
 *    communication table.
 */
#ifndef TRK_GDEV_CC_H
#define TRK_GDEV_CC_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80468378 (0x3C): starts the channel; `inputPendingPtrRef` receives the input-pending flag, `handler` is the interrupt callback. */
s32 gdev_cc_initialize(u32* inputPendingPtrRef, void (*handler)(void));

/* 0x804683B4 (0x8): shuts the channel down. */
s32 gdev_cc_shutdown(void);

/* 0x804683BC (0x24): opens the channel; fails when it is already open. */
s32 gdev_cc_open(void);

/* 0x804683E0 (0x8): closes the channel. */
s32 gdev_cc_close(void);

/* 0x804683E8 (0xB4): reads `len` bytes into `dst`. */
s32 gdev_cc_read(u8* dst, s32 len);

/* 0x8046849C (0x74): writes `len` bytes from `src`. */
s32 gdev_cc_write(const u8* src, s32 len);

/* 0x80468510 (0x24): releases the port before the target continues. */
s32 gdev_cc_pre_continue(void);

/* 0x80468534 (0x24): takes the port back after the target stops. */
s32 gdev_cc_post_stop(void);

/* 0x80468558 (0x74): returns the number of bytes waiting. */
s32 gdev_cc_peek(void);

/* 0x804685CC (0x24): enables the channel's interrupt. */
s32 gdev_cc_initinterrupts(void);

#ifdef __cplusplus
}
#endif

#endif
