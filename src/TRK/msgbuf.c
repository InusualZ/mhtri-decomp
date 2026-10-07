/*
 * TRK/msgbuf.c - the MetroTRK message buffers: pool init, get/release/reset, position and the typed append/read
 *    helpers.
 *
 * RANGE. .text 0x8046A2D0..0x8046AADC (15 functions in the map, 0x80C B); .data 0x8060F780..0x8060F7A8; .bss
 *    0x806F5590..0x806F6F38.
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. file name GUESS (MetroTRK `msgbuf`); `TRK_ReadBuffer`, `TRKAppendBuffer1_ui64`, `TRKAppendBuffer_ui32`,
 *    `TRKReadBuffer1_ui64`, `TRKReadBuffer_ui32` (map rows renamed from fn_) are GUESS (the `TRKAppendBuffer`/`TRKReadBuffer`
 *    family of the named neighbours: `Buffer1` moves one value, the plain name an array); `trk_message_buffers` and
 *    `trk_no_buffer_available_format` are GUESS (the pool and the report string).
 * EVIDENCE. `.bss` 0x806F5590 (the three-buffer pool) and `.data` 0x8060F780 (`No buffer available`) are read only here.
 *    Every typed helper moves its value through `TRK_AppendBuffer`/`TRK_ReadBuffer`, which the compiler inlines, and
 *    byte-swaps through a stack copy when `gTRKByteOrder` says the target is little-endian.
 * RESIDUALS. register allocation only, same opcodes: `TRKReadBuffer_ui32` and `TRKAppendBuffer_ui32` keep the data pointer and
 *    the count in r28/r29 where the target uses r31/r28; the other typed helpers (and `TRKAppendBuffer_ui8`,
 *    `TRKReadBuffer_ui8`) swap the `li` of the inlined count and error registers (99.4 to 99.8).
 * SHAPES. append clamps to the room left (0x301) and read clamps to the bytes left (0x302); both advance the cursor.
 */
#include "TRK/msgbuf.h"
#include "TRK/mem_TRK.h"
#include "TRK/nubinit.h"
#include "TRK/trk_report.h"

#pragma use_lmw_stmw on

#define TRK_MSG_BUFFER_LAST_ID (TRK_MSG_BUFFER_COUNT - 1)
#define TRK_ERR_NO_BUFFER 0x300
#define TRK_ERR_BUFFER_FULL 0x301
#define TRK_ERR_BUFFER_EMPTY 0x302

static TRKBuffer trk_message_buffers[TRK_MSG_BUFFER_COUNT];

s32 TRKInitializeMessageBuffers(void)
{
    trk_message_buffers[0].in_use = 0;
    trk_message_buffers[1].in_use = 0;
    trk_message_buffers[2].in_use = 0;
    return 0;
}

s32 TRK_GetFreeBuffer(s32* id, TRKBuffer** buffer)
{
    s32 err = TRK_ERR_NO_BUFFER;
    s32 i;
    TRKBuffer* candidate;

    *buffer = NULL;
    for (i = 0; i < TRK_MSG_BUFFER_COUNT; i++) {
        candidate = TRKGetBuffer(i);
        if (candidate->in_use == 0) {
            TRKResetBuffer(candidate, 1);
            candidate->in_use = 1;
            err = 0;
            *buffer = candidate;
            *id = i;
            i = TRK_MSG_BUFFER_COUNT;
        }
    }
    if (err == TRK_ERR_NO_BUFFER) {
        TRK_REPORT("MetroTRK - ERROR : No buffer available\n");
    }
    return err;
}

TRKBuffer* TRKGetBuffer(u32 id)
{
    TRKBuffer* buffer = NULL;

    if (id <= TRK_MSG_BUFFER_LAST_ID) {
        buffer = &trk_message_buffers[id];
    }
    return buffer;
}

void TRK_ReleaseBuffer(s32 id)
{
    if (id != -1 && (u32)id <= TRK_MSG_BUFFER_LAST_ID) {
        trk_message_buffers[id].in_use = 0;
    }
}

void TRKResetBuffer(TRKBuffer* buffer, s32 keep_data)
{
    buffer->length = 0;
    buffer->position = 0;
    if (!keep_data) {
        TRK_memset(buffer->data, 0, TRK_MSG_BUFFER_DATA_SIZE);
    }
}

s32 TRK_SetBufferPosition(TRKBuffer* buffer, u32 position)
{
    s32 err = 0;

    if (position > TRK_MSG_BUFFER_DATA_SIZE) {
        err = TRK_ERR_BUFFER_FULL;
    } else {
        buffer->position = position;
        if (position > buffer->length) {
            buffer->length = position;
        }
    }
    return err;
}

/* untyped: byte range */
s32 TRK_AppendBuffer(TRKBuffer* buffer, const void* data, s32 size)
{
    s32 err = 0;
    u32 count = size;
    u32 room;

    if (size == 0) {
        return 0;
    }
    room = TRK_MSG_BUFFER_DATA_SIZE - buffer->position;
    if (room < size) {
        err = TRK_ERR_BUFFER_FULL;
        count = room;
    }
    if (count == 1) {
        buffer->data[buffer->position] = *(const u8*)data;
    } else {
        TRK_memcpy(&buffer->data[buffer->position], data, count);
    }
    buffer->position += count;
    buffer->length = buffer->position;
    return err;
}

