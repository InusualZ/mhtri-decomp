/*
 * TRK/CircleBuffer.h - the MetroTRK circular byte queue and its lock helpers, owned by `TRK/CircleBuffer.c`.
 */
#ifndef TRK_CIRCLEBUFFER_H
#define TRK_CIRCLEBUFFER_H

#include "types.h"

typedef struct CircleBuffer {
    u8* read_ptr;      /* +0x00 next byte to read */
    u8* write_ptr;     /* +0x04 next byte to write */
    u8* start;         /* +0x08 first byte of the backing store */
    u32 size;          /* +0x0C bytes in the backing store */
    u32 bytes_used;    /* +0x10 bytes queued for reading */
    u32 bytes_free;    /* +0x14 bytes still writable */
    u32 lock_state;    /* +0x18 interrupt state saved by TRKAcquireMutex */
    u32 pad_0x1C;      /* +0x1C padding to the object size */
} CircleBuffer;        /* size: 0x20 */

#ifdef __cplusplus
extern "C" {
#endif

void TRKInitializeMutex(u32* mutex);
void TRKAcquireMutex(u32* mutex);
void TRKReleaseMutex(u32* mutex);
u32 CBGetBytesAvailableForRead(CircleBuffer* cb);
void CircleBufferInitialize(CircleBuffer* cb, u8* buffer, u32 size);
s32 CircleBufferWriteBytes(CircleBuffer* cb, const u8* src, u32 n);
s32 CircleBufferReadBytes(CircleBuffer* cb, u8* dst, u32 n);

#ifdef __cplusplus
}
#endif

#endif
