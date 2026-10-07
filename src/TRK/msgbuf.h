/*
 * TRK/msgbuf.h - the MetroTRK message buffer pool and its accessors, owned by `TRK/msgbuf.c`.
 */
#ifndef TRK_MSGBUF_H
#define TRK_MSGBUF_H

#include "types.h"

#define TRK_MSG_BUFFER_COUNT 3
#define TRK_MSG_BUFFER_DATA_SIZE 0x880

/* One message buffer of the pool. */
typedef struct TRKBuffer {
    /* +0x00 */ s32 in_use;                              /* nonzero while the buffer is handed out */
    /* +0x04 */ u32 length;                              /* bytes of valid data */
    /* +0x08 */ u32 position;                            /* read/write cursor into data */
    /* +0x0C */ u8 data[TRK_MSG_BUFFER_DATA_SIZE];       /* payload; the command byte is data[4] */
} TRKBuffer; /* size: 0x88C */

#ifdef __cplusplus
extern "C" {
#endif

/* 0x8046A2D0 (0x20): marks every buffer of the pool free; returns 0. */
s32 TRKInitializeMessageBuffers(void);

/* 0x8046A2F0 (0x9C): claims a free buffer, storing its id and address; returns 0 or 0x300 when none is free. */
s32 TRK_GetFreeBuffer(s32* id, TRKBuffer** buffer);

/* 0x8046A38C (0x24): returns the buffer with this id, or NULL when the id is out of range. */
TRKBuffer* TRKGetBuffer(u32 id);

/* 0x8046A3B0 (0x28): returns the buffer with this id to the pool (-1 is ignored). */
void TRK_ReleaseBuffer(s32 id);

/* 0x8046A3D8 (0x28): clears the cursor and length, and the data too when `keep_data` is 0. */
void TRKResetBuffer(TRKBuffer* buffer, s32 keep_data);

/* 0x8046A400 (0x30): moves the cursor, growing the length when it moves past the end; 0x301 when out of range. */
s32 TRK_SetBufferPosition(TRKBuffer* buffer, u32 position);

/* 0x8046A430 (0xA4): appends `size` bytes at the end of the buffer; returns 0 or 0x301 when the data was cut short. */
/* untyped: byte range */
s32 TRK_AppendBuffer(TRKBuffer* buffer, const void* data, s32 size);

/* 0x8046A4D4 (0x90): copies `size` bytes from the cursor into `data`; returns 0 or 0x302 when the data was cut short. */
/* untyped: byte range */
s32 TRK_ReadBuffer(TRKBuffer* buffer, void* data, s32 size);

/* 0x8046A564 (0xD0): appends one u32 in wire (big-endian) order. */
s32 TRKAppendBuffer1_ui32(TRKBuffer* buffer, u32 value);

/* 0x8046A634 (0xF4): appends one u64 in wire (big-endian) order. */
s32 TRKAppendBuffer1_ui64(TRKBuffer* buffer, u64 value);

/* 0x8046A728 (0x64): appends `size` bytes one at a time; returns the first error. */
/* untyped: byte range */
s32 TRKAppendBuffer_ui8(TRKBuffer* buffer, const void* data, s32 size);

/* 0x8046A78C (0xF0): appends `count` u32 values in wire order. */
s32 TRKAppendBuffer_ui32(TRKBuffer* buffer, const u32* data, s32 count);

/* 0x8046A87C (0xE0): reads one u64 from wire order. */
s32 TRKReadBuffer1_ui64(TRKBuffer* buffer, u64* value);

/* 0x8046A95C (0x98): reads `count` bytes one at a time. */
s32 TRKReadBuffer_ui8(TRKBuffer* buffer, u8* data, s32 count);

/* 0x8046A9F4 (0xE8): reads `count` u32 values from wire order. */
s32 TRKReadBuffer_ui32(TRKBuffer* buffer, u32* data, s32 count);

#ifdef __cplusplus
}
#endif

#endif
