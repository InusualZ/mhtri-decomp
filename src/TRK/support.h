/*
 * TRK/support.h - the MetroTRK request/reply helpers, owned by `TRK/support.c`.
 */
#ifndef TRK_SUPPORT_H
#define TRK_SUPPORT_H

#include "types.h"
#include "TRK/msgbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804698D0 (0x1C): returns the length of a NUL-terminated string. */
s32 TRKStringLength(const char* text);

/* 0x804698EC (0x1F4): reads or writes `*count` bytes of a host file in chunks; stores the bytes moved and the host status. */
s32 TRKSuppAccessFile(s32 handle, u8* data, u32* count, u32* io_result, u8 need_reply, u8 is_read);

/* 0x80469C08 (0x118): opens a host file by path; stores its handle and the host status. */
s32 TRKSuppOpenFile(const char* path, u8 mode, u32* handle, u32* io_result);

/* 0x80469D20 (0xE4): closes a host file; stores the host status. */
s32 TRKSuppCloseFile(s32 handle, u32* io_result);

/* 0x80469E04 (0x10C): moves a host file's position; stores the new position and the host status. */
s32 TRKSuppPositionFile(s32 handle, s32* position, u8 origin, u32* io_result);

/* 0x80469AE0 (0x128): sends `message` and waits for the debugger's reply; stores the reply buffer id. */
s32 TRK_RequestSend(TRKBuffer* message, s32* reply_id);

#ifdef __cplusplus
}
#endif

#endif
