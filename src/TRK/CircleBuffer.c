/*
 * TRK/CircleBuffer.c - the MetroTRK circular byte queue: initialise, write bytes, read bytes and their lock helpers.
 *
 * RANGE. .text 0x804685F0..0x80468868 (7 functions in the map, 0x278 B).
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. file name is the dump's prefix; `CircleBuffer*` and `CBGetBytesAvailableForRead` are the dump's;
 *    `TRKInitializeMutex`, `TRKAcquireMutex`, `TRKReleaseMutex` are GUESS (the dump labels the first `DBClose` and
 *    the other two `zz_`; bodies: empty, save OSDisableInterrupts, OSRestoreInterrupts of the saved state).
 *    `CircleBuffer` field names are GUESS (from the bodies).
 * EVIDENCE. the three helpers before the dump-named functions are called only by these.
 * RESIDUALS. none measured yet.
 * SHAPES. the lock state is a `u32` at +0x18 of the queue; both transfers split at the end of the backing store.
 */
#include "TRK/CircleBuffer.h"
#include "OS/OSDisableInterrupts.h"
#include "OS/OSRestoreInterrupts.h"
#include "Runtime.PPCEABI.H/memcpy.h"

#pragma dont_inline on
void TRKInitializeMutex(u32* mutex)
{
}

void TRKAcquireMutex(u32* mutex)
{
    *mutex = OSDisableInterrupts();
}

void TRKReleaseMutex(u32* mutex)
{
    OSRestoreInterrupts(*mutex);
}

u32 CBGetBytesAvailableForRead(CircleBuffer* cb)
{
    return cb->bytes_used;
}
#pragma dont_inline reset

void CircleBufferInitialize(CircleBuffer* cb, u8* buffer, u32 size)
{
    cb->start = buffer;
    cb->size = size;
    cb->read_ptr = buffer;
    cb->write_ptr = buffer;
    cb->bytes_used = 0;
    cb->bytes_free = size;
    TRKInitializeMutex(&cb->lock_state);
}

s32 CircleBufferWriteBytes(CircleBuffer* cb, const u8* src, u32 n)
{
    u32 room;

    if (n > cb->bytes_free) {
        return -1;
    }
    TRKAcquireMutex(&cb->lock_state);
    room = cb->size - (cb->write_ptr - cb->start);
    if (room >= n) {
        memcpy(cb->write_ptr, src, n);
        cb->write_ptr += n;
    } else {
        memcpy(cb->write_ptr, src, room);
        memcpy(cb->start, src + room, n - room);
        cb->write_ptr = cb->start + n - room;
    }
    if (cb->size == (u32)(cb->write_ptr - cb->start)) {
        cb->write_ptr = cb->start;
    }
    cb->bytes_free -= n;
    cb->bytes_used += n;
    TRKReleaseMutex(&cb->lock_state);
    return 0;
}

s32 CircleBufferReadBytes(CircleBuffer* cb, u8* dst, u32 n)
{
    u32 room;

    if (n > cb->bytes_used) {
        return -1;
    }
    TRKAcquireMutex(&cb->lock_state);
    room = cb->size - (cb->read_ptr - cb->start);
    if (n < room) {
        memcpy(dst, cb->read_ptr, n);
        cb->read_ptr += n;
    } else {
        memcpy(dst, cb->read_ptr, room);
        memcpy(dst + room, cb->start, n - room);
        cb->read_ptr = cb->start + n - room;
    }
    if (cb->size == (u32)(cb->read_ptr - cb->start)) {
        cb->read_ptr = cb->start;
    }
    cb->bytes_free += n;
    cb->bytes_used -= n;
    TRKReleaseMutex(&cb->lock_state);
    return 0;
}
