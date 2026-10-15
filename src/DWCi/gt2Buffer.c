/*
 * DWCi/gt2Buffer.c - the GameSpy GT2 byte buffer: allocation, free space, byte / big-endian short / block writes and
 *   shortening.
 *
 * RANGE. `.text` 0x8050CD20..0x8050CEE0 (6 functions); no data of its own. The left edge follows gt2Auth's last
 *   function, the right edge is gt2Callback's first (`DWCi/gt2Callback.c`); the GT2 files link in name order.
 * FLAGS. `cflags_base` (-func_align 16): the retail functions start 16-byte aligned.
 * NAMES. GameSpy SDK names (`gti2*Buffer*`); GUESS `DWCi_bufferAlloc` (gti2AllocateBuffer), GUESS `DWCi_bufferFrame`
 *   (gti2BufferWriteData). The file name is the GameSpy SDK's `gt2Buffer.c` (no `__FILE__` string).
 * RESIDUALS. none measured (every row matches).
 * SHAPES. `DWCi_bufferFrame` leaves through one `||` early return (retail's `beq` / `bne` / `b` exit).
 */
#include "DWCi/gt2Buffer.h"
#include "DWCi/dwc_nasfunc.h"
#include "Runtime.PPCEABI.H/memcpy.h"
#include "unsplit/Runtime.PPCEABI.H.h"

/* 0x8050CD20 (0x58): allocates `size` bytes of buffer storage. */
int DWCi_bufferAlloc(GTI2Buffer* buffer, u32 size) {
    buffer->buffer = (u8*)gsimalloc(size);
    if (buffer->buffer == NULL) {
        return 0;
    }
    buffer->size = size;
    return 1;
}

/* 0x8050CD80 (0x10): the free bytes left in the buffer. */
s32 gti2GetBufferFreeSpace(const GTI2Buffer* buffer) {
    return buffer->size - buffer->len;
}

/* 0x8050CD90 (0x18): appends one byte. */
void gti2BufferWriteByte(GTI2Buffer* buffer, u8 b) {
    buffer->buffer[buffer->len++] = b;
}

/* 0x8050CDB0 (0x2C): appends a big-endian 16-bit value. */
void gti2BufferWriteUShort(GTI2Buffer* buffer, u16 s) {
    buffer->buffer[buffer->len++] = (u8)(s >> 8);
    buffer->buffer[buffer->len++] = (u8)s;
}

/* 0x8050CDE0 (0x8C): appends `len` bytes (-1: a NUL-terminated string without its NUL). */
void DWCi_bufferFrame(GTI2Buffer* buffer, const u8* data, s32 len) {
    if (data == NULL || len == 0) {
        return;
    }
    if (len == -1) {
        len = strlen((const char*)data);
    }
    memcpy(buffer->buffer + buffer->len, data, len);
    buffer->len += len;
}

/* 0x8050CE70 (0x6C): removes `shortenBy` bytes at `start` (-1: the end of the buffer). */
void gti2BufferShorten(GTI2Buffer* buffer, s32 start, s32 shortenBy) {
    if (start == -1) {
        start = buffer->len - shortenBy;
    }
    memmove(buffer->buffer + start, buffer->buffer + start + shortenBy, buffer->len - start - shortenBy);
    buffer->len -= shortenBy;
}
