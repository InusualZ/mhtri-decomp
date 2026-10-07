/*
 * MSL_C/file_io.c - the stdio file layer: `abs`/`labs`, the buffer prepare/flush helpers, `__fwrite`, `fclose`,
 *    `fflush`, `_ftell`, `_fseek`.
 *
 * RANGE. .text 0x8045AB38..0x8045B3A0 (10 functions in the map, 0x868 B).
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS after the main entries; `ftell` (map row fn_8045B1D8) is a GUESS (a four-byte tail call of
 *    `_ftell`); the `FILE` field names are GUESSes from the way the functions use the record.
 * EVIDENCE. contiguous run with no data of its own; `__fwrite` calls `__prep_buffer`, `__flush_buffer` and
 *    `_fseek`, `fflush` calls `__flush_all` / `__flush_buffer`; the next function reads the locale character table
 *    (a different TU).
 * RESIDUALS. COARSE: probably several MSL files (arith, buffer_io, direct_io, file_io, FILE_POS), merged because
 *    nothing in the data proves the seams.
 * SHAPES. the stream record is a bitfield pair (`FileModes`, `FileState`) read through 32-bit loads; `__fwrite`
 *    copies into the buffer in chunks and flushes on a newline for line-buffered streams.
 */
#include "MSL_C/file_io.h"
#include "MSL_C/errno.h"
#include "MSL_C/alloc.h"
#include "MSL_C/mem.h"
#include "MSL_C/wchar_io.h"
#include "MSL_C/misc_io.h"
#include "Runtime.PPCEABI.H/memcpy.h"

#define EFPOS 0x28

int abs(int value)
{
    int sign = value >> 31;

    return (sign ^ value) - sign;
}

long labs(long value)
{
    long sign = value >> 31;

    return (sign ^ value) - sign;
}

void __prep_buffer(FILE* file)
{
    file->buffer_ptr = file->buffer;
    file->buffer_len = file->buffer_size - (file->position & file->buffer_alignment);
    file->buffer_pos = file->position;
}

s32 __flush_buffer(FILE* file, u32* bytes_flushed)
{
    u32 pending = file->buffer_ptr - file->buffer;
    s32 err;

    if (pending != 0) {
        file->buffer_len = pending;
        err = file->write_proc(file->handle, file->buffer, &file->buffer_len, file->idle_proc);
        if (bytes_flushed != NULL) {
            *bytes_flushed = file->buffer_len;
        }
        if (err != 0) {
            return err;
        }
        file->position += file->buffer_len;
    }
    __prep_buffer(file);
    return 0;
}

#pragma dont_inline on

/* untyped: byte range */
u32 __fwrite(const void* buffer, u32 size, u32 count, FILE* file)
{
    u32 bytes_to_go;
    u32 bytes_written;
    u32 chunk;
    u32 saved_buffer_size;
    u8* saved_buffer;
    u8* newline;
    s32 use_buffer;
    s32 unbuffered_io;

    if (fwide(file, 0) == 0) {
        fwide(file, -1);
    }
    bytes_to_go = size * count;
    if (bytes_to_go == 0 || file->state.error != 0 || file->mode.file_kind == 0) {
        return 0;
    }
    if (file->mode.file_kind == 2) {
        __stdio_atexit();
    }
    use_buffer = 1;
    unbuffered_io = 0;
    if (!file->mode.binary_io || file->mode.buffer_mode == 2) {
        unbuffered_io = 1;
    }
    if (unbuffered_io == 0 && file->mode.buffer_mode != 1) {
        use_buffer = 0;
    }
    if (file->state.io_state == 0) {
        if ((file->mode.io_mode & 2) != 0) {
            if ((file->mode.io_mode & 4) != 0 && _fseek(file, 0, 2) != 0) {
                return 0;
            }
            file->state.io_state = 1;
            __prep_buffer(file);
        }
    }
    if (file->state.io_state != 1) {
        file->state.error = 1;
        file->buffer_len = 0;
        return 0;
    }
    bytes_written = 0;
    if (bytes_to_go != 0 && (file->buffer_ptr != file->buffer || use_buffer != 0)) {
        file->buffer_len = file->buffer_size - (file->buffer_ptr - file->buffer);
        do {
            newline = NULL;
            chunk = file->buffer_len;
            if (chunk > bytes_to_go) {
                chunk = bytes_to_go;
            }
            if (file->mode.buffer_mode == 1 && chunk != 0) {
                newline = __memrchr(buffer, '\n', chunk);
                if (newline != NULL) {
                    chunk = newline + 1 - (u8*)buffer;
                }
            }
            if (chunk != 0) {
                memcpy(file->buffer_ptr, buffer, chunk);
                buffer = (u8*)buffer + chunk;
                file->buffer_ptr += chunk;
                bytes_to_go -= chunk;
                file->buffer_len -= chunk;
            }
            if ((file->buffer_len == 0 || newline != NULL || file->mode.buffer_mode == 0)
                && __flush_buffer(file, NULL) != 0) {
                file->state.error = 1;
                bytes_to_go = 0;
                file->buffer_len = 0;
            } else {
                bytes_written += chunk;
            }
        } while (bytes_to_go != 0 && use_buffer != 0);
    }
    if (bytes_to_go != 0 && use_buffer == 0) {
        saved_buffer = file->buffer;
        saved_buffer_size = file->buffer_size;
        file->buffer = (u8*)buffer;
        file->buffer_size = bytes_to_go;
        file->buffer_ptr = (u8*)buffer + bytes_to_go;
        if (__flush_buffer(file, &chunk) != 0) {
            file->state.error = 1;
            file->buffer_len = 0;
        } else {
            bytes_written += chunk;
        }
        file->buffer = saved_buffer;
        file->buffer_size = saved_buffer_size;
        __prep_buffer(file);
        file->buffer_len = 0;
    }
    if (file->mode.buffer_mode != 2) {
        file->buffer_len = 0;
    }
    return bytes_written / size;
}

