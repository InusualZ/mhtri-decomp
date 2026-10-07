/*
 * MSL_C/ansi_files.c - the standard stream table: `__files` (stdin/stdout/stderr with their three 0x100-byte
 *    buffers) and `__close_all` / `__flush_all`.
 *
 * RANGE. .text 0x804591A8..0x804592B8 (2 functions in the map, 0x110 B); .data 0x8060E958..0x8060EA98; .bss
 *    0x806F4D00..0x806F5000.
 * FLAGS. the `Runtime.PPCEABI.H` lib's `cflags_ppceabi`.
 * NAMES. file name GUESS (MSL `ansi_files`); `stdin_buffer` (map row lbl_806F4F00), `stdout_buffer` (lbl_806F4E00) and
 *    `stderr_buffer` (lbl_806F4D00) are a GUESS.
 * EVIDENCE. `.data` 0x8060E958 (`__files`, 0x140 B) is read by both functions; the three buffers `.bss`
 *    0x806F4D00..0x806F5000 are pointed to by the stream records (their `+0x1C` / `+0x24` words), and the
 *    records' callbacks are the console read/write/close functions of `MSL_C/uart_console_io.c` and
 *    `TRK/mslsupp.c`.
 * RESIDUALS. the `.bss` order of the three buffers: the target lays stderr's buffer first, ours follows first use.
 * SHAPES. the table is a linked list of four records; the last one is zero and only reachable through stderr.
 */
#include "MSL_C/ansi_files.h"
#include "MSL_C/file_io.h"
#include "MSL_C/alloc.h"
#include "MSL_C/uart_console_io.h"
#include "TRK/mslsupp.h"

#define CONSOLE_BUFFER_SIZE 0x100
#define KIND_CONSOLE 2

static u8 stdin_buffer[CONSOLE_BUFFER_SIZE];
static u8 stdout_buffer[CONSOLE_BUFFER_SIZE];
static u8 stderr_buffer[CONSOLE_BUFFER_SIZE];

FILE __files[4] = {
    {
        0,
        {0, 1, 1, KIND_CONSOLE, 0, 0},
        {0, 0, 0, 0},
        0,
        {0},
        0,
        stdin_buffer,
        CONSOLE_BUFFER_SIZE,
        stdin_buffer,
        0,
        0,
        0,
        0,
        NULL,
        __read_console,
        __write_console,
        __close_console,
        NULL,
        &__files[1],
    },
    {
        1,
        {0, 2, 1, KIND_CONSOLE, 0, 0},
        {0, 0, 0, 0},
        0,
        {0},
        0,
        stdout_buffer,
        CONSOLE_BUFFER_SIZE,
        stdout_buffer,
        0,
        0,
        0,
        0,
        NULL,
        __read_console,
        __write_console,
        __close_console,
        NULL,
        &__files[2],
    },
    {
        2,
        {0, 2, 0, KIND_CONSOLE, 0, 0},
        {0, 0, 0, 0},
        0,
        {0},
        0,
        stderr_buffer,
        CONSOLE_BUFFER_SIZE,
        stderr_buffer,
        0,
        0,
        0,
        0,
        NULL,
        __read_console,
        __write_console,
        __close_console,
        NULL,
        &__files[3],
    },
};

void __close_all(void)
{
    FILE* file = __files;
    FILE* done;

    while (file != NULL) {
        if (file->mode.file_kind != 0) {
            fclose(file);
        }
        done = file;
        file = file->next_file;
        if (done->is_dynamically_allocated != 0) {
            free(done);
        } else {
            done->mode.file_kind = 3;
            if (file != NULL && file->is_dynamically_allocated != 0) {
                done->next_file = NULL;
            }
        }
    }
}

s32 __flush_all(void)
{
    s32 result = 0;
    FILE* file = __files;

    while (file != NULL) {
        if (file->mode.file_kind != 0 && fflush(file) != 0) {
            result = -1;
        }
        file = file->next_file;
    }
    return result;
}
