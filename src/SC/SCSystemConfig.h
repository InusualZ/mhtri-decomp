/*
 * SC/SCSystemConfig.h - declarations of the symbols owned by `SC/SCSystemConfig.c` that other units call or read.
 */
#ifndef SC_SCSYSTEMCONFIG_H
#define SC_SCSYSTEMCONFIG_H

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 0x804DB050 - starts the SC configuration reader. */
void SCInit(void);

/* 0x804DB0F0 - the SC state machine: 0 = idle, 1 = busy, 2 = the configuration file was reloaded.
 * The band's compares are unsigned (`cmplwi r3,2` / `cmplwi r3,1`), so the status is `u32`. */
u32 SCCheckStatus(void);

/* 0x804DC280 / 0x804DBEB0 - read one typed item out of the SC configuration; FALSE when it is absent. */
BOOL SCFindU32Item(u32* value, u32 item);

BOOL SCFindU8Item(u8* value, u32 item); /* 0x804DC0C0 */
BOOL SCFindS8Item(s8* value, u32 item); /* 0x804DC1A0 */

BOOL SCFindByteArrayItem(void* value, u32 item, u32 size); /* untyped: byte range */

/* 0x804DBF90 - stores a byte range into an item; non-zero on success. */
BOOL SCReplaceByteArrayItem(const void* value, u32 item, u32 size); /* untyped: byte range */

/* 0x804DC360 - stores a U8 item (value, item); returns non-zero on success. */
BOOL SCReplaceU8Item(u32 value, u32 item);

/* 0x804DC480 - writes the pending configuration back; `callback` gets the result (0 = ok). */
void SCFlushAsync(void (*callback)(s32 result));

#ifdef __cplusplus
}
#endif

#endif
