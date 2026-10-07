/*
 * TRK/TRKAccessFile.h - the declaration of `TRKAccessFile`, owned by `TRK/TRKAccessFile.c`.
 */
#ifndef TRK_TRKACCESSFILE_H
#define TRK_TRKACCESSFILE_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x80468358 (0x8): host file access trap; `command` is 0xD0 (write) or 0xD1 (read), the byte count is read and updated
 * through `count`; returns the host status (0 ok, 2 end of file). */
/* untyped: byte range */
u8 TRKAccessFile(s32 command, s32 handle, u32* count, void* buffer);

#ifdef __cplusplus
}
#endif

#endif
