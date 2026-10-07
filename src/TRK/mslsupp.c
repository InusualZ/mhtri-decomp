/*
 * TRK/mslsupp.c - the MSL console file hooks over TRK: `__read_console`, `__TRK_write_console`, `__read_file`,
 *    `__write_file` and the shared file-access helper.
 *
 * RANGE. .text 0x8046BA60..0x8046BBEC (5 functions in the map, 0x18C B).
 * FLAGS. the `OS` lib's `cflags_os`.
 * NAMES. file name GUESS (MetroTRK `mslsupp`); `__read_console` and `__access_file` (map rows renamed from fn_8046BA60 and
 *    fn_8046BB40) are GUESS (the MSL names of the console read hook and of the helper both file hooks share).
 * EVIDENCE. dump names for the other three; every function calls `GetUseSerialIO` / `GetTRKConnected` /
 *    `TRKAccessFile`; the stream table of `ansi_files.c` stores the console hooks.
 * RESIDUALS. none known.
 * SHAPES. the console hooks return 1 when serial I/O is off and otherwise forward to the file hooks; the file hooks pass
 *    command byte 0xD1 (read) or 0xD0 (write) to the access helper, which returns 0 (ok), 2 (end of file) or 1 (error).
 */
#include "TRK/mslsupp.h"
#include "TRK/TRKAccessFile.h"
#include "TRK/msghndlr.h"
#include "TRK/targimpl.h"

#define TRK_FILE_ACCESS_WRITE 0xD0
#define TRK_FILE_ACCESS_READ 0xD1

s32 __read_console(s32 handle, u8* buffer, u32* count, void (*idle_proc)(void))
{
    if (GetUseSerialIO() == 0) {
        return 1;
    }
    return __read_file(0, buffer, count, idle_proc);
}

s32 __TRK_write_console(s32 handle, u8* buffer, u32* count, void (*idle_proc)(void))
{
    if (GetUseSerialIO() == 0) {
        return 1;
    }
    return __write_file(1, buffer, count, idle_proc);
}

s32 __read_file(s32 handle, u8* buffer, u32* count, void (*idle_proc)(void))
{
    return __access_file(handle, buffer, count, idle_proc, TRK_FILE_ACCESS_READ);
}

s32 __write_file(s32 handle, u8* buffer, u32* count, void (*idle_proc)(void))
{
    return __access_file(handle, buffer, count, idle_proc, TRK_FILE_ACCESS_WRITE);
}

s32 __access_file(s32 handle, u8* buffer, u32* count, void (*idle_proc)(void), s32 command)
{
    u32 size;
    u8 io_error;
    s32 result;

    if (!GetTRKConnected()) {
        return 1;
    }
    size = *count;
    io_error = TRKAccessFile(command, handle, &size, buffer);
    *count = size;
    switch (io_error) {
    case 0:
        result = 0;
        break;
    case 2:
        result = 2;
        break;
    default:
        result = 1;
        break;
    }
    return result;
}
