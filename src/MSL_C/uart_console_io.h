/*
 * MSL_C/uart_console_io.h - the console stream callbacks, owned by `MSL_C/uart_console_io.c`.
 */
#ifndef MSL_C_UART_CONSOLE_IO_H
#define MSL_C_UART_CONSOLE_IO_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80463CC0 (0xD0): writes `*count` bytes to the console (UART first, then TRK); returns 0, or 1 on failure. */
s32 __write_console(s32 handle, u8* buffer, u32* count, void (*idle_proc)(void));

/* 0x80463D90 (0x8): closes the console stream; always succeeds. */
s32 __close_console(s32 handle);

#ifdef __cplusplus
}
#endif

#endif