static inline s32 TRKAppendBuffer1_ui8(TRKBuffer* buffer, u8 value)
{
    if (buffer->position >= TRK_MSG_BUFFER_DATA_SIZE) {
        return TRK_ERR_BUFFER_FULL;
    }
    buffer->data[buffer->position++] = value;
    buffer->length++;
    return 0;
}

/* untyped: byte range */
s32 TRK_ReadBuffer(TRKBuffer* buffer, void* data, s32 size)
{
    s32 err = 0;
    s32 count = size;
    u32 left;

    if (size == 0) {
        return 0;
    }
    left = buffer->length - buffer->position;
    if (size > left) {
        err = TRK_ERR_BUFFER_EMPTY;
        count = left;
    }
    TRK_memcpy(data, &buffer->data[buffer->position], count);
    buffer->position += count;
    return err;
}

s32 TRKAppendBuffer1_ui32(TRKBuffer* buffer, u32 value)
{
    u8 swapped[4];
    u8* source;

    if (gTRKByteOrder.big_endian) {
        source = (u8*)&value;
    } else {
        swapped[0] = ((u8*)&value)[3];
        swapped[1] = ((u8*)&value)[2];
        swapped[2] = ((u8*)&value)[1];
        swapped[3] = ((u8*)&value)[0];
        source = swapped;
    }
    return TRK_AppendBuffer(buffer, source, 4);
}

s32 TRKAppendBuffer1_ui64(TRKBuffer* buffer, u64 value)
{
    u8 swapped[8];
    u8* source;

    if (gTRKByteOrder.big_endian) {
        source = (u8*)&value;
    } else {
        swapped[0] = ((u8*)&value)[7];
        swapped[1] = ((u8*)&value)[6];
        swapped[2] = ((u8*)&value)[5];
        swapped[3] = ((u8*)&value)[4];
        swapped[4] = ((u8*)&value)[3];
        swapped[5] = ((u8*)&value)[2];
        swapped[6] = ((u8*)&value)[1];
        swapped[7] = ((u8*)&value)[0];
        source = swapped;
    }
    return TRK_AppendBuffer(buffer, source, 8);
}

/* untyped: byte range */
s32 TRKAppendBuffer_ui8(TRKBuffer* buffer, const void* data, s32 size)
{
    const u8* bytes = (const u8*)data;
    s32 err = 0;
    s32 i;

    for (i = 0; err == 0 && i < size; i++, bytes++) {
        err = TRKAppendBuffer1_ui8(buffer, *bytes);
    }
    return err;
}

s32 TRKAppendBuffer_ui32(TRKBuffer* buffer, const u32* data, s32 count)
{
    s32 err = 0;
    s32 i;

    for (i = 0; err == 0 && i < count; i++) {
        err = TRKAppendBuffer1_ui32(buffer, *data);
        data++;
    }
    return err;
}

s32 TRKReadBuffer1_ui64(TRKBuffer* buffer, u64* value)
{
    u8 swapped[8];
    u8* destination;
    s32 err;

    if (gTRKByteOrder.big_endian) {
        destination = (u8*)value;
    } else {
        destination = swapped;
    }
    err = TRK_ReadBuffer(buffer, destination, 8);
    if (!gTRKByteOrder.big_endian && err == 0) {
        ((u8*)value)[0] = destination[7];
        ((u8*)value)[1] = destination[6];
        ((u8*)value)[2] = destination[5];
        ((u8*)value)[3] = destination[4];
        ((u8*)value)[4] = destination[3];
        ((u8*)value)[5] = destination[2];
        ((u8*)value)[6] = destination[1];
        ((u8*)value)[7] = destination[0];
    }
    return err;
}

s32 TRKReadBuffer_ui8(TRKBuffer* buffer, u8* data, s32 count)
{
    s32 err = 0;
    s32 i;

    for (i = 0; err == 0 && i < count; i++) {
        err = TRK_ReadBuffer(buffer, data + i, 1);
    }
    return err;
}

s32 TRKReadBuffer_ui32(TRKBuffer* buffer, u32* data, s32 count)
{
    u8 swapped[4];
    u8* destination;
    s32 i;
    s32 err = 0;

    for (i = 0; err == 0 && i < count; i++) {
        if (gTRKByteOrder.big_endian) {
            destination = (u8*)data;
        } else {
            destination = swapped;
        }
        err = TRK_ReadBuffer(buffer, destination, 4);
        if (!gTRKByteOrder.big_endian && err == 0) {
            ((u8*)data)[0] = destination[3];
            ((u8*)data)[1] = destination[2];
            ((u8*)data)[2] = destination[1];
            ((u8*)data)[3] = destination[0];
        }
        data++;
    }
    return err;
}