s32 fclose(FILE* file)
{
    s32 flush_result;
    s32 close_result;

    if (file == NULL) {
        return -1;
    }
    if (file->mode.file_kind == 0) {
        return 0;
    }
    flush_result = fflush(file);
    close_result = file->close_proc(file->handle);
    file->handle = 0;
    file->mode.file_kind = 0;
    if (file->state.free_buffer) {
        free(file->buffer);
    }
    return -(flush_result != 0 || close_result != 0);
}

s32 fflush(FILE* file)
{
    u32 position;

    if (file == NULL) {
        return __flush_all();
    }
    if (file->state.error != 0 || file->mode.file_kind == 0) {
        return -1;
    }
    if (file->mode.io_mode == 1) {
        return 0;
    }
    if (file->state.io_state >= 3) {
        file->state.io_state = 2;
    }
    if (file->state.io_state == 2) {
        file->buffer_len = 0;
    }
    if (file->state.io_state != 1) {
        file->state.io_state = 0;
        return 0;
    }
    if (file->mode.file_kind != 1) {
        position = 0;
    } else {
        position = ftell(file);
    }
    if (__flush_buffer(file, NULL) != 0) {
        file->state.error = 1;
        file->buffer_len = 0;
        return -1;
    }
    file->position = position;
    file->state.io_state = 0;
    file->buffer_len = 0;
    return 0;
}

s32 _ftell(FILE* file)
{
    s32 pushed_back = 0;
    u32 pending;
    s32 position;
    u8* scan;
    s32 remaining;

    if ((u8)(file->mode.file_kind + 0xFF) > 1 || file->state.error != 0) {
        errno = EFPOS;
        return -1;
    }
    if (file->state.io_state == 0) {
        return file->position;
    }
    scan = file->buffer;
    pending = file->buffer_ptr - scan;
    position = file->buffer_pos + pending;
    if (file->state.io_state >= 3) {
        pushed_back = file->state.io_state - 2;
        position -= pushed_back;
    }
    if (!file->mode.binary_io) {
        remaining = pending - pushed_back;
        while (remaining != 0) {
            if (*scan++ == '\n') {
                position++;
            }
            remaining--;
        }
    }
    return position;
}

s32 ftell(FILE* file)
{
    return _ftell(file);
}

s32 _fseek(FILE* file, u32 offset, s32 mode)
{
    u32 position = offset;

    if (file->mode.file_kind != 1 || file->state.error != 0) {
        errno = EFPOS;
        return -1;
    }
    if (file->state.io_state == 1 && __flush_buffer(file, NULL) != 0) {
        file->state.error = 1;
        file->buffer_len = 0;
        errno = EFPOS;
        return -1;
    }
    if (mode == 1) {
        mode = 0;
        position += _ftell(file);
    }
    if (mode != 2 && file->mode.io_mode != 3 && (u32)(file->state.io_state - 2) <= 1) {
        if (position >= file->position || position < file->buffer_pos) {
            file->state.io_state = 0;
        } else {
            file->buffer_ptr = file->buffer + (position - file->buffer_pos);
            file->buffer_len = file->position - position;
            file->state.io_state = 2;
        }
    } else {
        file->state.io_state = 0;
    }
    if (file->state.io_state == 0) {
        if (file->position_proc != NULL && file->position_proc(file->handle, &position, mode, file->idle_proc) != 0) {
            file->state.error = 1;
            file->buffer_len = 0;
            errno = EFPOS;
            return -1;
        }
        file->state.eof = 0;
        file->position = position;
        file->buffer_len = 0;
    }
    return 0;
}
